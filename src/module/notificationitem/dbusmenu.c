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

#include <libintl.h>

#include "module/dbusstuff/property.h"
#include "notificationitem_p.h"
#include "fcitx-utils/utils.h"
#include "fcitx/fcitx.h"

/*
 * libdbusmenu-gtk have a strange 30000 limitation, in order to leverage this, we need
 * some more hack
 *
 * max bit -> 14bit
 * lower 5 bit for menu, 0 -> 31 (IMHO it's enough)
 * higher 9 bit for index, 0 -> 511
 */

#define DBUS_MENU_IFACE "com.canonical.dbusmenu"
#define ACTION_ID(ID, IDX) (((IDX) << 5) | (ID))
#define ACTION_INDEX(ID) (((ID) & 0xffffffe0) >> 5)
#define ACTION_MENU(ID) ((ID) & 0x0000001f)
#define STATUS_ID(ISCOMP, IDX) ACTION_ID(0, (((ISCOMP) ? 0x100 : 0x000) + IDX + 8 + 1))
#define STATUS_INDEX(ID) ((ACTION_INDEX(ID) & 0x0ff) - 8 - 1)
#define STATUS_ISCOMP(ID) (!!(ACTION_INDEX(ID) & 0x100))

static const UT_icd ut_int32_icd = {
    sizeof(int32_t), NULL, NULL, NULL
};

const char* dbus_menu_interface =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\""
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
    "<node>"
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
    "<interface name=\"com.canonical.dbusmenu\">"
    "<property name=\"Version\" type=\"u\" access=\"read\"/>"
    "<property name=\"Status\" type=\"s\" access=\"read\"/>"
    "<signal name=\"ItemsPropertiesUpdated\">"
    "<arg type=\"a(ia{sv})\" direction=\"out\"/>"
    "<arg type=\"a(ias)\" direction=\"out\"/>"
    "</signal>"
    "<signal name=\"LayoutUpdated\">"
    "<arg name=\"revision\" type=\"u\" direction=\"out\"/>"
    "<arg name=\"parentId\" type=\"i\" direction=\"out\"/>"
    "</signal>"
    "<signal name=\"ItemActivationRequested\">"
    "<arg name=\"id\" type=\"i\" direction=\"out\"/>"
    "<arg name=\"timeStamp\" type=\"u\" direction=\"out\"/>"
    "</signal>"
    "<method name=\"Event\">"
    "<arg name=\"id\" type=\"i\" direction=\"in\"/>"
    "<arg name=\"eventId\" type=\"s\" direction=\"in\"/>"
    "<arg name=\"data\" type=\"v\" direction=\"in\"/>"
    "<arg name=\"timestamp\" type=\"u\" direction=\"in\"/>"
    "<annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>"
    "</method>"
    "<method name=\"GetProperty\">"
    "<arg type=\"v\" direction=\"out\"/>"
    "<arg name=\"id\" type=\"i\" direction=\"in\"/>"
    "<arg name=\"property\" type=\"s\" direction=\"in\"/>"
    "</method>"
    "<method name=\"GetLayout\">"
    "<arg type=\"u\" direction=\"out\"/>"
    "<arg name=\"parentId\" type=\"i\" direction=\"in\"/>"
    "<arg name=\"recursionDepth\" type=\"i\" direction=\"in\"/>"
    "<arg name=\"propertyNames\" type=\"as\" direction=\"in\"/>"
    "<arg name=\"item\" type=\"(ia{sv}av)\" direction=\"out\"/>"
    "</method>"
    "<method name=\"GetGroupProperties\">"
    "<arg type=\"a(ia{sv})\" direction=\"out\"/>"
    "<arg name=\"ids\" type=\"ai\" direction=\"in\"/>"
    "<arg name=\"propertyNames\" type=\"as\" direction=\"in\"/>"
    "</method>"
    "<method name=\"AboutToShow\">"
    "<arg type=\"b\" direction=\"out\"/>"
    "<arg name=\"id\" type=\"i\" direction=\"in\"/> "
    "</method>"
    "</interface>"
    "</node>";

static DBusHandlerResult FcitxDBusMenuEventHandler (DBusConnection  *connection,
                                                    DBusMessage     *message,
                                                    void            *user_data);

