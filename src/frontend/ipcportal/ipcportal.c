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
#include <uuid/uuid.h>

#include "fcitx/fcitx.h"
#include "fcitx/frontend.h"
#include "fcitx-utils/utils.h"
#include "module/dbus/fcitx-dbus.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx/configfile.h"
#include "fcitx/hook.h"
#include "ipcportal.h"

#define GetPortalIC(ic) ((FcitxPortalIC*) (ic)->privateic)

typedef struct _FcitxPortalCreateICPriv {
    DBusMessage* message;
    DBusConnection* conn;
} FcitxPortalCreateICPriv;

typedef struct _FcitxLastSentIMInfo
{
    char* name;
    char* uniqueName;
    char* langCode;
} FcitxLastSentIMInfo;

typedef struct _FcitxPortalIC {
    int id;
    char* sender;
    char path[64];
    uuid_t uuid;
    int width;
    int height;
    pid_t pid;
    char* surroundingText;
    unsigned int anchor;
    unsigned int cursor;
    boolean lastPreeditIsEmpty;
    FcitxLastSentIMInfo lastSentIMInfo;
} FcitxPortalIC;

typedef struct _FcitxPortalFrontend {
    int frontendid;
    int maxid;
    DBusConnection* _conn;
    FcitxInstance* owner;
} FcitxPortalFrontend;

typedef struct _FcitxPortalKeyEvent {
    FcitxKeySym sym;
    unsigned int state;
} FcitxPortalKeyEvent;

static void* PortalCreate(FcitxInstance* instance, int frontendid);
static boolean PortalDestroy(void* arg);
void PortalCreateIC(void* arg, FcitxInputContext* context, void *priv);
boolean PortalCheckIC(void* arg, FcitxInputContext* context, void* priv);
void PortalDestroyIC(void* arg, FcitxInputContext* context);
static void PortalEnableIM(void* arg, FcitxInputContext* ic);
static void PortalCloseIM(void* arg, FcitxInputContext* ic);
static void PortalCommitString(void* arg, FcitxInputContext* ic, const char* str);
static void PortalForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
static void PortalSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y);
static void PortalGetWindowRect(void* arg, FcitxInputContext* ic, int* x, int* y, int* w, int* h);
static void PortalUpdatePreedit(void* arg, FcitxInputContext* ic);
static void PortalDeleteSurroundingText(void* arg, FcitxInputContext* ic, int offset, unsigned int size);
static boolean PortalGetSurroundingText(void* arg, FcitxInputContext* ic, char** str, unsigned int* cursor, unsigned int* anchor);
static DBusHandlerResult PortalDBusEventHandler(DBusConnection *connection, DBusMessage *message, void *user_data);
static DBusHandlerResult PortalICDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data);
static void PortalICFocusIn(FcitxPortalFrontend* ipc, FcitxInputContext* ic);
static void PortalICFocusOut(FcitxPortalFrontend* ipc, FcitxInputContext* ic);
static void PortalICReset(FcitxPortalFrontend* ipc, FcitxInputContext* ic);
static void PortalICSetCursorRect(FcitxPortalFrontend* ipc, FcitxInputContext* ic, int x, int y, int w, int h);
static int PortalProcessKey(FcitxPortalFrontend* ipc, FcitxInputContext* callic, const uint32_t originsym, const uint32_t keycode, const uint32_t originstate, uint32_t t, FcitxKeyEventType type);
static boolean PortalCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic);
static void PortalUpdateIMInfoForIC(void* arg);
static pid_t PortalGetPid(void* arg, FcitxInputContext* ic);

