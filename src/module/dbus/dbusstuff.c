/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <unistd.h>
#include <dbus/dbus.h>

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx-utils/utarray.h"
#include "fcitx/instance.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"

#include "frontend/ipc/ipc.h"
#include "dbusstuff.h"
#include "dbuslauncher.h"
#include "dbussocket.h"
#include <fcitx-utils/handler-table.h>

typedef struct _FcitxDBus {
    DBusConnection *conn;
    DBusConnection *privconn;
    FcitxInstance* owner;
    FcitxDBusWatch* watches;
    DBusDaemonProperty daemon;
    char* serviceName;
    FcitxHandlerTable* handler;
    UT_array extraconns;
} FcitxDBus;

#define RETRY_INTERVAL 2
#define MAX_RETRY_TIMES 5

static void* DBusCreate(FcitxInstance* instance);
static void DBusSetFD(void* arg);
static void DBusProcessEvent(void* arg);
static void DBusDestroy(void* arg);
DECLARE_ADDFUNCTIONS(DBus)

typedef struct _FcitxDBusWatchNameNotify {
    void *owner;
    void *data;
    FcitxDestroyNotify destroy;
    FcitxDBusWatchNameCallback func;
} FcitxDBusWatchNameNotify;

static void
FcitxDBusWatchNameNotifyFreeFunc(void *obj)
{
    FcitxDBusWatchNameNotify *notify = obj;
    if (notify->destroy) {
        notify->destroy(notify->data);
    }
}

FCITX_DEFINE_PLUGIN(fcitx_dbus, module, FcitxModule) = {
    DBusCreate,
    DBusSetFD,
    DBusProcessEvent,
    DBusDestroy,
    NULL
};

DBusHandlerResult
DBusModuleFilter(DBusConnection* connection, DBusMessage* msg, void* user_data)
{
    FCITX_UNUSED(connection);

#if 0
    if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL ||
        dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL)
        FcitxLog(INFO, "%s %s %s", dbus_message_get_interface(msg), dbus_message_get_member(msg), dbus_message_get_path(msg));
#endif
    FcitxDBus* dbusmodule = (FcitxDBus*) user_data;
    if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected")) {
        FcitxInstanceEnd(dbusmodule->owner);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
        const char* service, *oldowner, *newowner;
        do {
            if (!dbus_message_get_args(msg, NULL,
                                       DBUS_TYPE_STRING, &service ,
                                       DBUS_TYPE_STRING, &oldowner ,
                                       DBUS_TYPE_STRING, &newowner ,
                                       DBUS_TYPE_INVALID)) {
                break;
            }

            FcitxDBusWatchNameNotify *notify;
            notify = fcitx_handler_table_first_strkey(dbusmodule->handler, service);
            if (!notify) {
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
            for (; notify; notify = fcitx_handler_table_next(dbusmodule->handler, notify)) {
                notify->func(notify->owner, notify->data, service, oldowner, newowner);
            }
            return DBUS_HANDLER_RESULT_HANDLED;
        } while(0);
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void DBusAddMatch(void* data, const void* key, size_t len, void* owner)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) owner;
    char* name = malloc(len + 1);
    memcpy(name, key, len);
    name[len] = '\0';
    char* rule = NULL;
    asprintf(&rule,
             "type='signal',"
             "sender='" DBUS_SERVICE_DBUS "',"
             "interface='" DBUS_INTERFACE_DBUS "',"
             "path='" DBUS_PATH_DBUS "',"
             "member='NameOwnerChanged',"
             "arg0='%s'",
             name);
    free(name);

    dbus_bus_add_match(dbusmodule->conn, rule, NULL);
    free(rule);
}


void DBusRemoveMatch(void* data, const void* key, size_t len, void* owner)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) owner;
    char* name = malloc(len + 1);
    memcpy(name, key, len);
    name[len] = '\0';
    char* rule = NULL;
    asprintf(&rule,
             "type='signal',"
             "sender='" DBUS_SERVICE_DBUS "',"
             "interface='" DBUS_INTERFACE_DBUS "',"
             "path='" DBUS_PATH_DBUS "',"
             "member='NameOwnerChanged',"
             "arg0='%s'",
             name);
    free(name);

    dbus_bus_remove_match(dbusmodule->conn, rule, NULL);
    free(rule);
}

