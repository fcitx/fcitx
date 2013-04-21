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
#include <time.h>

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

typedef struct _FcitxNotify FcitxNotify;

typedef struct {
    UT_hash_handle intern_hh;
    uint32_t intern_id;
    UT_hash_handle global_hh;
    uint32_t global_id;
    time_t time;
    int32_t ref_count;
    FcitxNotify *owner;

    FcitxDestroyNotify free_func;
    FcitxFreedesktopNotifyActionCallback callback;
    void *data;
} FcitxNotifyItem;

struct _FcitxNotify {
    FcitxInstance *owner;
    DBusConnection *conn;
    uint32_t notify_counter;
    FcitxNotifyItem *global_table;
    FcitxNotifyItem *intern_table;
    boolean timeout_added;
};

static DBusHandlerResult
FcitxNotifyDBusFilter(DBusConnection *connection, DBusMessage *message,
                      void *user_data);

static void*
FcitxNotifyCreate(FcitxInstance *instance)
{
    FcitxNotify *notify = fcitx_utils_new(FcitxNotify);
    notify->conn = FcitxDBusGetConnection(notify->owner);
    if (fcitx_unlikely(!notify->conn))
        goto connect_error;
    notify->owner = instance;
    notify->notify_counter = 1;

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
FcitxNotifyItemUnref(FcitxNotifyItem *item)
{
    if (fcitx_utils_atomic_add(&item->ref_count, -1) >= 0)
        return;
    FcitxNotify *notify = item->owner;
    HASH_DELETE(intern_hh, notify->intern_table, item);
    if (item->global_id)
        HASH_DELETE(global_hh, notify->global_table, item);
    if (item->free_func)
        item->free_func(item->data);
    free(item);
}

static void
FcitxNotifyCallback(DBusPendingCall *call, void *data)
{
    FcitxNotifyItem *item = (FcitxNotifyItem*)data;
    if (item->global_id)
        return;
    FcitxNotify *notify = item->owner;
    DBusMessage *msg = dbus_pending_call_steal_reply(call);
    if (msg) {
        uint32_t id;
        DBusError error;
        dbus_error_init(&error);
        dbus_message_get_args(msg, &error, DBUS_TYPE_UINT32,
                              &id , DBUS_TYPE_INVALID);
        item->global_id = id;
        HASH_ADD(global_hh, notify->global_table, global_id,
                 sizeof(uint32_t), item);
    }
}

static FcitxNotifyItem*
FcitxNotifyFindByGlobalId(FcitxNotify *notify, uint32_t global_id)
{
    FcitxNotifyItem *res = NULL;
    if (!global_id)
        return NULL;
    HASH_FIND(global_hh, notify->global_table, &global_id,
              sizeof(uint32_t), res);
    return res;
}

static uint32_t
FcitxNotifyGetGlobalId(FcitxNotify *notify, uint32_t intern_id)
{
    FcitxNotifyItem *res = NULL;
    if (!intern_id)
        return 0;
    HASH_FIND(intern_hh, notify->intern_table, &intern_id,
              sizeof(uint32_t), res);
    if (res)
        return res->global_id;
    return 0;
}

#define TIMEOUT_REAL_TIME (100)
#define TIMEOUT_ADD_TIME (TIMEOUT_REAL_TIME + 10)

static time_t
FcitxNotifyGetTime()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec;
}

static void FcitxNotifyCheckTimeout(FcitxNotify *notify);

static void
FcitxNotifyTimeoutCb(void *data)
{
    FcitxNotify *notify = (FcitxNotify*)data;
    notify->timeout_added = false;
    FcitxNotifyCheckTimeout(notify);
}

static void
FcitxNotifyCheckTimeout(FcitxNotify *notify)
{
    time_t cur = FcitxNotifyGetTime();
    FcitxNotifyItem *item;
    FcitxNotifyItem *next;
    boolean left = false;
    time_t earliest = 0;
    for (item = notify->intern_table;item;item = next) {
        next = item->intern_hh.next;
        if ((int64_t)(cur - item->time) > TIMEOUT_REAL_TIME) {
            FcitxNotifyItemUnref(item);
        } else {
            if (!left) {
                earliest = item->time;
                left = true;
            } else {
                if ((int64_t)(item->time - earliest) < 0) {
                    earliest = item->time;
                }
            }
        }
    }
    if (notify->timeout_added || !left)
        return;
    FcitxInstanceAddTimeout(notify->owner,
                            (TIMEOUT_ADD_TIME + earliest - cur) * 1000,
                            FcitxNotifyTimeoutCb, notify);
}

