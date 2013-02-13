/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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

#include <limits.h>
#include <dbus/dbus.h>

#include "fcitx/fcitx.h"
#include "fcitx/frontend.h"
#include "fcitx-utils/utils.h"
#include "module/dbus/fcitx-dbus.h"
#include "module/dbusstuff/property.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx/configfile.h"
#include "fcitx/hook.h"
#include "ipc.h"

#define GetIPCIC(ic) ((FcitxIPCIC*) (ic)->privateic)

#ifndef DBUS_ERROR_UNKNOWN_PROPERTY
#define DBUS_ERROR_UNKNOWN_PROPERTY           "org.freedesktop.DBus.Error.UnknownProperty"
#endif

typedef struct _FcitxIPCCreateICPriv {
    DBusMessage* message;
    DBusConnection* conn;
} FcitxIPCCreateICPriv;

typedef struct _FcitxIPCIC {
    int id;
    char path[32];
    char* appname;
    int width;
    int height;
    pid_t pid;
    char* surroundingText;
    unsigned int anchor;
    unsigned int cursor;
    boolean lastPreeditIsEmpty;
    boolean isPriv;
} FcitxIPCIC;

typedef struct _FcitxIPCFrontend {
    int frontendid;
    int maxid;
    DBusConnection* _conn;
    DBusConnection* _privconn;
    FcitxInstance* owner;
} FcitxIPCFrontend;

typedef struct _FcitxIPCKeyEvent {
    FcitxKeySym sym;
    unsigned int state;
} FcitxIPCKeyEvent;

static void* IPCCreate(FcitxInstance* instance, int frontendid);
static boolean IPCDestroy(void* arg);
void IPCCreateIC(void* arg, FcitxInputContext* context, void *priv);
boolean IPCCheckIC(void* arg, FcitxInputContext* context, void* priv);
void IPCDestroyIC(void* arg, FcitxInputContext* context);
static void IPCEnableIM(void* arg, FcitxInputContext* ic);
static void IPCCloseIM(void* arg, FcitxInputContext* ic);
static void IPCCommitString(void* arg, FcitxInputContext* ic, const char* str);
static void IPCForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
static void IPCSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y);
static void IPCGetWindowRect(void* arg, FcitxInputContext* ic, int* x, int* y, int* w, int* h);
static void IPCUpdatePreedit(void* arg, FcitxInputContext* ic);
static void IPCDeleteSurroundingText(void* arg, FcitxInputContext* ic, int offset, unsigned int size);
static boolean IPCGetSurroundingText(void* arg, FcitxInputContext* ic, char** str, unsigned int* cursor, unsigned int* anchor);
static void IPCUpdateClientSideUI(void* arg, FcitxInputContext* ic);
static DBusHandlerResult IPCDBusEventHandler(DBusConnection *connection, DBusMessage *message, void *user_data);
static DBusHandlerResult IPCICDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data);
static void IPCICFocusIn(FcitxIPCFrontend* ipc, FcitxInputContext* ic);
static void IPCICFocusOut(FcitxIPCFrontend* ipc, FcitxInputContext* ic);
static void IPCICReset(FcitxIPCFrontend* ipc, FcitxInputContext* ic);
static void IPCICSetCursorRect(FcitxIPCFrontend* ipc, FcitxInputContext* ic, int x, int y, int w, int h);
static int IPCProcessKey(FcitxIPCFrontend* ipc, FcitxInputContext* callic, const uint32_t originsym, const uint32_t keycode, const uint32_t originstate, uint32_t t, FcitxKeyEventType type);
static boolean IPCCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic);
static void IPCGetPropertyIMList(void* arg, DBusMessageIter* iter);
static void IPCSetPropertyIMList(void* arg, DBusMessageIter* iter);
static void IPCUpdateIMList(void* arg);
static pid_t IPCGetPid(void* arg, FcitxInputContext* ic);

const FcitxDBusPropertyTable propertTable[] = {
    { FCITX_IM_DBUS_INTERFACE, "IMList", "a(sssb)", IPCGetPropertyIMList, IPCSetPropertyIMList },
    { NULL, NULL, NULL, NULL, NULL }
};

