/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#include <stdlib.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include "module/dbus/dbusstuff.h"
#include "frontend/ipc/ipc.h"
#include "fcitx/fcitx.h"
#include "fcitxclient.h"
#include "fcitxconnection.h"
#include "marshall.h"

#ifdef _DEBUG
#define fcitx_gclient_debug(...) g_log ("fcitx-client",       \
                                      G_LOG_LEVEL_DEBUG,    \
                                      __VA_ARGS__)

#else
#define fcitx_gclient_debug(...)
#endif
typedef struct _ProcessKeyStruct ProcessKeyStruct;

/**
 * FcitxClient:
 *
 * A #FcitxClient allow to create a input context via DBus
 */

struct _ProcessKeyStruct {
    FcitxClient* self;
    GAsyncReadyCallback callback;
    void* user_data;
};

struct _FcitxClientPrivate {
    GDBusProxy* improxy;
    GDBusProxy* icproxy;
    char servicename[64];
    char icname[64];
    int id;
    GCancellable* cancellable;
    FcitxConnection* connection;
};

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name=\"" FCITX_IM_DBUS_INTERFACE "\">"
    "    <method name=\"CreateICv3\">\n"
    "      <arg name=\"appname\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"pid\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"icid\" direction=\"out\" type=\"i\"/>\n"
    "      <arg name=\"enable\" direction=\"out\" type=\"b\"/>\n"
    "      <arg name=\"keyval1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"keyval2\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state2\" direction=\"out\" type=\"u\"/>\n"
    "    </method>\n"
    "  </interface>"
    "</node>";


static const gchar ic_introspection_xml[] =
    "<node>\n"
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
    "    <method name=\"SetCursorRect\">\n"
    "      <arg name=\"x\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"y\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"w\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"h\" direction=\"in\" type=\"i\"/>\n"
    "    </method>\n"
    "    <method name=\"SetCapacity\">\n"
    "      <arg name=\"caps\" direction=\"in\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"SetSurroundingText\">\n"
    "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"cursor\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"anchor\" direction=\"in\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"SetSurroundingTextPosition\">\n"
    "      <arg name=\"cursor\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"anchor\" direction=\"in\" type=\"u\"/>\n"
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
    "    <signal name=\"DeleteSurroundingText\">\n"
    "      <arg name=\"offset\" type=\"i\"/>\n"
    "      <arg name=\"nchar\" type=\"u\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateFormattedPreedit\">\n"
    "      <arg name=\"str\" type=\"a(si)\"/>\n"
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
GType        fcitx_client_get_type(void) G_GNUC_CONST;

G_DEFINE_TYPE(FcitxClient, fcitx_client, G_TYPE_OBJECT);

#define FCITX_CLIENT_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), FCITX_TYPE_CLIENT, FcitxClientPrivate))

