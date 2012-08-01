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

#include "libintl.h"
#include "module/xkb/xkb.h"
#include "module/dbus/dbusstuff.h"
#include "fcitx-utils/log.h"
#include "fcitx/module.h"
#include <im/keyboard/isocodes.h>

static void* FcitxXkbDBusCreate(struct _FcitxInstance* instance);
typedef struct _FcitxXkbDBus {
    FcitxInstance* owner;
    FcitxXkbRules* rules;
    FcitxIsoCodes* isocodes;
} FcitxXkbDBus;

FCITX_DEFINE_PLUGIN(fcitx_xkbdbus, module, FcitxModule) = {
    FcitxXkbDBusCreate,
    NULL,
    NULL,
    NULL,
    NULL
};

static DBusHandlerResult FcitxXkbDBusEventHandler (DBusConnection  *connection,
                                                   DBusMessage     *message,
                                                   void            *user_data);

const char *introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node name=\"" FCITX_XKB_PATH "\">\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"" FCITX_XKB_INTERFACE "\">\n"
    "    <method name=\"SetDefaultLayout\">\n"
    "        <arg name=\"layout\" direction=\"in\" type=\"s\"/>"
    "        <arg name=\"variant\" direction=\"in\" type=\"s\"/>"
    "    </method>\n"
    "    <method name=\"GetLayouts\">\n"
    "      <arg name=\"layouts\" direction=\"out\" type=\"a(ssss)\"/>\n"
    "    </method>\n"
    "    <method name=\"GetLayoutForIM\">"
    "        <arg name=\"im\" direction=\"in\" type=\"s\"/>"
    "        <arg name=\"layout\" direction=\"out\" type=\"s\"/>"
    "        <arg name=\"variant\" direction=\"out\" type=\"s\"/>"
    "    </method>"
    "    <method name=\"SetLayoutForIM\">"
    "        <arg name=\"im\" direction=\"in\" type=\"s\"/>"
    "        <arg name=\"layout\" direction=\"in\" type=\"s\"/>"
    "        <arg name=\"variant\" direction=\"in\" type=\"s\"/>"
    "    </method>"
    "  </interface>\n"
    "</node>\n";

void* FcitxXkbDBusCreate(FcitxInstance* instance)
{
    FcitxXkbDBus* xkbdbus = fcitx_utils_new(FcitxXkbDBus);
    xkbdbus->owner = instance;
    do {
        FcitxModuleFunctionArg arg;
        DBusConnection* conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);
        if (conn == NULL) {
            FcitxLog(ERROR, "DBus Not initialized");
            break;
        }

        DBusObjectPathVTable fcitxIPCVTable = {NULL, &FcitxXkbDBusEventHandler, NULL, NULL, NULL, NULL };

        if (!dbus_connection_register_object_path(conn,  FCITX_XKB_PATH,
                &fcitxIPCVTable, xkbdbus)) {
            FcitxLog(ERROR, "No memory");
            break;
        }
        FcitxModuleFunctionArg args;
        FcitxXkbRules* rules = InvokeFunction(instance, FCITX_XKB, GETRULES, args);

        if (!rules)
            break;

        xkbdbus->rules = rules;
        xkbdbus->isocodes = FcitxXkbReadIsoCodes(ISOCODES_ISO639_XML, ISOCODES_ISO3166_XML);
        return xkbdbus;
    } while(0);

    free(xkbdbus);

    return NULL;
}

void FcitxXkbDBusAppendLayout(DBusMessageIter* sub, const char* layout, const char* variant, const char* description, const char* lang)
{
    DBusMessageIter ssub;
    if (!lang)
        lang = "";
    dbus_message_iter_open_container(sub, DBUS_TYPE_STRUCT, 0, &ssub);
    dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &layout);
    dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &variant);
    dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &description);
    dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &lang);
    dbus_message_iter_close_container(sub, &ssub);
}