const char * im_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
    "<node>"
    "<interface name=\"" DBUS_INTERFACE_INTROSPECTABLE "\">"
    "<method name=\"Introspect\">"
    "<arg name=\"data\" direction=\"out\" type=\"s\"/>"
    "</method>"
    "</interface>"
    "<interface name=\"" FCITX_IM_DBUS_INTERFACE "\">"
    "<method name=\"CreateInputContext\">"
    "<arg type=\"a(ss)\" direction=\"in\"/>"
    "<arg type=\"o\" direction=\"out\"/>"
    "<arg type=\"ay\" direction=\"out\"/>"
    "</method>"
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
    "<method name=\"DestroyIC\">"
    "</method>"
    "<method name=\"FocusIn\">"
    "</method>"
    "<method name=\"FocusOut\">"
    "</method>"
    "<method name=\"ProcessKeyEvent\">"
    "<arg type=\"u\" direction=\"in\"/>"
    "<arg type=\"u\" direction=\"in\"/>"
    "<arg type=\"u\" direction=\"in\"/>"
    "<arg type=\"b\" direction=\"in\"/>"
    "<arg type=\"u\" direction=\"in\"/>"
    "<arg type=\"b\" direction=\"out\"/>"
    "</method>"
    "<method name=\"Reset\">"
    "</method>"
    "<method name=\"SetCapability\">"
    "<arg type=\"t\" direction=\"in\"/>"
    "</method>"
    "<method name=\"SetCursorRect\">"
    "<arg type=\"i\" direction=\"in\"/>"
    "<arg type=\"i\" direction=\"in\"/>"
    "<arg type=\"i\" direction=\"in\"/>"
    "<arg type=\"i\" direction=\"in\"/>"
    "</method>"
    "<method name=\"SetSurroundingText\">"
    "<arg type=\"s\" direction=\"in\"/>"
    "<arg type=\"u\" direction=\"in\"/>"
    "<arg type=\"u\" direction=\"in\"/>"
    "</method>"
    "<method name=\"SetSurroundingTextPosition\">"
    "<arg type=\"u\" direction=\"in\"/>"
    "<arg type=\"u\" direction=\"in\"/>"
    "</method>"
    "<signal name=\"CommitString\">"
    "<arg type=\"s\"/>"
    "</signal>"
    "<signal name=\"CurrentIM\">"
    "<arg type=\"s\"/>"
    "<arg type=\"s\"/>"
    "<arg type=\"s\"/>"
    "</signal>"
    "<signal name=\"DeleteSurroundingText\">"
    "<arg type=\"i\"/>"
    "<arg type=\"u\"/>"
    "</signal>"
    "<signal name=\"ForwardKey\">"
    "<arg type=\"u\"/>"
    "<arg type=\"u\"/>"
    "<arg type=\"b\"/>"
    "</signal>"
    "<signal name=\"UpdateFormattedPreedit\">"
    "<arg type=\"a(si)\"/>"
    "<arg type=\"i\"/>"
    "</signal>"
    "</interface>"
    "</node>";

FCITX_DEFINE_PLUGIN(fcitx_ipcportal, frontend, FcitxFrontend) = {
    PortalCreate,
    PortalDestroy,
    PortalCreateIC,
    PortalCheckIC,
    PortalDestroyIC,
    PortalEnableIM,
    PortalCloseIM,
    PortalCommitString,
    PortalForwardKey,
    PortalSetWindowOffset,
    PortalGetWindowRect,
    PortalUpdatePreedit,
    NULL,
    NULL,
    PortalCheckICFromSameApplication,
    PortalGetPid,
    PortalDeleteSurroundingText,
    PortalGetSurroundingText
};

void* PortalCreate(FcitxInstance* instance, int frontendid)
{
    FcitxPortalFrontend* ipc = fcitx_utils_malloc0(sizeof(FcitxPortalFrontend));
    ipc->frontendid = frontendid;
    ipc->owner = instance;

    ipc->_conn = dbus_bus_get_private(DBUS_BUS_SESSION, NULL);
    if (ipc->_conn == NULL) {
        FcitxLog(ERROR, "DBus Not initialized");
        free(ipc);
        return NULL;
    }

    if (!FcitxDBusAttachConnection(instance, ipc->_conn)) {
        dbus_connection_close(ipc->_conn);
        dbus_connection_unref(ipc->_conn);
        ipc->_conn = NULL;
        free(ipc);
        return NULL;
    }

    int ret = dbus_bus_request_name(ipc->_conn, FCITX_PORTAL_SERVICE,
                                    0,
                                    NULL);
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        FcitxLog(INFO, "Portal Service exists.");
    }

    DBusObjectPathVTable fcitxPortalVTable = {NULL, &PortalDBusEventHandler, NULL, NULL, NULL, NULL };
    dbus_connection_register_object_path(ipc->_conn,  FCITX_IM_DBUS_PORTAL_PATH, &fcitxPortalVTable, ipc);
    dbus_connection_register_object_path(ipc->_conn,  FCITX_IM_DBUS_PORTAL_PATH_OLD, &fcitxPortalVTable, ipc);
    dbus_connection_flush(ipc->_conn);

    FcitxIMEventHook hook;
    hook.arg = ipc;
    hook.func = PortalUpdateIMInfoForIC;
    FcitxInstanceRegisterInputFocusHook(instance, hook);

    return ipc;
}

