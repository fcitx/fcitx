/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#include <dbus/dbus.h>
#include "config.h"
#include "fcitx/fcitx.h"

#include "libintl.h"
#include "module/dbus/fcitx-dbus.h"
#include "module/dbusstuff/property.h"
#include "notificationitem.h"
#include "notificationitem_p.h"
#include "fcitx-utils/log.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"

const char * _notification_item =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<node name=\"/StatusNotifierItem\">"
"<interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">"
"<method name=\"Introspect\">"
"<arg name=\"data\" direction=\"out\" type=\"s\"/>"
"</method>"
"</interface>"
"<interface name=\"" DBUS_INTERFACE_PROPERTIES "\">"
"<method name=\"Get\">"
"<arg name=\"interface_name\" direction=\"in\" type=\"s\"/>"
"<arg name=\"property_name\" direction=\"in\" type=\"s\"/>"
"<arg name=\"value\" direction=\"out\" type=\"v\"/>"
"</method>"
"<method name=\"Set\">"
"<arg name=\"interface_name\" direction=\"in\" type=\"s\"/>"
"<arg name=\"property_name\" direction=\"in\" type=\"s\"/>"
"<arg name=\"value\" direction=\"in\" type=\"v\"/>"
"</method>"
"<method name=\"GetAll\">"
"<arg name=\"interface_name\" direction=\"in\" type=\"s\"/>"
"<arg name=\"values\" direction=\"out\" type=\"a{sv}\"/>"
"</method>"
"<signal name=\"PropertiesChanged\">"
"<arg name=\"interface_name\" type=\"s\"/>"
"<arg name=\"changed_properties\" type=\"a{sv}\"/>"
"<arg name=\"invalidated_properties\" type=\"as\"/>"
"</signal>"
"</interface>"
"<interface name=\"org.kde.StatusNotifierItem\">"
"<property name=\"Id\" type=\"s\" access=\"read\" />"
"<property name=\"Category\" type=\"s\" access=\"read\" />"
"<property name=\"Status\" type=\"s\" access=\"read\" />"
"<property name=\"IconName\" type=\"s\" access=\"read\" />"
"<property name=\"AttentionIconName\" type=\"s\" access=\"read\" />"
"<property name=\"Title\" type=\"s\" access=\"read\" />"
"<property access=\"read\" type=\"(sa(iiay)ss)\" name=\"ToolTip\" />"
"<property name=\"IconThemePath\" type=\"s\" access=\"read\" />"
"<property name=\"Menu\" type=\"o\" access=\"read\" />"
"<property name=\"XAyatanaLabel\" type=\"s\" access=\"read\" />"
"<property name=\"XAyatanaLabelGuide\" type=\"s\" access=\"read\" />"
"<property name=\"XAyatanaOrderingIndex\" type=\"u\" access=\"read\" />"
"<method name=\"Scroll\">"
"<arg type=\"i\" name=\"delta\" direction=\"in\" />"
"<arg type=\"s\" name=\"orientation\" direction=\"in\" />"
"</method>"
"<method name=\"Activate\">"
"<arg type=\"i\" name=\"x\" direction=\"in\" />"
"<arg type=\"i\" name=\"y\" direction=\"in\" />"
"</method>"
"<method name=\"SecondaryActivate\">"
"<arg type=\"i\" name=\"x\" direction=\"in\" />"
"<arg type=\"i\" name=\"y\" direction=\"in\" />"
"</method>"
"<signal name=\"NewIcon\">"
"</signal>"
"<signal name=\"NewToolTip\">"
"</signal>"
"<signal name=\"NewIconThemePath\">"
"<arg type=\"s\" name=\"icon_theme_path\" direction=\"out\" />"
"</signal>"
"<signal name=\"NewAttentionIcon\">"
"</signal>"
"<signal name=\"NewStatus\">"
"<arg type=\"s\" name=\"status\" direction=\"out\" />"
"</signal>"
"<signal name=\"NewTitle\">"
"</signal>"
"<signal name=\"XAyatanaNewLabel\">"
"<arg type=\"s\" name=\"label\" direction=\"out\" />"
"<arg type=\"s\" name=\"guide\" direction=\"out\" />"
"</signal>"
"</interface>"
"</node>"
;

#define NOTIFICATION_WATCHER_DBUS_ADDR    "org.kde.StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_OBJ     "/StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_IFACE   "org.kde.StatusNotifierWatcher"

#define NOTIFICATION_ITEM_DBUS_IFACE      "org.kde.StatusNotifierItem"
#define NOTIFICATION_ITEM_DEFAULT_OBJ     "/StatusNotifierItem"

