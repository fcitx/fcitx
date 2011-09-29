/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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

#include <dbus/dbus.h>
#include <limits.h>

#include "fcitx/fcitx.h"
#include "fcitx/frontend.h"
#include "fcitx-utils/utils.h"
#include "module/dbus/dbusstuff.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx/configfile.h"
#include "fcitx/hook.h"
#include "ipc.h"

#define GetIPCIC(ic) ((FcitxIPCIC*) (ic)->privateic)

typedef struct _FcitxIPCIC {
    int id;
    char path[32];
    char* appname;
} FcitxIPCIC;

typedef struct _FcitxIPCFrontend {
    int frontendid;
    int maxid;
    DBusConnection* conn;
    FcitxInstance* owner;
} FcitxIPCFrontend;

typedef struct _FcitxDBusPropertyTable {
    char* interface;
    char* name;
    char* type;
    void (*getfunc)(void* arg, DBusMessageIter* iter);
    void (*setfunc)(void* arg, DBusMessageIter* iter);
} FcitxDBusPropertyTable;

static void* IPCCreate(FcitxInstance* instance, int frontendid);
static boolean IPCDestroy(void* arg);
void IPCCreateIC(void* arg, FcitxInputContext* context, void *priv);
boolean IPCCheckIC(void* arg, FcitxInputContext* context, void* priv);
void IPCDestroyIC(void* arg, FcitxInputContext* context);
static void IPCEnableIM(void* arg, FcitxInputContext* ic);
static void IPCCloseIM(void* arg, FcitxInputContext* ic);
static void IPCCommitString(void* arg, FcitxInputContext* ic, char* str);
static void IPCForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
static void IPCSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y);
static void IPCGetWindowPosition(void* arg, FcitxInputContext* ic, int* x, int* y);
static void IPCUpdatePreedit(void* arg, FcitxInputContext* ic);
static void IPCUpdateClientSideUI(void* arg, FcitxInputContext* ic);
static DBusHandlerResult IPCDBusEventHandler(DBusConnection *connection, DBusMessage *message, void *user_data);
static DBusHandlerResult IPCICDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data);
static void IPCICFocusIn(FcitxIPCFrontend* ipc, FcitxInputContext* ic);
static void IPCICFocusOut(FcitxIPCFrontend* ipc, FcitxInputContext* ic);
static void IPCICReset(FcitxIPCFrontend* ipc, FcitxInputContext* ic);
static void IPCICSetCursorLocation(FcitxIPCFrontend* ipc, FcitxInputContext* ic, int x, int y);
static int IPCProcessKey(FcitxIPCFrontend* ipc, FcitxInputContext* callic, uint32_t originsym, uint32_t keycode, uint32_t originstate, uint32_t time, FcitxKeyEventType type);
static boolean IPCCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic);
static void IPCGetPropertyIMList(void* arg, DBusMessageIter* iter);
static void IPCSetPropertyIMList(void* arg, DBusMessageIter* iter);
static void IPCUpdateIMList(void* arg);

static void FcitxDBusPropertyGet(FcitxIPCFrontend* ipc, DBusMessage* message);
static void FcitxDBusPropertySet(FcitxIPCFrontend* ipc, DBusMessage* message);
static void FcitxDBusPropertyGetAll(FcitxIPCFrontend* ipc, DBusMessage* message);

const FcitxDBusPropertyTable propertTable[] = {
    { FCITX_IM_DBUS_INTERFACE, "IMList", "a(sssb)", IPCGetPropertyIMList, IPCSetPropertyIMList },
    { NULL, NULL, NULL, NULL, NULL }
};

