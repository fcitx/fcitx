/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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
#include "notificationitem.h"
#include "fcitx-utils/log.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"

const char * _notification_item =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<node name=\"/StatusNotifierItem\">\n"
"  <interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">\n"
"    <method name=\"Introspect\">\n"
"      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"" DBUS_INTERFACE_PROPERTIES "\">\n"
"    <method name=\"Get\">\n"
"      <arg name=\"interface_name\" direction=\"in\" type=\"s\"/>\n"
"      <arg name=\"property_name\" direction=\"in\" type=\"s\"/>\n"
"      <arg name=\"value\" direction=\"out\" type=\"v\"/>\n"
"    </method>\n"
"    <method name=\"Set\">\n"
"      <arg name=\"interface_name\" direction=\"in\" type=\"s\"/>\n"
"      <arg name=\"property_name\" direction=\"in\" type=\"s\"/>\n"
"      <arg name=\"value\" direction=\"in\" type=\"v\"/>\n"
"    </method>\n"
"    <method name=\"GetAll\">\n"
"      <arg name=\"interface_name\" direction=\"in\" type=\"s\"/>\n"
"      <arg name=\"values\" direction=\"out\" type=\"a{sv}\"/>\n"
"    </method>\n"
"    <signal name=\"PropertiesChanged\">\n"
"      <arg name=\"interface_name\" type=\"s\"/>\n"
"      <arg name=\"changed_properties\" type=\"a{sv}\"/>\n"
"      <arg name=\"invalidated_properties\" type=\"as\"/>\n"
"    </signal>\n"
"  </interface>\n"
"   <interface name=\"org.kde.StatusNotifierItem\">\n"
"\n"
"<!-- Properties -->\n"
"       <property name=\"Id\" type=\"s\" access=\"read\" />\n"
"       <property name=\"Category\" type=\"s\" access=\"read\" />\n"
"       <property name=\"Status\" type=\"s\" access=\"read\" />\n"
"       <property name=\"IconName\" type=\"s\" access=\"read\" />\n"
"       <property name=\"AttentionIconName\" type=\"s\" access=\"read\" />\n"
"       <property name=\"Title\" type=\"s\" access=\"read\" />\n"
"       <property access=\"read\" type=\"(sa(iiay)ss)\" name=\"ToolTip\" />\n"
"       <!-- An additional path to add to the theme search path\n"
"            to find the icons specified above. -->\n"
"       <property name=\"IconThemePath\" type=\"s\" access=\"read\" />\n"
"       <property name=\"Menu\" type=\"o\" access=\"read\" />\n"
"\n"
"<!-- Methods -->\n"
"       <method name=\"Scroll\">\n"
"           <arg type=\"i\" name=\"delta\" direction=\"in\" />\n"
"           <arg type=\"s\" name=\"orientation\" direction=\"in\" />\n"
"       </method>\n"
"       <method name=\"SecondaryActivate\">\n"
"           <arg type=\"i\" name=\"x\" direction=\"in\" />\n"
"           <arg type=\"i\" name=\"y\" direction=\"in\" />\n"
"       </method>\n"
"\n"
"<!-- Signals -->\n"
"       <signal name=\"NewIcon\">\n"
"       </signal>\n"
"       <signal name=\"NewToolTip\">\n"
"       </signal>\n"
"       <signal name=\"NewIconThemePath\">\n"
"           <arg type=\"s\" name=\"icon_theme_path\" direction=\"out\" />\n"
"       </signal>\n"
"       <signal name=\"NewAttentionIcon\">\n"
"       </signal>\n"
"       <signal name=\"NewStatus\">\n"
"           <arg type=\"s\" name=\"status\" direction=\"out\" />\n"
"       </signal>\n"
"       <signal name=\"NewTitle\">\n"
"       </signal>\n"
"\n"
"   </interface>\n"
"</node>\n"
;

#define NOTIFICATION_WATCHER_DBUS_ADDR    "org.kde.StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_OBJ     "/StatusNotifierWatcher"
#define NOTIFICATION_WATCHER_DBUS_IFACE   "org.kde.StatusNotifierWatcher"

#define NOTIFICATION_ITEM_DBUS_IFACE      "org.kde.StatusNotifierItem"
#define NOTIFICATION_ITEM_DEFAULT_OBJ     "/StatusNotifierItem"