const char * im_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
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
    "<interface name=\"" FCITX_IM_DBUS_INTERFACE "\">"
    "<method name=\"CreateIC\">"
    "<arg name=\"icid\" direction=\"out\" type=\"i\"/>"
    "<arg name=\"keyval1\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"state1\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"keyval2\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"state2\" direction=\"out\" type=\"u\"/>"
    "</method>"
    "<method name=\"CreateICv2\">"
    "<arg name=\"appname\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"icid\" direction=\"out\" type=\"i\"/>"
    "<arg name=\"enable\" direction=\"out\" type=\"b\"/>"
    "<arg name=\"keyval1\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"state1\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"keyval2\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"state2\" direction=\"out\" type=\"u\"/>"
    "</method>"
    "<method name=\"CreateICv3\">"
    "<arg name=\"appname\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"pid\" direction=\"in\" type=\"i\"/>"
    "<arg name=\"icid\" direction=\"out\" type=\"i\"/>"
    "<arg name=\"enable\" direction=\"out\" type=\"b\"/>"
    "<arg name=\"keyval1\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"state1\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"keyval2\" direction=\"out\" type=\"u\"/>"
    "<arg name=\"state2\" direction=\"out\" type=\"u\"/>"
    "</method>"
    "<method name=\"Exit\">"
    "</method>"
    "<method name=\"GetCurrentIM\">"
    "<arg name=\"im\" direction=\"out\" type=\"s\"/>"
    "</method>"
    "<method name=\"SetCurrentIM\">"
    "<arg name=\"im\" direction=\"in\" type=\"s\"/>"
    "</method>"
    "<method name=\"ReloadConfig\">"
    "</method>"
    "<method name=\"ReloadAddonConfig\">"
    "<arg name=\"addon\" direction=\"in\" type=\"s\"/>"
    "</method>"
    "<method name=\"Restart\">"
    "</method>"
    "<method name=\"Configure\">"
    "</method>"
    "<method name=\"ConfigureAddon\">"
    "<arg name=\"addon\" direction=\"in\" type=\"s\"/>"
    "</method>"
    "<method name=\"ConfigureIM\">"
    "<arg name=\"im\" direction=\"in\" type=\"s\"/>"
    "</method>"
    "<method name=\"GetCurrentUI\">"
    "<arg name=\"addon\" direction=\"out\" type=\"s\"/>"
    "</method>"
    "<method name=\"GetIMAddon\">"
    "<arg name=\"im\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"addon\" direction=\"out\" type=\"s\"/>"
    "</method>"
    "<method name=\"ActivateIM\">"
    "</method>"
    "<method name=\"InactivateIM\">"
    "</method>"
    "<method name=\"ToggleIM\">"
    "</method>"
    "<method name=\"GetCurrentState\">"
    "<arg name=\"state\" direction=\"out\" type=\"i\"/>"
    "</method>"
    "<property access=\"readwrite\" type=\"a(sssb)\" name=\"IMList\">"
    "<annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "</property>"
    "</interface>"
    "</node>";

const char * ic_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
    "<node>"
    "<interface name=\"org.freedesktop.DBus.Introspectable\">"
    "<method name=\"Introspect\">"
    "<arg name=\"data\" direction=\"out\" type=\"s\"/>"
    "</method>"
    "</interface>"
    "<interface name=\"" FCITX_IC_DBUS_INTERFACE "\">"
    "<method name=\"EnableIC\">"
    "</method>"
    "<method name=\"CloseIC\">"
    "</method>"
    "<method name=\"FocusIn\">"
    "</method>"
    "<method name=\"FocusOut\">"
    "</method>"
    "<method name=\"Reset\">"
    "</method>"
    "<method name=\"CommitPreedit\">"
    "</method>"
    "<method name=\"MouseEvent\">"
    "<arg name=\"x\" direction=\"in\" type=\"i\"/>"
    "</method>"
    "<method name=\"SetCursorLocation\">"
    "<arg name=\"x\" direction=\"in\" type=\"i\"/>"
    "<arg name=\"y\" direction=\"in\" type=\"i\"/>"
    "</method>"
    "<method name=\"SetCursorRect\">"
    "<arg name=\"x\" direction=\"in\" type=\"i\"/>"
    "<arg name=\"y\" direction=\"in\" type=\"i\"/>"
    "<arg name=\"w\" direction=\"in\" type=\"i\"/>"
    "<arg name=\"h\" direction=\"in\" type=\"i\"/>"
    "</method>"
    "<method name=\"SetCapacity\">"
    "<arg name=\"caps\" direction=\"in\" type=\"u\"/>"
    "</method>"
    "<method name=\"SetSurroundingText\">"
    "<arg name=\"text\" direction=\"in\" type=\"s\"/>"
    "<arg name=\"cursor\" direction=\"in\" type=\"u\"/>"
    "<arg name=\"anchor\" direction=\"in\" type=\"u\"/>"
    "</method>"
    "<method name=\"SetSurroundingTextPosition\">"
    "<arg name=\"cursor\" direction=\"in\" type=\"u\"/>"
    "<arg name=\"anchor\" direction=\"in\" type=\"u\"/>"
    "</method>"
    "<method name=\"DestroyIC\">"
    "</method>"
    "<method name=\"ProcessKeyEvent\">"
    "<arg name=\"keyval\" direction=\"in\" type=\"u\"/>"
    "<arg name=\"keycode\" direction=\"in\" type=\"u\"/>"
    "<arg name=\"state\" direction=\"in\" type=\"u\"/>"
    "<arg name=\"type\" direction=\"in\" type=\"i\"/>"
    "<arg name=\"time\" direction=\"in\" type=\"u\"/>"
    "<arg name=\"ret\" direction=\"out\" type=\"i\"/>"
    "</method>"
    "<signal name=\"EnableIM\">"
    "</signal>"
    "<signal name=\"CloseIM\">"
    "</signal>"
    "<signal name=\"CommitString\">"
    "<arg name=\"str\" type=\"s\"/>"
    "</signal>"
    "<signal name=\"UpdatePreedit\">"
    "<arg name=\"str\" type=\"s\"/>"
    "<arg name=\"cursorpos\" type=\"i\"/>"
    "</signal>"
    "<signal name=\"DeleteSurroundingText\">"
    "<arg name=\"offset\" type=\"i\"/>"
    "<arg name=\"nchar\" type=\"u\"/>"
    "</signal>"
    "<signal name=\"UpdateFormattedPreedit\">"
    "<arg name=\"str\" type=\"a(si)\"/>"
    "<arg name=\"cursorpos\" type=\"i\"/>"
    "</signal>"
    "<signal name=\"UpdateClientSideUI\">"
    "<arg name=\"auxup\" type=\"s\"/>"
    "<arg name=\"auxdown\" type=\"s\"/>"
    "<arg name=\"preedit\" type=\"s\"/>"
    "<arg name=\"candidateword\" type=\"s\"/>"
    "<arg name=\"imname\" type=\"s\"/>"
    "<arg name=\"cursorpos\" type=\"i\"/>"
    "</signal>"
    "<signal name=\"ForwardKey\">"
    "<arg name=\"keyval\" type=\"u\"/>"
    "<arg name=\"state\" type=\"u\"/>"
    "<arg name=\"type\" type=\"i\"/>"
    "</signal>"
    "</interface>"
    "</node>";