#ifndef DBUS_TIMEOUT_USE_DEFAULT
#  define DBUS_TIMEOUT_USE_DEFAULT (-1)
#endif

static void* FcitxNotificationItemCreate(struct _FcitxInstance* instance);
static void FcitxNotificationItemDestroy(void* arg);

FCITX_DEFINE_PLUGIN(fcitx_notificationitem, module, FcitxModule) = {
    FcitxNotificationItemCreate,
    FcitxNotificationItemDestroy,
    NULL,
    NULL,
    NULL
};

static DBusHandlerResult FcitxNotificationItemEventHandler (DBusConnection  *connection,
                                                            DBusMessage     *message,
                                                            void            *user_data);

static void NotificationWatcherServiceExistCallback(DBusPendingCall *call, void *data);
static void FcitxNotificationItemRegisterSuccess(DBusPendingCall *call, void *data);

static void FcitxNotificationItemOwnerChanged(void* user_data, void* arg, const char* serviceName, const char* oldName, const char* newName);

static void FcitxNotificationItemGetId(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetCategory(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetStatus(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetIconName(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetAttentionIconName(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetTitle(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetIconThemePath(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetMenu(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetToolTip(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetXAyatanaLabel(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetXAyatanaLabelGuide(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetXAyatanaOrderingIndex(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemIMChanged(void* arg);
static void FcitxNotificationItemUpdateIMList(void* arg);
static boolean FcitxNotificationItemEnable(FcitxNotificationItem* notificationitem, FcitxNotificationItemAvailableCallback callback, void *data);
static void FcitxNotificationItemDisable(FcitxNotificationItem* notificationitem);
static void FcitxNotificationItemSetAvailable(FcitxNotificationItem* notificationitem, boolean available);


const FcitxDBusPropertyTable propertTable[] = {
    { NOTIFICATION_ITEM_DBUS_IFACE, "Id", "s", FcitxNotificationItemGetId, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "Category", "s", FcitxNotificationItemGetCategory, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "Status", "s", FcitxNotificationItemGetStatus, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "IconName", "s", FcitxNotificationItemGetIconName, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "AttentionIconName", "s", FcitxNotificationItemGetAttentionIconName, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "Title", "s", FcitxNotificationItemGetTitle, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "IconThemePath", "s", FcitxNotificationItemGetIconThemePath, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "Menu", "o", FcitxNotificationItemGetMenu, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "ToolTip", "(sa(iiay)ss)", FcitxNotificationItemGetToolTip, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "XAyatanaLabel", "s", FcitxNotificationItemGetXAyatanaLabel, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "XAyatanaLabelGuide", "s", FcitxNotificationItemGetXAyatanaLabelGuide, NULL },
    { NOTIFICATION_ITEM_DBUS_IFACE, "XAyatanaOrderingIndex", "u", FcitxNotificationItemGetXAyatanaOrderingIndex, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

DECLARE_ADDFUNCTIONS(NotificationItem)

void* FcitxNotificationItemCreate(FcitxInstance* instance)
{
    FcitxNotificationItem* notificationitem = fcitx_utils_new(FcitxNotificationItem);
    notificationitem->owner = instance;
    DBusError err;
    dbus_error_init(&err);
    do {
        DBusConnection *conn = FcitxDBusGetConnection(instance);
        if (conn == NULL) {
            FcitxLog(ERROR, "DBus Not initialized");
            break;
        }

        notificationitem->conn = conn;

        DBusObjectPathVTable fcitxIPCVTable = {NULL, &FcitxNotificationItemEventHandler, NULL, NULL, NULL, NULL };
        if (!dbus_connection_register_object_path(notificationitem->conn, NOTIFICATION_ITEM_DEFAULT_OBJ, &fcitxIPCVTable, notificationitem)) {
            FcitxLog(ERROR, "No memory");
            break;
        }

        if (!FcitxDBusMenuCreate(notificationitem)) {
            FcitxLog(ERROR, "No memory");
            break;
        }


        int id = FcitxDBusWatchName(instance,
                                    NOTIFICATION_WATCHER_DBUS_ADDR,
                                    notificationitem,
                                    FcitxNotificationItemOwnerChanged,
                                    NULL,
                                    NULL);

        if (id == 0) {
            break;
        }

        const char* notificationWatcher = NOTIFICATION_WATCHER_DBUS_ADDR;
        DBusMessage* message = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "NameHasOwner");
        dbus_message_append_args(message, DBUS_TYPE_STRING, &notificationWatcher, DBUS_TYPE_INVALID);

        DBusPendingCall* call = NULL;
        dbus_bool_t reply =
            dbus_connection_send_with_reply(notificationitem->conn, message,
                                            &call, DBUS_TIMEOUT_USE_DEFAULT);
        if (reply == TRUE) {
            dbus_pending_call_set_notify(call,
                                         NotificationWatcherServiceExistCallback,
                                         notificationitem,
                                         NULL);
            dbus_pending_call_unref(call);
        }
        dbus_connection_flush(notificationitem->conn);
        dbus_message_unref(message);

        FcitxIMEventHook hk;
        hk.arg = notificationitem;
        hk.func = FcitxNotificationItemIMChanged;
        FcitxInstanceRegisterIMChangedHook(instance, hk);
        FcitxInstanceRegisterInputFocusHook(instance, hk);
        FcitxInstanceRegisterInputUnFocusHook(instance, hk);

        hk.func = FcitxNotificationItemUpdateIMList;
        FcitxInstanceRegisterUpdateIMListHook(instance, hk);

        dbus_error_free(&err);

        FcitxNotificationItemAddFunctions(instance);

        return notificationitem;
    } while(0);

    dbus_error_free(&err);
    FcitxNotificationItemDestroy(notificationitem);

    return NULL;
}

void FcitxNotificationItemDestroy(void* arg)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;
    if (notificationitem->conn) {
        dbus_connection_unregister_object_path(notificationitem->conn, NOTIFICATION_ITEM_DEFAULT_OBJ);
        dbus_connection_unregister_object_path(notificationitem->conn, "/MenuBar");
    }

    free(notificationitem);
}

void FcitxNotificationItemRegister(FcitxNotificationItem* notificationitem)
{
    if (!notificationitem->serviceName) {
        FcitxLog(ERROR, "This should not happen, please report bug.");
        return;
    }

    DBusMessage *message =
        dbus_message_new_method_call(NOTIFICATION_WATCHER_DBUS_ADDR,
                                     NOTIFICATION_WATCHER_DBUS_OBJ,
                                     NOTIFICATION_WATCHER_DBUS_IFACE,
                                     "RegisterStatusNotifierItem");
    dbus_message_append_args(message,
                             DBUS_TYPE_STRING, &notificationitem->serviceName,
                             DBUS_TYPE_INVALID);

    DBusPendingCall *call = NULL;
    dbus_bool_t reply =
        dbus_connection_send_with_reply(notificationitem->conn, message,
                                        &call, DBUS_TIMEOUT_USE_DEFAULT);
    dbus_message_unref(message);
    if (reply == TRUE) {
        dbus_pending_call_set_notify(call,
                                     FcitxNotificationItemRegisterSuccess,
                                     notificationitem,
                                     NULL);
        dbus_pending_call_unref(call);
    }
}

void FcitxNotificationItemRegisterSuccess(DBusPendingCall *call, void *data)
{
    FcitxNotificationItem *notificationitem = (FcitxNotificationItem*)data;
    if (notificationitem->callback) {
        notificationitem->callback(notificationitem->data, true);
    }
}

void NotificationWatcherServiceExistCallback(DBusPendingCall *call, void *data)
{
    FcitxNotificationItem *notificationitem = (FcitxNotificationItem*)data;
    DBusMessage *msg = dbus_pending_call_steal_reply(call);

    if (msg) {
        dbus_bool_t has = FALSE;
        DBusError error;
        dbus_error_init(&error);
        dbus_message_get_args(msg, &error, DBUS_TYPE_BOOLEAN, &has,
                              DBUS_TYPE_INVALID);
        dbus_message_unref(msg);
        dbus_error_free(&error);
        FcitxNotificationItemSetAvailable(notificationitem, has);
    }
}

DBusHandlerResult FcitxNotificationItemEventHandler (DBusConnection  *connection,
                                            DBusMessage     *message,
                                            void            *user_data)
{
    FcitxNotificationItem* notificationitem = user_data;
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage *reply = NULL;
    boolean flush = false;
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(message);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &_notification_item, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(message, NOTIFICATION_ITEM_DBUS_IFACE, "Scroll")) {
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, NOTIFICATION_ITEM_DBUS_IFACE, "Activate")) {
        FcitxInstanceChangeIMState(notificationitem->owner, FcitxInstanceGetCurrentIC(notificationitem->owner));
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, NOTIFICATION_ITEM_DBUS_IFACE, "SecondaryActivate")) {
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Get")) {
        reply = FcitxDBusPropertyGet(notificationitem, propertTable, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Set")) {
        reply = FcitxDBusPropertySet(notificationitem, propertTable, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "GetAll")) {
        reply = FcitxDBusPropertyGetAll(notificationitem, propertTable, message);
    }

    if (reply) {
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        if (flush) {
            dbus_connection_flush(connection);
        }
        result = DBUS_HANDLER_RESULT_HANDLED;
    }
    return result;
}

char* FcitxNotificationItemGetIconNameString(FcitxNotificationItem* notificationitem)
{
    char* iconName = NULL;
    FcitxIM* im = FcitxInstanceGetCurrentIM(notificationitem->owner);
    const char* icon = "";
    if (im) {
        if (strncmp(im->uniqueName, "fcitx-keyboard-",
                    strlen("fcitx-keyboard-")) != 0) {
            icon = im->strIconName;
        } else {
            return strdup("input-keyboard");
        }
    }
    boolean result = CheckAddPrefix(&icon);
    fcitx_utils_alloc_cat_str(iconName, result ? "fcitx-" : "", icon);
    return iconName;
}

void FcitxNotificationItemGetId(void* arg, DBusMessageIter* iter)
{
    const char* id = "Fcitx";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &id);
}

void FcitxNotificationItemGetCategory(void* arg, DBusMessageIter* iter)
{
    const char* category = "SystemServices";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &category);
}

void FcitxNotificationItemGetStatus(void* arg, DBusMessageIter* iter)
{
    const char* active = "Active";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &active);
#if 0
    const char* active = "Active";
    const char* passive = "Passive";

    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;
    UT_array* imes = FcitxInstanceGetIMEs(notificationitem->owner);
    const char* status = (utarray_len(imes) > 1) ? active : passive;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &status);
#endif
}

void FcitxNotificationItemGetTitle(void* arg, DBusMessageIter* iter)
{
    const char* title = _("Input Method");
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &title);
}

void FcitxNotificationItemGetIconName(void* arg, DBusMessageIter* iter)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(notificationitem->owner);
    if (ic == NULL) {
        const char* iconName = "input-keyboard";
        dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &iconName);
    } else {
        char* iconName = FcitxNotificationItemGetIconNameString(notificationitem);
        dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &iconName);
        free(iconName);
    }
}

void FcitxNotificationItemGetAttentionIconName(void* arg, DBusMessageIter* iter)
{
    const char* iconName = "";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &iconName);
}

void FcitxNotificationItemGetIconThemePath(void* arg, DBusMessageIter* iter)
{
    const char* iconThemePath = "";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &iconThemePath);
}