static void* FcitxNotificationItemCreate(struct _FcitxInstance* instance);
typedef struct _FcitxNotificationItem {
    FcitxInstance* owner;
    DBusConnection* conn;
    FcitxNotificationItemAvailableCallback callback;
    void* data;
    boolean available;
    int index;
    char* serviceName;
} FcitxNotificationItem;

FCITX_DEFINE_PLUGIN(fcitx_notificationitem, module, FcitxModule) = {
    FcitxNotificationItemCreate,
    NULL,
    NULL,
    NULL,
    NULL
};


static inline DBusMessage* FcitxNotificationItemUnknownMethod(DBusMessage *msg)
{
    DBusMessage* reply = dbus_message_new_error_printf(msg,
                                                       DBUS_ERROR_UNKNOWN_METHOD,
                                                       "No such method with signature (%s)",
                                                       dbus_message_get_signature(msg));
    return reply;
}

static DBusHandlerResult FcitxNotificationItemEventHandler (DBusConnection  *connection,
                                                            DBusMessage     *message,
                                                            void            *user_data);

static void NotificationWatcherServiceExistCallback(DBusPendingCall *call, void *data);
static void FcitxNotificationItemRegisterSuccess(DBusPendingCall *call, void *data);

typedef struct _FcitxDBusPropertyTable {
    char* interface;
    char* name;
    char* type;
    void (*getfunc)(void* arg, DBusMessageIter* iter);
    void (*setfunc)(void* arg, DBusMessageIter* iter);
} FcitxDBusPropertyTable;