enum {
    CONNECTED_SIGNAL,
    ENABLE_IM_SIGNAL,
    CLOSE_IM_SIGNAL,
    FORWARD_KEY_SIGNAL,
    COMMIT_STRING_SIGNAL,
    DELETE_SURROUNDING_TEXT_SIGNAL,
    UPDATED_FORMATTED_PREEDIT_SIGNAL,
    DISCONNECTED_SIGNAL,
    UPDATE_CLIENT_SIDE_UI_SIGNAL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static GDBusInterfaceInfo *_fcitx_client_get_interface_info(void);
static GDBusInterfaceInfo *_fcitx_client_get_clientic_info(void);
static void _fcitx_client_create_ic(FcitxConnection* connection, gpointer user_data);
static void _fcitx_client_disconnect(FcitxConnection* connection, gpointer user_data);
static void _fcitx_client_create_ic_phase1_finished(GObject* source_object, GAsyncResult* res, gpointer user_data);
static void _fcitx_client_create_ic_cb(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_client_create_ic_phase2_finished(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_client_g_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, gpointer user_data);
static void fcitx_client_init(FcitxClient *self);
static void fcitx_client_finalize(GObject *object);
static void fcitx_client_dispose(GObject *object);
static void _fcitx_client_clean_up(FcitxClient* self, gboolean dont_emit_disconn);

static void fcitx_client_class_init(FcitxClientClass *klass);

static void _item_free(gpointer arg);

static GDBusInterfaceInfo *
_fcitx_client_get_interface_info(void)
{
    static gsize has_info = 0;
    static GDBusInterfaceInfo *info = NULL;
    if (g_once_init_enter(&has_info)) {
        GDBusNodeInfo *introspection_data;
        introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
        info = introspection_data->interfaces[0];
        g_once_init_leave(&has_info, 1);
    }
    return info;
}

static GDBusInterfaceInfo *
_fcitx_client_get_clientic_info(void)
{
    static gsize has_info = 0;
    static GDBusInterfaceInfo *info = NULL;
    if (g_once_init_enter(&has_info)) {
        GDBusNodeInfo *introspection_data;
        introspection_data = g_dbus_node_info_new_for_xml(ic_introspection_xml, NULL);
        info = introspection_data->interfaces[0];
        g_once_init_leave(&has_info, 1);
    }
    return info;
}

static void
fcitx_client_finalize(GObject *object)
{
    if (G_OBJECT_CLASS(fcitx_client_parent_class)->finalize != NULL)
        G_OBJECT_CLASS(fcitx_client_parent_class)->finalize(object);
}

static void
fcitx_client_dispose(GObject *object)
{
    FcitxClient *self = FCITX_CLIENT(object);

    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "DestroyIC", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }

#ifndef g_signal_handlers_disconnect_by_data
#define g_signal_handlers_disconnect_by_data(instance, data) \
    g_signal_handlers_disconnect_matched ((instance), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (data))
#endif
    g_signal_handlers_disconnect_by_data(self->priv->connection,
                                         self);
    g_object_unref(self->priv->connection);
    _fcitx_client_clean_up(self, TRUE);

    if (G_OBJECT_CLASS(fcitx_client_parent_class)->dispose != NULL)
        G_OBJECT_CLASS(fcitx_client_parent_class)->dispose(object);
}

/**
 * fcitx_client_enable_ic:
 * @self: A #FcitxClient
 *
 * tell fcitx activate current ic
 **/
FCITX_EXPORT_API
void fcitx_client_enable_ic(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "EnableIC", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_close_ic:
 * @self: A #FcitxClient
 *
 * tell fcitx inactivate current ic
 **/
FCITX_EXPORT_API
void fcitx_client_close_ic(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "CloseIC", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_focus_in:
 * @self: A #FcitxClient
 *
 * tell fcitx current client has focus
 **/
FCITX_EXPORT_API
void fcitx_client_focus_in(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "FocusIn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_focus_out:
 * @self: A #FcitxClient
 *
 * tell fcitx current client has lost focus
 **/
FCITX_EXPORT_API
void fcitx_client_focus_out(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "FocusOut", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_reset:
 * @self: A #FcitxClient
 *
 * tell fcitx current client is reset from client side
 **/
FCITX_EXPORT_API
void fcitx_client_reset(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "Reset", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_set_capacity:
 * @self: A #FcitxClient
 * @flags: capacity
 *
 * set client capacity of Fcitx
 **/
FCITX_EXPORT_API
void fcitx_client_set_capacity(FcitxClient* self, guint flags)
{
    uint32_t iflags = flags;
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "SetCapacity", g_variant_new("(u)", iflags), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_set_cusor_rect:
 * @self A #FcitxClient
 * @x x of cursor
 * @y y of cursor
 * @w width of cursor
 * @h height of cursor
 *
 * Deprecated:
 *
 * tell fcitx current client's cursor geometry info
 **/
FCITX_EXPORT_API
void fcitx_client_set_cusor_rect(FcitxClient* self, int x, int y, int w, int h)
{
    fcitx_client_set_cursor_rect(self, x, y, w, h);
}

/**
 * fcitx_client_set_cursor_rect:
 * @self: A #FcitxClient
 * @x: x of cursor
 * @y: y of cursor
 * @w: width of cursor
 * @h: height of cursor
 *
 * tell fcitx current client's cursor geometry info
 **/
FCITX_EXPORT_API
void fcitx_client_set_cursor_rect(FcitxClient* self, int x, int y, int w, int h)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "SetCursorRect", g_variant_new("(iiii)", x, y, w, h), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

/**
 * fcitx_client_set_surrounding_text:
 * @self: A #FcitxClient
 * @text: (transfer none) (allow-none): surroundng text
 * @cursor: cursor position coresponding to text
 * @anchor: anchor position coresponding to text
 **/
FCITX_EXPORT_API
void fcitx_client_set_surrounding_text(FcitxClient* self, gchar* text, guint cursor, guint anchor)
{
    if (self->priv->icproxy) {
        if (text) {
            g_dbus_proxy_call(self->priv->icproxy, "SetSurroundingText", g_variant_new("(suu)", text, cursor, anchor), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
        }
        else {
            g_dbus_proxy_call(self->priv->icproxy, "SetSurroundingTextPosition", g_variant_new("(uu)", cursor, anchor), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
        }
    }
}

/**
 * fcitx_client_process_key:
 * @self: A #FcitxClient
 * @cb: callback
 * @user_data: user data
 * @keyval: key value
 * @keycode: hardware key code
 * @state: key state
 * @type: event type
 * @t: timestamp
 *
 * Deprecated:
 *
 * send a key event to fcitx asynchronizely, you need to use #g_dbus_proxy_call_finish with this function
 **/
FCITX_EXPORT_API
void fcitx_client_process_key(FcitxClient* self, GAsyncReadyCallback cb, gpointer user_data, guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 t)
{
    int itype = type;
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy,
                          "ProcessKeyEvent",
                          g_variant_new("(uuuiu)", keyval, keycode, state, itype, t),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1, NULL,
                          cb,
                          user_data);
    }
}

/**
 * fcitx_client_process_key_finish:
 * @self: A #FcitxClient
 * @res: result
 *
 * use this function with #fcitx_client_process_key_async
 *
 * Returns: process key result
 **/
FCITX_EXPORT_API
gint fcitx_client_process_key_finish(FcitxClient* self, GAsyncResult* res)
{
    gint ret = -1;
    if (!self->priv->icproxy)
        return -1;

    GVariant* result = g_dbus_proxy_call_finish(self->priv->icproxy, res, NULL);
    if (result) {
        g_variant_get(result, "(i)", &ret);
        g_variant_unref(result);
    }
    return ret;
}

void _process_key_data_free(ProcessKeyStruct* pk)
{
    g_object_unref(pk->self);
    g_free(pk);
}

void
_fcitx_client_process_key_cb(GObject *source_object,
                             GAsyncResult *res,
                             gpointer user_data)
{
    FCITX_UNUSED(source_object);
    ProcessKeyStruct* pk = user_data;
    pk->callback(G_OBJECT(pk->self), res, pk->user_data);
    _process_key_data_free(pk);
}

void _fcitx_client_process_key_cancelled(GCancellable* cancellable, gpointer user_data)
{
    FCITX_UNUSED(cancellable);
    ProcessKeyStruct* pk = user_data;
    _process_key_data_free(pk);
}

/**
 * fcitx_client_process_key_async:
 * @self: A #FcitxClient
 * @keyval: key value
 * @keycode: hardware key code
 * @state: key state
 * @type: event type
 * @t: timestamp
 * @timeout_msec: timeout in millisecond
 * @cancellable: cancellable
 * @callback: (scope async) (closure user_data): callback
 * @user_data: (closure): user data
 *
 * use this function with #fcitx_client_process_key_finish
 **/
FCITX_EXPORT_API
void fcitx_client_process_key_async(FcitxClient* self,
                                    guint32 keyval, guint32 keycode,
                                    guint32 state, gint type, guint32 t,
                                    gint timeout_msec,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
    int itype = type;
    if (self->priv->icproxy) {
        ProcessKeyStruct* pk = g_new(ProcessKeyStruct, 1);
        pk->self = g_object_ref(self);
        pk->callback = callback;
        pk->user_data = user_data;
        g_dbus_proxy_call(self->priv->icproxy,
                          "ProcessKeyEvent",
                          g_variant_new("(uuuiu)", keyval, keycode, state, itype, t),
                          G_DBUS_CALL_FLAGS_NONE,
                          timeout_msec,
                          cancellable,
                          _fcitx_client_process_key_cb,
                          pk);
    }
}

/**
 * fcitx_client_process_key_sync:
 * @self: A #FcitxClient
 * @keyval: key value
 * @keycode: hardware key code
 * @state: key state
 * @type: event type
 * @t: timestamp
 *
 * send a key event to fcitx synchronizely
 *
 * Returns: the key is processed or not
 */
FCITX_EXPORT_API
int fcitx_client_process_key_sync(FcitxClient* self, guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 t)
{
    int itype = type;
    int ret = -1;
    if (self->priv->icproxy) {
        GVariant* result =  g_dbus_proxy_call_sync(self->priv->icproxy,
                            "ProcessKeyEvent",
                            g_variant_new("(uuuiu)", keyval, keycode, state, itype, t),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1, NULL,
                            NULL);

        if (result) {
            g_variant_get(result, "(i)", &ret);
            g_variant_unref(result);
        }
    }

    return ret;
}

static void
fcitx_client_init(FcitxClient *self)
{
    self->priv = FCITX_CLIENT_GET_PRIVATE(self);

    sprintf(self->priv->servicename, "%s-%d", FCITX_DBUS_SERVICE, fcitx_utils_get_display_number());

    self->priv->connection = NULL;
    self->priv->cancellable = NULL;
    self->priv->improxy = NULL;
    self->priv->icproxy = NULL;
    self->priv->connection = fcitx_connection_new();

    g_signal_connect (self->priv->connection, "connected", (GCallback) _fcitx_client_create_ic, self);
    g_signal_connect (self->priv->connection, "disconnected", (GCallback) _fcitx_client_disconnect, self);
}

static void
_fcitx_client_create_ic(FcitxConnection* connection, gpointer user_data)
{
    FCITX_UNUSED(connection);
    fcitx_gclient_debug("_fcitx_client_create_ic");
    FcitxClient *self = user_data;

    _fcitx_client_clean_up(self, FALSE);

    g_object_ref(self);
    self->priv->cancellable = g_cancellable_new ();
    g_dbus_proxy_new(
        fcitx_connection_get_g_dbus_connection(self->priv->connection),
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
        _fcitx_client_get_interface_info(),
        self->priv->servicename,
        FCITX_IM_DBUS_PATH,
        FCITX_IM_DBUS_INTERFACE,
        self->priv->cancellable,
        _fcitx_client_create_ic_phase1_finished,
        self
    );
}

static void
_fcitx_client_disconnect(FcitxConnection* connection, gpointer user_data)
{
    FCITX_UNUSED(connection);
    FcitxClient *self = user_data;
    _fcitx_client_clean_up(self, FALSE);
}

static void
_fcitx_client_create_ic_phase1_finished(GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data)
{
    FCITX_UNUSED(source_object);
    fcitx_gclient_debug("_fcitx_client_create_ic_phase1_finished");
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CLIENT(user_data));
    FcitxClient* self = (FcitxClient*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    if (self->priv->improxy)
        g_object_unref(self->priv->improxy);
    self->priv->improxy = g_dbus_proxy_new_finish(res, NULL);

    do {
        if (!self->priv->improxy) {
            break;
        }

        gchar* owner_name = g_dbus_proxy_get_name_owner(self->priv->improxy);

        if (!owner_name) {
            g_object_unref(self->priv->improxy);
            self->priv->improxy = NULL;
            break;
        }
        g_free(owner_name);
    } while(0);

    if (!self->priv->improxy) {
        /* unref for create_ic */
        g_object_unref(self);
        return;
    }

    self->priv->cancellable = g_cancellable_new ();
    char* appname = fcitx_utils_get_process_name();
    int pid = getpid();
    g_dbus_proxy_call(
        self->priv->improxy,
        "CreateICv3",
        g_variant_new("(si)", appname, pid),
        G_DBUS_CALL_FLAGS_NONE,
        -1,           /* timeout */
        self->priv->cancellable,
        _fcitx_client_create_ic_cb,
        self
    );
    free(appname);

}

static void
_fcitx_client_create_ic_cb(GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
    FcitxClient* self = (FcitxClient*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    GVariant* result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, NULL);

    if (!result) {
        /* unref for _fcitx_client_phase1_finish */
        g_object_unref(self);
        return;
    }

    gboolean enable;
    guint32 key1, state1, key2, state2;
    g_variant_get(result, "(ibuuuu)", &self->priv->id, &enable, &key1, &state1, &key2, &state2);
    g_variant_unref(result);

    sprintf(self->priv->icname, FCITX_IC_DBUS_PATH, self->priv->id);

    self->priv->cancellable = g_cancellable_new ();
    g_dbus_proxy_new(
        fcitx_connection_get_g_dbus_connection(self->priv->connection),
        G_DBUS_PROXY_FLAGS_NONE,
        _fcitx_client_get_clientic_info(),
        self->priv->servicename,
        self->priv->icname,
        FCITX_IC_DBUS_INTERFACE,
        self->priv->cancellable,
        _fcitx_client_create_ic_phase2_finished,
        self
    );
}


static void
_fcitx_client_create_ic_phase2_finished(GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data)
{
    FCITX_UNUSED(source_object);
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CLIENT(user_data));
    FcitxClient* self = (FcitxClient*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    if (self->priv->icproxy)
        g_object_unref(self->priv->icproxy);
    self->priv->icproxy = g_dbus_proxy_new_finish(res, NULL);

    do {
        if (!self->priv->icproxy)
            break;

        gchar* owner_name = g_dbus_proxy_get_name_owner(self->priv->icproxy);

        if (!owner_name) {
            g_object_unref(self->priv->icproxy);
            self->priv->icproxy = NULL;
            break;
        }
        g_free(owner_name);
    } while(0);

    if (self->priv->icproxy) {
        g_signal_connect(self->priv->icproxy, "g-signal", G_CALLBACK(_fcitx_client_g_signal), self);
        g_signal_emit(user_data, signals[CONNECTED_SIGNAL], 0);
    }

    /* unref for _fcitx_client_create_ic_cb */
    g_object_unref(self);
}

static void
_item_free(gpointer arg)
{
    FcitxPreeditItem* item = arg;
    free(item->string);
    free(item);
}

static void
_fcitx_client_g_signal(GDBusProxy *proxy,
                       gchar      *sender_name,
                       gchar      *signal_name,
                       GVariant   *parameters,
                       gpointer    user_data)
{
    FCITX_UNUSED(proxy);
    FCITX_UNUSED(sender_name);
    if (strcmp(signal_name, "EnableIM") == 0) {
        g_signal_emit(user_data, signals[ENABLE_IM_SIGNAL], 0);
    }
    else if (strcmp(signal_name, "CloseIM") == 0) {
        g_signal_emit(user_data, signals[CLOSE_IM_SIGNAL], 0);
    }
    else if (strcmp(signal_name, "CommitString") == 0) {
        const gchar* data = NULL;
        g_variant_get(parameters, "(s)", &data);
        if (data) {
            g_signal_emit(user_data, signals[COMMIT_STRING_SIGNAL], 0, data);
        }
    }
    else if (strcmp(signal_name, "ForwardKey") == 0) {
        guint32 key, state;
        gint32 type;
        g_variant_get(parameters, "(uui)", &key, &state, &type);
        g_signal_emit(user_data, signals[FORWARD_KEY_SIGNAL], 0, key, state, type);
    }
    else if (strcmp(signal_name, "DeleteSurroundingText") == 0) {
        guint32 nchar;
        gint32 offset;
        g_variant_get(parameters, "(iu)", &offset, &nchar);
        g_signal_emit(user_data, signals[DELETE_SURROUNDING_TEXT_SIGNAL], 0, offset, nchar);
    }
    else if (strcmp(signal_name, "UpdateClientSideUI") == 0) {
        const gchar* auxup, *auxdown, *preedit, *candidate, *imname;
        int cursor;
        g_variant_get(parameters, "(sssssi)", &auxup, &auxdown, &preedit, &candidate, &imname, &cursor);
        g_signal_emit(user_data, signals[UPDATE_CLIENT_SIDE_UI_SIGNAL], 0, auxup, auxdown, preedit, candidate, imname, cursor);
    }
    else if (strcmp(signal_name, "UpdateFormattedPreedit") == 0) {
        int cursor_pos;
        GPtrArray* array = g_ptr_array_new_with_free_func(_item_free);
        GVariantIter* iter;
        g_variant_get(parameters, "(a(si)i)", &iter, &cursor_pos);

        gchar* string;
        int type;
        while (g_variant_iter_next(iter, "(si)", &string, &type, NULL)) {
            FcitxPreeditItem* item = g_malloc0(sizeof(FcitxPreeditItem));
            item->string = strdup(string);
            item->type = type;
            g_ptr_array_add(array, item);
            g_free(string);
        }
        g_variant_iter_free(iter);
        g_signal_emit(user_data, signals[UPDATED_FORMATTED_PREEDIT_SIGNAL], 0, array, cursor_pos);
        g_ptr_array_free(array, TRUE);
    }
}

static void
fcitx_client_class_init(FcitxClientClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = fcitx_client_dispose;
    gobject_class->finalize = fcitx_client_finalize;

    g_type_class_add_private (klass, sizeof (FcitxClientPrivate));

    /* install signals */
    /**
     * FcitxClient::connected:
     * @self: A #FcitxClient
     *
     * Emit when connected to fcitx and created ic
     */
    signals[CONNECTED_SIGNAL] = g_signal_new(
                                     "connected",
                                     FCITX_TYPE_CLIENT,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

    /**
     * FcitxClient::disconnected:
     * @self: A #FcitxClient
     *
     * Emit when disconnected from fcitx
     */
    signals[DISCONNECTED_SIGNAL] = g_signal_new(
                                     "disconnected",
                                     FCITX_TYPE_CLIENT,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

    /**
     * FcitxClient::enable-im:
     * @self: A #FcitxClient
     *
     * Emit when input method is enabled
     */
    signals[ENABLE_IM_SIGNAL] = g_signal_new(
                                    "enable-im",
                                    FCITX_TYPE_CLIENT,
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL,
                                    NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE,
                                    0);
    /**
     * FcitxClient::close-im:
     * @self: A #FcitxClient
     *
     * Emit when input method is closed
     */
    signals[CLOSE_IM_SIGNAL] = g_signal_new(
                                   "close-im",
                                   FCITX_TYPE_CLIENT,
                                   G_SIGNAL_RUN_LAST,
                                   0,
                                   NULL,
                                   NULL,
                                   g_cclosure_marshal_VOID__VOID,
                                   G_TYPE_NONE,
                                   0);
    /**
     * FcitxClient::forward-key:
     * @self: A #FcitxClient
     * @keyval: key value
     * @state: key state
     * @type: event type
     *
     * Emit when input method ask for forward a key
     */
    signals[FORWARD_KEY_SIGNAL] = g_signal_new(
                                      "forward-key",
                                      FCITX_TYPE_CLIENT,
                                      G_SIGNAL_RUN_LAST,
                                      0,
                                      NULL,
                                      NULL,
                                      fcitx_marshall_VOID__UINT_UINT_INT,
                                      G_TYPE_NONE,
                                      3,
                                      G_TYPE_UINT, G_TYPE_INT, G_TYPE_INT
                                  );
    /**
     * FcitxClient::commit-string:
     * @self: A #FcitxClient
     * @string: string to be commited
     *
     * Emit when input method commit one string
     */
    signals[COMMIT_STRING_SIGNAL] = g_signal_new(
                                        "commit-string",
                                        FCITX_TYPE_CLIENT,
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_VOID__STRING,
                                        G_TYPE_NONE,
                                        1,
                                        G_TYPE_STRING
                                    );

    /**
     * FcitxClient::delete-surrounding-text:
     * @self: A #FcitxClient
     * @cursor: deletion start
     * @len: deletion length
     *
     * Emit when input method need to delete surrounding text
     */
    signals[DELETE_SURROUNDING_TEXT_SIGNAL] = g_signal_new(
                "delete-surrounding-text",
                FCITX_TYPE_CLIENT,
                G_SIGNAL_RUN_LAST,
                0,
                NULL,
                NULL,
                fcitx_marshall_VOID__INT_UINT,
                G_TYPE_NONE,
                2,
                G_TYPE_INT, G_TYPE_UINT
            );

    /**
     * FcitxClient::update-client-side-ui:
     * @self: A #FcitxClient
     * @auxup: (transfer none): Aux up string
     * @auxdown: (transfer none): Aux down string
     * @preedit: (transfer none): preedit string
     * @candidateword: (transfer none): candidateword in one line
     * @imname: (transfer none): input method name
     * @cursor_pos: cursor position
     *
     * Emit when input method need to update client side ui
     */
    signals[UPDATE_CLIENT_SIDE_UI_SIGNAL] = g_signal_new(
            "update-client-side-ui",
            FCITX_TYPE_CLIENT,
            G_SIGNAL_RUN_LAST,
            0,
            NULL,
            NULL,
            fcitx_marshall_VOID__STRING_STRING_STRING_STRING_STRING_INT,
            G_TYPE_NONE,
            6,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    /**
     * FcitxClient::update-formatted-preedit:
     * @self: A #FcitxClient
     * @preedit: (transfer none) (element-type FcitxPreeditItem): An #FcitxPreeditItem List
     * @cursor: cursor postion by utf8 byte
     *
     * Emit when input method need to delete surrounding text
     */
    signals[UPDATED_FORMATTED_PREEDIT_SIGNAL] = g_signal_new(
                "update-formatted-preedit",
                FCITX_TYPE_CLIENT,
                G_SIGNAL_RUN_LAST,
                0,
                NULL,
                NULL,
                fcitx_marshall_VOID__BOXED_INT,
                G_TYPE_NONE,
                2,
                G_TYPE_PTR_ARRAY, G_TYPE_INT
            );
}

/**
 * fcitx_client_new:
 *
 * New a #FcitxClient
 *
 * Returns: A newly allocated #FcitxClient
 **/
FCITX_EXPORT_API
FcitxClient*
fcitx_client_new()
{
    FcitxClient* self = g_object_new(FCITX_TYPE_CLIENT, NULL);
    return FCITX_CLIENT(self);
}

/**
 * fcitx_client_is_valid:
 * @self: A #FcitxClient
 *
 * Check #FcitxClient is valid to communicate with Fcitx
 *
 * Returns: #FcitxClient is valid or not
 **/
FCITX_EXPORT_API
gboolean
fcitx_client_is_valid(FcitxClient* self)
{
    return self->priv->icproxy != NULL;
}

static void
_fcitx_client_clean_up(FcitxClient* self, gboolean dont_emit_disconn)
{
    if (self->priv->cancellable) {
        g_cancellable_cancel (self->priv->cancellable);
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }

    if (self->priv->improxy) {
        g_object_unref(self->priv->improxy);
        self->priv->improxy = NULL;
    }

    if (self->priv->icproxy) {
        g_signal_handlers_disconnect_by_func(self->priv->icproxy,
                                             G_CALLBACK(_fcitx_client_g_signal),
                                             self);
        g_object_unref(self->priv->icproxy);
        self->priv->icproxy = NULL;
        if (!dont_emit_disconn)
            g_signal_emit(self, signals[DISCONNECTED_SIGNAL], 0);
    }

}

// kate: indent-mode cstyle; replace-tabs on;