boolean PortalDestroy(void* arg)
{
    FCITX_UNUSED(arg);
    return true;
}

void PortalCreateIC(void* arg, FcitxInputContext* context, void* priv)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;
    FcitxPortalIC* ipcic = fcitx_utils_new(FcitxPortalIC);
    FcitxPortalCreateICPriv* ipcpriv = (FcitxPortalCreateICPriv*) priv;
    DBusMessage* message = ipcpriv->message;
    DBusMessage *reply = dbus_message_new_method_return(message);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(ipc->owner);

    context->privateic = ipcic;

    ipcic->id = ipc->maxid;
    ipcic->sender = strdup(dbus_message_get_sender(message));
    ipc->maxid ++;
    ipcic->lastPreeditIsEmpty = false;
    sprintf(ipcic->path, FCITX_IC_DBUS_PORTAL_PATH, ipcic->id);
    uuid_generate(ipcic->uuid);

    DBusMessageIter iter, array, sub;
    dbus_message_iter_init(message, &iter);
    /* Message type is a(ss) */
    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        /* First pass: calculate string length */
        dbus_message_iter_recurse(&iter, &array);
        while (dbus_message_iter_get_arg_type(&array) == DBUS_TYPE_STRUCT) {
            dbus_message_iter_recurse(&array, &sub);
            do {
                char *property = NULL, *value = NULL;
                if (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_STRING) {
                    break;
                }
                dbus_message_iter_get_basic(&sub, &property);
                dbus_message_iter_next(&sub);
                if (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_STRING) {
                    break;
                }
                dbus_message_iter_get_basic(&sub, &value);
                dbus_message_iter_next(&sub);
                if (property && value && strcmp(property, "program") == 0) {
                    ((FcitxInputContext2*)context)->prgname = strdup(value);
                }
            } while(0);
            dbus_message_iter_next(&array);
        }
    }



    int icpid = 0;
    ipcic->pid = icpid;

    if (config->shareState == ShareState_PerProgram) {
        FcitxInstanceSetICStateFromSameApplication(ipc->owner, ipc->frontendid, context);
    }

    char *path = ipcic->path;
    dbus_message_append_args(reply,
                             DBUS_TYPE_OBJECT_PATH, &path,
                             // FIXME uuid
                             DBUS_TYPE_INVALID);
    DBusMessageIter args;
    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "y", &array);
    for (int i = 0; i < sizeof(uuid_t); i++) {
        dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &ipcic->uuid[i]);
    }
    dbus_message_iter_close_container(&args, &array);
    dbus_connection_send(ipcpriv->conn, reply, NULL);
    dbus_message_unref(reply);

    DBusObjectPathVTable vtable = {NULL, &PortalICDBusEventHandler, NULL, NULL, NULL, NULL };
    dbus_connection_register_object_path(ipc->_conn, ipcic->path, &vtable, ipc);
    dbus_connection_flush(ipc->_conn);
}

boolean PortalCheckIC(void* arg, FcitxInputContext* context, void* priv)
{
    FCITX_UNUSED(arg);
    int *id = (int*)priv;
    if (GetPortalIC(context)->id == *id)
        return true;
    return false;
}

void PortalDestroyIC(void* arg, FcitxInputContext* context)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;
    FcitxPortalIC* ipcic = GetPortalIC(context);

    dbus_connection_unregister_object_path(ipc->_conn, GetPortalIC(context)->path);
    fcitx_utils_free(ipcic->lastSentIMInfo.name);
    fcitx_utils_free(ipcic->lastSentIMInfo.uniqueName);
    fcitx_utils_free(ipcic->lastSentIMInfo.langCode);
    fcitx_utils_free(ipcic->surroundingText);
    fcitx_utils_free(ipcic->sender);
    free(context->privateic);
    context->privateic = NULL;
}

void PortalSendSignal(FcitxPortalFrontend* ipc, DBusMessage* msg)
{
    if (ipc->_conn) {
        dbus_connection_send(ipc->_conn, msg, NULL);
        dbus_connection_flush(ipc->_conn);
    }
    dbus_message_unref(msg);
}

void PortalEnableIM(void* arg, FcitxInputContext* ic)
{
}

void PortalCloseIM(void* arg, FcitxInputContext* ic)
{
}