const char * im_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
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
    "  <interface name=\"" FCITX_IM_DBUS_INTERFACE "\">\n"
    "    <method name=\"CreateIC\">\n"
    "      <arg name=\"icid\" direction=\"out\" type=\"i\"/>\n"
    "      <arg name=\"keyval1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"keyval2\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state2\" direction=\"out\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"CreateICv2\">\n"
    "      <arg name=\"appname\" direction=\"int\" type=\"s\"/>\n"
    "      <arg name=\"icid\" direction=\"out\" type=\"i\"/>\n"
    "      <arg name=\"enable\" direction=\"out\" type=\"b\"/>\n"
    "      <arg name=\"keyval1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"keyval2\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state2\" direction=\"out\" type=\"u\"/>\n"
    "    </method>\n"
    "    <property access=\"readwrite\" type=\"a(sssb)\" name=\"IMList\">\n"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>\n"
    "  </interface>\n"
    "</node>\n";

const char * ic_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"" FCITX_IC_DBUS_INTERFACE "\">\n"
    "    <method name=\"EnableIC\">\n"
    "    </method>\n"
    "    <method name=\"CloseIC\">\n"
    "    </method>\n"
    "    <method name=\"FocusIn\">\n"
    "    </method>\n"
    "    <method name=\"FocusOut\">\n"
    "    </method>\n"
    "    <method name=\"Reset\">\n"
    "    </method>\n"
    "    <method name=\"SetCursorLocation\">\n"
    "      <arg name=\"x\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"y\" direction=\"in\" type=\"i\"/>\n"
    "    </method>\n"
    "    <method name=\"SetCapacity\">\n"
    "      <arg name=\"caps\" direction=\"in\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"DestroyIC\">\n"
    "    </method>\n"
    "    <method name=\"ProcessKeyEvent\">\n"
    "      <arg name=\"keyval\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"keycode\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"state\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"type\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"time\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"ret\" direction=\"out\" type=\"i\"/>\n"
    "    </method>\n"
    "    <signal name=\"EnableIM\">\n"
    "    </signal>\n"
    "    <signal name=\"CloseIM\">\n"
    "    </signal>\n"
    "    <signal name=\"CommitString\">\n"
    "      <arg name=\"str\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdatePreedit\">\n"
    "      <arg name=\"str\" type=\"s\"/>\n"
    "      <arg name=\"cursorpos\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateClientSideUI\">\n"
    "      <arg name=\"auxup\" type=\"s\"/>\n"
    "      <arg name=\"auxdown\" type=\"s\"/>\n"
    "      <arg name=\"preedit\" type=\"s\"/>\n"
    "      <arg name=\"candidateword\" type=\"s\"/>\n"
    "      <arg name=\"imname\" type=\"s\"/>\n"
    "      <arg name=\"cursorpos\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"ForwardKey\">\n"
    "      <arg name=\"keyval\" type=\"u\"/>\n"
    "      <arg name=\"state\" type=\"u\"/>\n"
    "      <arg name=\"type\" type=\"i\"/>\n"
    "    </signal>\n"
    "  </interface>\n"
    "</node>\n";

FCITX_EXPORT_API
FcitxFrontend frontend = {
    IPCCreate,
    IPCDestroy,
    IPCCreateIC,
    IPCCheckIC,
    IPCDestroyIC,
    IPCEnableIM,
    IPCCloseIM,
    IPCCommitString,
    IPCForwardKey,
    IPCSetWindowOffset,
    IPCGetWindowPosition,
    IPCUpdatePreedit,
    IPCUpdateClientSideUI,
    NULL,
    IPCCheckICFromSameApplication,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* IPCCreate(FcitxInstance* instance, int frontendid)
{
    FcitxIPCFrontend* ipc = fcitx_malloc0(sizeof(FcitxIPCFrontend));
    ipc->frontendid = frontendid;
    ipc->owner = instance;

    FcitxModuleFunctionArg arg;

    ipc->conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);
    if (ipc->conn == NULL) {
        FcitxLog(ERROR, "DBus Not initialized");
        free(ipc);
        return NULL;
    }

    DBusObjectPathVTable fcitxIPCVTable = {NULL, &IPCDBusEventHandler, NULL, NULL, NULL, NULL };

    if (!dbus_connection_register_object_path(ipc->conn,  FCITX_IM_DBUS_PATH,
            &fcitxIPCVTable, ipc)) {
        FcitxLog(ERROR, "No memory");
        free(ipc);
        return NULL;
    }
    
    FcitxIMEventHook hook;
    hook.arg = ipc;
    hook.func = IPCUpdateIMList;
    RegisterUpdateIMListHook(instance, hook);

    return ipc;
}