static void FcitxDBusMenuGetVersion(void* arg, DBusMessageIter* iter);
static void FcitxDBusMenuGetStatus(void* arg, DBusMessageIter* iter);
static void FcitxDBusMenuEvent(FcitxNotificationItem* notificationitem, DBusMessage* message);
static DBusMessage* FcitxDBusMenuGetGroupProperties(FcitxNotificationItem* notificationitem, DBusMessage* message);
static DBusMessage* FcitxDBusMenuGetProperty(FcitxNotificationItem* notificationitem, DBusMessage* message);
static DBusMessage* FcitxDBusMenuAboutToShow(FcitxNotificationItem* notificationitem, DBusMessage* message);
static DBusMessage* FcitxDBusMenuGetLayout(FcitxNotificationItem* notificationitem, DBusMessage* message);
static void FcitxDBusMenuFillLayooutItemWrap(FcitxNotificationItem* notificationitem, int32_t id, int depth, FcitxStringHashSet* properties, DBusMessageIter* iter);
static void FcitxDBusMenuFillLayooutItem(FcitxNotificationItem* notificationitem, int32_t id, int depth, FcitxStringHashSet* properties, DBusMessageIter* iter);

const FcitxDBusPropertyTable dbusMenuPropertyTable[] = {
    { DBUS_MENU_IFACE, "Version", "u", FcitxDBusMenuGetVersion, NULL },
    { DBUS_MENU_IFACE, "Status", "s", FcitxDBusMenuGetStatus, NULL },
    { NULL, NULL, NULL, NULL, NULL }
};

boolean FcitxDBusMenuCreate(FcitxNotificationItem* notificationitem)
{
    DBusObjectPathVTable fcitxIPCVTable = {NULL, &FcitxDBusMenuEventHandler, NULL, NULL, NULL, NULL };
    if (dbus_connection_register_object_path(notificationitem->conn, "/MenuBar", &fcitxIPCVTable, notificationitem)) {
        return true;
    }
    return false;
}

