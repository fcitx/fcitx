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

typedef struct _FcitxDBus {
    DBusConnection *conn;
    DBusConnection *privconn;
    FcitxInstance* owner;
    FcitxDBusWatch* watches;
    DBusDaemonProperty daemon;
    char* serviceName;
} FcitxDBus;

#define RETRY_INTERVAL 2
#define MAX_RETRY_TIMES 5

static void* DBusCreate(FcitxInstance* instance);
static void DBusSetFD(void* arg);
static void DBusProcessEvent(void* arg);
static void DBusDestroy(void* arg);
static void* DBusGetConnection(void* arg, FcitxModuleFunctionArg args);
static void* DBusGetPrivateConnection(void* arg, FcitxModuleFunctionArg args);

FCITX_DEFINE_PLUGIN(fcitx_dbus, module, FcitxModule) = {
    DBusCreate,
    DBusSetFD,
    DBusProcessEvent,
    DBusDestroy,
    NULL
};

DBusHandlerResult DBusModuleFilter(DBusConnection* connection, DBusMessage* msg, void* user_data)
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
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void* DBusCreate(FcitxInstance* instance)
{
    FcitxDBus *dbusmodule = (FcitxDBus*) fcitx_utils_malloc0(sizeof(FcitxDBus));
    FcitxAddon* dbusaddon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_DBUS_NAME);
    dbusmodule->owner = instance;

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

        if (!dbus_connection_set_watch_functions(conn, DBusAddWatch, DBusRemoveWatch,
                NULL, &dbusmodule->watches, NULL)) {
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
            }
        } while (request_retry);

        dbus_connection_flush(dbusmodule->conn);
    } while(0);

    DBusConnection* privconn = NULL;
    do {
        int noPrivateDBus = fcitx_utils_get_boolean_env("FCITX_NO_PRIVATE_DBUS", false);
        if (noPrivateDBus)
            break;

        char* file;
        FILE* dbusfp = FcitxXDGGetFileWithPrefix("dbus", "daemon.conf", "r", &file);
        if (dbusfp) {
            fclose(dbusfp);
        }
        else {
            free(file);
            file = NULL;
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

        if (!dbus_connection_set_watch_functions(privconn, DBusAddWatch, DBusRemoveWatch,
                NULL, &dbusmodule->watches, NULL)) {
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

        char* command = fcitx_utils_get_fcitx_path_with_filename("bindir", "/fcitx-dbus-watcher");
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

    FcitxModuleAddFunction(dbusaddon, DBusGetConnection);
    FcitxModuleAddFunction(dbusaddon, DBusGetPrivateConnection);
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
    if (dbusmodule->conn) {
        dbus_bus_release_name(dbusmodule->conn, dbusmodule->serviceName, NULL);
    }
    if (dbusmodule->privconn) {
        dbus_bus_release_name(dbusmodule->privconn, dbusmodule->serviceName, NULL);
    }
    DBusKill(&dbusmodule->daemon);
    free(dbusmodule->serviceName);
    free(dbusmodule);
}

void* DBusGetConnection(void* arg, FcitxModuleFunctionArg args)
{
    FCITX_UNUSED(args);
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;
    return dbusmodule->conn;
}

void* DBusGetPrivateConnection(void* arg, FcitxModuleFunctionArg args)
{
    FCITX_UNUSED(args);
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;
    return dbusmodule->privconn;
}

void DBusSetFD(void* arg)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    FcitxInstance* instance = dbusmodule->owner;
    fd_set *rfds =  FcitxInstanceGetReadFDSet(instance);
    fd_set *wfds =  FcitxInstanceGetWriteFDSet(instance);
    fd_set *efds =  FcitxInstanceGetExceptFDSet(instance);

    DBusUpdateFDSet(dbusmodule->watches, rfds, wfds, efds);
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
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