boolean IPCDestroy(void* arg)
{
    return true;
}

void IPCCreateIC(void* arg, FcitxInputContext* context, void* priv)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxIPCIC* ipcic = (FcitxIPCIC*) fcitx_malloc0(sizeof(FcitxIPCIC));
    DBusMessage* message = (DBusMessage*) priv;
    DBusMessage *reply = dbus_message_new_method_return(message);
    FcitxConfig* config = FcitxInstanceGetConfig(ipc->owner);

    context->privateic = ipcic;

    ipcic->id = ipc->maxid;
    ipc->maxid ++;
    sprintf(ipcic->path, FCITX_IC_DBUS_PATH, ipcic->id);

    uint32_t arg1, arg2, arg3, arg4;
    arg1 = config->hkTrigger[0].sym;
    arg2 = config->hkTrigger[0].state;
    arg3 = config->hkTrigger[1].sym;
    arg4 = config->hkTrigger[1].state;
    if (dbus_message_is_method_call(message, FCITX_IM_DBUS_INTERFACE, "CreateIC")) {
        /* CreateIC v1 indicates that default state can only be disabled */
        context->state = IS_CLOSED;
        ipcic->appname = NULL;
        dbus_message_append_args(reply,
                                 DBUS_TYPE_INT32, &ipcic->id,
                                 DBUS_TYPE_UINT32, &arg1,
                                 DBUS_TYPE_UINT32, &arg2,
                                 DBUS_TYPE_UINT32, &arg3,
                                 DBUS_TYPE_UINT32, &arg4,
                                 DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(message, FCITX_IM_DBUS_INTERFACE, "CreateICv2")) {

        DBusError error;
        dbus_error_init(&error);
        char* appname;
        if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &appname, DBUS_TYPE_INVALID))
            ipcic->appname = NULL;
        else {
            if (strlen(appname) == 0)
                ipcic->appname = NULL;
            else
                ipcic->appname = strdup(appname);
        }

        if (config->shareState == ShareState_PerProgram)
            SetICStateFromSameApplication(ipc->owner, ipc->frontendid, context);

        boolean arg0 = context->state != IS_CLOSED;

        dbus_error_free(&error);
        dbus_message_append_args(reply,
                                 DBUS_TYPE_INT32, &ipcic->id,
                                 DBUS_TYPE_BOOLEAN, &arg0,
                                 DBUS_TYPE_UINT32, &arg1,
                                 DBUS_TYPE_UINT32, &arg2,
                                 DBUS_TYPE_UINT32, &arg3,
                                 DBUS_TYPE_UINT32, &arg4,
                                 DBUS_TYPE_INVALID);
    }
    dbus_connection_send(ipc->conn, reply, NULL);
    dbus_message_unref(reply);

    DBusObjectPathVTable vtable = {NULL, &IPCICDBusEventHandler, NULL, NULL, NULL, NULL };
    if (!dbus_connection_register_object_path(ipc->conn, ipcic->path, &vtable, ipc)) {
        return ;
    }

    dbus_connection_flush(ipc->conn);

}

boolean IPCCheckIC(void* arg, FcitxInputContext* context, void* priv)
{
    int *id = (int*) priv;
    if (GetIPCIC(context)->id == *id)
        return true;
    return false;
}