DBusHandlerResult FcitxDBusMenuEventHandler(DBusConnection* connection, DBusMessage* message, void* user_data)
{
    FcitxNotificationItem* notificationitem = user_data;
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage *reply = NULL;
    boolean flush = false;
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(message);
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &dbus_menu_interface, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(message, DBUS_MENU_IFACE, "Event")) {
        /* this is no reply */
        FcitxDBusMenuEvent(notificationitem, message);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(message, DBUS_MENU_IFACE, "GetProperty")) {
        reply = FcitxDBusMenuGetProperty(notificationitem, message);
    } else if (dbus_message_is_method_call(message, DBUS_MENU_IFACE, "GetLayout")) {
        reply = FcitxDBusMenuGetLayout(notificationitem, message);
    } else if (dbus_message_is_method_call(message, DBUS_MENU_IFACE, "GetGroupProperties")) {
        reply = FcitxDBusMenuGetGroupProperties(notificationitem, message);
    } else if (dbus_message_is_method_call(message, DBUS_MENU_IFACE, "AboutToShow")) {
        reply = FcitxDBusMenuAboutToShow(notificationitem, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Get")) {
        reply = FcitxDBusPropertyGet(notificationitem, dbusMenuPropertyTable, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Set")) {
        reply = FcitxDBusPropertySet(notificationitem, dbusMenuPropertyTable, message);
    } else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "GetAll")) {
        reply = FcitxDBusPropertyGetAll(notificationitem, dbusMenuPropertyTable, message);
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

void FcitxDBusMenuDoEvent(void* arg)
{
    FcitxNotificationItem* notificationitem = (FcitxNotificationItem*) arg;
    FcitxInstance* instance = notificationitem->owner;

    int32_t id = notificationitem->pendingActionId;
    notificationitem->pendingActionId = -1;

    int32_t menu = ACTION_MENU(id);
    int32_t index = ACTION_INDEX(id);
    if (index <= 0)
        return;

    if (menu == 0) {
        if (index <= 8 && index > 0) {
            switch(index) {
                case 1:
                    {
                        char* args[] = {
                            "xdg-open",
                            "http://fcitx-im.org/",
                            0
                        };
                        fcitx_utils_start_process(args);
                    }
                    break;
                case 4:
                    {
                        FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
                        if (im && im->owner) {
                            fcitx_utils_launch_configure_tool_for_addon(im->uniqueName);
                        }
                        else {
                            fcitx_utils_launch_configure_tool();
                        }
                    }
                    break;
                case 5:
                    fcitx_utils_launch_configure_tool();
                    break;
                case 6:
                    fcitx_utils_launch_restart();
                    break;
                case 7:
                    FcitxInstanceEnd(instance);
                    break;
            }
        } else {
            int index = STATUS_INDEX(id);
            const char* name = NULL;
            if (STATUS_ISCOMP(id)) {
                UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
                FcitxUIComplexStatus* compstatus = (FcitxUIComplexStatus*) utarray_eltptr(uicompstats, index);
                if (compstatus) {
                    name = compstatus->name;
                }
            } else {
                UT_array* uistats = FcitxInstanceGetUIStats(instance);
                FcitxUIStatus* status = (FcitxUIStatus*) utarray_eltptr(uistats, index);
                if (status) {
                    name = status->name;
                }
            }
            if (name) {
                FcitxUIUpdateStatus(instance, name);
            }
        }
    } else if (menu > 0) {
        UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
        FcitxUIMenu** menup = (FcitxUIMenu**) utarray_eltptr(uimenus, menu - 1), *menu;
        if (!menup)
            return;
        menu = *menup;
        if (menu->MenuAction) {
            menu->MenuAction(menu, index - 1);
        }
    }
}

void FcitxDBusMenuEvent(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    /* signature isvu */
    DBusMessageIter args;
    dbus_message_iter_init(message, &args);

    int32_t id;
    char* type;
    do {
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT32)
            break;
        dbus_message_iter_get_basic(&args, &id);
        dbus_message_iter_next(&args);
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_STRING)
            break;
        dbus_message_iter_get_basic(&args, &type);
        dbus_message_iter_next(&args);
        if (strcmp(type, "clicked") != 0)
            break;
        /* TODO parse variant, but no one actually use this */
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_VARIANT)
            break;
        dbus_message_iter_next(&args);
        /* timestamp, useless for us */
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32)
            break;
        dbus_message_iter_next(&args);

        if (!FcitxInstanceCheckTimeoutByFunc(notificationitem->owner, FcitxDBusMenuDoEvent)) {
            notificationitem->pendingActionId = id;
            FcitxInstanceAddTimeout(notificationitem->owner, 50, FcitxDBusMenuDoEvent, notificationitem);
        }
    } while(0);
}

void FcitxDBusMenuAppendProperty(DBusMessageIter* iter, FcitxStringHashSet* properties, const char* name, int type, const void* data) {
    if (properties && !fcitx_utils_string_hash_set_contains(properties, name))
        return;
    DBusMessageIter entry;
    dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &name);
    DBusMessageIter variant;
    char typeString[2] = {(char)type, '\0'};
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, typeString, &variant);
    dbus_message_iter_append_basic(&variant, type, data);
    dbus_message_iter_close_container(&entry, &variant);

    dbus_message_iter_close_container(iter, &entry);
}