void PortalCommitString(void* arg, FcitxInputContext* ic, const char* str)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;

    if (!fcitx_utf8_check_string(str))
        return;
    DBusMessage* msg = dbus_message_new_signal(GetPortalIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "CommitString"); // name of the signal

    dbus_message_append_args(msg, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
    PortalSendSignal(ipc, msg);
}

void PortalForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;
    DBusMessage* msg = dbus_message_new_signal(GetPortalIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "ForwardKey"); // name of the signal

    uint32_t keyval = (uint32_t) sym;
    uint32_t keystate = (uint32_t) state;
    dbus_bool_t type = event == FCITX_RELEASE_KEY;
    dbus_message_append_args(msg, DBUS_TYPE_UINT32, &keyval, DBUS_TYPE_UINT32, &keystate, DBUS_TYPE_BOOLEAN, &type, DBUS_TYPE_INVALID);
    PortalSendSignal(ipc, msg);
}

void PortalSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y)
{
    FCITX_UNUSED(arg);
    ic->offset_x = x;
    ic->offset_y = y - GetPortalIC(ic)->height;
}

void PortalGetWindowRect(void* arg, FcitxInputContext* ic, int* x, int* y, int* w, int* h)
{
    FCITX_UNUSED(arg);
    *x = ic->offset_x;
    *y = ic->offset_y;
    *w = GetPortalIC(ic)->width;
    *h = GetPortalIC(ic)->height;
}