void FcitxNotificationItemGetMenu(void* arg, DBusMessageIter* iter)
{
    const char* menu = "/MenuBar";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &menu);
}

void FcitxNotificationItemGetToolTip(void* arg, DBusMessageIter* iter)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;
    DBusMessageIter sub, ssub;
    char* iconNameToFree = NULL, *iconName, *title, *content;
    dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, 0, &sub);
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(notificationitem->owner);
    if (ic == NULL) {
        iconName = "input-keyboard";
        title = _("No input window");
        content = "";
    } else {
        iconName = FcitxNotificationItemGetIconNameString(notificationitem);
        iconNameToFree = iconName;
        FcitxIM* im = FcitxInstanceGetCurrentIM(notificationitem->owner);
        title = im ? im->strName : _("Disabled");
        content = im ? "" : _("Input Method Disabled");
    }
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &iconName);
    dbus_message_iter_open_container(&sub, DBUS_TYPE_ARRAY, "(iiay)", &ssub);
    dbus_message_iter_close_container(&sub, &ssub);
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &title);
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &content);
    dbus_message_iter_close_container(iter, &sub);

    fcitx_utils_free(iconNameToFree);
}

const char* FcitxNotificationItemGetLabel(FcitxNotificationItem* notificationitem)
{
    const char* label = "";
#if 0
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(notificationitem->owner);
    if (ic) {
        FcitxIM* im = FcitxInstanceGetCurrentIM(notificationitem->owner);
        if (im) {
            label = im->strName;
        }
    }
#endif
    return label;
}