void FcitxDBusMenuFillProperty(FcitxNotificationItem* notificationitem, int32_t id, FcitxStringHashSet* properties, DBusMessageIter* iter)
{
    FcitxInstance* instance = notificationitem->owner;
    /* append a{sv} */
    DBusMessageIter sub;
    dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", &sub);
    int32_t menu = ACTION_MENU(id);
    int32_t index = ACTION_INDEX(id);

    /* index == 0 means it has a sub menu */
    if (index == 0) {
        const char* value = "submenu";
        FcitxDBusMenuAppendProperty(&sub, properties, "children-display", DBUS_TYPE_STRING, &value);
    }
    if (menu == 0) {
        if (index <= 8 && index > 0) {
            const char* value;
            switch(index) {
                case 1:
                    value = _("Online Help");
                    FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &value);
                    value = "help-contents";
                    FcitxDBusMenuAppendProperty(&sub, properties, "icon-name", DBUS_TYPE_STRING, &value);
                    break;
                case 2:
                case 3:
                case 8:
                    value = "separator";
                    FcitxDBusMenuAppendProperty(&sub, properties, "type", DBUS_TYPE_STRING, &value);
                    break;
                case 4:
                    value = _("Configure Current Input Method");
                    FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &value);
                    break;
                case 5:
                    value = _("Configure");
                    FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &value);
                    /* this icon sucks on KDE, why configure doesn't have "configure" */
#if 0
                    value = "preferences-system";
                    FcitxDBusMenuAppendProperty(&sub, properties, "icon-name", DBUS_TYPE_STRING, &value);
#endif
                    break;
                case 6:
                    value = _("Restart");
                    FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &value);
                    value = "view-refresh";
                    FcitxDBusMenuAppendProperty(&sub, properties, "icon-name", DBUS_TYPE_STRING, &value);
                    break;
                case 7:
                    value = _("Exit");
                    FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &value);
                    value = "application-exit";
                    FcitxDBusMenuAppendProperty(&sub, properties, "icon-name", DBUS_TYPE_STRING, &value);
                    break;
            }
        } else {
            int index = STATUS_INDEX(id);
            const char* name = NULL;
            const char* icon = NULL;
            char* needfree = NULL;
            if (STATUS_ISCOMP(id)) {
                UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
                FcitxUIComplexStatus* compstatus = (FcitxUIComplexStatus*) utarray_eltptr(uicompstats, index);
                if (compstatus) {
                    name = compstatus->shortDescription;
                    icon = compstatus->getIconName(compstatus->arg);
                }
            } else {
                UT_array* uistats = FcitxInstanceGetUIStats(instance);
                FcitxUIStatus* status = (FcitxUIStatus*) utarray_eltptr(uistats, index);
                if (status) {
                    name = status->shortDescription;

                    fcitx_utils_alloc_cat_str(needfree, "fcitx-", status->name,
                                            ((status->getCurrentStatus(status->arg)) ?
                                            "-active" : "-inactive"));
                    icon = needfree;
                }
            }

            if (name) {
                FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &name);
            }
            if (icon) {
                FcitxDBusMenuAppendProperty(&sub, properties, "icon-name", DBUS_TYPE_STRING, &icon);
            }
            fcitx_utils_free(needfree);
        }
    } else {
        UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
        FcitxUIMenu** menupp = (FcitxUIMenu**) utarray_eltptr(uimenus, menu - 1), *menup;
        if (menupp) {
            menup = *menupp;
            if (index == 0) {
                FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &menup->name);
            } else if (index > 0) {
                FcitxMenuItem* item = (FcitxMenuItem*) utarray_eltptr(&menup->shell, index - 1);
                if (item) {
                    FcitxDBusMenuAppendProperty(&sub, properties, "label", DBUS_TYPE_STRING, &item->tipstr);
                    if (menup->mark != -1) {
                        const char* radio = "radio";
                        FcitxDBusMenuAppendProperty(&sub, properties, "toggle-type", DBUS_TYPE_STRING, &radio);
                        int32_t toggleState = 0;
                        if (menup->mark == index - 1) {
                            toggleState = 1;
                        }
                        FcitxDBusMenuAppendProperty(&sub, properties, "toggle-state", DBUS_TYPE_INT32, &toggleState);
                    }
                }
            }
        }
    }
    dbus_message_iter_close_container(iter, &sub);
}

void FcitxDBusMenuFillLayooutItemWrap(FcitxNotificationItem* notificationitem, int32_t id, int depth, FcitxStringHashSet* properties, DBusMessageIter* iter)
{
    DBusMessageIter variant;
    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, "(ia{sv}av)", &variant);
    FcitxDBusMenuFillLayooutItem(notificationitem, id, depth, properties, &variant);
    dbus_message_iter_close_container(iter, &variant);
}

