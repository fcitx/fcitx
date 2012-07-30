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
#include "marshall.h"

typedef struct _ProcessKeyStruct ProcessKeyStruct;

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
    guint watch_id;
    GFileMonitor* monitor;
    GCancellable* cancellable;
    GDBusConnection* connection;
    gboolean connection_is_bus;
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
    CONNTECTED_SIGNAL,
    ENABLE_IM_SIGNAL,
    CLOSE_IM_SIGNAL,
    FORWARD_KEY_SIGNAL,
    COMMIT_STRING_SIGNAL,
    DELETE_SURROUNDING_TEXT_SIGNAL,
    UPDATED_FORMATTED_PREEDIT_SIGNAL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static GDBusInterfaceInfo *_fcitx_client_get_interface_info(void);
static GDBusInterfaceInfo *_fcitx_client_get_clientic_info(void);
static void _fcitx_client_create_ic(FcitxClient* self);
static void _fcitx_client_create_ic_phase1_finished(GObject* source_object, GAsyncResult* res, gpointer user_data);
static void _fcitx_client_create_ic_cb(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_client_create_ic_phase2_finished(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_client_g_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, gpointer user_data);
static void fcitx_client_init(FcitxClient *self);
static void fcitx_client_finalize(GObject *object);
static void fcitx_client_dispose(GObject *object);
static void _fcitx_client_appear (GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);
static void _fcitx_client_vanish (GDBusConnection *connection, const gchar *name, gpointer user_data);
static void _fcitx_client_socket_file_changed_cb (GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer            user_data);
static gchar* _fcitx_get_address ();
static void _fcitx_client_create_ic_phase0_bus_finished(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_client_create_ic_phase0_connection_finished(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_client_clean_up(FcitxClient* self);
static void _fcitx_client_unwatch(FcitxClient* self);
static void _fcitx_client_watch(FcitxClient* self);

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

    if (self->priv->monitor) {
        g_signal_handlers_disconnect_by_func(self->priv->monitor,
                                             G_CALLBACK(_fcitx_client_socket_file_changed_cb),
                                             self);
        g_object_unref(self->priv->monitor);
        self->priv->monitor= NULL;
    }

    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "DestroyIC", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
    _fcitx_client_unwatch(self);

    _fcitx_client_clean_up(self);

    if (G_OBJECT_CLASS(fcitx_client_parent_class)->dispose != NULL)
        G_OBJECT_CLASS(fcitx_client_parent_class)->dispose(object);
}

FCITX_EXPORT_API
void fcitx_client_focus_in(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "FocusIn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

FCITX_EXPORT_API
void fcitx_client_focus_out(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "FocusOut", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

FCITX_EXPORT_API
void fcitx_client_reset(FcitxClient* self)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "Reset", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

FCITX_EXPORT_API
void fcitx_client_set_capacity(FcitxClient* self, guint flags)
{
    uint32_t iflags = flags;
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "SetCapacity", g_variant_new("(u)", iflags), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

FCITX_EXPORT_API
void fcitx_client_set_cusor_rect(FcitxClient* self, int x, int y, int w, int h)
{
    fcitx_client_set_cursor_rect(self, x, y, w, h);
}

FCITX_EXPORT_API
void fcitx_client_set_cursor_rect(FcitxClient* self, int x, int y, int w, int h)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "SetCursorRect", g_variant_new("(iiii)", x, y, w, h), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

FCITX_EXPORT_API
void fcitx_client_set_surrounding_text(FcitxClient* self, gchar* text, guint cursor, guint anchor)
{
    if (self->priv->icproxy) {
        g_dbus_proxy_call(self->priv->icproxy, "SetSurroundingText", g_variant_new("(suu)", text, cursor, anchor), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

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

FCITX_EXPORT_API
gint fcitx_client_process_key_finish(FcitxClient* client, GAsyncResult* res)
{
    gint ret = -1;
    GError* error = NULL;
    GVariant* result = g_dbus_proxy_call_finish(client->priv->icproxy, res, &error);
    if (error) {
        g_error_free(error);
    }
    else if (result) {
        g_variant_get(result, "(i)", &ret);
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
    ProcessKeyStruct* pk = user_data;
    pk->callback(G_OBJECT(pk->self), res, pk->user_data);
    _process_key_data_free(pk);
}

void _fcitx_client_process_key_cancelled(GCancellable* cancellable, gpointer user_data)
{
    ProcessKeyStruct* pk = user_data;
    _process_key_data_free(pk);
}

FCITX_EXPORT_API
void fcitx_client_process_key_async(FcitxClient* self,
                                    guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 t,
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

FCITX_EXPORT_API
int fcitx_client_process_key_sync(FcitxClient* self, guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 t)
{
    int itype = type;
    GError *error = NULL;
    int ret = -1;
    if (self->priv->icproxy) {
        GVariant* result =  g_dbus_proxy_call_sync(self->priv->icproxy,
                            "ProcessKeyEvent",
                            g_variant_new("(uuuiu)", keyval, keycode, state, itype, t),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1, NULL,
                            &error);

        if (error)
        {
            g_error_free(error);
        }
        else {
            g_variant_get(result, "(i)", &ret);
        }
    }

    return ret;
}


static void
_fcitx_client_appear (GDBusConnection *connection,
                      const gchar     *name,
                      const gchar     *name_owner,
                      gpointer         user_data)
{
    FcitxClient* self = (FcitxClient*) user_data;
    gboolean new_owner_good = name_owner && (name_owner[0] != '\0');
    if (new_owner_good) {
        _fcitx_client_create_ic(self);
    }
}

static void
_fcitx_client_vanish (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
    FcitxClient* self = (FcitxClient*) user_data;
    _fcitx_client_clean_up(self);
}

static gchar*
_fcitx_get_socket_path()
{
    gchar* path;
    gchar* addressFile = g_strdup_printf("%s-%d", dbus_get_local_machine_id(), fcitx_utils_get_display_number());

    path = g_build_filename (g_get_user_config_dir (),
            "fcitx",
            "dbus",
            addressFile,
            NULL);
    g_free(addressFile);
    return path;
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
    self->priv->watch_id = 0;
    self->priv->connection_is_bus = FALSE;

    gchar* path = _fcitx_get_socket_path();
    GFile* file = g_file_new_for_path(path);
    self->priv->monitor = g_file_monitor_file(file, 0, NULL, NULL);

    g_signal_connect (self->priv->monitor, "changed", (GCallback) _fcitx_client_socket_file_changed_cb, self);

    g_object_unref(file);
    g_free(path);

    _fcitx_client_create_ic(self);
}

static void
_fcitx_client_create_ic(FcitxClient *self)
{
    g_warning("create_ic %p", self);

    _fcitx_client_clean_up(self);
    self->priv->cancellable = g_cancellable_new ();

    gchar* address = _fcitx_get_address();
    /* a trick for prevent cancellable being called after finalize */
    g_object_ref(self);
    if (address) {
        /* since we have a possible valid address here */
        _fcitx_client_unwatch(self);
        g_dbus_connection_new_for_address(address,
                G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                NULL,
                self->priv->cancellable,
                _fcitx_client_create_ic_phase0_connection_finished,
                self);
    }
    else {
        g_bus_get(G_BUS_TYPE_SESSION,
                  self->priv->cancellable,
                  _fcitx_client_create_ic_phase0_bus_finished,
                  self
                );
    }

};

static void
_fcitx_client_connection_closed(GDBusConnection *connection,
                                gboolean         remote_peer_vanished,
                                GError          *error,
                                gpointer         user_data)
{
    FcitxClient* self = (FcitxClient*) user_data;
    _fcitx_client_clean_up(self);
}

static void
_fcitx_client_create_ic_phase0_bus_finished(GObject *source_object,
                                            GAsyncResult *res,
                                            gpointer user_data)
{
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CLIENT(user_data));

    FcitxClient* self = (FcitxClient*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    GError* error = NULL;
    GDBusConnection* connection = g_bus_get_finish(res, &error);
    if (error) {
        // g_warning ("Connect to daemon failed: %s.", error->message);
        g_error_free(error);
    }

    if (!connection) {
        /* unref for _fcitx_client_create_ic */
        g_object_unref(self);
        return;
    }
    self->priv->connection = connection;
    self->priv->connection_is_bus = TRUE;
    g_signal_connect(connection, "closed", G_CALLBACK(_fcitx_client_connection_closed), self);
    self->priv->cancellable = g_cancellable_new ();
    g_dbus_proxy_new(
        connection,
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
_fcitx_client_watch(FcitxClient* self)
{
    if (self->priv->watch_id)
        return;

    self->priv->watch_id = g_bus_watch_name(
                       G_BUS_TYPE_SESSION,
                       self->priv->servicename,
                       G_BUS_NAME_WATCHER_FLAGS_NONE,
                       _fcitx_client_appear,
                       _fcitx_client_vanish,
                       self,
                       NULL
                   );
}

static void
_fcitx_client_unwatch(FcitxClient* self)
{
    if (self->priv->watch_id)
        g_bus_unwatch_name(self->priv->watch_id);
    self->priv->watch_id = 0;
}

static void
_fcitx_client_create_ic_phase0_connection_finished(GObject *source_object,
                                                   GAsyncResult *res,
                                                   gpointer user_data)

{
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CLIENT(user_data));
    FcitxClient* self = (FcitxClient*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    GError* error;
    GDBusConnection* connection = g_dbus_connection_new_for_address_finish(res, &error);
    if (error) {
        // g_warning ("Connect to daemon failed: %s.", error->message);
        g_error_free(error);
    }

    /* hey! if we failed here. we'd try traditional dbus way */
    if (!connection || g_dbus_connection_is_closed(connection)) {
        _fcitx_client_watch(self);
        /* unref for _fcitx_client_create_ic */
        g_object_unref(self);
        return;
    }

    self->priv->connection = connection;
    self->priv->connection_is_bus = FALSE;
    g_signal_connect(self->priv->connection, "closed", G_CALLBACK(_fcitx_client_connection_closed), self);

    self->priv->cancellable = g_cancellable_new ();
    g_dbus_proxy_new(
        connection,
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
_fcitx_client_create_ic_phase1_finished(GObject *source_object,
                                        GAsyncResult *res,
                                        gpointer user_data)
{
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CLIENT(user_data));
    FcitxClient* self = (FcitxClient*) user_data;
    GError* error = NULL;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    if (self->priv->improxy)
        g_object_unref(self->priv->improxy);
    self->priv->improxy = g_dbus_proxy_new_finish(res, &error);
    if (error) {
        // g_warning ("Create fcitx input method proxy failed: %s.", error->message);
        g_error_free(error);
    }

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
        /* unref for phase0 finish */
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
    GError* error = NULL;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    GVariant* result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);

    if (error) {
        g_error_free(error);
        /* unref for _fcitx_client_phase1_finish */
        g_object_unref(self);
        return;
    }

    gboolean enable;
    guint32 key1, state1, key2, state2;
    g_variant_get(result, "(ibuuuu)", &self->priv->id, &enable, &key1, &state1, &key2, &state2);

    sprintf(self->priv->icname, FCITX_IC_DBUS_PATH, self->priv->id);

    self->priv->cancellable = g_cancellable_new ();
    g_dbus_proxy_new(
        self->priv->connection,
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
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CLIENT(user_data));
    FcitxClient* self = (FcitxClient*) user_data;
    GError* error = NULL;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }
    if (self->priv->icproxy)
        g_object_unref(self->priv->icproxy);
    self->priv->icproxy = g_dbus_proxy_new_finish(res, &error);

    if (error) {
        g_error_free(error);
    }

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
        g_signal_emit(user_data, signals[CONNTECTED_SIGNAL], 0);
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
     * FcitxClient::connected
     *
     * @client: A FcitxClient
     *
     * Emit when connected to fcitx
     */
    signals[CONNTECTED_SIGNAL] = g_signal_new(
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
     * FcitxClient::enable-im
     *
     * @client: A FcitxClient
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
     * FcitxClient::enable-im
     *
     * @client: A FcitxClient
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
     * FcitxClient::enable-im
     *
     * @client: A FcitxClient
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
     * FcitxClient::commit-string
     *
     * @client: A FcitxClient
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
     * FcitxClient::delete-surrounding-text
     *
     * @client: A FcitxClient
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
     * FcitxClient::update-formatted-preedit
     *
     * @client: A FcitxClient
     * @preedit: (transfer none) (element-type FcitxPreeditItem): An FcitxPreeditItem List
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

FCITX_EXPORT_API
FcitxClient*
fcitx_client_new()
{
    FcitxClient* self = g_object_new(FCITX_TYPE_CLIENT, NULL);

    if (self != NULL) {
        return FCITX_CLIENT(self);
    }
    else
        return NULL;
    return self;
}

FCITX_EXPORT_API
gboolean
fcitx_client_is_valid(FcitxClient* self)
{
    return self->priv->icproxy != NULL;
}

static void
_fcitx_client_socket_file_changed_cb (GFileMonitor       *monitor,
                                      GFile              *file,
                                      GFile              *other_file,
                                      GFileMonitorEvent   event_type,
                                      gpointer            user_data)
{
    FcitxClient* client = user_data;
    if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
        event_type != G_FILE_MONITOR_EVENT_CREATED &&
        event_type != G_FILE_MONITOR_EVENT_DELETED)
        return;

    _fcitx_client_create_ic(client);
}

static gchar*
_fcitx_get_address ()
{
    gchar* address = NULL;
    address = g_strdup (g_getenv("FCITX_DBUS_ADDRESS"));
    if (address)
        return address;

    gchar* path = _fcitx_get_socket_path();
    FILE* fp = fopen(path, "r");

    if (!fp)
        return NULL;

    const int BUFSIZE = 1024;

    char buffer[BUFSIZE];
    size_t sz = fread(buffer, sizeof(char), BUFSIZE, fp);
    fclose(fp);
    if (sz == 0)
        return NULL;
    char* p = buffer;
    while(*p)
        p++;
    size_t addrlen = p - buffer;
    if (sz != addrlen + 2 * sizeof(pid_t) + 1)
        return NULL;

    /* skip '\0' */
    p++;
    pid_t *ppid = (pid_t*) p;
    pid_t daemonpid = ppid[0];
    pid_t fcitxpid = ppid[1];

    if (!fcitx_utils_pid_exists(daemonpid)
        || !fcitx_utils_pid_exists(fcitxpid))
        return NULL;

    address = g_strdup(buffer);

    return address;
}

static void
_fcitx_client_clean_up(FcitxClient* self)
{
    if (self->priv->connection) {
        g_signal_handlers_disconnect_by_func(self->priv->connection,
                                             G_CALLBACK(_fcitx_client_connection_closed),
                                             self);
        if (!self->priv->connection_is_bus) {
            g_dbus_connection_close_sync(self->priv->connection, NULL, NULL);
        }
        g_object_unref(self->priv->connection);
        self->priv->connection = NULL;
    }

    if (self->priv->cancellable) {
        g_signal_handlers_disconnect_matched (
                self->priv->cancellable,
                G_SIGNAL_MATCH_DATA,
                0, 0, NULL, NULL, self);
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
    }

}

// kate: indent-mode cstyle; replace-tabs on;