void FcitxXkbDBusGetLayouts(FcitxXkbDBus* xkbdbus, DBusMessage* message)
{
    DBusMessageIter iter, sub;
    dbus_message_iter_init_append(message, &iter);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(ssss)", &sub);

    FcitxXkbRules* rules = xkbdbus->rules;
    FcitxIsoCodes* isocodes = xkbdbus->isocodes;
    char* lang = NULL;
#define GET_LANG \
    do { \
        char** plang = NULL; \
        plang = (char**) utarray_front(layoutInfo->languages); \
        lang = NULL; \
        if (plang) { \
            FcitxIsoCodes639Entry* entry = FcitxIsoCodesGetEntry(isocodes, *plang); \
            if (entry) { \
                lang = entry->iso_639_1_code; \
            } \
        } \
    } while (0)

    if (rules) {
        FcitxXkbLayoutInfo* layoutInfo;
        for (layoutInfo = (FcitxXkbLayoutInfo*) utarray_front(rules->layoutInfos);
            layoutInfo != NULL;
            layoutInfo = (FcitxXkbLayoutInfo*) utarray_next(rules->layoutInfos, layoutInfo))
        {
            char* description;
            asprintf(&description, "%s",
                     dgettext("xkeyboard-config", layoutInfo->description));

            GET_LANG;
            FcitxXkbDBusAppendLayout(&sub, layoutInfo->name, "", description, lang);
            free(description);
            FcitxXkbVariantInfo* variantInfo;
            for (variantInfo = (FcitxXkbVariantInfo*) utarray_front(layoutInfo->variantInfos);
                variantInfo != NULL;
                variantInfo = (FcitxXkbVariantInfo*) utarray_next(layoutInfo->variantInfos, variantInfo))
            {
                char* description;
                asprintf(&description, "%s - %s",
                        dgettext("xkeyboard-config", layoutInfo->description),
                        dgettext("xkeyboard-config", variantInfo->description));

                GET_LANG;
                FcitxXkbDBusAppendLayout(&sub, layoutInfo->name, variantInfo->name, description, lang);
                free(description);
            }
        }
    }
    else {
        char* description;
        asprintf(&description, "%s",
                dgettext("xkeyboard-config", "English (US)"));

        FcitxXkbDBusAppendLayout(&sub, "us", "", description, "en");
        free(description);
    }
    dbus_message_iter_close_container(&iter, &sub);
}

DBusHandlerResult FcitxXkbDBusEventHandler (DBusConnection  *connection,
                                            DBusMessage     *message,
                                            void            *user_data)
{
    FcitxXkbDBus* xkbdbus = user_data;
    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        DBusMessage *reply = dbus_message_new_method_return(message);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &introspection_xml, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(message, FCITX_XKB_INTERFACE, "GetLayouts")) {
        DBusMessage *reply = dbus_message_new_method_return(message);
        FcitxXkbDBusGetLayouts(xkbdbus, reply);
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        dbus_connection_flush(connection);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(message, FCITX_XKB_INTERFACE, "SetLayoutForIM")) {
        DBusError error;
        dbus_error_init(&error);
        char* im, *layout, *variant;
        if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &im, DBUS_TYPE_STRING, &layout, DBUS_TYPE_STRING, &variant, DBUS_TYPE_INVALID)) {
            FcitxModuleFunctionArg args;
            args.args[0] = im;
            args.args[1] = layout;
            args.args[2] = variant;
            InvokeFunction(xkbdbus->owner, FCITX_XKB, SETLAYOUTOVERRIDE, args);
        }
        DBusMessage *reply = dbus_message_new_method_return(message);
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        dbus_connection_flush(connection);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(message, FCITX_XKB_INTERFACE, "SetDefaultLayout")) {
        DBusError error;
        dbus_error_init(&error);
        char *layout, *variant;
        if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &layout, DBUS_TYPE_STRING, &variant, DBUS_TYPE_INVALID)) {
            FcitxModuleFunctionArg args;
            args.args[0] = layout;
            args.args[1] = variant;
            InvokeFunction(xkbdbus->owner, FCITX_XKB, SETDEFAULTLAYOUT, args);
        }
        DBusMessage *reply = dbus_message_new_method_return(message);
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        dbus_connection_flush(connection);
        return DBUS_HANDLER_RESULT_HANDLED;
    }  else if (dbus_message_is_method_call(message, FCITX_XKB_INTERFACE, "GetLayoutForIM")) {
        DBusError error;
        dbus_error_init(&error);
        char* im = NULL, *layout = NULL, *variant = NULL;
        if (dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &im, DBUS_TYPE_INVALID)) {
            FcitxModuleFunctionArg args;
            args.args[0] = im;
            args.args[1] = &layout;
            args.args[2] = &variant;
            InvokeFunction(xkbdbus->owner, FCITX_XKB, GETLAYOUTOVERRIDE, args);

            if (!layout)
                layout = "";
            if (!variant)
                variant = "";

            DBusMessage *reply = dbus_message_new_method_return(message);
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &layout, DBUS_TYPE_STRING, &variant, DBUS_TYPE_INVALID);
            dbus_connection_send(connection, reply, NULL);
            dbus_message_unref(reply);
            dbus_connection_flush(connection);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