void IPCDestroyIC(void* arg, FcitxInputContext* context)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxIPCIC* ipcic = GetIPCIC(context);

    dbus_connection_unregister_object_path(ipc->conn, GetIPCIC(context)->path);
    if (ipcic->appname)
        free(ipcic->appname);
    free(context->privateic);
    context->privateic = NULL;
}

void IPCEnableIM(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "EnableIM"); // name of the signal

    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
}

void IPCCloseIM(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "CloseIM"); // name of the signal

    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
}

void IPCCommitString(void* arg, FcitxInputContext* ic, char* str)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "CommitString"); // name of the signal

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);

    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
}

void IPCForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "ForwardKey"); // name of the signal

    uint32_t keyval = (uint32_t) sym;
    uint32_t keystate = (uint32_t) state;
    int32_t type = (int) event;
    dbus_message_append_args(msg, DBUS_TYPE_UINT32, &keyval, DBUS_TYPE_UINT32, &keystate, DBUS_TYPE_INT32, &type, DBUS_TYPE_INVALID);

    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
}

void IPCSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y)
{
    ic->offset_x = x;
    ic->offset_y = y;
}

void IPCGetWindowPosition(void* arg, FcitxInputContext* ic, int* x, int* y)
{
    *x = ic->offset_x;
    *y = ic->offset_y;
}


static DBusHandlerResult IPCDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) user_data;
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        DBusMessage *reply = dbus_message_new_method_return(msg);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &im_introspection_xml, DBUS_TYPE_INVALID);
        dbus_connection_send(ipc->conn, reply, NULL);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_PROPERTIES, "Get")) {
        FcitxDBusPropertyGet(ipc, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_PROPERTIES, "Set")) {
        FcitxDBusPropertySet(ipc, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_PROPERTIES, "GetAll")) {
        FcitxDBusPropertyGetAll(ipc, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "CreateIC")) {
        CreateIC(ipc->owner, ipc->frontendid, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "CreateICv2")) {
        CreateIC(ipc->owner, ipc->frontendid, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static DBusHandlerResult IPCICDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) user_data;
    int id;
    sscanf(dbus_message_get_path(msg), FCITX_IC_DBUS_PATH, &id);
    FcitxInputContext* ic = FindIC(ipc->owner, ipc->frontendid, &id);
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        DBusMessage *reply = dbus_message_new_method_return(msg);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &ic_introspection_xml, DBUS_TYPE_INVALID);
        dbus_connection_send(ipc->conn, reply, NULL);
        dbus_message_unref(reply);
        result = DBUS_HANDLER_RESULT_HANDLED;
    }

    if (result == DBUS_HANDLER_RESULT_NOT_YET_HANDLED && ic) {
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "EnableIC")) {
            EnableIM(ipc->owner, ic, false);
            DBusMessage *reply = dbus_message_new_method_return(msg);
            dbus_connection_send(ipc->conn, reply, NULL);
            dbus_message_unref(reply);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "CloseIC")) {
            CloseIM(ipc->owner, ic);
            DBusMessage *reply = dbus_message_new_method_return(msg);
            dbus_connection_send(ipc->conn, reply, NULL);
            dbus_message_unref(reply);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "FocusIn")) {
            IPCICFocusIn(ipc, ic);
            DBusMessage *reply = dbus_message_new_method_return(msg);
            dbus_connection_send(ipc->conn, reply, NULL);
            dbus_message_unref(reply);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "FocusOut")) {
            IPCICFocusOut(ipc, ic);
            DBusMessage *reply = dbus_message_new_method_return(msg);
            dbus_connection_send(ipc->conn, reply, NULL);
            dbus_message_unref(reply);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "Reset")) {
            IPCICReset(ipc, ic);
            DBusMessage *reply = dbus_message_new_method_return(msg);
            dbus_connection_send(ipc->conn, reply, NULL);
            dbus_message_unref(reply);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCursorLocation")) {
            int x, y;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID)) {
                IPCICSetCursorLocation(ipc, ic, x, y);
                DBusMessage *reply = dbus_message_new_method_return(msg);
                dbus_connection_send(ipc->conn, reply, NULL);
                dbus_message_unref(reply);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCapacity")) {
            uint32_t flags;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_UINT32, &flags, DBUS_TYPE_INVALID)) {
                ic->contextCaps = flags;
                DBusMessage *reply = dbus_message_new_method_return(msg);
                dbus_connection_send(ipc->conn, reply, NULL);
                dbus_message_unref(reply);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "DestroyIC")) {
            DestroyIC(ipc->owner, ipc->frontendid, &id);
            DBusMessage *reply = dbus_message_new_method_return(msg);
            dbus_connection_send(ipc->conn, reply, NULL);
            dbus_message_unref(reply);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "ProcessKeyEvent")) {
            uint32_t keyval, keycode, state, t;
            int ret, itype;
            FcitxKeyEventType type;
            if (dbus_message_get_args(msg, &error,
                                      DBUS_TYPE_UINT32, &keyval,
                                      DBUS_TYPE_UINT32, &keycode,
                                      DBUS_TYPE_UINT32, &state,
                                      DBUS_TYPE_INT32, &itype,
                                      DBUS_TYPE_UINT32, &t,
                                      DBUS_TYPE_INVALID)) {
                type = itype;
                ret = IPCProcessKey(ipc, ic, keyval, keycode, state, t, type);

                DBusMessage *reply = dbus_message_new_method_return(msg);

                dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
                dbus_connection_send(ipc->conn, reply, NULL);
                dbus_message_unref(reply);
                dbus_connection_flush(ipc->conn);
            }

            result = DBUS_HANDLER_RESULT_HANDLED;
        }
        dbus_error_free(&error);
    }
    return result;
}

