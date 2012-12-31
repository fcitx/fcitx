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
#include "fcitxconnection.h"
#include "marshall.h"

/**
 * FcitxConnection:
 *
 * A FcitxConnection allow to create a input context via DBus
 */

#define fcitx_gclient_debug(...) g_log ("fcitx-connection", \
                                      G_LOG_LEVEL_DEBUG,    \
                                      __VA_ARGS__)
struct _FcitxConnectionPrivate {
    char servicename[64];
    guint watch_id;
    GFileMonitor* monitor;
    GCancellable* cancellable;
    GDBusConnection* connection;
    gboolean connection_is_bus;
};

FCITX_EXPORT_API
GType        fcitx_connection_get_type(void) G_GNUC_CONST;

G_DEFINE_TYPE(FcitxConnection, fcitx_connection, G_TYPE_OBJECT);

#define FCITX_CONNECTION_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), FCITX_TYPE_CONNECTION, FcitxConnectionPrivate))

enum {
    CONNECTED_SIGNAL,
    DISCONNECTED_SIGNAL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};
static void _fcitx_connection_connect(FcitxConnection* self, gboolean use_session_bus);
static void fcitx_connection_init(FcitxConnection *self);
static void fcitx_connection_finalize(GObject *object);
static void fcitx_connection_dispose(GObject *object);
static void _fcitx_connection_appear(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);
static void _fcitx_connection_vanish (GDBusConnection *connection, const gchar *name, gpointer user_data);
static void _fcitx_connection_socket_file_changed_cb (GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer            user_data);
static gchar* _fcitx_get_address ();
static void _fcitx_connection_bus_finished(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_connection_connection_finished(GObject *source_object, GAsyncResult *res, gpointer user_data);
static void _fcitx_connection_clean_up(FcitxConnection* self, gboolean dont_emit_disconn);
static void _fcitx_connection_unwatch(FcitxConnection* self);
static void _fcitx_connection_watch(FcitxConnection* self);

static void fcitx_connection_class_init(FcitxConnectionClass *klass);

static void
fcitx_connection_finalize(GObject *object)
{
    if (G_OBJECT_CLASS(fcitx_connection_parent_class)->finalize != NULL)
        G_OBJECT_CLASS(fcitx_connection_parent_class)->finalize(object);
}

static void
fcitx_connection_dispose(GObject *object)
{
    FcitxConnection *self = FCITX_CONNECTION(object);

    if (self->priv->monitor) {
        g_signal_handlers_disconnect_by_func(self->priv->monitor,
                                             G_CALLBACK(_fcitx_connection_socket_file_changed_cb),
                                             self);
        g_object_unref(self->priv->monitor);
        self->priv->monitor= NULL;
    }

    _fcitx_connection_unwatch(self);

    _fcitx_connection_clean_up(self, TRUE);

    if (G_OBJECT_CLASS(fcitx_connection_parent_class)->dispose != NULL)
        G_OBJECT_CLASS(fcitx_connection_parent_class)->dispose(object);
}

static gboolean
_fcitx_connection_new_service_appear(gpointer user_data)
{
    FcitxConnection* self = (FcitxConnection*) user_data;
    if (!self->priv->connection || g_dbus_connection_is_closed(self->priv->connection))
        _fcitx_connection_connect(self, FALSE);
    return FALSE;
}

static void
_fcitx_connection_appear(GDBusConnection *connection, const gchar *name,
                         const gchar *name_owner, gpointer user_data)
{
    FCITX_UNUSED(connection);
    FCITX_UNUSED(name);
    FcitxConnection* self = (FcitxConnection*) user_data;
    gboolean new_owner_good = name_owner && (name_owner[0] != '\0');
    if (new_owner_good) {
        g_timeout_add_full(G_PRIORITY_DEFAULT,
                           100,
                           _fcitx_connection_new_service_appear,
                           g_object_ref(self),
                           g_object_unref);
    }
}

static void
_fcitx_connection_vanish(GDBusConnection *connection, const gchar *name,
                         gpointer user_data)
{
    FCITX_UNUSED(connection);
    FCITX_UNUSED(name);
    FcitxConnection* self = (FcitxConnection*) user_data;
    _fcitx_connection_clean_up(self, FALSE);
}

static gchar*
_fcitx_get_socket_path()
{
    char* machineId = dbus_get_local_machine_id();
    gchar* path;
    gchar* addressFile = g_strdup_printf("%s-%d", machineId, fcitx_utils_get_display_number());
    dbus_free(machineId);

    path = g_build_filename (g_get_user_config_dir (),
            "fcitx",
            "dbus",
            addressFile,
            NULL);
    g_free(addressFile);
    return path;
}

static void
fcitx_connection_init(FcitxConnection *self)
{
    self->priv = FCITX_CONNECTION_GET_PRIVATE(self);

    sprintf(self->priv->servicename, "%s-%d", FCITX_DBUS_SERVICE,
            fcitx_utils_get_display_number());

    self->priv->connection = NULL;
    self->priv->cancellable = NULL;
    self->priv->watch_id = 0;
    self->priv->connection_is_bus = FALSE;

    gchar* path = _fcitx_get_socket_path();
    GFile* file = g_file_new_for_path(path);
    self->priv->monitor = g_file_monitor_file(file, 0, NULL, NULL);

    g_signal_connect(self->priv->monitor, "changed",
                     (GCallback)_fcitx_connection_socket_file_changed_cb, self);

    g_object_unref(file);
    g_free(path);

    _fcitx_connection_connect(self, FALSE);
}

static void
_fcitx_connection_connect(FcitxConnection *self, gboolean use_session_bus)
{
    fcitx_gclient_debug("_fcitx_connection_create_ic");
    _fcitx_connection_unwatch(self);
    _fcitx_connection_clean_up(self, FALSE);
    self->priv->cancellable = g_cancellable_new ();

    g_object_ref(self);
    if (!use_session_bus) {
        gchar* address = _fcitx_get_address();
        /* a trick for prevent cancellable being called after finalize */
        if (address) {
            /* since we have a possible valid address here */
            g_dbus_connection_new_for_address(address,
                    G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                    NULL,
                    self->priv->cancellable,
                    _fcitx_connection_connection_finished,
                    self);
            g_free(address);
            return;
        }
    }

    _fcitx_connection_watch(self);
    g_bus_get(G_BUS_TYPE_SESSION,
              self->priv->cancellable,
              _fcitx_connection_bus_finished,
              self
            );
};

static void
_fcitx_connection_connection_closed(GDBusConnection *connection,
                                    gboolean remote_peer_vanished,
                                    GError *error, gpointer user_data)
{
    FCITX_UNUSED(connection);
    FCITX_UNUSED(remote_peer_vanished);
    FCITX_UNUSED(error);
    fcitx_gclient_debug("_fcitx_connection_connection_closed");
    FcitxConnection* self = (FcitxConnection*)user_data;
    _fcitx_connection_clean_up(self, FALSE);

    _fcitx_connection_watch(self);
}

static void
_fcitx_connection_bus_finished(GObject *source_object, GAsyncResult *res,
                               gpointer user_data)
{
    FCITX_UNUSED(source_object);
    fcitx_gclient_debug("_fcitx_connection_bus_finished");
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CONNECTION(user_data));

    FcitxConnection* self = (FcitxConnection*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }

    GDBusConnection* connection = g_bus_get_finish(res, NULL);

    if (connection) {
        _fcitx_connection_clean_up(self, FALSE);
        self->priv->connection = connection;
        self->priv->connection_is_bus = TRUE;
        g_signal_connect(connection, "closed",
                         G_CALLBACK(_fcitx_connection_connection_closed), self);
        g_signal_emit(self, signals[CONNECTED_SIGNAL], 0);
    }
    /* unref for _fcitx_connection_connect */
    g_object_unref(self);
}