void FcitxDBusMenuFillLayooutItem(FcitxNotificationItem* notificationitem, int32_t id, int depth, FcitxStringHashSet* properties, DBusMessageIter* iter)
{
    FcitxInstance* instance = notificationitem->owner;
    DBusMessageIter sub, array;
    dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, NULL, &sub);
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &id);
    FcitxDBusMenuFillProperty(notificationitem, id, properties, &sub);

    dbus_message_iter_open_container(&sub, DBUS_TYPE_ARRAY, "v", &array);

    /* for dbus menu, we have
     * root (0,0) -> online help (0,1)
     *            -> separator (0,2)
     *            -> some status (0,8 + X) do cache
     *            -> separator (0,8)
     *            -> registered menu (x,0) -> (x,1) , (x,2), (x,3)
     *            -> separator (0,3)
     *            -> configure current (0,4)
     *            -> configure (0,5)
     *            -> restart (0,6)
     *            -> exit (0,7)
     */

    /* using != 0 can make -1 recursive to infinite */
    if (depth != 0) {
        int32_t menu = ACTION_MENU(id);
        int32_t index = ACTION_INDEX(id);

        UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
        /* we ONLY support submenu in top level menu */
        if (menu == 0) {
            if (index == 0) {
                FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,1), depth - 1, properties, &array);
                FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,2), depth - 1, properties, &array);
                boolean flag = false;

                /* copied from classicui.c */
                FcitxUIStatus* status;
                UT_array* uistats = FcitxInstanceGetUIStats(instance);
                int i;
                for (i = 0, status = (FcitxUIStatus*) utarray_front(uistats);
                     status != NULL;
                     i++, status = (FcitxUIStatus*) utarray_next(uistats, status)) {
                    if (!status->visible)
                        continue;

                    flag = true;
                    FcitxDBusMenuFillLayooutItemWrap(notificationitem, STATUS_ID(0,i), depth - 1, properties, &array);
                }

                FcitxUIComplexStatus* compstatus;
                UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
                for (i = 0, compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
                     compstatus != NULL;
                     i++, compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
                    ) {
                    if (!compstatus->visible)
                        continue;
                    if (FcitxUIGetMenuByStatusName(instance, compstatus->name))
                        continue;

                    flag = true;
                    FcitxDBusMenuFillLayooutItemWrap(notificationitem, STATUS_ID(1,i), depth - 1, properties, &array);
                }

                if (flag)
                    FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,8), depth - 1, properties, &array);
                if (utarray_len(uimenus) > 0) {
                    FcitxUIMenu **menupp;
                    int i = 1;
                    for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
                         menupp != NULL;
                         menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp)) {
                        do {
                            if (!menupp) {
                                break;
                            }
                            FcitxUIMenu* menup = *menupp;
                            if (!menup->visible) {
                                break;
                            }
                            if (menup->candStatusBind) {
                                FcitxUIComplexStatus* compStatus = FcitxUIGetComplexStatusByName(instance, menup->candStatusBind);
                                if (compStatus && !compStatus->visible) {
                                    break;
                                }
                            }
                            FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(i,0), depth - 1, properties, &array);
                        } while(0);
                        i ++;
                    }
                    FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,3), depth - 1, properties, &array);
                }
                FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,4), depth - 1, properties, &array);
                FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,5), depth - 1, properties, &array);
                FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,6), depth - 1, properties, &array);
                FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(0,7), depth - 1, properties, &array);
            }
        } else {
            if (index == 0) {
                FcitxUIMenu** menupp = (FcitxUIMenu**) utarray_eltptr(uimenus, menu - 1), *menup;
                if (menupp) {
                    menup = *menupp;
                    menup->UpdateMenu(menup);

                    unsigned int i = 0;
                    unsigned int len = utarray_len(&menup->shell);
                    for (i = 0; i < len; i++) {
                        FcitxDBusMenuFillLayooutItemWrap(notificationitem, ACTION_ID(menu,i + 1), depth - 1, properties, &array);
                    }
                }
            }
        }
    }
    dbus_message_iter_close_container(&sub, &array);
    dbus_message_iter_close_container(iter, &sub);
}

DBusMessage* FcitxDBusMenuGetLayout(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    /* signature iias */
    DBusMessageIter args;
    dbus_message_iter_init(message, &args);

    DBusMessage* reply = NULL;

    int32_t id, recursionDepth;
    do {
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT32)
            break;
        dbus_message_iter_get_basic(&args, &id);
        dbus_message_iter_next(&args);

        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INT32)
            break;
        dbus_message_iter_get_basic(&args, &recursionDepth);
        dbus_message_iter_next(&args);

        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
            break;

        DBusMessageIter sub;
        dbus_message_iter_recurse(&args, &sub);
        FcitxStringHashSet* properties = NULL;
        while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
            char* property;
            dbus_message_iter_get_basic(&sub, &property);

            if (!fcitx_utils_string_hash_set_contains(properties, property)) {
                properties = fcitx_utils_string_hash_set_insert(properties, property);
            }
            dbus_message_iter_next(&sub);
        }

        reply = dbus_message_new_method_return(message);

        /* out put is u(ia{sv}av) */
        DBusMessageIter iter;
        dbus_message_iter_init_append(reply, &iter);
        dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &notificationitem->revision);
        FcitxDBusMenuFillLayooutItem(notificationitem, id, recursionDepth, properties, &iter);

        fcitx_utils_free_string_hash_set(properties);
    } while(0);

    if (!reply) {
        reply = FcitxDBusPropertyUnknownMethod(message);
    }

    return reply;
}