static DBusHandlerResult PortalDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) user_data;
    DBusMessage* reply = NULL;
    boolean flush = true;
    DBusHandlerResult result = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        reply = dbus_message_new_method_return(msg);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &im_introspection_xml, DBUS_TYPE_INVALID);
    } else if (dbus_message_is_method_call(msg, FCITX_IM_DBUS_INTERFACE, "CreateInputContext")) {
        /* we have no choice here, so just return */
        FcitxPortalCreateICPriv ipcpriv;
        ipcpriv.message = msg;
        ipcpriv.conn = connection;
        FcitxInstanceCreateIC(ipc->owner, ipc->frontendid, &ipcpriv);
        return DBUS_HANDLER_RESULT_HANDLED;
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


static DBusHandlerResult PortalICDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) user_data;
    int id = -1;
    sscanf(dbus_message_get_path(msg), FCITX_IC_DBUS_PORTAL_PATH, &id);
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
        if (strcmp(dbus_message_get_sender(msg), GetPortalIC(ic)->sender) != 0) {
            reply = dbus_message_new_error(msg, "org.fcitx.Fcitx.Error", "Invalid sender");
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "FocusIn")) {
            PortalICFocusIn(ipc, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "FocusOut")) {
            PortalICFocusOut(ipc, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "Reset")) {
            PortalICReset(ipc, ic);
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "MouseEvent")) {
            reply = dbus_message_new_method_return(msg);
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCursorRect")) {
            int x, y, w, h;
            if (dbus_message_get_args(msg, &error,
                DBUS_TYPE_INT32, &x, DBUS_TYPE_INT32, &y,
                DBUS_TYPE_INT32, &w, DBUS_TYPE_INT32, &h,
                DBUS_TYPE_INVALID)) {
                PortalICSetCursorRect(ipc, ic, x, y, w, h);
                reply = dbus_message_new_method_return(msg);
            }
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetCapability")) {
            uint64_t flags;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_UINT64, &flags, DBUS_TYPE_INVALID)) {
                ic->contextCaps = flags;
                if (!(ic->contextCaps & CAPACITY_SURROUNDING_TEXT)) {
                    if (GetPortalIC(ic)->surroundingText)
                        free(GetPortalIC(ic)->surroundingText);
                    GetPortalIC(ic)->surroundingText = NULL;
                }
                if (ic->contextCaps & CAPACITY_GET_IM_INFO_ON_FOCUS) {
                    PortalUpdateIMInfoForIC(ipc);
                }
                reply = dbus_message_new_method_return(msg);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetSurroundingText")) {
            char* text;
            uint32_t cursor, anchor;
            if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &text,  DBUS_TYPE_UINT32, &cursor, DBUS_TYPE_UINT32, &anchor, DBUS_TYPE_INVALID)) {
                FcitxPortalIC* ipcic = GetPortalIC(ic);
                if (!ipcic->surroundingText || strcmp(ipcic->surroundingText, text) != 0 || cursor != ipcic->cursor || anchor != ipcic->anchor)
                {
                    fcitx_utils_free(ipcic->surroundingText);
                    ipcic->surroundingText = strdup(text);
                    ipcic->cursor = cursor;
                    ipcic->anchor = anchor;
                    FcitxInstanceNotifyUpdateSurroundingText(ipc->owner, ic);
                }
                reply = dbus_message_new_method_return(msg);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "SetSurroundingTextPosition")) {
            uint32_t cursor, anchor;
            if (dbus_message_get_args(msg, &error,  DBUS_TYPE_UINT32, &cursor, DBUS_TYPE_UINT32, &anchor, DBUS_TYPE_INVALID)) {
                FcitxPortalIC* ipcic = GetPortalIC(ic);
                if (cursor != ipcic->cursor || anchor != ipcic->anchor)
                {
                    ipcic->cursor = cursor;
                    ipcic->anchor = anchor;
                    FcitxInstanceNotifyUpdateSurroundingText(ipc->owner, ic);
                }
                reply = dbus_message_new_method_return(msg);
            }
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "DestroyIC")) {
            FcitxInstanceDestroyIC(ipc->owner, ipc->frontendid, &id);
            reply = dbus_message_new_method_return(msg);
            result = DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call(msg, FCITX_IC_DBUS_INTERFACE, "ProcessKeyEvent")) {
            uint32_t keyval, keycode, state, t;
            dbus_bool_t ret;
            dbus_bool_t itype;
            FcitxKeyEventType type;
            if (dbus_message_get_args(msg, &error,
                                      DBUS_TYPE_UINT32, &keyval,
                                      DBUS_TYPE_UINT32, &keycode,
                                      DBUS_TYPE_UINT32, &state,
                                      DBUS_TYPE_BOOLEAN, &itype,
                                      DBUS_TYPE_UINT32, &t,
                                      DBUS_TYPE_INVALID)) {
                type = itype;
                ret = PortalProcessKey(ipc, ic, keyval, keycode, state, t, type) > 0;

                reply = dbus_message_new_method_return(msg);

                dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &ret, DBUS_TYPE_INVALID);
                flush = true;
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

static int PortalProcessKey(FcitxPortalFrontend* ipc, FcitxInputContext* callic, const uint32_t originsym, const uint32_t keycode, const uint32_t originstate, uint32_t t, FcitxKeyEventType type)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(ipc->owner);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(ipc->owner);
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);

    if (ic == NULL || ic->frontendid != callic->frontendid || GetPortalIC(ic)->id != GetPortalIC(callic)->id) {
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

static void PortalICFocusIn(FcitxPortalFrontend* ipc, FcitxInputContext* ic)
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

static void PortalICFocusOut(FcitxPortalFrontend* ipc, FcitxInputContext* ic)
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

static void PortalICReset(FcitxPortalFrontend* ipc, FcitxInputContext* ic)
{
    FcitxInputContext* currentic = FcitxInstanceGetCurrentIC(ipc->owner);
    if (ic && ic == currentic) {
        FcitxUICloseInputWindow(ipc->owner);
        FcitxInstanceResetInput(ipc->owner);
    }

    return;
}

static void PortalICSetCursorRect(FcitxPortalFrontend* ipc, FcitxInputContext* ic, int x, int y, int w, int h)
{
    ic->offset_x = x;
    ic->offset_y = y;
    GetPortalIC(ic)->width = w;
    GetPortalIC(ic)->height = h;
    FcitxUIMoveInputWindow(ipc->owner);

    return;
}

void PortalUpdatePreedit(void* arg, FcitxInputContext* ic)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(ipc->owner);
    FcitxMessages* clientPreedit = FcitxInputStateGetClientPreedit(input);
    for (int i = 0; i < FcitxMessagesGetMessageCount(clientPreedit) ; i ++) {
        char* str = FcitxMessagesGetMessageString(clientPreedit, i);
        if (!fcitx_utf8_check_string(str))
            return;
    }

    /* a small optimization, don't need to update empty preedit */
    FcitxPortalIC* ipcic = GetPortalIC(ic);
    if (ipcic->lastPreeditIsEmpty && FcitxMessagesGetMessageCount(clientPreedit) == 0)
        return;

    ipcic->lastPreeditIsEmpty = (FcitxMessagesGetMessageCount(clientPreedit) == 0);

    DBusMessage* msg = dbus_message_new_signal(GetPortalIC(ic)->path, // object name of the signal
                    FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                    "UpdateFormattedPreedit"); // name of the signal

    DBusMessageIter args, array, sub;
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "(si)", &array);
    for (int i = 0; i < FcitxMessagesGetMessageCount(clientPreedit) ; i ++) {
        dbus_message_iter_open_container(&array, DBUS_TYPE_STRUCT, 0, &sub);
        char* str = FcitxMessagesGetMessageString(clientPreedit, i);
        char* needtofree = FcitxInstanceProcessOutputFilter(ipc->owner, str);
        if (needtofree) {
            str = needtofree;
        }
        int type = FcitxMessagesGetClientMessageType(clientPreedit, i);
        // This protocol uses "underline" instead of "non underline"
        type = type ^ MSG_NOUNDERLINE;
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &str);
        dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &type);
        dbus_message_iter_close_container(&array, &sub);
        if (needtofree)
            free(needtofree);
    }
    dbus_message_iter_close_container(&args, &array);

    int iCursorPos = FcitxInputStateGetClientCursorPos(input);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &iCursorPos);

    PortalSendSignal(ipc, msg);
}