void FcitxNotificationItemGetXAyatanaLabel(void* arg, DBusMessageIter* iter)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;
    const char* label = FcitxNotificationItemGetLabel(notificationitem);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &label);
}

void FcitxNotificationItemGetXAyatanaLabelGuide(void* arg, DBusMessageIter* iter)
{
    const char* label = "";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &label);
}

void FcitxNotificationItemGetXAyatanaOrderingIndex(void* arg, DBusMessageIter* iter)
{
    uint32_t index = 0;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &index);
}

void FcitxNotificationItemIMChanged(void* arg)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;

#define SEND_SIGNAL(NAME) do { \
    DBusMessage* msg = dbus_message_new_signal(NOTIFICATION_ITEM_DEFAULT_OBJ, \
                                               NOTIFICATION_ITEM_DBUS_IFACE, \
                                               NAME); \
    if (msg) { \
        dbus_connection_send(notificationitem->conn, msg, NULL); \
        dbus_message_unref(msg); \
    } \
    } while(0)

    SEND_SIGNAL("NewIcon");
    SEND_SIGNAL("NewToolTip");
#if 0
    do {
        DBusMessage* msg = dbus_message_new_signal(NOTIFICATION_ITEM_DEFAULT_OBJ,
                                                   NOTIFICATION_ITEM_DBUS_IFACE,
                                                   "XAyatanaNewLabel");
        if (msg) {
            const char* label = FcitxNotificationItemGetLabel(notificationitem);
            const char* guide = "XXXXXXXXXXXXXXX";
            dbus_message_append_args(msg,
                                     DBUS_TYPE_STRING, &label,
                                     DBUS_TYPE_STRING, &guide,
                                     DBUS_TYPE_INVALID);
            dbus_connection_send(notificationitem->conn, msg, NULL);
            dbus_message_unref(msg);
        }
    } while(0);