static uint32_t
FcitxNotifySendNotification(FcitxNotify *notify, const char *appName,
                            uint32_t replaceId, const char *appIcon,
                            const char *summary, const char *body,
                            const FcitxFreedesktopNotifyAction *actions,
                            int32_t timeout,
                            FcitxFreedesktopNotifyActionCallback callback,
                            void *userData, FcitxDestroyNotify freeFunc)
{
    DBusMessage *msg =
        dbus_message_new_method_call(NOTIFICATIONS_SERVICE_NAME,
                                     NOTIFICATIONS_PATH,
                                     NOTIFICATIONS_INTERFACE_NAME,
                                     "Notify");
    if (!appName)
        appName = "fcitx";
    replaceId = FcitxNotifyGetGlobalId(notify, replaceId);
    if (!appIcon)
        appIcon = "fcitx";
    dbus_message_append_args(msg,
                             DBUS_TYPE_STRING, &appName,
                             DBUS_TYPE_UINT32, &replaceId,
                             DBUS_TYPE_STRING, &appIcon,
                             DBUS_TYPE_STRING, &summary,
                             DBUS_TYPE_STRING, &body,
                             DBUS_TYPE_INVALID);

    // append arguments onto signal
    DBusMessageIter args;
    dbus_message_iter_init_append(msg, &args);

    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &sub);
    if (actions) {
        for (;actions->id && actions->name;actions++) {
            dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING,
                                           &actions->id);
            dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING,
                                           &actions->name);
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
    dbus_message_unref(msg);

    if (!reply)
        return 0;

    uint32_t intern_id =
        fcitx_utils_atomic_add((int32_t*)&notify->notify_counter, 1);
    FcitxNotifyItem *item = fcitx_utils_new(FcitxNotifyItem);
    item->intern_id = intern_id;
    item->time = FcitxNotifyGetTime();
    item->free_func = freeFunc;
    item->callback = callback;
    item->data = userData;
    // One for pending notify callback, one for table
    item->ref_count = 2;
    item->owner = notify;

    HASH_ADD(intern_hh, notify->intern_table, intern_id,
             sizeof(uint32_t), item);
    dbus_pending_call_set_notify(call, FcitxNotifyCallback, item,
                                 (DBusFreeFunction)FcitxNotifyItemUnref);
    FcitxNotifyCheckTimeout(notify);
    return intern_id;
}

static DBusHandlerResult
FcitxNotifyDBusFilter(DBusConnection *connection, DBusMessage *message,
                      void *user_data)
{
    FcitxNotify *notify = (FcitxNotify*)user_data;
    if (dbus_message_is_signal(message, "org.freedesktop.Notifications",
                               "ActionInvoked")) {
        DBusError error;
        uint32_t id = 0;
        const char *key = NULL;
        dbus_error_init(&error);
        if (dbus_message_get_args(message, &error, DBUS_TYPE_UINT32,
                                  &id, DBUS_TYPE_STRING, &key,
                                  DBUS_TYPE_INVALID)) {
            FcitxNotifyItem *item = FcitxNotifyFindByGlobalId(notify, id);
            if (item && item->callback) {
                item->callback(item->data, item->intern_id, key);
            }
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(message, "org.freedesktop.Notifications",
                                      "NotificationClosed")) {
        DBusError error;
        uint32_t id = 0;
        uint32_t reason = 0;
        dbus_error_init(&error);
        if (dbus_message_get_args(message, &error, DBUS_TYPE_UINT32,
                                  &id, DBUS_TYPE_UINT32, &reason,
                                  DBUS_TYPE_INVALID)) {
            FcitxNotifyItem *item = FcitxNotifyFindByGlobalId(notify, id);
            FcitxNotifyItemUnref(item);
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