FCITX_DEFINE_PLUGIN(fcitx_ipc, frontend, FcitxFrontend) = {
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
    IPCGetWindowRect,
    IPCUpdatePreedit,
    IPCUpdateClientSideUI,
    NULL,
    IPCCheckICFromSameApplication,
    IPCGetPid,
    IPCDeleteSurroundingText,
    IPCGetSurroundingText
};

void* IPCCreate(FcitxInstance* instance, int frontendid)
{
    FcitxIPCFrontend* ipc = fcitx_utils_malloc0(sizeof(FcitxIPCFrontend));
    ipc->frontendid = frontendid;
    ipc->owner = instance;

    ipc->_conn = FcitxDBusGetConnection(instance);
    ipc->_privconn = FcitxDBusGetPrivConnection(instance);

    if (ipc->_conn == NULL && ipc->_privconn == NULL) {
        FcitxLog(ERROR, "DBus Not initialized");
        free(ipc);
        return NULL;
    }


    DBusObjectPathVTable fcitxIPCVTable = {NULL, &IPCDBusEventHandler, NULL, NULL, NULL, NULL };

    if (ipc->_conn) {
        dbus_connection_register_object_path(ipc->_conn,  FCITX_IM_DBUS_PATH, &fcitxIPCVTable, ipc);
    }

    if (ipc->_privconn) {
        dbus_connection_register_object_path(ipc->_privconn, FCITX_IM_DBUS_PATH, &fcitxIPCVTable, ipc);
    }

    FcitxIMEventHook hook;
    hook.arg = ipc;
    hook.func = IPCUpdateIMList;
    FcitxInstanceRegisterUpdateIMListHook(instance, hook);

    return ipc;
}

boolean IPCDestroy(void* arg)
{
    FCITX_UNUSED(arg);
    return true;
}

void IPCCreateIC(void* arg, FcitxInputContext* context, void* priv)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxIPCIC* ipcic = fcitx_utils_new(FcitxIPCIC);
    FcitxIPCCreateICPriv* ipcpriv = (FcitxIPCCreateICPriv*) priv;
    DBusMessage* message = ipcpriv->message;
    DBusMessage *reply = dbus_message_new_method_return(message);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(ipc->owner);

    context->privateic = ipcic;

    ipcic->id = ipc->maxid;
    ipc->maxid ++;
    ipcic->lastPreeditIsEmpty = false;
    ipcic->isPriv = (ipcpriv->conn != ipc->_conn);
    sprintf(ipcic->path, FCITX_IC_DBUS_PATH, ipcic->id);

    uint32_t arg1, arg2, arg3, arg4;
    arg1 = config->hkTrigger[0].sym;
    arg2 = config->hkTrigger[0].state;
    arg3 = config->hkTrigger[1].sym;
    arg4 = config->hkTrigger[1].state;
    if (dbus_message_is_method_call(message, FCITX_IM_DBUS_INTERFACE, "CreateIC")) {
        /* CreateIC v1 indicates that default state can only be disabled */
        context->state = IS_CLOSED;
        context->contextCaps |= CAPACITY_CLIENT_SIDE_CONTROL_STATE;
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
            if (strlen(appname) == 0) {
                ipcic->appname = NULL;
            } else {
                ipcic->appname = strdup(appname);
            }
        }

        if (config->shareState == ShareState_PerProgram)
            FcitxInstanceSetICStateFromSameApplication(ipc->owner, ipc->frontendid, context);

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
    } else if (dbus_message_is_method_call(message, FCITX_IM_DBUS_INTERFACE, "CreateICv3")) {

        DBusError error;
        dbus_error_init(&error);
        char* appname = NULL;
        int icpid = 0;
        if (!dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &appname, DBUS_TYPE_INT32, &icpid, DBUS_TYPE_INVALID))
            ipcic->appname = NULL;
        else {
            if (strlen(appname) == 0) {
                ipcic->appname = NULL;
            } else {
                ipcic->appname = strdup(appname);
            }
        }
        ipcic->pid = icpid;

        if (config->shareState == ShareState_PerProgram)
            FcitxInstanceSetICStateFromSameApplication(ipc->owner, ipc->frontendid, context);

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
    dbus_connection_send(ipcpriv->conn, reply, NULL);
    dbus_message_unref(reply);

    DBusObjectPathVTable vtable = {NULL, &IPCICDBusEventHandler, NULL, NULL, NULL, NULL };
    if (!ipcic->isPriv) {
        if (ipc->_conn) {
            dbus_connection_register_object_path(ipc->_conn, ipcic->path, &vtable, ipc);
            dbus_connection_flush(ipc->_conn);
        }
    }
    else {
        if (ipc->_privconn) {
            dbus_connection_register_object_path(ipc->_privconn, ipcic->path, &vtable, ipc);
            dbus_connection_flush(ipc->_privconn);
        }
    }
}