static int IPCProcessKey(FcitxIPCFrontend* ipc, FcitxInputContext* callic, uint32_t originsym, uint32_t keycode, uint32_t originstate, uint32_t t, FcitxKeyEventType type)
{
    FcitxInputContext* ic = GetCurrentIC(ipc->owner);
    FcitxConfig* config = FcitxInstanceGetConfig(ipc->owner);
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);

    if (ic == NULL) {
        SetCurrentIC(ipc->owner, callic);
    }
    ic = callic;
    FcitxKeySym sym;
    unsigned int state;

    originstate = originstate - (originstate & KEY_NUMLOCK) - (originstate & KEY_CAPSLOCK) - (originstate & KEY_SCROLLLOCK);
    originstate &= KEY_USED_MASK;
    GetKey(originsym, originstate, &sym, &state);
    FcitxLog(DEBUG,
             "KeyRelease=%d  state=%d  KEYCODE=%d  KEYSYM=%u ",
             (type == FCITX_RELEASE_KEY), state, keycode, sym);

    if (originsym == 0)
        return 0;

    if (ic->state == IS_CLOSED && type == FCITX_PRESS_KEY && IsHotKey(sym, state, config->hkTrigger)) {
        EnableIM(ipc->owner, ic, false);
        FcitxInputStateSetKeyReleased(input, KR_OTHER);
        return 1;
    }

    INPUT_RETURN_VALUE retVal = ProcessKey(ipc->owner, type,
                                           t,
                                           sym, state);

    if (retVal & IRV_FLAG_FORWARD_KEY || retVal == IRV_TO_PROCESS)
        return 0;
    else
        return 1;
}

static void IPCICFocusIn(FcitxIPCFrontend* ipc, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;

    if (!SetCurrentIC(ipc->owner, ic))
        return;

    if (ic) {
        OnInputFocus(ipc->owner);
    } else {
        CloseInputWindow(ipc->owner);
        MoveInputWindow(ipc->owner);
    }

    return;
}

