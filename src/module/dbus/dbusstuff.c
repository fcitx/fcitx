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
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx-utils/utarray.h"
#include "fcitx/instance.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"

#include "frontend/ipc/ipc.h"
#include "dbusstuff.h"


typedef struct _FcitxDBusWatch {
    DBusWatch *watch;
    struct _FcitxDBusWatch *next;
} FcitxDBusWatch;

typedef struct _FcitxDBus {
    DBusConnection *conn;
    FcitxInstance* owner;
    FcitxDBusWatch* watches;
} FcitxDBus;

#define RETRY_INTERVAL 2
#define MAX_RETRY_TIMES 5

static void* DBusCreate(FcitxInstance* instance);
static void DBusSetFD(void* arg);
static void DBusProcessEvent(void* arg);
static void* DBusGetConnection(void* arg, FcitxModuleFunctionArg args);
static dbus_bool_t FcitxDBusAddWatch(DBusWatch *watch, void *data);
static void FcitxDBusRemoveWatch(DBusWatch *watch, void *data);

FCITX_EXPORT_API
FcitxModule module = {
    DBusCreate,
    DBusSetFD,
    DBusProcessEvent,
    NULL,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* DBusCreate(FcitxInstance* instance)
{
    FcitxDBus *dbusmodule = (FcitxDBus*) fcitx_utils_malloc0(sizeof(FcitxDBus));
    FcitxAddon* dbusaddon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_DBUS_NAME);
    DBusError err;

    dbus_threads_init_default();

    // first init dbus
    dbus_error_init(&err);

    int retry = 0;
    DBusConnection* conn = NULL;

    do {
        conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (dbus_error_is_set(&err)) {
            FcitxLog(WARNING, _("Connection Error (%s)"), err.message);
            dbus_error_free(&err);
            dbus_error_init(&err);
        }

        if (NULL == conn) {
            sleep(RETRY_INTERVAL * MAX_RETRY_TIMES);
            retry ++;
        }
    } while (NULL == conn && retry < MAX_RETRY_TIMES);

    if (NULL == conn) {
        free(dbusmodule);
        return NULL;
    }

    if (!dbus_connection_set_watch_functions(conn, FcitxDBusAddWatch, FcitxDBusRemoveWatch,
            NULL, dbusmodule, NULL)) {
        FcitxLog(WARNING, _("Add Watch Function Error"));
        dbus_error_free(&err);
        free(dbusmodule);
        return NULL;
    }

    dbusmodule->conn = conn;
    dbusmodule->owner = instance;

    boolean request_retry = false;
    char* servicename = NULL;
    asprintf(&servicename, "%s-%d", FCITX_DBUS_SERVICE, fcitx_utils_get_display_number());
    do {
        request_retry = false;

        // request a name on the bus
        int ret = dbus_bus_request_name(conn, servicename,
                                        DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                        &err);
        if (dbus_error_is_set(&err)) {
            FcitxLog(WARNING, _("Name Error (%s)"), err.message);
            dbus_error_free(&err);
            free(servicename);
            free(dbusmodule);
            return NULL;
        }
        if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
            FcitxLog(WARNING, "DBus Service Already Exists");
            
            if (FcitxInstanceIsTryReplace(instance)) {
                FcitxInstanceResetTryReplace(instance);
                DBusMessage* message = dbus_message_new_method_call(servicename, FCITX_IM_DBUS_PATH, FCITX_IM_DBUS_INTERFACE, "Exit");
                dbus_connection_send(dbusmodule->conn, message, NULL);
                /* synchronize call here */
                DBusMessage* reply = dbus_connection_send_with_reply_and_block(dbusmodule->conn, message, 0, &err);
                
                if (dbus_error_is_set(&err)) {
                    dbus_error_free(&err);
                    dbus_error_init(&err);
                }
                
                if (reply)
                    dbus_message_unref(reply);
                dbus_message_unref(message);
                
                /* sleep for a while and retry */
                sleep(1);
                
                request_retry = true;
                continue;
            }
            
            dbus_error_free(&err);
            free(servicename);
            free(dbusmodule);
            return NULL;
        }
    } while (request_retry);

    free(servicename);

    dbus_connection_flush(conn);
    AddFunction(dbusaddon, DBusGetConnection);
    dbus_error_free(&err);

    return dbusmodule;
}

void* DBusGetConnection(void* arg, FcitxModuleFunctionArg args)
{
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;
    return dbusmodule->conn;
}

static dbus_bool_t FcitxDBusAddWatch(DBusWatch *watch, void *data)
{
    FcitxDBusWatch *w;
    FcitxDBus* dbusmodule = (FcitxDBus*) data;

    for (w = dbusmodule->watches; w; w = w->next)
        if (w->watch == watch)
            return TRUE;

    if (!(w = fcitx_utils_malloc0(sizeof(FcitxDBusWatch))))
        return FALSE;

    w->watch = watch;
    w->next = dbusmodule->watches;
    dbusmodule->watches = w;
    return TRUE;
}

static void FcitxDBusRemoveWatch(DBusWatch *watch, void *data)
{
    FcitxDBusWatch **up, *w;
    FcitxDBus* dbusmodule = (FcitxDBus*) data;

    for (up = &(dbusmodule->watches), w = dbusmodule->watches; w; w = w->next) {
        if (w->watch == watch) {
            *up = w->next;
            free(w);
        } else
            up = &(w->next);
    }
}

void DBusSetFD(void* arg)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    FcitxDBusWatch *w;
    FcitxInstance* instance = dbusmodule->owner;

    for (w = dbusmodule->watches; w; w = w->next)
        if (dbus_watch_get_enabled(w->watch)) {
            unsigned int flags = dbus_watch_get_flags(w->watch);
            int fd = dbus_watch_get_unix_fd(w->watch);

            if (FcitxInstanceGetMaxFD(instance) < fd)
                FcitxInstanceSetMaxFD(instance, fd);

            if (flags & DBUS_WATCH_READABLE)
                FD_SET(fd, FcitxInstanceGetReadFDSet(instance));

            if (flags & DBUS_WATCH_WRITABLE)
                FD_SET(fd, FcitxInstanceGetWriteFDSet(instance));

            FD_SET(fd, FcitxInstanceGetExceptFDSet(instance));
        }
}


void DBusProcessEvent(void* arg)
{
    FcitxDBus* dbusmodule = (FcitxDBus*) arg;
    DBusConnection *connection = (DBusConnection *)dbusmodule->conn;
    FcitxInstance* instance = dbusmodule->owner;
    FcitxDBusWatch *w;

    for (w = dbusmodule->watches; w; w = w->next) {
        if (dbus_watch_get_enabled(w->watch)) {
            unsigned int flags = 0;
            int fd = dbus_watch_get_unix_fd(w->watch);

            if (FD_ISSET(fd, FcitxInstanceGetReadFDSet(instance)))
                flags |= DBUS_WATCH_READABLE;

            if (FD_ISSET(fd, FcitxInstanceGetWriteFDSet(instance)))
                flags |= DBUS_WATCH_WRITABLE;

            if (FD_ISSET(fd, FcitxInstanceGetExceptFDSet(instance)))
                flags |= DBUS_WATCH_ERROR;

            if (flags != 0)
                dbus_watch_handle(w->watch, flags);
        }
    }

    if (connection) {
        dbus_connection_ref(connection);
        while (dbus_connection_dispatch(connection) == DBUS_DISPATCH_DATA_REMAINS);
        dbus_connection_unref(connection);
    }
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