boolean IPCCheckIC(void* arg, FcitxInputContext* context, void* priv)
{
    FCITX_UNUSED(arg);
    int *id = (int*)priv;
    if (GetIPCIC(context)->id == *id)
        return true;
    return false;
}

void IPCDestroyIC(void* arg, FcitxInputContext* context)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxIPCIC* ipcic = GetIPCIC(context);

    if (!ipcic->isPriv) {
        if (ipc->_conn)
            dbus_connection_unregister_object_path(ipc->_conn, GetIPCIC(context)->path);
    }
    else {
        if (ipc->_privconn)
            dbus_connection_unregister_object_path(ipc->_privconn, GetIPCIC(context)->path);
    }
    fcitx_utils_free(ipcic->appname);
    fcitx_utils_free(ipcic->surroundingText);
    free(context->privateic);
    context->privateic = NULL;
}

void IPCSendSignal(FcitxIPCFrontend* ipc, FcitxIPCIC* ipcic, DBusMessage* msg)
{
    if (!ipcic || !ipcic->isPriv) {
        if (ipc->_conn) {
            dbus_uint32_t serial = 0; // unique number to associate replies with requests
            dbus_connection_send(ipc->_conn, msg, &serial);
            dbus_connection_flush(ipc->_conn);
        }
    }
    if (!ipcic || ipcic->isPriv) {
        if (ipc->_privconn) {
            dbus_uint32_t serial = 0; // unique number to associate replies with requests
            dbus_connection_send(ipc->_privconn, msg, &serial);
            dbus_connection_flush(ipc->_privconn);
        }
    }
    dbus_message_unref(msg);
}

void IPCEnableIM(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "EnableIM"); // name of the signal

    IPCSendSignal(ipc, GetIPCIC(ic), msg);
}

void IPCCloseIM(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "CloseIM"); // name of the signal
    IPCSendSignal(ipc, GetIPCIC(ic), msg);
}

void IPCCommitString(void* arg, FcitxInputContext* ic, const char* str)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;

    if (!fcitx_utf8_check_string(str))
        return;
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "CommitString"); // name of the signal

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
    IPCSendSignal(ipc, GetIPCIC(ic), msg);
}

void IPCForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "ForwardKey"); // name of the signal

    uint32_t keyval = (uint32_t) sym;
    uint32_t keystate = (uint32_t) state;
    int32_t type = (int) event;
    dbus_message_append_args(msg, DBUS_TYPE_UINT32, &keyval, DBUS_TYPE_UINT32, &keystate, DBUS_TYPE_INT32, &type, DBUS_TYPE_INVALID);
    IPCSendSignal(ipc, GetIPCIC(ic), msg);
}

void IPCSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y)
{
    FCITX_UNUSED(arg);
    ic->offset_x = x;
    ic->offset_y = y;
}

void IPCGetWindowRect(void* arg, FcitxInputContext* ic, int* x, int* y, int* w, int* h)
{
    FCITX_UNUSED(arg);
    *x = ic->offset_x;
    *y = ic->offset_y;
    *w = GetIPCIC(ic)->width;
    *h = GetIPCIC(ic)->height;
}