static void IPCICFocusOut(FcitxIPCFrontend* ipc, FcitxInputContext* ic)
{
    FcitxInputContext* currentic = GetCurrentIC(ipc->owner);
    if (ic && ic == currentic) {
        SetCurrentIC(ipc->owner, NULL);
        CloseInputWindow(ipc->owner);
        OnInputUnFocus(ipc->owner);
    }

    return;
}

static void IPCICReset(FcitxIPCFrontend* ipc, FcitxInputContext* ic)
{
    FcitxInputContext* currentic = GetCurrentIC(ipc->owner);
    if (ic && ic == currentic) {
        CloseInputWindow(ipc->owner);
        ResetInput(ipc->owner);
    }

    return;
}

static void IPCICSetCursorLocation(FcitxIPCFrontend* ipc, FcitxInputContext* ic, int x, int y)
{
    ic->offset_x = x;
    ic->offset_y = y;
    MoveInputWindow(ipc->owner);

    return;
}

void IPCUpdatePreedit(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "UpdatePreedit"); // name of the signal

    char* strPreedit = MessagesToCString(FcitxInputStateGetClientPreedit(input));
    char* str = ProcessOutputFilter(ipc->owner, strPreedit);
    if (str) {
        free(strPreedit);
        strPreedit = str;
    }

    int iCursorPos = FcitxInputStateGetClientCursorPos(input);

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &strPreedit, DBUS_TYPE_INT32, &iCursorPos, DBUS_TYPE_INVALID);

    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
    free(strPreedit);
}

void IPCUpdateClientSideUI(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "UpdateClientSideUI"); // name of the signal

    char *str;
    char* strAuxUp = MessagesToCString(FcitxInputStateGetAuxUp(input));
    str = ProcessOutputFilter(ipc->owner, strAuxUp);
    if (str) {
        free(strAuxUp);
        strAuxUp = str;
    }
    char* strAuxDown = MessagesToCString(FcitxInputStateGetAuxDown(input));
    str = ProcessOutputFilter(ipc->owner, strAuxDown);
    if (str) {
        free(strAuxDown);
        strAuxDown = str;
    }
    char* strPreedit = MessagesToCString(FcitxInputStateGetPreedit(input));
    str = ProcessOutputFilter(ipc->owner, strPreedit);
    if (str) {
        free(strPreedit);
        strPreedit = str;
    }
    char* candidateword = CandidateWordToCString(ipc->owner);
    str = ProcessOutputFilter(ipc->owner, candidateword);
    if (str) {
        free(candidateword);
        candidateword = str;
    }
    FcitxIM* im = GetCurrentIM(ipc->owner);
    char* imname = NULL;
    if (im == NULL)
        imname = "En";
    else
        imname = im->strName;

    int iCursorPos = FcitxInputStateGetCursorPos(input);

    dbus_message_append_args(msg,
                             DBUS_TYPE_STRING, &strAuxUp,
                             DBUS_TYPE_STRING, &strAuxDown,
                             DBUS_TYPE_STRING, &strPreedit,
                             DBUS_TYPE_STRING, &candidateword,
                             DBUS_TYPE_STRING, &imname,
                             DBUS_TYPE_INT32, &iCursorPos,
                             DBUS_TYPE_INVALID);

    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
    free(strAuxUp);
    free(strAuxDown);
    free(strPreedit);
    free(candidateword);
}

boolean IPCCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic)
{
    FcitxIPCIC* ipcicToCheck = GetIPCIC(icToCheck);
    FcitxIPCIC* ipcic = GetIPCIC(ic);
    if (ipcic->appname == NULL || ipcicToCheck->appname == NULL)
        return false;
    return strcmp(ipcicToCheck->appname, ipcic->appname) == 0;
}

