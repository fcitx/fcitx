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

#ifndef CLIENT_IM_H
#define CLIENT_IM_H


#include <gio/gio.h>

/*
 * Type macros
 */

/* define GOBJECT macros */
#define FCITX_TYPE_CLIENT         (fcitx_client_get_type ())
#define FCITX_CLIENT(o) \
        (G_TYPE_CHECK_INSTANCE_CAST ((o), FCITX_TYPE_CLIENT, FcitxClient))
#define FCITX_IS_CLIENT(object) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((object), FCITX_TYPE_CLIENT))
#define FCITX_CLIENT_CLASS(k) \
        (G_TYPE_CHECK_CLASS_CAST((k), FCITX_TYPE_CLIENT, FcitxClientClass))
#define FCITX_CLIENT_GET_CLASS(o) \
        (G_TYPE_INSTANCE_GET_CLASS ((o), FCITX_TYPE_CLIENT, FcitxClientClass))

G_BEGIN_DECLS

typedef struct _FcitxClient        FcitxClient;
typedef struct _FcitxClientClass   FcitxClientClass;
typedef struct _FcitxClientPrivate FcitxClientPrivate;
typedef struct _FcitxPreeditItem   FcitxPreeditItem;

/**
 * FcitxClient:
 *
 * A FcitxClient allow to create a input context via DBus
 */
struct _FcitxClient {
    GObject parent_instance;
    /* instance member */
    FcitxClientPrivate* priv;
};

struct _FcitxClientClass {
    GObjectClass parent_class;
    /* signals */

    /*< private >*/
    /* padding */
};

struct _FcitxPreeditItem {
    gchar* string;
    gint32 type;
};

GType        fcitx_client_get_type(void) G_GNUC_CONST;

/**
 * fcitx_client_new
 *
 * @returns: A newly allocated FcitxClient
 *
 * New a FcitxClient
 **/
FcitxClient* fcitx_client_new();

/**
 * fcitx_client_is_valid:
 *
 * @client: A FcitxClient
 * @returns: FcitxClient is valid or not
 *
 * Check FcitxClient is valid to communicate with Fcitx
 **/
gboolean fcitx_client_is_valid(FcitxClient* client);

/**
 * fcitx_client_process_key_sync:
 *
 * @client: A FcitxClient
 * @keyval: key value
 * @state: key state
 * @type: event type
 * @ts: timestamp
 *
 * @returns: the key is processed or not
 *
 * send a key event to fcitx synchronizely
 */
int fcitx_client_process_key_sync(FcitxClient* client, guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 ts);

/**
 * fcitx_client_process_key:
 *
 * @client A FcitxClient
 * @cb callback
 * @user_data user data
 * @keyval key value
 * @keycode hardware key code
 * @state key state
 * @type event type
 * @t timestamp
 *
 * @deprecated
 *
 * send a key event to fcitx asynchronizely, you need to use g_dbus_proxy_call_finish with this function
 **/
void fcitx_client_process_key(FcitxClient* client, GAsyncReadyCallback cb, gpointer user_data, guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 t);

/**
 * fcitx_client_process_key_async:
 *
 * @client A FcitxClient
 * @keyval key value
 * @keycode hardware key code
 * @state key state
 * @type event type
 * @t timestamp
 * @cancellable cancellable
 * @cb callback
 * @user_data user data
 *
 * use this function with fcitx_client_process_key_finish
 * @see fcitx_client_process_key_finish
 **/
void fcitx_client_process_key_async(FcitxClient* self,
                                    guint32 keyval, guint32 keycode, guint32 state, gint type, guint32 t,
                                    gint timeout_msec,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data);
/**
 * fcitx_client_process_key_finish:
 *
 * @client A FcitxClient
 * @res result
 *
 * @returns process key result
 *
 * use this function with fcitx_client_process_key_async
 * @see fcitx_client_process_key_async
 **/
gint fcitx_client_process_key_finish(FcitxClient* client, GAsyncResult* res);

/**
 * fcitx_client_enable_ic:
 *
 * @client A FcitxClient
 *
 * tell fcitx activate current ic
 **/
void fcitx_client_enable_ic(FcitxClient* self);

/**
 * fcitx_client_focus_in:
 *
 * @client A FcitxClient
 *
 * tell fcitx inactivate current ic
 **/
void fcitx_client_close_ic(FcitxClient* self);

/**
 * fcitx_client_focus_in:
 *
 * @client A FcitxClient
 *
 * tell fcitx current client has focus
 **/
void fcitx_client_focus_in(FcitxClient* client);

/**
 * fcitx_client_focus_out:
 *
 * @client A FcitxClient
 *
 * tell fcitx current client has lost focus
 **/
void fcitx_client_focus_out(FcitxClient* client);

/**
 * fcitx_client_set_cusor_rect:
 *
 * @client A FcitxClient
 * @x x of cursor
 * @y y of cursor
 * @w width of cursor
 * @h height of cursor
 *
 * tell fcitx current client's cursor geometry info
 **/
void fcitx_client_set_cusor_rect(FcitxClient* client, int x, int y, int w, int h);

/**
 * fcitx_client_set_cusor_rect:
 *
 * @client A FcitxClient
 * @x x of cursor
 * @y y of cursor
 * @w width of cursor
 * @h height of cursor
 *
 * tell fcitx current client's cursor geometry info
 **/
void fcitx_client_set_cursor_rect(FcitxClient* client, int x, int y, int w, int h);

/**
 * fcitx_client_set_surrounding_text:
 *
 * @client A FcitxClient
 * @text surroundng text
 * @cursor cursor position coresponding to text
 * @anchor anchor position coresponding to text
 **/
void fcitx_client_set_surrounding_text(FcitxClient* client, gchar* text, guint cursor, guint anchor);

/**
 * fcitx_client_set_capacity:
 *
 * @client A FcitxClient
 *
 * set client capacity of Fcitx
 **/
void fcitx_client_set_capacity(FcitxClient* client, guint flags);

/**
 * fcitx_client_reset:
 *
 * @client A FcitxClient
 *
 * tell fcitx current client is reset from client side
 **/
void fcitx_client_reset(FcitxClient* client);

G_END_DECLS

#endif //CLIENT_IM_H