static DBusHandlerResult IPCDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) user_data;
    FcitxInstance* instance = ipc->owner;
    DBusMessage* reply = NULL;
    boolean flush = true;
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(msg);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &im_introspection_xml, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_PROPERTIES, "Get")) {
        reply = FcitxDBusPropertyGet(ipc, propertTable, msg);
    } else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_PROPERTIES, "Set")) {
        reply = FcitxDBusPropertySet(ipc, propertTable, msg);
    } else if (dbus_message_is_method_call(msg, DBUS_INTERFACE_PROPERTIES, "GetAll")) {
        reply = FcitxDBusPropertyGetAll(ipc, propertTable, msg);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "CreateIC")
            || dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "CreateICv2")
            || dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "CreateICv3")
            ) {
        /* we have no choice here, so just return */
        FcitxIPCCreateICPriv ipcpriv;
        ipcpriv.message = msg;
        ipcpriv.conn = connection;
        FcitxInstanceCreateIC(ipc->owner, ipc->frontendid, &ipcpriv);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "Exit")) {
        FcitxLog(INFO, "Receive message ask for quit");
        reply = dbus_message_new_method_return(msg);
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
        dbus_connection_flush(connection);
        FcitxInstanceEnd(instance);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "GetCurrentIM")) {
        reply = dbus_message_new_method_return(msg);
        FcitxIM* im = FcitxInstanceGetCurrentIM(ipc->owner);
        const char* name = "";
        if (im) {
            name = im->uniqueName;
        }
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "SetCurrentIM")) {
        DBusError error;
        dbus_error_init(&error);
        char* imname = NULL;
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &imname, DBUS_TYPE_INVALID)) {
            FcitxInstanceSwitchIMByName(instance, imname);
            reply = dbus_message_new_method_return(msg);
        } else {
            reply = FcitxDBusPropertyUnknownMethod(msg);
        }
        dbus_error_free(&error);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "GetIMAddon")) {
        DBusError error;
        dbus_error_init(&error);
        char* imname = NULL;
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &imname, DBUS_TYPE_INVALID)) {
            FcitxIM* im = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, imname);
            const char* name = "";
            if (im && im->owner)
                name = im->owner->name;
            reply = dbus_message_new_method_return(msg);
            dbus_message_append_args(reply, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);
        } else {
            reply = FcitxDBusPropertyUnknownMethod(msg);
        }
        dbus_error_free(&error);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "GetCurrentUI")) {
        const char* name = "";
        FcitxAddon* uiaddon  = FcitxInstanceGetCurrentUI(instance);
        if (uiaddon) {
            name = uiaddon->name;
        }
        reply = dbus_message_new_method_return(msg);
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "Configure")) {
        reply = dbus_message_new_method_return(msg);
        fcitx_utils_launch_configure_tool();
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "ActivateIM")) {
        FcitxInstanceEnableIM(ipc->owner, FcitxInstanceGetCurrentIC(ipc->owner), false);
        reply = dbus_message_new_method_return(msg);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "InactivateIM")) {
        FcitxInstanceCloseIM(ipc->owner, FcitxInstanceGetCurrentIC(ipc->owner));
        reply = dbus_message_new_method_return(msg);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "ToggleIM")) {
        FcitxInstanceChangeIMState(ipc->owner, FcitxInstanceGetCurrentIC(ipc->owner));
        reply = dbus_message_new_method_return(msg);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "GetCurrentState")) {
        int r = FcitxInstanceGetCurrentState(ipc->owner);
        reply = dbus_message_new_method_return(msg);
        dbus_message_append_args(reply,
                                 DBUS_TYPE_INT32, &r,
                                 DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "ConfigureAddon")) {
        DBusError error;
        dbus_error_init(&error);
        char* addonname = NULL;
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &addonname, DBUS_TYPE_INVALID)) {
            if (addonname) {
                fcitx_utils_launch_configure_tool_for_addon(addonname);
            }
            reply = dbus_message_new_method_return(msg);
        } else {
            reply = FcitxDBusPropertyUnknownMethod(msg);
        }
        dbus_error_free(&error);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "ConfigureIM")) {
        DBusError error;
        dbus_error_init(&error);
        char* imname = NULL;
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &imname, DBUS_TYPE_INVALID)) {
            if (imname) {
                fcitx_utils_launch_configure_tool_for_addon(imname);
            }
            reply = dbus_message_new_method_return(msg);
        } else {
            reply = FcitxDBusPropertyUnknownMethod(msg);
        }
        dbus_error_free(&error);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "ReloadAddonConfig")) {
        DBusError error;
        dbus_error_init(&error);
        char* addonname = NULL;
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &addonname, DBUS_TYPE_INVALID)) {
            if (addonname) {
                FcitxInstanceReloadAddonConfig(instance, addonname);
            }
            reply = dbus_message_new_method_return(msg);
        } else {
            reply = FcitxDBusPropertyUnknownMethod(msg);
        }
        dbus_error_free(&error);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "ReloadConfig")) {
        FcitxInstanceReloadConfig(instance);
        reply = dbus_message_new_method_return(msg);
        flush = true;
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "Restart")) {
        fcitx_utils_launch_restart();
        reply = dbus_message_new_method_return(msg);
        flush = true;
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


static DBusHandlerResult IPCICDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) user_data;
    int id = -1;
    sscanf(dbus_message_get_path(msg), FCITX_IC_DBUS_PATH, &id);
    FcitxInputContext* ic = FcitxInstanceFindIC(ipc->owner, ipc->frontendid, &id);
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage *reply = NULL;
    boolean flush = false;

    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(msg);
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &ic_introspection_xml, DBUS_TYPE_INVALID);
    }

    /* ic is not NULL */
    if (!reply && ic) {
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "EnableIC")) {
            FcitxInstanceEnableIM(ipc->owner, ic, false);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "CloseIC")) {
            FcitxInstanceCloseIM(ipc->owner, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "FocusIn")) {
            IPCICFocusIn(ipc, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "FocusOut")) {
            IPCICFocusOut(ipc, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "Reset")) {
            IPCICReset(ipc, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "MouseEvent")) {
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCursorLocation")) {
            int x, y;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y, DBUS_TYPE_INVALID)) {
                IPCICSetCursorRect(ipc, ic, x, y, 0, 0);
                reply = dbus_message_new_method_return(msg);
            } else {
                reply = FcitxDBusPropertyUnknownMethod(msg);
            }
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCursorRect")) {
            int x, y, w, h;
            if (dbus_message_get_args(msg, &error,
                DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y,
                DBUS_TYPE_INT32, &w, DBUS_TYPE_INT32, &h,
                DBUS_TYPE_INVALID)) {
                IPCICSetCursorRect(ipc, ic, x, y, w, h);
                reply = dbus_message_new_method_return(msg);
            } else {
                reply = FcitxDBusPropertyUnknownMethod(msg);
            }
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCapacity")) {
            uint32_t flags;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_UINT32, &flags, DBUS_TYPE_INVALID)) {
                ic->contextCaps = flags;
                if (!(ic->contextCaps & CAPACITY_SURROUNDING_TEXT)) {
                    if (GetIPCIC(ic)->surroundingText)
                        free(GetIPCIC(ic)->surroundingText);
                    GetIPCIC(ic)->surroundingText = NULL;
                }
                reply = dbus_message_new_method_return(msg);
            } else {
                reply = FcitxDBusPropertyUnknownMethod(msg);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetSurroundingText")) {
            char* text;
            uint32_t cursor, anchor;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &text,  DBUS_TYPE_UINT32, &cursor, DBUS_TYPE_UINT32, &anchor, DBUS_TYPE_INVALID)) {
                FcitxIPCIC* ipcic = GetIPCIC(ic);
                if (!ipcic->surroundingText || strcmp(ipcic->surroundingText, text) != 0 || cursor != ipcic->cursor || anchor != ipcic->anchor)
                {
                    fcitx_utils_free(ipcic->surroundingText);
                    ipcic->surroundingText = strdup(text);
                    ipcic->cursor = cursor;
                    ipcic->anchor = anchor;
                    FcitxInstanceNotifyUpdateSurroundingText(ipc->owner, ic);
                }
                reply = dbus_message_new_method_return(msg);
            } else {
                reply = FcitxDBusPropertyUnknownMethod(msg);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetSurroundingTextPosition")) {
            uint32_t cursor, anchor;
            if (dbus_message_get_args(msg, &error,  DBUS_TYPE_UINT32, &cursor, DBUS_TYPE_UINT32, &anchor, DBUS_TYPE_INVALID)) {
                FcitxIPCIC* ipcic = GetIPCIC(ic);
                if (cursor != ipcic->cursor || anchor != ipcic->anchor)
                {
                    ipcic->cursor = cursor;
                    ipcic->anchor = anchor;
                    FcitxInstanceNotifyUpdateSurroundingText(ipc->owner, ic);
                }
                reply = dbus_message_new_method_return(msg);
            } else {
                reply = FcitxDBusPropertyUnknownMethod(msg);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "DestroyIC")) {
            FcitxInstanceDestroyIC(ipc->owner, ipc->frontendid, &id);
            reply = dbus_message_new_method_return(msg);
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

                reply = dbus_message_new_method_return(msg);

                dbus_message_append_args(reply, DBUS_TYPE_INT32, &ret, DBUS_TYPE_INVALID);
                flush = true;
            } else {
                reply = FcitxDBusPropertyUnknownMethod(msg);
            }
        }
        dbus_error_free(&error);
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