static void
_fcitx_connection_watch(FcitxConnection* self)
{
    if (self->priv->watch_id)
        return;
    fcitx_gclient_debug("_fcitx_connection_watch");

    self->priv->watch_id = g_bus_watch_name(
                       G_BUS_TYPE_SESSION,
                       self->priv->servicename,
                       G_BUS_NAME_WATCHER_FLAGS_NONE,
                       _fcitx_connection_appear,
                       _fcitx_connection_vanish,
                       self,
                       NULL
                   );
}

static void
_fcitx_connection_unwatch(FcitxConnection* self)
{
    if (self->priv->watch_id)
        g_bus_unwatch_name(self->priv->watch_id);
    self->priv->watch_id = 0;
}

static void
_fcitx_connection_connection_finished(GObject *source_object,
                                      GAsyncResult *res, gpointer user_data)

{
    FCITX_UNUSED(source_object);
    fcitx_gclient_debug("_fcitx_connection_connection_finished");
    g_return_if_fail (user_data != NULL);
    g_return_if_fail (FCITX_IS_CONNECTION(user_data));
    FcitxConnection* self = (FcitxConnection*) user_data;
    if (self->priv->cancellable) {
        g_object_unref (self->priv->cancellable);
        self->priv->cancellable = NULL;
    }

    GError* error = NULL;
    GDBusConnection* connection = g_dbus_connection_new_for_address_finish(res, &error);

    gboolean is_cancel = FALSE;
    if (error) {
        if (g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            is_cancel = TRUE;
        g_error_free(error);
    }

    /* hey! if we failed here. we'd try traditional dbus way */
    if (!connection || g_dbus_connection_is_closed(connection)) {
        if (connection)
            g_object_unref(connection);

        if (!is_cancel)
            _fcitx_connection_connect(self, TRUE);
    }
    else {
        g_dbus_connection_set_exit_on_close(connection, FALSE);

        _fcitx_connection_clean_up(self, FALSE);
        self->priv->connection = connection;
        self->priv->connection_is_bus = FALSE;
        g_signal_connect(self->priv->connection, "closed", G_CALLBACK(_fcitx_connection_connection_closed), self);
        g_signal_emit(self, signals[CONNECTED_SIGNAL], 0);
    }

    /* unref for _fcitx_connection_connect */
    g_object_unref(self);
}


static void
fcitx_connection_class_init(FcitxConnectionClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = fcitx_connection_dispose;
    gobject_class->finalize = fcitx_connection_finalize;

    g_type_class_add_private (klass, sizeof (FcitxConnectionPrivate));

    /* install signals */
    /**
     * FcitxConnection::connected:
     * @connection: A FcitxConnection
     *
     * Emit when connected to fcitx and created ic
     */
    signals[CONNECTED_SIGNAL] = g_signal_new(
                                     "connected",
                                     FCITX_TYPE_CONNECTION,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

    /**
     * FcitxConnection::disconnected:
     * @connection: A FcitxConnection
     *
     * Emit when disconnected from fcitx
     */
    signals[DISCONNECTED_SIGNAL] = g_signal_new(
                                     "disconnected",
                                     FCITX_TYPE_CONNECTION,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

}

/**
 * fcitx_connection_new:
 *
 * New a #FcitxConnection
 *
 * Returns: A newly allocated #FcitxConnection
 **/
FCITX_EXPORT_API
FcitxConnection*
fcitx_connection_new()
{
    FcitxConnection* self = g_object_new(FCITX_TYPE_CONNECTION, NULL);
    return FCITX_CONNECTION(self);
}

/**
 * fcitx_connection_is_valid:
 * @connection: A #FcitxConnection
 *
 * Check #FcitxConnection is valid to communicate with Fcitx
 *
 * Returns: #FcitxConnection is valid or not
 **/
FCITX_EXPORT_API
gboolean
fcitx_connection_is_valid(FcitxConnection* self)
{
    return self->priv->connection != NULL;
}

/**
 * fcitx_connection_get_g_dbus_connection:
 * @connection: A #FcitxConnection
 *
 * Return the current #GDBusConnection
 *
 * Returns: (transfer none): #GDBusConnection for current connection
 **/
FCITX_EXPORT_API
GDBusConnection*
fcitx_connection_get_g_dbus_connection(FcitxConnection* connection)
{
    return connection->priv->connection;
}

static void
_fcitx_connection_socket_file_changed_cb(GFileMonitor *monitor, GFile *file,
                                         GFile *other_file,
                                         GFileMonitorEvent event_type,
                                         gpointer user_data)
{
    FCITX_UNUSED(monitor);
    FCITX_UNUSED(file);
    FCITX_UNUSED(other_file);
    FcitxConnection* connection = user_data;
    if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
        event_type != G_FILE_MONITOR_EVENT_CREATED &&
        event_type != G_FILE_MONITOR_EVENT_DELETED)
        return;

    _fcitx_connection_connect(connection, FALSE);
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
    g_free(path);

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
_fcitx_connection_clean_up(FcitxConnection* self, gboolean dont_emit_disconn)
{
    if (self->priv->connection) {
        g_signal_handlers_disconnect_by_func(self->priv->connection,
                                             G_CALLBACK(_fcitx_connection_connection_closed),
                                             self);
        if (!self->priv->connection_is_bus) {
            g_dbus_connection_close_sync(self->priv->connection, NULL, NULL);
        }
        g_object_unref(self->priv->connection);
        self->priv->connection = NULL;
        if (!dont_emit_disconn)
            g_signal_emit(self, signals[DISCONNECTED_SIGNAL], 0);
    }
}

// kate: indent-mode cstyle; replace-tabs on;