void FcitxDBusPropertyGet(FcitxIPCFrontend* ipc, DBusMessage* message)
{
    DBusError error;
    dbus_error_init(&error);
    char *interface;
    char *property;
    DBusMessage* reply = dbus_message_new_method_return(message);
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
        DBusMessageIter args, variant;
        dbus_message_iter_init_append(reply, &args);
        dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "a(sssb)", &variant );
        if (propertTable[index].getfunc)
            propertTable[index].getfunc(ipc, &variant);
        dbus_message_iter_close_container(&args, &variant);
    }
    dbus_connection_send(ipc->conn, reply, NULL);
    dbus_message_unref(reply);
}

void FcitxDBusPropertySet(FcitxIPCFrontend* ipc, DBusMessage* message)
{
    DBusError error;
    dbus_error_init(&error);
    char *interface;
    char *property;
    DBusMessage* reply = dbus_message_new_method_return(message);
    
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
    
    dbus_message_iter_recurse  (&args, &variant);
    int index = 0;
    while (propertTable[index].interface != NULL) {
        if (strcmp(propertTable[index].interface, interface) == 0
                && strcmp(propertTable[index].name, property) == 0)
            break;
        index ++;
    }
    if (propertTable[index].setfunc)
        propertTable[index].setfunc(ipc, &variant);

dbus_property_set_end:
    dbus_connection_send(ipc->conn, reply, NULL);
    dbus_message_unref(reply);
}

void FcitxDBusPropertyGetAll(FcitxIPCFrontend* ipc, DBusMessage* message)
{
    DBusError error;
    dbus_error_init(&error);
    char *interface;
    DBusMessage* reply = dbus_message_new_method_return(message);
    if (dbus_message_get_args(message, &error,
                              DBUS_TYPE_STRING, &interface,
                              DBUS_TYPE_INVALID)) {
        int index = 0;
        DBusMessageIter args;
        dbus_message_iter_init_append(reply, &args);
        DBusMessageIter array, entry;
        dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"{sv}", &array);
    
        while (propertTable[index].interface != NULL) {
            if (strcmp(propertTable[index].interface, interface) == 0 && propertTable[index].getfunc)
            {
                dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY,
                                             NULL, &entry);
                
                dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &propertTable[index].name);
                DBusMessageIter variant;
                dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, propertTable[index].type, &variant );
                propertTable[index].getfunc(ipc, &variant);
                dbus_message_iter_close_container(&entry, &variant);

                dbus_message_iter_close_container(&array, &entry);
            }
            index ++;
        }
        dbus_message_iter_close_container(&args, &array);
    }
    dbus_connection_send(ipc->conn, reply, NULL);
    dbus_message_unref(reply);
}


void IPCGetPropertyIMList(void* arg, DBusMessageIter* args)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxInstance* instance = ipc->owner;
    DBusMessageIter sub, ssub;
    dbus_message_iter_open_container(args, DBUS_TYPE_ARRAY, "(sssb)", &sub);
    FcitxIM* ime;
    UT_array* imes = FcitxInstanceGetIMEs(instance);
    for (ime = (FcitxIM*) utarray_front(imes);
            ime != NULL;
            ime = (FcitxIM*) utarray_next(imes, ime)) {
        dbus_message_iter_open_container(&sub, DBUS_TYPE_STRUCT, 0, &ssub);
        boolean enable = true;
        char* name = ime->strName;
        char* uniqueName = ime->uniqueName;
        char* langCode = ime->langCode;
        dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &name);
        dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &uniqueName);
        dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &langCode);
        dbus_message_iter_append_basic(&ssub, DBUS_TYPE_BOOLEAN, &enable);
        dbus_message_iter_close_container(&sub, &ssub);
    }

    UT_array* availimes = FcitxInstanceGetAvailIMEs(instance);
    for (ime = (FcitxIM*) utarray_front(availimes);
            ime != NULL;
            ime = (FcitxIM*) utarray_next(availimes, ime)) {
        if (!GetIMFromIMList(imes, ime->uniqueName)) {
            dbus_message_iter_open_container(&sub, DBUS_TYPE_STRUCT, 0, &ssub);
            boolean enable = false;
            char* name = ime->strName;
            char* uniqueName = ime->uniqueName;
            char* langCode = ime->langCode;
            dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &name);
            dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &uniqueName);
            dbus_message_iter_append_basic(&ssub, DBUS_TYPE_STRING, &langCode);
            dbus_message_iter_append_basic(&ssub, DBUS_TYPE_BOOLEAN, &enable);
            dbus_message_iter_close_container(&sub, &ssub);
        }
    }
    dbus_message_iter_close_container(args, &sub);
}

