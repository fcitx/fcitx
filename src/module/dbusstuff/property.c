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

#include <string.h>

#include <dbus/dbus.h>
#include "property.h"

DBusMessage* FcitxDBusPropertyGet(void* arg, const FcitxDBusPropertyTable* propertTable, DBusMessage* message)
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
                propertTable[index].getfunc(arg, &variant);
            dbus_message_iter_close_container(&args, &variant);
        }
        else {
            reply = dbus_message_new_error_printf(message, DBUS_ERROR_UNKNOWN_PROPERTY, "No such property ('%s.%s')", interface, property);
        }
    }
    else {
        reply = FcitxDBusPropertyUnknownMethod(message);
    }

    return reply;
}

DBusMessage* FcitxDBusPropertySet(void* arg, const FcitxDBusPropertyTable* propertTable, DBusMessage* message)
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
        propertTable[index].setfunc(arg, &variant);
        reply = dbus_message_new_method_return(message);
    }
    else {
        reply = dbus_message_new_error_printf(message, DBUS_ERROR_UNKNOWN_PROPERTY, "No such property ('%s.%s')", interface, property);
    }

dbus_property_set_end:
    if (!reply)
        reply = FcitxDBusPropertyUnknownMethod(message);

    return reply;
}

DBusMessage* FcitxDBusPropertyGetAll(void* arg, const FcitxDBusPropertyTable* propertTable, DBusMessage* message)
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
                propertTable[index].getfunc(arg, &variant);
                dbus_message_iter_close_container(&entry, &variant);

                dbus_message_iter_close_container(&array, &entry);
            }
            index ++;
        }
        dbus_message_iter_close_container(&args, &array);
    }
    if (!reply)
        reply = FcitxDBusPropertyUnknownMethod(message);

    return reply;
}