void* DBusCreate(FcitxInstance* instance)
{
    FcitxDBus *dbusmodule = (FcitxDBus*) fcitx_utils_malloc0(sizeof(FcitxDBus));
    dbusmodule->owner = instance;
    utarray_init(&dbusmodule->extraconns, fcitx_ptr_icd);

    DBusError err;

    if (FcitxInstanceIsTryReplace(instance)) {
        fcitx_utils_launch_tool("fcitx-remote", "-e");
        sleep(1);
    }

    dbus_threads_init_default();

    // first init dbus
    dbus_error_init(&err);

    int retry = 0;
    DBusConnection* conn = NULL;
    char* servicename = NULL;
    asprintf(&servicename, "%s-%d", FCITX_DBUS_SERVICE,
             fcitx_utils_get_display_number());

    /* do session dbus initialize */
    do {
        if (!getenv("DISPLAY") && !getenv("DBUS_SESSION_BUS_ADDRESS")) {
            FcitxLog(WARNING, "Without DISPLAY or DBUS_SESSION_BUS_ADDRESS session bus will not work");
            break;
        }
        /* try to get session dbus */
        while (1) {
            conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
            if (dbus_error_is_set(&err)) {
                FcitxLog(WARNING, "Connection Error (%s)", err.message);
                dbus_error_free(&err);
                dbus_error_init(&err);
            }

            if (NULL == conn && retry < MAX_RETRY_TIMES) {
                retry ++;
                sleep(RETRY_INTERVAL * retry);
            } else {
                break;
            }
        }

        if (NULL == conn) {
            break;
        }

        if (!dbus_connection_add_filter(conn, DBusModuleFilter, dbusmodule, NULL))
            break;

        if (!dbus_connection_set_watch_functions(conn, DBusAddWatch,
                                                 DBusRemoveWatch, NULL,
                                                 &dbusmodule->watches, NULL)) {
            FcitxLog(WARNING, "Add Watch Function Error");
            dbus_error_free(&err);
            dbus_error_init(&err);
            dbus_connection_unref(conn);
            conn = NULL;
            break;
        }

        /* from here we know dbus connection is successful, now we need to register the service */
        dbus_connection_set_exit_on_disconnect(conn, FALSE);
        dbusmodule->conn = conn;

        boolean request_retry = false;

        int replaceCountdown = FcitxInstanceIsTryReplace(instance) ? 3 : 0;
        FcitxInstanceResetTryReplace(instance);
        do {
            request_retry = false;

            // request a name on the bus
            int ret = dbus_bus_request_name(conn, servicename,
                                            DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                            &err);
            if (dbus_error_is_set(&err)) {
                FcitxLog(WARNING, "Name Error (%s)", err.message);
                goto dbus_init_failed;
            }
            if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
                FcitxLog(WARNING, "DBus Service Already Exists");

                if (replaceCountdown > 0) {
                    replaceCountdown --;
                    fcitx_utils_launch_tool("fcitx-remote", "-e");

                    /* sleep for a while and retry */
                    sleep(1);

                    request_retry = true;
                    continue;
                }

                /* if we know fcitx exists, we should exit. */
                dbus_error_free(&err);
                free(servicename);
                free(dbusmodule);

                FcitxInstanceEnd(instance);
                return NULL;
            } else {
                dbus_bus_request_name(conn, FCITX_DBUS_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL);
            }
        } while (request_retry);

        dbus_connection_flush(dbusmodule->conn);
    } while(0);

    DBusConnection* privconn = NULL;
    do {
        int noPrivateDBus = fcitx_utils_get_boolean_env("FCITX_NO_PRIVATE_DBUS", false);
        if (noPrivateDBus)
            break;

        char* file = NULL;
        FILE* dbusfp = FcitxXDGGetFileWithPrefix("dbus", "daemon.conf", "r", &file);

        // even we don't have daemon.conf here, we can still use the default one.
        if (dbusfp) {
            fclose(dbusfp);
        }

        dbusmodule->daemon = DBusLaunch(file);
        fcitx_utils_free(file);
        if (dbusmodule->daemon.pid == 0)
            break;

        privconn = dbus_connection_open(dbusmodule->daemon.address, &err);

        if (dbus_error_is_set(&err)) {
            FcitxLog(ERROR, "Private dbus daemon connection error (%s)",
                     err.message);
            break;
        }

        dbus_bus_register(privconn, &err);

        if (dbus_error_is_set(&err)) {
            FcitxLog(ERROR, "Private dbus bus register error (%s)", err.message);
            break;
        }

        dbus_bus_request_name(privconn, FCITX_DBUS_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL);

        int ret = dbus_bus_request_name(privconn, servicename,
                                        DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                        &err);

        if (dbus_error_is_set(&err)) {
            FcitxLog(WARNING, "Private Name Error (%s)", err.message);
            break;
        }
        if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
            FcitxLog(ERROR, "Private DBus Service Already Exists, fcitx being hacked?");
            break;
        }

        if (!dbus_connection_add_filter(privconn, DBusModuleFilter, dbusmodule, NULL))
            break;

        if (!dbus_connection_set_watch_functions(privconn, DBusAddWatch,
                                                 DBusRemoveWatch, NULL,
                                                 &dbusmodule->watches, NULL)) {
            FcitxLog(WARNING, "Add Watch Function Error");
            break;
        }

        char* addressFile = NULL;
        char* localMachineId = dbus_get_local_machine_id();
        asprintf(&addressFile, "%s-%d", localMachineId,
                 fcitx_utils_get_display_number());
        dbus_free(localMachineId);

        FILE* fp = FcitxXDGGetFileUserWithPrefix("dbus", addressFile, "w", NULL);
        free(addressFile);
        if (!fp)
            break;

        fprintf(fp, "%s", dbusmodule->daemon.address);
        fwrite("\0", sizeof(char), 1, fp);
        pid_t curPid = getpid();
        fwrite(&dbusmodule->daemon.pid, sizeof(pid_t), 1, fp);
        fwrite(&curPid, sizeof(pid_t), 1, fp);
        fclose(fp);

        dbusmodule->privconn = privconn;

        char* command = fcitx_utils_get_fcitx_path_with_filename("bindir", "fcitx-dbus-watcher");
        char* pidstring = NULL;
        asprintf(&pidstring, "%d", dbusmodule->daemon.pid);
        char* args[] = {
            command,
            dbusmodule->daemon.address,
            pidstring,
            NULL
        };
        fcitx_utils_start_process(args);
        free(command);
        free(pidstring);

    } while(0);

    if (!dbusmodule->privconn) {
        if (privconn) {
            dbus_connection_unref(privconn);
            DBusKill(&dbusmodule->daemon);
        }
    }

    FcitxHandlerKeyDataVTable vtable;
    vtable.size = 0;
    vtable.owner = dbusmodule;
    vtable.init = DBusAddMatch;
    vtable.free = DBusRemoveMatch;
    dbusmodule->handler = fcitx_handler_table_new_with_keydata(sizeof(FcitxDBusWatchNameNotify), FcitxDBusWatchNameNotifyFreeFunc, &vtable);

    FcitxDBusAddFunctions(instance);
    dbus_error_free(&err);

    dbusmodule->serviceName = servicename;
    return dbusmodule;

