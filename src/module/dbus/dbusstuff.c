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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx-utils/utarray.h"
#include "fcitx/instance.h"
#include "fcitx-utils/log.h"
#include <dbus/dbus.h>
#include <libintl.h>
#include "dbusstuff.h"
#include <unistd.h>
#include <fcitx-utils/utils.h>

typedef struct FcitxDBus {
    DBusConnection *conn;
    UT_array handlers;
    FcitxInstance* owner;
} FcitxDBus;

const UT_icd handler_icd = {sizeof(FcitxDBusEventHandler), 0, 0, 0};
static void* DBusCreate(FcitxInstance* instance);
static void* DBusRun(void* arg);
static void* DBusGetConnection(void* arg, FcitxModuleFunctionArg args);
static void* DBusAddEventHandler(void* arg, FcitxModuleFunctionArg args);
static void* DBusRemoveEventHandler(void* arg, FcitxModuleFunctionArg args);

FCITX_EXPORT_API
FcitxModule module = {
    DBusCreate,
    DBusRun,
    NULL,
    NULL
};

void* DBusCreate(FcitxInstance* instance)
{
    FcitxDBus *dbusmodule = (FcitxDBus*) fcitx_malloc0(sizeof(FcitxDBus));
    FcitxAddon* dbusaddon = GetAddonByName(&instance->addons, FCITX_DBUS_NAME);
    DBusError err;
    
    dbus_threads_init_default();

    // first init dbus
    dbus_error_init(&err);
    DBusConnection* conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        FcitxLog(DEBUG, _("Connection Error (%s)"), err.message);
        dbus_error_free(&err);
    }
    if (NULL == conn) {
        return NULL;
    }

    // request a name on the bus
    int ret = dbus_bus_request_name(conn, FCITX_DBUS_SERVICE,
                                DBUS_NAME_FLAG_REPLACE_EXISTING,
                                &err);
    if (dbus_error_is_set(&err)) {
        FcitxLog(DEBUG, _("Name Error (%s)"), err.message);
        dbus_error_free(&err);
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        return NULL;
    }

    dbus_connection_flush(conn);
    utarray_init(&dbusmodule->handlers, &handler_icd);
    dbusmodule->conn = conn;
    dbusmodule->owner = instance;
    AddFunction(dbusaddon, DBusGetConnection);
    AddFunction(dbusaddon, DBusAddEventHandler);
    AddFunction(dbusaddon, DBusRemoveEventHandler);

    return dbusmodule;
}

void* DBusRun(void* arg)
{
    FcitxDBus *dbusmodule = (FcitxDBus*) arg;
    DBusMessage *msg;
    DBusConnection *conn = dbusmodule->conn;

    for (;;) {
        FcitxLock(dbusmodule->owner);
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);
        if ( NULL == msg) {
            usleep(16000);
        }
        else {
            FcitxDBusEventHandler* handler;
            for ( handler = (FcitxDBusEventHandler *) utarray_front(&dbusmodule->handlers);
                    handler != NULL;
                    handler = (FcitxDBusEventHandler *) utarray_next(&dbusmodule->handlers, handler))
                if (handler->eventHandler (handler->instance, msg))
                    break;
            dbus_message_unref(msg);
        }
        FcitxUnlock(dbusmodule->owner);
    }
    return NULL;
}

void* DBusGetConnection(void* arg, FcitxModuleFunctionArg args)
{
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;
    return dbusmodule->conn;
}

void* DBusAddEventHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;
    FcitxDBusEventHandler handler;
    handler.eventHandler = args.args[0];
    handler.instance = args.args[1];
    utarray_push_back(&dbusmodule->handlers, &handler);
    return NULL;
}

void* DBusRemoveEventHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxDBus* dbusmodule = (FcitxDBus*)arg;
    FcitxDBusEventHandler* handler;
    int i = 0;
    for ( i = 0 ;
            i < utarray_len(&dbusmodule->handlers);
            i ++)
    {
        handler = (FcitxDBusEventHandler*) utarray_eltptr(&dbusmodule->handlers, i);
        if (handler->instance == args.args[0])
            break;
    }
    utarray_erase(&dbusmodule->handlers, i, 1);
    return NULL;
}