static int IPCProcessKey(FcitxIPCFrontend* ipc, FcitxInputContext* callic, const uint32_t originsym, const uint32_t keycode, const uint32_t originstate, uint32_t t, FcitxKeyEventType type)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(ipc->owner);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(ipc->owner);
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);

    if (ic == NULL || ic->frontendid != callic->frontendid || GetIPCIC(ic)->id != GetIPCIC(callic)->id) {
        FcitxInstanceSetCurrentIC(ipc->owner, callic);
        FcitxUIOnInputFocus(ipc->owner);
    }
    ic = callic;
    FcitxKeySym sym;
    unsigned int state;

    state = originstate & FcitxKeyState_SimpleMask;
    state &= FcitxKeyState_UsedMask;
    FcitxHotkeyGetKey(originsym, state, &sym, &state);
    FcitxLog(DEBUG,
             "KeyRelease=%d  state=%d  KEYCODE=%d  KEYSYM=%u ",
             (type == FCITX_RELEASE_KEY), state, keycode, sym);

    if (originsym == 0)
        return 0;

    if (ic->state == IS_CLOSED && type == FCITX_PRESS_KEY && FcitxHotkeyIsHotKey(sym, state, config->hkTrigger)) {
        FcitxInstanceEnableIM(ipc->owner, ic, false);
        return 1;
    }
    else if (ic->state == IS_CLOSED) {
        return 0;
    }

    FcitxInputStateSetKeyCode(input, keycode);
    FcitxInputStateSetKeySym(input, originsym);
    FcitxInputStateSetKeyState(input, originstate);
    INPUT_RETURN_VALUE retVal = FcitxInstanceProcessKey(ipc->owner, type,
                                           t,
                                           sym, state);
    FcitxInputStateSetKeyCode(input, 0);
    FcitxInputStateSetKeySym(input, 0);
    FcitxInputStateSetKeyState(input, 0);

    if (retVal & IRV_FLAG_FORWARD_KEY || retVal == IRV_TO_PROCESS)
        return 0;
    else
        return 1;
}