static void FcitxNotificationItemGetId(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetCategory(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetStatus(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetIconName(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetAttentionIconName(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetTitle(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetIconThemePath(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetMenu(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemGetToolTip(void* arg, DBusMessageIter* iter);
static void FcitxNotificationItemIMChanged(void* arg);
static boolean FcitxNotificationItemEnable(FcitxNotificationItem* notificationitem, FcitxNotificationItemAvailableCallback callback, void *data);
static void FcitxNotificationItemDisable(FcitxNotificationItem* notificationitem);
static DBusHandlerResult FcitxNotificationItemDBusFilter(DBusConnection* connection, DBusMessage* msg, void* user_data);
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

        if (!dbus_connection_add_filter(notificationitem->conn, FcitxNotificationItemDBusFilter, notificationitem, NULL)) {
            FcitxLog(ERROR, "No memory");
            break;
        }

        DBusObjectPathVTable fcitxIPCVTable = {NULL, &FcitxNotificationItemEventHandler, NULL, NULL, NULL, NULL };
        dbus_connection_register_object_path(notificationitem->conn, NOTIFICATION_ITEM_DEFAULT_OBJ, &fcitxIPCVTable, notificationitem);

        dbus_bus_add_match(notificationitem->conn,
                           "type='signal',"
                           "interface='" DBUS_INTERFACE_DBUS "',"
                           "path='" DBUS_PATH_DBUS "',"
                           "member='NameOwnerChanged',"
                           "arg0='" NOTIFICATION_WATCHER_DBUS_ADDR "'",
                           &err);
        dbus_connection_flush(notificationitem->conn);
        if (dbus_error_is_set(&err)) {
            FcitxLog(ERROR, "Match Error (%s)", err.message);
            break;
        }

        const char* notificationWatcher = NOTIFICATION_WATCHER_DBUS_ADDR;
        DBusMessage* message = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "NameHasOwner");
        dbus_message_append_args(message, DBUS_TYPE_STRING, &notificationWatcher, DBUS_TYPE_INVALID);

        DBusPendingCall* call = NULL;
        dbus_bool_t reply = dbus_connection_send_with_reply(notificationitem->conn, message,
                                                            &call,
                                                            0);
        if (reply == TRUE) {
            dbus_pending_call_set_notify(call,
                                         NotificationWatcherServiceExistCallback,
                                         notificationitem,
                                         NULL);
        }
        dbus_connection_flush(notificationitem->conn);
        dbus_message_unref(message);

        FcitxIMEventHook hk;
        hk.arg = notificationitem;
        hk.func = FcitxNotificationItemIMChanged;
        FcitxInstanceRegisterIMChangedHook(instance, hk);
        FcitxInstanceRegisterInputFocusHook(instance, hk);
        FcitxInstanceRegisterInputUnFocusHook(instance, hk);

        dbus_error_free(&err);

        FcitxNotificationItemAddFunctions(instance);

        return notificationitem;
    } while(0);

    dbus_error_free(&err);
    free(notificationitem);

    return NULL;
}

void FcitxNotificationItemRegister(FcitxNotificationItem* notificationitem)
{
    if (!notificationitem->serviceName) {
        FcitxLog(ERROR, "This should not happen, please report bug.");
        return;
    }

    DBusMessage* message = dbus_message_new_method_call(NOTIFICATION_WATCHER_DBUS_ADDR,
                                                        NOTIFICATION_WATCHER_DBUS_OBJ,
                                                        NOTIFICATION_WATCHER_DBUS_IFACE,
                                                        "RegisterStatusNotifierItem");
    dbus_message_append_args(message, DBUS_TYPE_STRING, &notificationitem->serviceName, DBUS_TYPE_INVALID);

    DBusPendingCall* call = NULL;
    dbus_bool_t reply = dbus_connection_send_with_reply(notificationitem->conn, message,
                                                        &call,
                                                        0);
    if (reply == TRUE) {
        dbus_pending_call_set_notify(call,
                                     FcitxNotificationItemRegisterSuccess,
                                     notificationitem,
                                     NULL);
    }
}

void FcitxNotificationItemRegisterSuccess(DBusPendingCall *call, void *data)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) data;
    if (notificationitem->callback) {
        notificationitem->callback(notificationitem->data, true);
    }
}

void NotificationWatcherServiceExistCallback(DBusPendingCall *call, void *data)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) data;

    DBusMessage* msg = dbus_pending_call_steal_reply(call);

    if (msg) {
        dbus_bool_t has;
        DBusError error;
        dbus_error_init(&error);
        dbus_message_get_args(msg, &error, DBUS_TYPE_BOOLEAN, &has , DBUS_TYPE_INVALID);
        FcitxNotificationItemSetAvailable(notificationitem, has);
        dbus_message_unref(msg);
        dbus_error_free(&error);
    }

    dbus_pending_call_cancel(call);
    dbus_pending_call_unref(call);
}

DBusMessage* FcitxDBusPropertyGet(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    DBusError error;
    dbus_error_init(&error);
    char *interface;
    char *property;
    DBusMessage* reply = NULL;
    if (dbus_message_get_args(message, &error,
                              DBUS_TYPE_STRING, &interface,
                              DBUS_TYPE_STRING, &property,
                              DBUS_TYPE_INVALID)) {
        int index = 0;
        while (propertTable[index].interface != NULL) {
            if (strcmp(propertTable[index].interface, interface) == 0
                    && strcmp(propertTable[index].name, property) == 0)
                break;
            index ++;
        }

        if (propertTable[index].interface) {
            DBusMessageIter args, variant;
            reply = dbus_message_new_method_return(message);
            dbus_message_iter_init_append(reply, &args);
            dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, propertTable[index].type, &variant);
            if (propertTable[index].getfunc)
                propertTable[index].getfunc(notificationitem, &variant);
            dbus_message_iter_close_container(&args, &variant);
        }
        else {
            reply = dbus_message_new_error_printf(message, DBUS_ERROR_UNKNOWN_PROPERTY, "No such property ('%s.%s')", interface, property);
        }
    }
    else {
        reply = FcitxNotificationItemUnknownMethod(message);
    }

    return reply;
}

DBusMessage* FcitxDBusPropertySet(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    DBusError error;
    dbus_error_init(&error);
    char *interface;
    char *property;
    DBusMessage* reply = NULL;

    DBusMessageIter args, variant;
    dbus_message_iter_init(message, &args);

    if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING)
        goto dbus_property_set_end;

    dbus_message_iter_get_basic(&args, &interface);
    dbus_message_iter_next(&args);

    if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING)
        goto dbus_property_set_end;
    dbus_message_iter_get_basic(&args, &property);
    dbus_message_iter_next(&args);

    if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_VARIANT)
        goto dbus_property_set_end;

    dbus_message_iter_recurse(&args, &variant);
    int index = 0;
    while (propertTable[index].interface != NULL) {
        if (strcmp(propertTable[index].interface, interface) == 0
                && strcmp(propertTable[index].name, property) == 0)
            break;
        index ++;
    }
    if (propertTable[index].setfunc) {
        propertTable[index].setfunc(notificationitem, &variant);
        reply = dbus_message_new_method_return(message);
    }
    else {
        reply = dbus_message_new_error_printf(message, DBUS_ERROR_UNKNOWN_PROPERTY, "No such property ('%s.%s')", interface, property);
    }