dbus_init_failed:
    dbus_error_free(&err);
    fcitx_utils_free(servicename);
    if (conn)
        dbus_connection_unref(conn);
    DBusKill(&dbusmodule->daemon);
    fcitx_utils_free(dbusmodule);
    return NULL;
}

void DBusDestroy(void* arg) {
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;

    fcitx_handler_table_free(dbusmodule->handler);

    if (dbusmodule->conn) {
        dbus_bus_release_name(dbusmodule->conn, dbusmodule->serviceName, NULL);
        dbus_connection_unref(dbusmodule->conn);
    }
    if (dbusmodule->privconn) {
        dbus_bus_release_name(dbusmodule->privconn, dbusmodule->serviceName, NULL);
        dbus_connection_unref(dbusmodule->privconn);
    }
    DBusKill(&dbusmodule->daemon);
    free(dbusmodule->serviceName);

    dbus_shutdown();

    free(dbusmodule);
}

void DBusSetFD(void* arg)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    FcitxInstance* instance = dbusmodule->owner;
    fd_set *rfds =  FcitxInstanceGetReadFDSet(instance);
    fd_set *wfds =  FcitxInstanceGetWriteFDSet(instance);
    fd_set *efds =  FcitxInstanceGetExceptFDSet(instance);

    int maxfd = DBusUpdateFDSet(dbusmodule->watches, rfds, wfds, efds);

    if (FcitxInstanceGetMaxFD(instance) < maxfd) {
        FcitxInstanceSetMaxFD(instance, maxfd);
    }
}


void DBusProcessEvent(void* arg)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    FcitxInstance* instance = dbusmodule->owner;
    fd_set *rfds =  FcitxInstanceGetReadFDSet(instance);
    fd_set *wfds =  FcitxInstanceGetWriteFDSet(instance);
    fd_set *efds =  FcitxInstanceGetExceptFDSet(instance);

    DBusProcessEventForWatches(dbusmodule->watches, rfds, wfds, efds);
    DBusProcessEventForConnection(dbusmodule->conn);
    DBusProcessEventForConnection(dbusmodule->privconn);
    utarray_foreach(connection, &dbusmodule->extraconns, DBusConnection *) {
        DBusProcessEventForConnection(*connection);
    }
}

boolean DBusWatchName(void* arg,
                      const char* name,
                      void *owner,
                      FcitxDBusWatchNameCallback func,
                      void *data,
                      FcitxDestroyNotify destroy)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;

    if (!dbusmodule->conn) {
        return 0;
    }

    FcitxDBusWatchNameNotify notify;
    notify.data = data;
    notify.destroy = destroy;
    notify.owner = owner;
    notify.func = func;
    return fcitx_handler_table_append_strkey(dbusmodule->handler, name, &notify);
}

void DBusUnwatchName(void* arg, int id)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;

    fcitx_handler_table_remove_by_id_full(dbusmodule->handler, id);
}

boolean DBusAttachConnection(void *arg, DBusConnection *conn) {
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    dbus_connection_ref(conn);
    if (!dbus_connection_set_watch_functions(conn, DBusAddWatch,
                                                DBusRemoveWatch, NULL,
                                                &dbusmodule->watches, NULL)) {
        FcitxLog(WARNING, "Add Watch Function Error");
        dbus_connection_unref(conn);
        return false;
    }
    utarray_push_back(&dbusmodule->extraconns, &conn);
    return true;
}

void DBusDeattachConnection(void *arg, DBusConnection *conn) {

    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    utarray_foreach(connection, &dbusmodule->extraconns, DBusConnection *) {
        if (*connection == conn) {
            utarray_remove_quick(&dbusmodule->extraconns, utarray_eltidx(&dbusmodule->extraconns, connection));
            dbus_connection_unref(conn);
            break;
        }
    }
}

#include "fcitx-dbus-addfunctions.h"