static void IPCICFocusIn(FcitxIPCFrontend* ipc, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;

    FcitxInputContext* oldic = FcitxInstanceGetCurrentIC(ipc->owner);

    if (oldic && oldic != ic)
        FcitxUICommitPreedit(ipc->owner);

    if (!FcitxInstanceSetCurrentIC(ipc->owner, ic))
        return;


    if (ic) {
        FcitxUIOnInputFocus(ipc->owner);
    } else {
        FcitxUICloseInputWindow(ipc->owner);
        FcitxUIMoveInputWindow(ipc->owner);
    }

    return;
}

static void IPCICFocusOut(FcitxIPCFrontend* ipc, FcitxInputContext* ic)
{
    FcitxInputContext* currentic = FcitxInstanceGetCurrentIC(ipc->owner);
    if (ic && ic == currentic) {
        FcitxUICommitPreedit(ipc->owner);
        FcitxUICloseInputWindow(ipc->owner);
        FcitxInstanceSetCurrentIC(ipc->owner, NULL);
        FcitxUIOnInputUnFocus(ipc->owner);
    }

    return;
}

static void IPCICReset(FcitxIPCFrontend* ipc, FcitxInputContext* ic)
{
    FcitxInputContext* currentic = FcitxInstanceGetCurrentIC(ipc->owner);
    if (ic && ic == currentic) {
        FcitxUICloseInputWindow(ipc->owner);
        FcitxInstanceResetInput(ipc->owner);
    }

    return;
}

static void IPCICSetCursorRect(FcitxIPCFrontend* ipc, FcitxInputContext* ic, int x, int y, int w, int h)
{
    ic->offset_x = x;
    ic->offset_y = y;
    GetIPCIC(ic)->width = w;
    GetIPCIC(ic)->height = h;
    FcitxUIMoveInputWindow(ipc->owner);

    return;
}

void IPCUpdatePreedit(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);
    FcitxMessages* clientPreedit = FcitxInputStateGetClientPreedit(input);
    int i = 0;
    for (i = 0; i < FcitxMessagesGetMessageCount(clientPreedit) ; i ++) {
        char* str = FcitxMessagesGetMessageString(clientPreedit, i);
        if (!fcitx_utf8_check_string(str))
            return;
    }

    /* a small optimization, don't need to update empty preedit */
    FcitxIPCIC* ipcic = GetIPCIC(ic);
    if (ipcic->lastPreeditIsEmpty && FcitxMessagesGetMessageCount(clientPreedit) == 0)
        return;

    ipcic->lastPreeditIsEmpty = (FcitxMessagesGetMessageCount(clientPreedit) == 0);

    if (ic->contextCaps & CAPACITY_FORMATTED_PREEDIT) {
        DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                        FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                        "UpdateFormattedPreedit"); // name of the signal

        DBusMessageIter args, array, sub;
        dbus_message_iter_init_append(msg, &args);
        dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "(si)", &array);
        int i = 0;
        for (i = 0; i < FcitxMessagesGetMessageCount(clientPreedit) ; i ++) {
            dbus_message_iter_open_container(&array, DBUS_TYPE_STRUCT, 0, &sub);
            char* str = FcitxMessagesGetMessageString(clientPreedit, i);
            char* needtofree = FcitxInstanceProcessOutputFilter(ipc->owner, str);
            if (needtofree) {
                str = needtofree;
            }
            int type = FcitxMessagesGetClientMessageType(clientPreedit, i);
            dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &str);
            dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &type);
            dbus_message_iter_close_container(&array, &sub);
            if (needtofree)
                free(needtofree);
        }
        dbus_message_iter_close_container(&args, &array);

        int iCursorPos = FcitxInputStateGetClientCursorPos(input);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &iCursorPos);

        IPCSendSignal(ipc, GetIPCIC(ic), msg);
    }
    else {
        FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
        FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);
        DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                        FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                        "UpdatePreedit"); // name of the signal

        char* strPreedit = FcitxUIMessagesToCString(FcitxInputStateGetClientPreedit(input));
        char* str = FcitxInstanceProcessOutputFilter(ipc->owner, strPreedit);
        if (str) {
            free(strPreedit);
            strPreedit = str;
        }

        int iCursorPos = FcitxInputStateGetClientCursorPos(input);

        dbus_message_append_args(msg, DBUS_TYPE_STRING, &strPreedit, DBUS_TYPE_INT32, &iCursorPos, DBUS_TYPE_INVALID);

        IPCSendSignal(ipc, GetIPCIC(ic), msg);
        free(strPreedit);
    }
}