DBusMessage* FcitxDBusMenuGetProperty(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    /* TODO implement this, document said this only for debug so we ignore it for now */

    /* signature is */
    DBusMessage* reply = NULL;
    reply = FcitxDBusPropertyUnknownMethod(message);

    return reply;
}

DBusMessage* FcitxDBusMenuGetGroupProperties(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    /* signature aias */
    DBusMessageIter args;
    dbus_message_iter_init(message, &args);

    DBusMessage* reply = NULL;

    do {
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
            break;

        DBusMessageIter sub;
        dbus_message_iter_recurse(&args, &sub);
        UT_array ids;
        utarray_init(&ids, &ut_int32_icd);
        while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_INT32) {
            int32_t id;
            dbus_message_iter_get_basic(&sub, &id);

            utarray_push_back(&ids, &id);
            dbus_message_iter_next(&sub);
        }
        dbus_message_iter_next(&args);

        dbus_message_iter_recurse(&args, &sub);
        FcitxStringHashSet* properties = NULL;
        if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY) {
            utarray_done(&ids);
            break;
        }

        while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
            char* property;
            dbus_message_iter_get_basic(&sub, &property);

            if (!fcitx_utils_string_hash_set_contains(properties, property)) {
                properties = fcitx_utils_string_hash_set_insert(properties, property);
            }
            dbus_message_iter_next(&sub);
        }

        reply = dbus_message_new_method_return(message);

        /* out put is a(ia{sv}) */
        DBusMessageIter iter;
        dbus_message_iter_init_append(reply, &iter);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(ia{sv})", &sub);
        int i = 0;
        for (; i < utarray_len(&ids); i ++) {
            int32_t id = *(int32_t*) utarray_eltptr(&ids, i);
            DBusMessageIter ssub;
            dbus_message_iter_open_container(&sub, DBUS_TYPE_STRUCT, NULL, &ssub);
            dbus_message_iter_append_basic(&ssub, DBUS_TYPE_INT32, &id);
            FcitxDBusMenuFillProperty(notificationitem, id, properties, &ssub);
            dbus_message_iter_close_container(&sub, &ssub);
        }
        dbus_message_iter_close_container(&iter, &sub);

        utarray_done(&ids);
        fcitx_utils_free_string_hash_set(properties);
    } while(0);

    if (!reply) {
        reply = FcitxDBusPropertyUnknownMethod(message);
    }

    return reply;
}

DBusMessage* FcitxDBusMenuAboutToShow(FcitxNotificationItem* notificationitem, DBusMessage* message)
{
    DBusMessage* reply = NULL;
    /* signature i out b */
    int32_t id;
    DBusError err;
    dbus_error_init(&err);
    if (dbus_message_get_args(message, &err, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID)) {
        reply = dbus_message_new_method_return(message);
        /* for fcitx, we always return true */
        dbus_bool_t needUpdate = TRUE;
        dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &needUpdate, DBUS_TYPE_INVALID);
        notificationitem->revision++;
        DBusMessage* sig = dbus_message_new_signal("/MenuBar", DBUS_MENU_IFACE, "LayoutUpdated");
        dbus_message_append_args(sig, DBUS_TYPE_UINT32, &notificationitem->revision, DBUS_TYPE_INT32, &id, DBUS_TYPE_INVALID);
        dbus_connection_send(notificationitem->conn, sig, NULL);
        dbus_message_unref(sig);
    } else {
        reply = FcitxDBusPropertyUnknownMethod(message);
    }

    dbus_error_free(&err);
    return reply;
}

void FcitxDBusMenuGetVersion(void* arg, DBusMessageIter* iter)
{
    unsigned int version = 2;
    dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &version);
}

void FcitxDBusMenuGetStatus(void* arg, DBusMessageIter* iter)
{
    const char* status = "normal";
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &status);
}
