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
#include <libintl.h>

#include "fcitx/module.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"
#include "fcitx-utils/uthash.h"
#include "fcitx-utils/stringmap.h"
#include "fcitx-utils/desktop-parse.h"
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

typedef enum {
    NOTIFY_SENT,
    NOTIFY_TO_BE_REMOVE,
} FcitxNotifyState;

typedef struct _FcitxNotify FcitxNotify;

typedef struct {
    UT_hash_handle intern_hh;
    uint32_t intern_id;
    UT_hash_handle global_hh;
    uint32_t global_id;
    time_t time;
    int32_t ref_count;
    FcitxNotify *owner;
    FcitxNotifyState state;

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
    FcitxDesktopFile dconfig;
    FcitxStringMap *hide_notify;
};

#define TIMEOUT_REAL_TIME (100)
#define TIMEOUT_ADD_TIME (TIMEOUT_REAL_TIME + 10)

static time_t
FcitxNotifyGetTime()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec;
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

static FcitxNotifyItem*
FcitxNotifyFindByInternId(FcitxNotify *notify, uint32_t intern_id)
{
    FcitxNotifyItem *res = NULL;
    if (!intern_id)
        return NULL;
    HASH_FIND(intern_hh, notify->intern_table, &intern_id,
              sizeof(uint32_t), res);
    return res;
}

static void
FcitxNotifyItemRemoveInternal(FcitxNotify *notify, FcitxNotifyItem *item)
{
    if (item->intern_id) {
        HASH_DELETE(intern_hh, notify->intern_table, item);
        item->intern_id = 0;
    }
}

static void
FcitxNotifyItemRemoveGlobal(FcitxNotify *notify, FcitxNotifyItem *item)
{
    if (item->global_id) {
        HASH_DELETE(global_hh, notify->global_table, item);
        item->global_id = 0;
    }
}

static void
FcitxNotifyItemUnref(FcitxNotifyItem *item)
{
    if (fcitx_utils_atomic_add(&item->ref_count, -1) > 1)
        return;
    FcitxNotify *notify = item->owner;
    FcitxNotifyItemRemoveInternal(notify, item);
    FcitxNotifyItemRemoveGlobal(notify, item);
    if (item->free_func)
        item->free_func(item->data);
    free(item);
}

static void
FcitxNotifyItemAddInternal(FcitxNotify *notify, FcitxNotifyItem *item)
{
    if (item->intern_id) {
        FcitxNotifyItem *old_item =
            FcitxNotifyFindByInternId(notify, item->intern_id);
        // VERY VERY unlikely.
        if (fcitx_unlikely(old_item)) {
            FcitxNotifyItemRemoveInternal(notify, old_item);
            FcitxNotifyItemUnref(old_item);
        }
        HASH_ADD(intern_hh, notify->intern_table, intern_id,
                 sizeof(uint32_t), item);
    }
}

static void
FcitxNotifyItemAddGlobal(FcitxNotify *notify, FcitxNotifyItem *item)
{
    if (item->global_id) {
        FcitxNotifyItem *old_item =
            FcitxNotifyFindByGlobalId(notify, item->global_id);
        if (fcitx_unlikely(old_item))
            FcitxNotifyItemRemoveGlobal(notify, old_item);
        HASH_ADD(global_hh, notify->global_table, global_id,
                 sizeof(uint32_t), item);
    }
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
            if (fcitx_likely(item)) {
                FcitxNotifyItemRemoveGlobal(notify, item);
                FcitxNotifyItemUnref(item);
            }
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
FcitxNotifyLoadDConfig(FcitxNotify *notify)
{
    FILE *fp;
    fcitx_string_map_clear(notify->hide_notify);
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-notify.config",
                                       "r", NULL);
    if (fp) {
        if (fcitx_desktop_file_load_fp(&notify->dconfig, fp)) {
            FcitxDesktopGroup *grp;
            grp = fcitx_desktop_file_ensure_group(&notify->dconfig,
                                                  "Notify/Notify");
            FcitxDesktopEntry *ety;
            ety = fcitx_desktop_group_ensure_entry(grp, "HiddenNotify");
            if (ety->value) {
                fcitx_string_map_from_string(notify->hide_notify,
                                             ety->value, ';');
            }
        }
        fclose(fp);
    }
}

static void
FcitxNotifySaveDConfig(FcitxNotify *notify)
{
    FILE *fp;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-notify.config",
                                       "w", NULL);
    if (fp) {
        FcitxDesktopGroup *grp;
        grp = fcitx_desktop_file_ensure_group(&notify->dconfig,
                                              "Notify/Notify");
        FcitxDesktopEntry *ety;
        ety = fcitx_desktop_group_ensure_entry(grp, "HiddenNotify");
        char *val = fcitx_string_map_to_string(notify->hide_notify, ';');
        fcitx_desktop_entry_set_value(ety, val);
        free(val);
        fcitx_desktop_file_write_fp(&notify->dconfig, fp);
        fclose(fp);
    }
}