void PortalDeleteSurroundingText(void* arg, FcitxInputContext* ic, int offset, unsigned int size)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;
    FcitxPortalIC* ipcic = GetPortalIC(ic);

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


    DBusMessage* msg = dbus_message_new_signal(GetPortalIC(ic)->path, // object name of the signal
                       FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                       "DeleteSurroundingText"); // name of the signal

    dbus_message_append_args(msg, DBUS_TYPE_INT32, &offset, DBUS_TYPE_UINT32, &size, DBUS_TYPE_INVALID);

    PortalSendSignal(ipc, msg);
}


boolean PortalGetSurroundingText(void* arg, FcitxInputContext* ic, char** str, unsigned int *cursor, unsigned int *anchor)
{
    FCITX_UNUSED(arg);
    FcitxPortalIC* ipcic = GetPortalIC(ic);

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

boolean PortalCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic)
{
    FCITX_UNUSED(arg);
    FcitxInputContext2* ic2ToCheck = (FcitxInputContext2*)icToCheck;
    FcitxInputContext2* ic2 = (FcitxInputContext2*)ic;
    if (ic2->prgname == NULL || ic2ToCheck->prgname == NULL)
        return false;
    return strcmp(ic2ToCheck->prgname, ic2->prgname) == 0;
}

void PortalUpdateIMInfoForIC(void* arg)
{
    FcitxPortalFrontend* ipc = (FcitxPortalFrontend*) arg;
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(ipc->owner);
    if (ic && (ic->contextCaps & CAPACITY_GET_IM_INFO_ON_FOCUS) && ic->frontendid == ipc->frontendid) {
        FcitxIM* im = FcitxInstanceGetCurrentIM(ipc->owner);
        const char* name = (im && im->strName && fcitx_utf8_check_string(im->strName)) ? im->strName : "";
        const char* uniqueName = (im && im->uniqueName && fcitx_utf8_check_string(im->uniqueName)) ? im->uniqueName : "";
        const char* langCode = (im && fcitx_utf8_check_string(im->langCode)) ? im->langCode : "";

        if (fcitx_utils_strcmp0(GetPortalIC(ic)->lastSentIMInfo.name, name) == 0 &&
            fcitx_utils_strcmp0(GetPortalIC(ic)->lastSentIMInfo.uniqueName, uniqueName) == 0 &&
            fcitx_utils_strcmp0(GetPortalIC(ic)->lastSentIMInfo.langCode, langCode) == 0) {
            return;
        }

        DBusMessage* msg = dbus_message_new_signal(GetPortalIC(ic)->path, // object name of the signal
                        FCITX_IC_DBUS_INTERFACE, // interface name of the signal
                        "CurrentIM"); // name of the signal

        fcitx_utils_string_swap(&GetPortalIC(ic)->lastSentIMInfo.name, name);
        fcitx_utils_string_swap(&GetPortalIC(ic)->lastSentIMInfo.uniqueName, uniqueName);
        fcitx_utils_string_swap(&GetPortalIC(ic)->lastSentIMInfo.langCode, langCode);

        dbus_message_append_args(msg, DBUS_TYPE_STRING, &name, DBUS_TYPE_STRING, &uniqueName, DBUS_TYPE_STRING, &langCode, DBUS_TYPE_INVALID);
        PortalSendSignal(ipc, msg);
    }
}

pid_t PortalGetPid(void* arg, FcitxInputContext* ic)
{
    FCITX_UNUSED(arg);
    return GetPortalIC(ic)->pid;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