#endif
}

void FcitxNotificationItemUpdateIMList(void* arg)
{
    DBusMessage* msg = dbus_message_new_signal(NOTIFICATION_ITEM_DEFAULT_OBJ,
                                               NOTIFICATION_ITEM_DBUS_IFACE,
                                               "NewStatus");
    if (msg) {
        const char* active = "Active";
        dbus_message_append_args(msg, DBUS_TYPE_STRING, &active, DBUS_TYPE_INVALID);
#if 0
        FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;

        dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &active);
        const char* active = "Active";
        const char* passive = "Passive";

        UT_array* imes = FcitxInstanceGetIMEs(notificationitem->owner);
        const char* status = (utarray_len(imes) > 1) ? active : passive;
        dbus_message_append_args(msg, DBUS_TYPE_STRING, &status, DBUS_TYPE_INVALID);
        dbus_connection_send(notificationitem->conn, msg, NULL);
        dbus_message_unref(msg);
#endif
    }
}

boolean FcitxNotificationItemEnable(FcitxNotificationItem* notificationitem, FcitxNotificationItemAvailableCallback callback, void* data)
{
    if (!callback || notificationitem->callback)
        return false;
    if (notificationitem->serviceName) {
        FcitxLog(ERROR, "This should not happen, please report bug.");
        return false;
    }
    notificationitem->callback = callback;
    notificationitem->data = data;
    asprintf(&notificationitem->serviceName, "org.kde.StatusNotifierItem-%u-%d", getpid(), ++notificationitem->index);

    /* once we have name, request it first */
    DBusError err;
    dbus_error_init(&err);
    dbus_bus_request_name(notificationitem->conn, notificationitem->serviceName,
                                    DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                    &err);
    if (dbus_error_is_set(&err)) {
        FcitxLog(WARNING, "NotificationItem Name Error (%s)", err.message);
    }
    dbus_error_free(&err);

    if (notificationitem->available) {
        if (notificationitem->callback) {
            FcitxNotificationItemRegister(notificationitem);
        }
    }
    return true;
}

void FcitxNotificationItemDisable(FcitxNotificationItem* notificationitem)
{
    notificationitem->callback = NULL;
    notificationitem->data = NULL;

    if (notificationitem->serviceName) {
        dbus_bus_release_name(notificationitem->conn, notificationitem->serviceName, NULL);
        free(notificationitem->serviceName);
        notificationitem->serviceName = NULL;
    }
}

void FcitxNotificationItemOwnerChanged(void* user_data, void* arg, const char* serviceName, const char* oldName, const char* newName)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) user_data;
    /* old die and no new one */
    if (strcmp(serviceName, NOTIFICATION_WATCHER_DBUS_ADDR) != 0) {
        return;
    }

    FcitxNotificationItemSetAvailable (notificationitem, strlen(newName) > 0);
}

void FcitxNotificationItemSetAvailable(FcitxNotificationItem* notificationitem, boolean available)
{
    if (notificationitem->available != available) {
        notificationitem->available = available;
        if (available) {
            if (notificationitem->callback) {
                FcitxNotificationItemRegister(notificationitem);
            }
        } else {
            if (notificationitem->callback) {
                if (notificationitem->callback) {
                    notificationitem->callback(notificationitem->data, false);
                }
            }
        }
    }
}

#include "fcitx-notificationitem-addfunctions.h"
