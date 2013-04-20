/***************************************************************************
 *   Copyright (C) 2012~2013 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#include <dbus/dbus.h>

#include "fcitx/module.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"
#include "fcitx-utils/uthash.h"
#include "module/dbus/fcitx-dbus.h"
#include "freedesktop-notify.h"

#define NOTIFICATIONS_SERVICE_NAME "org.freedesktop.Notifications"
#define NOTIFICATIONS_INTERFACE_NAME "org.freedesktop.Notifications"
#define NOTIFICATIONS_PATH "/org/freedesktop/Notifications"
#define NOTIFICATIONS_MATCH_NAMES                   \
    "sender='" NOTIFICATIONS_SERVICE_NAME "',"      \
    "interface='" NOTIFICATIONS_INTERFACE_NAME "'," \
    "path='" NOTIFICATIONS_PATH "'"
#define NOTIFICATIONS_MATCH_SIGNAL              \
    "type='signal',"                            \
    NOTIFICATIONS_MATCH_NAMES
#define NOTIFICATIONS_MATCH_ACTION              \
    NOTIFICATIONS_MATCH_SIGNAL ","              \
    "member='ActionInvoked'"
#define NOTIFICATIONS_MATCH_CLOSED              \
    NOTIFICATIONS_MATCH_SIGNAL ","              \
    "member='NotificationClosed'"

static void *FcitxNotifyCreate(FcitxInstance *instance);
static void FcitxNotifyDestroy(void *arg);

DECLARE_ADDFUNCTIONS(FreeDesktopNotify)

FCITX_DEFINE_PLUGIN(fcitx_freedesktop_notify, module, FcitxModule) = {
    FcitxNotifyCreate,
    NULL,
    NULL,
    FcitxNotifyDestroy,
    NULL
};

typedef struct {
    FcitxInstance *owner;
    DBusConnection *conn;
    int notify_counter;
} FcitxNotify;

typedef struct {
    FcitxNotify *owner;
    FcitxFreedesktopNotifyCallback callback;
    FcitxDestroyNotify freeFunc;
    void *data;
} FcitxNotifyStruct;

static DBusHandlerResult
FcitxNotifyDBusFilter(DBusConnection *connection, DBusMessage *message,
                      void *user_data);

static void FcitxNotifySendNotification(
    FcitxNotify *notify,
    const char *appName,
    uint32_t replaceId,
    const char *appIcon,
    const char *summary,
    const char *body,
    const char **actions,
    int actionsCount,
    int32_t timeout,
    FcitxFreedesktopNotifyCallback callback,
    void *userData,
    FcitxDestroyNotify freeFunc);

static void*
FcitxNotifyCreate(FcitxInstance *instance)
{
    FcitxNotify *notify = fcitx_utils_new(FcitxNotify);
    notify->owner = instance;
    notify->conn = FcitxDBusGetConnection(notify->owner);
    if (fcitx_unlikely(!notify->conn))
        goto connect_error;

    DBusError err;
    dbus_error_init(&err);

    dbus_bus_add_match(notify->conn, NOTIFICATIONS_MATCH_ACTION, &err);
    if (fcitx_unlikely(dbus_error_is_set(&err)))
        goto filter_error;

    dbus_bus_add_match(notify->conn, NOTIFICATIONS_MATCH_CLOSED, &err);
    if (fcitx_unlikely(dbus_error_is_set(&err)))
        goto filter_error;

    if (fcitx_unlikely(!dbus_connection_add_filter(notify->conn,
                                                   FcitxNotifyDBusFilter,
                                                   notify, NULL)))
        goto filter_error;
    dbus_error_free(&err);

    FcitxFreeDesktopNotifyAddFunctions(instance);

    return notify;
filter_error:
    dbus_bus_remove_match(notify->conn, NOTIFICATIONS_MATCH_ACTION, NULL);
    dbus_bus_remove_match(notify->conn, NOTIFICATIONS_MATCH_CLOSED, NULL);
    dbus_error_free(&err);
connect_error:
    free(notify);
    return NULL;
}

static void
FcitxNotifyCallback(DBusPendingCall *call, void *data)
{
    FcitxNotifyStruct *s = (FcitxNotifyStruct*)data;

    DBusMessage *msg = dbus_pending_call_steal_reply(call);
    if (msg) {
        uint32_t id;
        DBusError error;
        dbus_error_init(&error);
        dbus_message_get_args(msg, &error, DBUS_TYPE_UINT32,
                              &id , DBUS_TYPE_INVALID);
        s->callback(s->data, id);
    }
}

static void
FcitxNotifyCallbackFree(void *arg)
{
    FcitxNotifyStruct *s = (FcitxNotifyStruct*)arg;
    s->freeFunc(s->data);
    free(s);
}

static void
FcitxNotifySendNotification(
    FcitxNotify *notify,
    const char *appName,
    uint32_t replaceId,
    const char *appIcon,
    const char *summary,
    const char *body,
    const char **actions,
    int actionsCount,
    int32_t timeout,
    FcitxFreedesktopNotifyCallback callback,
    void *userData,
    FcitxDestroyNotify freeFunc)
{
    if (actionsCount % 2 != 0)
        return;

    DBusMessage *msg =
        dbus_message_new_method_call(NOTIFICATIONS_SERVICE_NAME,
                                     NOTIFICATIONS_PATH,
                                     NOTIFICATIONS_INTERFACE_NAME,
                                     "Notify");
    dbus_message_append_args(
        msg,
        DBUS_TYPE_STRING, &appName,
        DBUS_TYPE_UINT32, &replaceId,
        DBUS_TYPE_STRING, &appIcon,
        DBUS_TYPE_STRING, &summary,
        DBUS_TYPE_STRING, &body,
        DBUS_TYPE_INVALID
        );

    // append arguments onto signal
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);

    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &sub);
    if (actions && actionsCount) {
        int i = 0;
        for (i = 0; i < actionsCount; i++) {
            dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &actions[i]);
        }
    }

    dbus_message_iter_close_container(&args, &sub);

    /* TODO: support hints */
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &sub);
    dbus_message_iter_close_container(&args, &sub);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &timeout);
    DBusPendingCall *call = NULL;
    dbus_bool_t reply =
        dbus_connection_send_with_reply(notify->conn, msg, &call, 0);

    if (callback) {
        if (reply) {
            FcitxNotifyStruct *s = fcitx_utils_new(FcitxNotifyStruct);
            s->owner = notify;
            s->callback = callback;
            s->data = userData;

            dbus_pending_call_set_notify(call,
                                         FcitxNotifyCallback,
                                         s,
                                         FcitxNotifyCallbackFree);
        } else {
            freeFunc(userData);
        }
    }
    dbus_message_unref(msg);
}

static DBusHandlerResult
FcitxNotifyDBusFilter(DBusConnection *connection, DBusMessage *message,
                      void *user_data)
{
    // FcitxNotify *notify = (FcitxNotify*)user_data;
    if (dbus_message_is_signal(message, "org.freedesktop.Notifications",
                               "ActionInvoked")) {
        DBusError error;
        uint32_t id = 0;
        const char *key = NULL;
        dbus_error_init(&error);
        if (dbus_message_get_args(message, &error, DBUS_TYPE_UINT32,
                                  &id, DBUS_TYPE_STRING, &key,
                                  DBUS_TYPE_INVALID)) {
            // TODO
            FcitxLog(INFO, "action invoked %u %s", id, key);
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void
FcitxNotifyDestroy(void *arg)
{
    FcitxNotify *notify = (FcitxNotify*)arg;
    dbus_connection_remove_filter(notify->conn, FcitxNotifyDBusFilter, notify);
    dbus_bus_remove_match(notify->conn, NOTIFICATIONS_MATCH_ACTION, NULL);
    dbus_bus_remove_match(notify->conn, NOTIFICATIONS_MATCH_CLOSED, NULL);
    free(arg);
}

#include "fcitx-freedesktop-notify-addfunctions.h"