static void*
FcitxNotifyCreate(FcitxInstance *instance)
{
    FcitxNotify *notify = fcitx_utils_new(FcitxNotify);
    notify->owner = instance;
    notify->notify_counter = 1;
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

    notify->hide_notify = fcitx_string_map_new(NULL, '\0');
    fcitx_desktop_file_init(&notify->dconfig, NULL, NULL);
    FcitxNotifyLoadDConfig(notify);

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
_FcitxNotifyMarkNotifyRemove(FcitxNotify *notify, FcitxNotifyItem *item)
{
    item->state = NOTIFY_TO_BE_REMOVE;
}

static void
_FcitxNotifyCloseNotification(FcitxNotify *notify, FcitxNotifyItem *item)
{
    DBusMessage *msg =
        dbus_message_new_method_call(NOTIFICATIONS_SERVICE_NAME,
                                     NOTIFICATIONS_PATH,
                                     NOTIFICATIONS_INTERFACE_NAME,
                                     "CloseNotification");
    dbus_message_append_args(msg,
                             DBUS_TYPE_UINT32, &item->global_id,
                             DBUS_TYPE_INVALID);
    dbus_connection_send(notify->conn, msg, NULL);
    dbus_message_unref(msg);
    FcitxNotifyItemRemoveGlobal(notify, item);
    FcitxNotifyItemUnref(item);
}

static void
FcitxNotifyCloseNotification(FcitxNotify *notify, uint32_t intern_id)
{
    FcitxNotifyItem *item = FcitxNotifyFindByInternId(notify, intern_id);
    if (!item)
        return;
    if (!item->global_id) {
        _FcitxNotifyMarkNotifyRemove(notify, item);
    } else {
        _FcitxNotifyCloseNotification(notify, item);
    }
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
        dbus_message_unref(msg);
        dbus_error_free(&error);
        item->global_id = id;
        FcitxNotifyItemAddGlobal(notify, item);
        if (item->state == NOTIFY_TO_BE_REMOVE) {
            _FcitxNotifyCloseNotification(notify, item);
        }
    }
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
            /**
             * Remove from internal id table first so that it will not be
             * unref'ed here if it has not been unref'ed by libdbus.
             **/
            FcitxNotifyItemRemoveInternal(notify, item);
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
    FcitxNotifyItem *replace_item =
        FcitxNotifyFindByInternId(notify, replaceId);
    if (!replace_item) {
        replaceId = 0;
    } else {
        replaceId = replace_item->global_id;
        if (!replace_item->global_id) {
            _FcitxNotifyMarkNotifyRemove(notify, replace_item);
        } else {
            FcitxNotifyItemRemoveGlobal(notify, replace_item);
            FcitxNotifyItemUnref(replace_item);
        }
    }
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
        dbus_connection_send_with_reply(notify->conn, msg, &call,
                                        TIMEOUT_REAL_TIME * 1000 / 2);
    dbus_message_unref(msg);

    if (!reply)
        return 0;

    uint32_t intern_id;
    while (fcitx_unlikely((intern_id = fcitx_utils_atomic_add(
                               (int32_t*)&notify->notify_counter, 1)) == 0)) {
    }
    FcitxNotifyItem *item = fcitx_utils_new(FcitxNotifyItem);
    item->intern_id = intern_id;
    item->time = FcitxNotifyGetTime();
    item->free_func = freeFunc;
    item->callback = callback;
    item->data = userData;
    // One for pending notify callback, one for table
    item->ref_count = 2;
    item->owner = notify;

    FcitxNotifyItemAddInternal(notify, item);
    dbus_pending_call_set_notify(call, FcitxNotifyCallback, item,
                                 (DBusFreeFunction)FcitxNotifyItemUnref);
    dbus_pending_call_unref(call);
    FcitxNotifyCheckTimeout(notify);
    return intern_id;
}

typedef struct {
    FcitxNotify *notify;
    char tip_id[0];
} FcitxNotifyShowTipData;

static void
FcitxNotifyShowTipCallback(void *arg, uint32_t id, const char *action)
{
    FcitxNotifyShowTipData *data = arg;
    FCITX_UNUSED(id);
    if (!strcmp(action, "dont-show")) {
        fcitx_string_map_set(data->notify->hide_notify,
                             data->tip_id, true);
    }
}

static void
FcitxNotifyShowTip(FcitxNotify *notify, const char *appName,
                   const char *appIcon, const char *summary, const char *body,
                   int32_t timeout, const char *tip_id)
{
    if (fcitx_unlikely(!tip_id) ||
        fcitx_string_map_get(notify->hide_notify, tip_id, false))
        return;
    fcitx_string_map_set(notify->hide_notify, tip_id, false);
    const FcitxFreedesktopNotifyAction actions[] = {
        {
            .id = "dont-show",
            .name = _("Do not show again."),
        }, {
            NULL, NULL
        }
    };
    size_t len = strlen(tip_id);
    FcitxNotifyShowTipData *data =
        malloc(sizeof(FcitxNotifyShowTipData) + len + 1);
    data->notify = notify;
    memcpy(data->tip_id, tip_id, len + 1);
    FcitxNotifySendNotification(notify, appName, 0, appIcon, summary,
                                body, actions, timeout,
                                FcitxNotifyShowTipCallback,
                                data, free);
}

static void
FcitxNotifyDestroy(void *arg)
{
    FcitxNotify *notify = (FcitxNotify*)arg;

    FcitxNotifySaveDConfig(notify);
    dbus_connection_remove_filter(notify->conn, FcitxNotifyDBusFilter, notify);
    dbus_bus_remove_match(notify->conn, NOTIFICATIONS_MATCH_ACTION, NULL);
    dbus_bus_remove_match(notify->conn, NOTIFICATIONS_MATCH_CLOSED, NULL);
    fcitx_string_map_free(notify->hide_notify);
    fcitx_desktop_file_done(&notify->dconfig);
    free(arg);
}

#include "fcitx-freedesktop-notify-addfunctions.h"