void IPCSetPropertyIMList(void* arg, DBusMessageIter* args)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxInstance* instance = ipc->owner;
    DBusMessageIter sub, ssub;
    
    if (dbus_message_iter_get_arg_type(args) != DBUS_TYPE_ARRAY)
        return;
    
    dbus_message_iter_recurse(args, &sub);
    
    char* result = NULL;
    while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRUCT)
    {
        dbus_message_iter_recurse(&sub, &ssub);
        char* name, *uniqueName, *langCode;
        boolean enable;
        if (dbus_message_iter_get_arg_type(&ssub) != DBUS_TYPE_STRING)
            goto ipc_set_imlist_end;
        dbus_message_iter_get_basic(&ssub, &name);
        dbus_message_iter_next(&ssub);
        
        if (dbus_message_iter_get_arg_type(&ssub) != DBUS_TYPE_STRING)
            goto ipc_set_imlist_end;
        dbus_message_iter_get_basic(&ssub, &uniqueName);
        dbus_message_iter_next(&ssub);
        
        if (dbus_message_iter_get_arg_type(&ssub) != DBUS_TYPE_STRING)
            goto ipc_set_imlist_end;
        dbus_message_iter_get_basic(&ssub, &langCode);
        dbus_message_iter_next(&ssub);
        
        if (dbus_message_iter_get_arg_type(&ssub) != DBUS_TYPE_BOOLEAN)
            goto ipc_set_imlist_end;
        dbus_message_iter_get_basic(&ssub, &enable);
        dbus_message_iter_next(&ssub);
        
        char* newresult;
        if (result == NULL)
            asprintf(&newresult, "%s:%s", uniqueName, enable ? "True": "False");
        else
            asprintf(&newresult, "%s,%s:%s", result, uniqueName, enable ? "True": "False");
        if (result)
            free(result);
        result = newresult;
ipc_set_imlist_end:
        dbus_message_iter_next(&sub);
    }
    
    FcitxLog(DEBUG, "%s", result);
    if (result)
    {
        FcitxProfile* profile = FcitxInstanceGetProfile(instance);
        if (profile->imList)
            free(profile->imList);
        profile->imList = result;
        UpdateIMList(instance);
    }
}

void IPCUpdateIMList(void* arg)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    DBusMessage* msg = dbus_message_new_signal(FCITX_IM_DBUS_PATH, // object name of the signal
                    DBUS_INTERFACE_PROPERTIES, // interface name of the signal
                    "PropertiesChanged"); // name of the signal
    
    dbus_uint32_t serial;
    DBusMessageIter args;
    DBusMessageIter changed_properties, invalidated_properties;
    char sinterface[] = FCITX_IM_DBUS_INTERFACE;
    char sproperty[] = "IMList";
    char* interface = sinterface;
    char* property = sproperty;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interface);
    
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &changed_properties);
    dbus_message_iter_close_container(&args, &changed_properties);
    
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &invalidated_properties);
    dbus_message_iter_append_basic(&invalidated_properties, DBUS_TYPE_STRING, &property);
    dbus_message_iter_close_container(&args, &invalidated_properties);
    
    if (!dbus_connection_send(ipc->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }
    dbus_connection_flush(ipc->conn);
    dbus_message_unref(msg);
}



// kate: indent-mode cstyle; space-indent on; indent-width 0;