dbus_property_set_end:
    if (!reply)
        reply = FcitxNotificationItemUnknownMethod(message);

    return reply;
}

DBusMessage* FcitxDBusPropertyGetAll(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    DBusError error;
    dbus_error_init(&error);
    char *interface;
    DBusMessage* reply = NULL;
    if (dbus_message_get_args(message, &error,
                              DBUS_TYPE_STRING, &interface,
                              DBUS_TYPE_INVALID)) {
        reply = dbus_message_new_method_return(message);
        int index = 0;
        DBusMessageIter args;
        dbus_message_iter_init_append(reply, &args);
        DBusMessageIter array, entry;
        dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &array);

        while (propertTable[index].interface != NULL) {
            if (strcmp(propertTable[index].interface, interface) == 0 && propertTable[index].getfunc) {
                dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY,
                                                 NULL, &entry);

                dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &propertTable[index].name);
                DBusMessageIter variant;
                dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, propertTable[index].type, &variant);
                propertTable[index].getfunc(notificationitem, &variant);
                dbus_message_iter_close_container(&entry, &variant);

                dbus_message_iter_close_container(&array, &entry);
            }
            index ++;
        }
        dbus_message_iter_close_container(&args, &array);
    }
    if (!reply)
        reply = FcitxNotificationItemUnknownMethod(message);

    return reply;
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
    } else if (dbus_message_is_method_call(message, NOTIFICATION_ITEM_DBUS_IFACE, "SecondaryActivate")) {
        reply = dbus_message_new_method_return(message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Get")) {
        reply = FcitxDBusPropertyGet(notificationitem, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Set")) {
        reply = FcitxDBusPropertySet(notificationitem, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "GetAll")) {
        reply = FcitxDBusPropertyGetAll(notificationitem, message);
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
    if (im)
        icon = im->strIconName;
    fcitx_utils_alloc_cat_str(iconName, (icon[0] == '\0' || icon[0] == '/') ?
                              "" : "fcitx-", icon);
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
    const char* status = "Active";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &status);
}

void FcitxNotificationItemGetTitle(void* arg, DBusMessageIter* iter)
{
    const char* title = "Fcitx";
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
    const char* menu = "/NO_DBUS_MENU";
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

void FcitxNotificationItemIMChanged(void* arg)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;

#define SEND_SIGNAL(NAME) do { \
    DBusMessage* msg = dbus_message_new_signal(NOTIFICATION_ITEM_DEFAULT_OBJ, \
                                               NOTIFICATION_ITEM_DBUS_IFACE, \
                                               NAME); \
    if (msg) { \
        dbus_connection_send(notificationitem->conn, msg, NULL); \
    } \
    } while(0)

    SEND_SIGNAL("NewIcon");
    SEND_SIGNAL("NewToolTip");
    SEND_SIGNAL("NewTitle");
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

DBusHandlerResult FcitxNotificationItemDBusFilter(DBusConnection* connection, DBusMessage* msg, void* user_data)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) user_data;
    if (dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
        const char* service, *oldowner, *newowner;
        DBusError error;
        dbus_error_init(&error);
        do {
            if (!dbus_message_get_args(msg, &error,
                                       DBUS_TYPE_STRING, &service ,
                                       DBUS_TYPE_STRING, &oldowner ,
                                       DBUS_TYPE_STRING, &newowner ,
                                       DBUS_TYPE_INVALID)) {
                break;
            }
            /* old die and no new one */
            if (strcmp(service, NOTIFICATION_WATCHER_DBUS_ADDR) != 0) {
                break;
            }

            FcitxNotificationItemSetAvailable (notificationitem, strlen(newowner) > 0);
            return DBUS_HANDLER_RESULT_HANDLED;
        } while(0);
        dbus_error_free(&error);
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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