void IPCDeleteSurroundingText(void* arg, FcitxInputContext* ic, int offset, unsigned int size)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxIPCIC* ipcic = GetIPCIC(ic);

    /*
     * do the real deletion here, and client might update it, but input method itself
     * would expect a up to date value after this call.
     *
     * Make their life easier.
     */
    if (ipcic->surroundingText) {
        int cursor_pos = ipcic->cursor + offset;
        size_t len = fcitx_utf8_strlen (ipcic->surroundingText);
        if (cursor_pos >= 0 && len - cursor_pos >= size) {
            /*
             * the original size must be larger, so we can do in-place copy here
             * without alloc new string
             */
            char* start = fcitx_utf8_get_nth_char(ipcic->surroundingText, cursor_pos);
            char* end = fcitx_utf8_get_nth_char(start, size);

            int copylen = strlen(end);

            memmove (start, end, sizeof(char) * copylen);
            start[copylen] = 0;
            ipcic->cursor = cursor_pos;
        } else {
            ipcic->surroundingText[0] = '\0';
            ipcic->cursor = 0;
        }
        ipcic->anchor = ipcic->cursor;
    }


    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "DeleteSurroundingText"); // name of the signal

    dbus_message_append_args(msg, DBUS_TYPE_INT32, &offset, DBUS_TYPE_UINT32, &size, DBUS_TYPE_INVALID);

    IPCSendSignal(ipc, GetIPCIC(ic), msg);
}


boolean IPCGetSurroundingText(void* arg, FcitxInputContext* ic, char** str, unsigned int *cursor, unsigned int *anchor)
{
    FCITX_UNUSED(arg);
    FcitxIPCIC* ipcic = GetIPCIC(ic);

    if (!ipcic->surroundingText)
        return false;

    if (str)
        *str = strdup(ipcic->surroundingText);

    if (cursor)
        *cursor = ipcic->cursor;

    if (anchor)
        *anchor = ipcic->anchor;

    return true;
}


void IPCUpdateClientSideUI(void* arg, FcitxInputContext* ic)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);
    DBusMessage* msg = dbus_message_new_signal(GetIPCIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "UpdateClientSideUI"); // name of the signal

    char *str;
    char* strAuxUp = FcitxUIMessagesToCString(FcitxInputStateGetAuxUp(input));
    str = FcitxInstanceProcessOutputFilter(ipc->owner, strAuxUp);
    if (str) {
        free(strAuxUp);
        strAuxUp = str;
    }
    char* strAuxDown = FcitxUIMessagesToCString(FcitxInputStateGetAuxDown(input));
    str = FcitxInstanceProcessOutputFilter(ipc->owner, strAuxDown);
    if (str) {
        free(strAuxDown);
        strAuxDown = str;
    }
    char* strPreedit = FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
    str = FcitxInstanceProcessOutputFilter(ipc->owner, strPreedit);
    if (str) {
        free(strPreedit);
        strPreedit = str;
    }
    char* candidateword = FcitxUICandidateWordToCString(ipc->owner);
    str = FcitxInstanceProcessOutputFilter(ipc->owner, candidateword);
    if (str) {
        free(candidateword);
        candidateword = str;
    }
    FcitxIM* im = FcitxInstanceGetCurrentIM(ipc->owner);
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

    IPCSendSignal(ipc, GetIPCIC(ic), msg);
    free(strAuxUp);
    free(strAuxDown);
    free(strPreedit);
    free(candidateword);
}

boolean IPCCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic)
{
    FCITX_UNUSED(arg);
    FcitxIPCIC* ipcicToCheck = GetIPCIC(icToCheck);
    FcitxIPCIC* ipcic = GetIPCIC(ic);
    if (ipcic->appname == NULL || ipcicToCheck->appname == NULL)
        return false;
    return strcmp(ipcicToCheck->appname, ipcic->appname) == 0;
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
        if (!FcitxInstanceGetIMFromIMList(instance, IMAS_Enable, ime->uniqueName)) {
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
    while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRUCT) {
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
        if (!result) {
            fcitx_utils_alloc_cat_str(newresult, uniqueName, ":",
                                      enable ? "True" : "False");
        } else {
            fcitx_utils_alloc_cat_str(newresult, result, ",", uniqueName, ":",
                                      enable ? "True" : "False");
        }
        if (result)
            free(result);
        result = newresult;
    ipc_set_imlist_end:
        dbus_message_iter_next(&sub);
    }

    FcitxLog(DEBUG, "%s", result);
    if (result) {
        FcitxProfile* profile = FcitxInstanceGetProfile(instance);
        if (profile->imList)
            free(profile->imList);
        profile->imList = result;
        FcitxInstanceUpdateIMList(instance);
    }
}

void IPCUpdateIMList(void* arg)
{
    FcitxIPCFrontend* ipc = (FcitxIPCFrontend*) arg;
    DBusMessage* msg = dbus_message_new_signal(FCITX_IM_DBUS_PATH, // object name of the signal
                       DBUS_INTERFACE_PROPERTIES, // interface name of the signal
                       "PropertiesChanged"); // name of the signal

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

    IPCSendSignal(ipc, NULL, msg);
}

pid_t IPCGetPid(void* arg, FcitxInputContext* ic)
{
    FCITX_UNUSED(arg);
    return GetIPCIC(ic)->pid;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
