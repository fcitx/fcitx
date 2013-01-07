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

/**
 * @file fcitximcontext.c
 *
 * This is a gtk im module for fcitx, using DBus as a protocol.
 *        This is compromise to gtk and firefox, users are being sucked by them
 *        again and again.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "fcitx/fcitx.h"
#include "fcitximcontext.h"
#include "fcitx-config/fcitx-config.h"
#include "client.h"
#include "fcitx-utils/log.h"
#include <dbus/dbus-glib.h>

#if !GTK_CHECK_VERSION (2, 91, 0)
# define DEPRECATED_GDK_KEYSYMS 1
#endif

#if GTK_CHECK_VERSION (2, 24, 0)
# define NEW_GDK_WINDOW_GET_DISPLAY
#endif

struct _FcitxIMContext {
    GtkIMContext parent;

    GdkWindow *client_window;
    GdkRectangle area;
    FcitxIMClient* client;
    GtkIMContext* slave;
    int has_focus;
    guint32 time;
    gboolean use_preedit;
    gboolean is_inpreedit;
    char* preedit_string;
    int cursor_pos;
    FcitxCapacityFlags capacity;
    PangoAttrList* attrlist;
};

typedef struct _ProcessKeyStruct {
    FcitxIMContext* context;
    GdkEventKey* event;
} ProcessKeyStruct;

struct _FcitxIMContextClass {
    GtkIMContextClass parent;
    /* klass members */
};

/* functions prototype */
static void     fcitx_im_context_class_init(FcitxIMContextClass   *klass);
static void     fcitx_im_context_class_fini (FcitxIMContextClass *klass);
static void     fcitx_im_context_init(FcitxIMContext        *im_context);
static void     fcitx_im_context_finalize(GObject               *obj);
static void     fcitx_im_context_set_client_window(GtkIMContext          *context,
        GdkWindow             *client_window);
static gboolean fcitx_im_context_filter_keypress(GtkIMContext          *context,
        GdkEventKey           *key);
static void     fcitx_im_context_reset(GtkIMContext          *context);
static void     fcitx_im_context_focus_in(GtkIMContext          *context);
static void     fcitx_im_context_focus_out(GtkIMContext          *context);
static void     fcitx_im_context_set_cursor_location(GtkIMContext          *context,
        GdkRectangle             *area);
static void     fcitx_im_context_set_use_preedit(GtkIMContext          *context,
        gboolean               use_preedit);
static void
fcitx_im_context_set_surrounding (GtkIMContext *context,
                                 const gchar *text,
                                 gint len,
                                 gint cursor_index);
static void     fcitx_im_context_get_preedit_string(GtkIMContext          *context,
        gchar                **str,
        PangoAttrList        **attrs,
        gint                  *cursor_pos);


static gboolean
_set_cursor_location_internal(FcitxIMContext *fcitxcontext);
static void
_slave_commit_cb(GtkIMContext *slave,
                 gchar *string,
                 FcitxIMContext *context);
static void
_slave_preedit_changed_cb(GtkIMContext *slave,
                          FcitxIMContext *context);
static void
_slave_preedit_start_cb(GtkIMContext *slave,
                        FcitxIMContext *context);
static void
_slave_preedit_end_cb(GtkIMContext *slave,
                      FcitxIMContext *context);
static gboolean
_slave_retrieve_surrounding_cb(GtkIMContext *slave,
                               FcitxIMContext *context);
static gboolean
_slave_delete_surrounding_cb(GtkIMContext *slave,
                             gint offset_from_cursor,
                             guint nchars,
                             FcitxIMContext *context);
static void
_fcitx_im_context_enable_im_cb(DBusGProxy* proxy, void* user_data);
static void
_fcitx_im_context_close_im_cb(DBusGProxy* proxy, void* user_data);
static void
_fcitx_im_context_commit_string_cb(DBusGProxy* proxy, char* str, void* user_data);
static void
_fcitx_im_context_forward_key_cb(DBusGProxy* proxy, guint keyval, guint state, gint type, void* user_data);
static void
_fcitx_im_context_update_preedit_cb(DBusGProxy* proxy, char* str, int cursor_pos, void* user_data);
static void
_fcitx_im_context_delete_surrounding_text_cb (DBusGProxy* proxy,
                                          gint offset_from_cursor,
                                          guint nchars,
                                          void* user_data);
static void
_fcitx_im_context_connect_cb(FcitxIMClient* client, void* user_data);
static void
_fcitx_im_context_destroy_cb(FcitxIMClient* client, void* user_data);
static void
_fcitx_im_context_process_key_cb(DBusGProxy *proxy, DBusGProxyCall *call_id, gpointer user_data);
static void
_fcitx_im_context_set_capacity(FcitxIMContext* fcitxcontext, gboolean force);

static GdkEventKey *
_create_gdk_event(FcitxIMContext *fcitxcontext,
                  guint keyval,
                  guint state,
                  FcitxKeyEventType type
                 );


static gboolean
_key_is_modifier(guint keyval);

static void
_request_surrounding_text (FcitxIMContext **context);

static gint
_key_snooper_cb (GtkWidget   *widget,
                 GdkEventKey *event,
                 gpointer     user_data);

static GType _fcitx_type_im_context = 0;
static GtkIMContextClass *parent_class = NULL;

static guint _signal_commit_id = 0;
static guint _signal_preedit_changed_id = 0;
static guint _signal_preedit_start_id = 0;
static guint _signal_preedit_end_id = 0;
static guint _signal_delete_surrounding_id = 0;
static guint _signal_retrieve_surrounding_id = 0;
static gboolean _use_sync_mode = 0;

static GtkIMContext *_focus_im_context = NULL;
static const gchar *_no_snooper_apps = NO_SNOOPER_APPS;
static gboolean _use_key_snooper = _ENABLE_SNOOPER;
static guint    _key_snooper_id = 0;

/* Copied from gtk+2.0-2.20.1/modules/input/imcedilla.c to fix crosbug.com/11421.
* Overwrite the original Gtk+'s compose table in gtk+-2.x.y/gtk/gtkimcontextsimple.c. */

/* The difference between this and the default input method is the handling
* of C+acute - this method produces C WITH CEDILLA rather than C WITH ACUTE.
* For languages that use CCedilla and not acute, this is the preferred mapping,
* and is particularly important for pt_BR, where the us-intl keyboard is
* used extensively.
*/
static guint16 cedilla_compose_seqs[] = {
#ifdef DEPRECATED_GDK_KEYSYMS
    GDK_dead_acute, GDK_C, 0, 0, 0, 0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
    GDK_dead_acute, GDK_c, 0, 0, 0, 0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
    GDK_Multi_key, GDK_apostrophe, GDK_C, 0, 0, 0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
    GDK_Multi_key, GDK_apostrophe, GDK_c, 0, 0, 0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
    GDK_Multi_key, GDK_C, GDK_apostrophe, 0, 0, 0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
    GDK_Multi_key, GDK_c, GDK_apostrophe, 0, 0, 0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
#else
    GDK_KEY_dead_acute, GDK_KEY_C, 0, 0, 0, 0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
    GDK_KEY_dead_acute, GDK_KEY_c, 0, 0, 0, 0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
    GDK_KEY_Multi_key, GDK_KEY_apostrophe, GDK_KEY_C, 0, 0, 0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
    GDK_KEY_Multi_key, GDK_KEY_apostrophe, GDK_KEY_c, 0, 0, 0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
    GDK_KEY_Multi_key, GDK_KEY_C, GDK_KEY_apostrophe, 0, 0, 0x00C7, /* LATIN_CAPITAL_LETTER_C_WITH_CEDILLA */
    GDK_KEY_Multi_key, GDK_KEY_c, GDK_KEY_apostrophe, 0, 0, 0x00E7, /* LATIN_SMALL_LETTER_C_WITH_CEDILLA */
#endif
};

void
fcitx_im_context_register_type(GTypeModule *type_module)
{
    static const GTypeInfo fcitx_im_context_info = {
        sizeof(FcitxIMContextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) fcitx_im_context_class_init,
        (GClassFinalizeFunc) fcitx_im_context_class_fini,
        NULL, /* klass data */
        sizeof(FcitxIMContext),
        0,
        (GInstanceInitFunc) fcitx_im_context_init,
        0
    };

    if (!_fcitx_type_im_context) {
        if (type_module) {
            _fcitx_type_im_context =
                g_type_module_register_type(type_module,
                                            GTK_TYPE_IM_CONTEXT,
                                            "FcitxIMContext",
                                            &fcitx_im_context_info,
                                            (GTypeFlags)0);
        } else {
            _fcitx_type_im_context =
                g_type_register_static(GTK_TYPE_IM_CONTEXT,
                                       "FcitxIMContext",
                                       &fcitx_im_context_info,
                                       (GTypeFlags)0);
        }
    }
}

GType
fcitx_im_context_get_type(void)
{
    if (_fcitx_type_im_context == 0) {
        fcitx_im_context_register_type(NULL);
    }

    g_assert(_fcitx_type_im_context != 0);
    return _fcitx_type_im_context;
}

FcitxIMContext *
fcitx_im_context_new(void)
{
    GObject *obj = g_object_new(FCITX_TYPE_IM_CONTEXT, NULL);
    return FCITX_IM_CONTEXT(obj);
}

///
static void
fcitx_im_context_class_init(FcitxIMContextClass *klass)
{
    GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    parent_class = (GtkIMContextClass *) g_type_class_peek_parent (klass);

    im_context_class->set_client_window = fcitx_im_context_set_client_window;
    im_context_class->filter_keypress = fcitx_im_context_filter_keypress;
    im_context_class->reset = fcitx_im_context_reset;
    im_context_class->get_preedit_string = fcitx_im_context_get_preedit_string;
    im_context_class->focus_in = fcitx_im_context_focus_in;
    im_context_class->focus_out = fcitx_im_context_focus_out;
    im_context_class->set_cursor_location = fcitx_im_context_set_cursor_location;
    im_context_class->set_use_preedit = fcitx_im_context_set_use_preedit;
    im_context_class->set_surrounding = fcitx_im_context_set_surrounding;
    gobject_class->finalize = fcitx_im_context_finalize;

    _signal_commit_id =
        g_signal_lookup("commit", G_TYPE_FROM_CLASS(klass));
    g_assert(_signal_commit_id != 0);

    _signal_preedit_changed_id =
        g_signal_lookup("preedit-changed", G_TYPE_FROM_CLASS(klass));
    g_assert(_signal_preedit_changed_id != 0);

    _signal_preedit_start_id =
        g_signal_lookup("preedit-start", G_TYPE_FROM_CLASS(klass));
    g_assert(_signal_preedit_start_id != 0);

    _signal_preedit_end_id =
        g_signal_lookup("preedit-end", G_TYPE_FROM_CLASS(klass));
    g_assert(_signal_preedit_end_id != 0);

    _signal_delete_surrounding_id =
        g_signal_lookup("delete-surrounding", G_TYPE_FROM_CLASS(klass));
    g_assert(_signal_delete_surrounding_id != 0);

    _signal_retrieve_surrounding_id =
        g_signal_lookup("retrieve-surrounding", G_TYPE_FROM_CLASS(klass));
    g_assert(_signal_retrieve_surrounding_id != 0);

    _use_key_snooper = !fcitx_utils_get_boolean_env ("IBUS_DISABLE_SNOOPER", !(_ENABLE_SNOOPER))
                       && !fcitx_utils_get_boolean_env("FCITX_DISABLE_SNOOPER", !(_ENABLE_SNOOPER));
    /* env IBUS_DISABLE_SNOOPER does not exist */
    if (_use_key_snooper) {
        /* disable snooper if app is in _no_snooper_apps */
        const gchar * prgname = g_get_prgname ();
        if (g_getenv ("IBUS_NO_SNOOPER_APPS")) {
            _no_snooper_apps = g_getenv ("IBUS_NO_SNOOPER_APPS");
        }
        if (g_getenv ("FCITX_NO_SNOOPER_APPS")) {
            _no_snooper_apps = g_getenv ("FCITX_NO_SNOOPER_APPS");
        }
        gchar **p;
        gchar ** apps = g_strsplit (_no_snooper_apps, ",", 0);
        for (p = apps; *p != NULL; p++) {
            if (g_regex_match_simple (*p, prgname, 0, 0)) {
                _use_key_snooper = FALSE;
                break;
            }
        }
        g_strfreev (apps);
    }

    /* make ibus fix benefits us */
    _use_sync_mode = fcitx_utils_get_boolean_env("IBUS_ENABLE_SYNC_MODE", FALSE)
                     || fcitx_utils_get_boolean_env("FCITX_ENABLE_SYNC_MODE", FALSE);
    /* always install snooper */
    if (_key_snooper_id == 0)
        _key_snooper_id = gtk_key_snooper_install (_key_snooper_cb, NULL);
}


static void
fcitx_im_context_class_fini (FcitxIMContextClass *klass)
{
    if (_key_snooper_id != 0) {
        gtk_key_snooper_remove (_key_snooper_id);
        _key_snooper_id = 0;
    }
}


static void
fcitx_im_context_init(FcitxIMContext *context)
{
    FcitxLog(DEBUG, "fcitx_im_context_init");
    context->client = NULL;
    context->area.x = -1;
    context->area.y = -1;
    context->area.width = 0;
    context->area.height = 0;
    context->use_preedit = TRUE;
    context->cursor_pos = 0;
    context->preedit_string = NULL;
    context->attrlist = NULL;
    context->capacity = CAPACITY_SURROUNDING_TEXT;

    context->slave = gtk_im_context_simple_new();
    gtk_im_context_simple_add_table(GTK_IM_CONTEXT_SIMPLE(context->slave),
                                    cedilla_compose_seqs,
                                    4,
                                    G_N_ELEMENTS(cedilla_compose_seqs) / (4 + 2));

    g_signal_connect(context->slave,
                     "commit",
                     G_CALLBACK(_slave_commit_cb),
                     context);
    g_signal_connect(context->slave,
                     "preedit-start",
                     G_CALLBACK(_slave_preedit_start_cb),
                     context);
    g_signal_connect(context->slave,
                     "preedit-end",
                     G_CALLBACK(_slave_preedit_end_cb),
                     context);
    g_signal_connect(context->slave,
                     "preedit-changed",
                     G_CALLBACK(_slave_preedit_changed_cb),
                     context);
    g_signal_connect(context->slave,
                     "retrieve-surrounding",
                     G_CALLBACK(_slave_retrieve_surrounding_cb),
                     context);
    g_signal_connect(context->slave,
                     "delete-surrounding",
                     G_CALLBACK(_slave_delete_surrounding_cb),
                     context);

    context->time = GDK_CURRENT_TIME;

    context->client = FcitxIMClientOpen(_fcitx_im_context_connect_cb, _fcitx_im_context_destroy_cb, G_OBJECT(context));
}

static void
fcitx_im_context_finalize(GObject *obj)
{
    FcitxLog(DEBUG, "fcitx_im_context_finalize");
    FcitxIMContext *context = FCITX_IM_CONTEXT(obj);

    FcitxIMClientClose(context->client);
    context->client = NULL;

    if (context->slave) {
        g_object_unref(context->slave);
        context->slave = NULL;
    }

    if (context->preedit_string) {
        g_free(context->preedit_string);
        context->preedit_string = NULL;
    }

    if (context->attrlist) {
        pango_attr_list_unref(context->attrlist);
        context->attrlist = NULL;
    }

    G_OBJECT_CLASS(parent_class)->finalize (obj);
}


static void
set_ic_client_window(FcitxIMContext *context,
                     GdkWindow       *client_window)
{
    if (!client_window)
        return;

    if (context->client_window) {
        g_object_unref(context->client_window);
        context->client_window = NULL;
    }

    if (client_window != NULL)
        context->client_window = g_object_ref(client_window);

    if (context->slave)
        gtk_im_context_set_client_window(context->slave, client_window);
}


///
static void
fcitx_im_context_set_client_window(GtkIMContext          *context,
                                   GdkWindow             *client_window)
{
    FcitxLog(DEBUG, "fcitx_im_context_set_client_window");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);
    set_ic_client_window(fcitxcontext, client_window);
}

static void
process_key_data_free(gpointer p)
{
    ProcessKeyStruct* pks = p;
    g_object_unref(pks->context);
    gdk_event_free((GdkEvent *)pks->event);
    g_free(p);
}

///
static gboolean
fcitx_im_context_filter_keypress(GtkIMContext *context,
                                 GdkEventKey  *event)
{
    FcitxLog(DEBUG, "fcitx_im_context_filter_keypress");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    /* check this first, since we use key snooper, most key will be handled. */
    if (IsFcitxIMClientValid(fcitxcontext->client) ) {
        /* XXX it is a workaround for some applications do not set client window. */
        if (fcitxcontext->client_window == NULL && event->window != NULL) {
            gtk_im_context_set_client_window((GtkIMContext *)fcitxcontext, event->window);

            /* set_cursor_location_internal() will get origin from X server,
            * it blocks UI. So delay it to idle callback. */
            gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE,
                            (GSourceFunc) _set_cursor_location_internal,
                            g_object_ref(fcitxcontext),
                            (GDestroyNotify) g_object_unref);
        }
    }

    if (G_UNLIKELY(event->state & FcitxKeyState_HandledMask))
        return TRUE;

    if (G_UNLIKELY(event->state & FcitxKeyState_IgnoredMask))
        return gtk_im_context_filter_keypress(fcitxcontext->slave, event);

    if (IsFcitxIMClientValid(fcitxcontext->client) && fcitxcontext->has_focus) {
        _request_surrounding_text (&fcitxcontext);
        if (G_UNLIKELY(!fcitxcontext))
            return FALSE;

        fcitxcontext->time = event->time;

        if (_use_sync_mode) {
            int ret = FcitxIMClientProcessKeySync(fcitxcontext->client,
                                                  event->keyval,
                                                  event->hardware_keycode,
                                                  event->state,
                                                  (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                                  event->time);
            if (ret <= 0) {
                event->state |= FcitxKeyState_IgnoredMask;
                return gtk_im_context_filter_keypress(fcitxcontext->slave, event);
            } else {
                event->state |= FcitxKeyState_HandledMask;
                return TRUE;
            }
        } else {
            ProcessKeyStruct* pks = g_malloc0(sizeof(ProcessKeyStruct));
            pks->context = g_object_ref(fcitxcontext);
            pks->event = (GdkEventKey *)  gdk_event_copy((GdkEvent *) event);

            FcitxIMClientProcessKey(fcitxcontext->client,
                                    _fcitx_im_context_process_key_cb,
                                    pks,
                                    process_key_data_free,
                                    event->keyval,
                                    event->hardware_keycode,
                                    event->state,
                                    (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                    event->time);
            event->state |= FcitxKeyState_HandledMask;
            return TRUE;
        }
    } else {
        return gtk_im_context_filter_keypress(fcitxcontext->slave, event);
    }
    return FALSE;
}

static void
_fcitx_im_context_process_key_cb(DBusGProxy *proxy,
                                 DBusGProxyCall *call_id,
                                 gpointer user_data)
{
    ProcessKeyStruct* pks = user_data;
    GdkEventKey* event = pks->event;
    GError* error = NULL;
    int ret;
    dbus_g_proxy_end_call(proxy, call_id, &error, G_TYPE_INT, &ret, G_TYPE_INVALID);
    if (ret <= 0) {
        event->state |= FcitxKeyState_IgnoredMask;
        gdk_event_put((GdkEvent *)event);
    }
}

static void
_fcitx_im_context_update_preedit_cb(DBusGProxy* proxy, char* str, int cursor_pos, void* user_data)
{
    FcitxLog(DEBUG, "_fcitx_im_context_commit_string_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);

    gboolean visible = false;

    if (context->preedit_string != NULL) {
        if (strlen(context->preedit_string) != 0)
            visible = true;
        g_free(context->preedit_string);
        context->preedit_string = NULL;
    }
    context->preedit_string = g_strdup(str);
    if (context->attrlist) {
        pango_attr_list_unref(context->attrlist);
        context->attrlist = NULL;
    }
    char* tempstr = g_strndup(str, cursor_pos);
    context->cursor_pos =  fcitx_utf8_strlen(tempstr);
    g_free(tempstr);

    gboolean new_visible = false;

    if (context->preedit_string != NULL) {
        if (strlen(context->preedit_string) != 0)
            new_visible = true;
    }
    gboolean flag = new_visible != visible;

    if (new_visible) {
        if (flag) {
            /* invisible => visible */
            g_signal_emit(context, _signal_preedit_start_id, 0);
        }
        g_signal_emit(context, _signal_preedit_changed_id, 0);
    } else {
        if (flag) {
            /* visible => invisible */
            g_signal_emit(context, _signal_preedit_changed_id, 0);
            g_signal_emit(context, _signal_preedit_end_id, 0);
        } else {
            /* still invisible */
            /* do nothing */
        }
    }
}

static void
_fcitx_im_context_update_formatted_preedit_cb(DBusGProxy* proxy, GPtrArray* array, int cursor_pos, void* user_data)
{
    FcitxLog(DEBUG, "_fcitx_im_context_commit_string_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);

    gboolean visible = false;

    if (context->preedit_string != NULL) {
        if (strlen(context->preedit_string) != 0)
            visible = true;
        g_free(context->preedit_string);
        context->preedit_string = NULL;
    }

    if (context->attrlist != NULL) {
        pango_attr_list_unref(context->attrlist);
    }

    context->attrlist = pango_attr_list_new();

    GString* gstr = g_string_new(NULL);

    int i = 0;
    for (i = 0; i < array->len; i++) {
        size_t bytelen = strlen(gstr->str);
        GValueArray* preedit = g_ptr_array_index(array, i);
        const gchar* s = g_value_get_string(g_value_array_get_nth(preedit, 0));
        gint type = g_value_get_int(g_value_array_get_nth(preedit, 1));

        PangoAttribute *pango_attr = NULL;
        if ((type & MSG_NOUNDERLINE) == 0) {
            pango_attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
            pango_attr->start_index = bytelen;
            pango_attr->end_index = bytelen + strlen(s);
            pango_attr_list_insert(context->attrlist, pango_attr);
        }

        if (type & MSG_HIGHLIGHT) {
            gboolean hasColor;
            GdkColor fg;
            GdkColor bg;
            memset(&fg, 0, sizeof(GdkColor));
            memset(&bg, 0, sizeof(GdkColor));

            if (context->client_window) {
                GtkWidget *widget;
                gdk_window_get_user_data (context->client_window,
                                        (gpointer *)&widget);
                if (GTK_IS_WIDGET(widget)) {
                    hasColor = true;
                    GtkStyle* style = gtk_widget_get_style(widget);
                    fg = style->text[GTK_STATE_SELECTED];
                    bg = style->bg[GTK_STATE_SELECTED];
                }
            }

            if (!hasColor) {
                fg.red = 0xffff;
                fg.green = 0xffff;
                fg.blue = 0xffff;
                bg.red = 0x43ff;
                bg.green = 0xacff;
                bg.blue = 0xe8ff;
            }

            pango_attr = pango_attr_foreground_new(fg.red, fg.green, fg.blue);
            pango_attr->start_index = bytelen;
            pango_attr->end_index = bytelen + strlen(s);
            pango_attr_list_insert(context->attrlist, pango_attr);
            pango_attr = pango_attr_background_new(bg.red, bg.green, bg.blue);
            pango_attr->start_index = bytelen;
            pango_attr->end_index = bytelen + strlen(s);
            pango_attr_list_insert(context->attrlist, pango_attr);
        }
        gstr = g_string_append(gstr, s);
    }

    gchar* str = g_string_free(gstr, FALSE);

    context->preedit_string = str;
    char* tempstr = g_strndup(str, cursor_pos);
    context->cursor_pos =  fcitx_utf8_strlen(tempstr);
    g_free(tempstr);

    gboolean new_visible = FALSE;

    if (context->preedit_string != NULL && context->preedit_string[0] == 0) {
        g_free(context->preedit_string);
        context->preedit_string = NULL;
    }

    if (context->preedit_string != NULL)
        new_visible = TRUE;

    gboolean flag = new_visible != visible;

    if (new_visible) {
        if (flag) {
            /* invisible => visible */
            g_signal_emit(context, _signal_preedit_start_id, 0);
        }
        g_signal_emit(context, _signal_preedit_changed_id, 0);
    } else {
        if (flag) {
            /* visible => invisible */
            g_signal_emit(context, _signal_preedit_changed_id, 0);
            g_signal_emit(context, _signal_preedit_end_id, 0);
        } else {
            /* still invisible */
            /* do nothing */
        }
    }
}

///
static void
fcitx_im_context_focus_in(GtkIMContext *context)
{
    FcitxLog(DEBUG, "fcitx_im_context_focus_in");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    if (fcitxcontext->has_focus)
        return;

    _fcitx_im_context_set_capacity(fcitxcontext, FALSE);

    fcitxcontext->has_focus = true;

    if (_focus_im_context != NULL) {
        g_assert (_focus_im_context != context);
        gtk_im_context_focus_out (_focus_im_context);
        g_assert (_focus_im_context == NULL);
    }

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxIMClientFocusIn(fcitxcontext->client);
    }

    gtk_im_context_focus_in(fcitxcontext->slave);

    /* set_cursor_location_internal() will get origin from X server,
     * it blocks UI. So delay it to idle callback. */
    gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE,
                    (GSourceFunc) _set_cursor_location_internal,
                    g_object_ref(fcitxcontext),
                    (GDestroyNotify) g_object_unref);

    _request_surrounding_text (&fcitxcontext);
    if (G_UNLIKELY(!fcitxcontext))
        return;

    g_object_add_weak_pointer ((GObject *) context,
                               (gpointer *) &_focus_im_context);
    _focus_im_context = context;

    return;
}

static void
fcitx_im_context_focus_out(GtkIMContext *context)
{
    FcitxLog(DEBUG, "fcitx_im_context_focus_out");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    if (!fcitxcontext->has_focus) {
        return;
    }

    g_assert (context == _focus_im_context);
    g_object_remove_weak_pointer ((GObject *) context,
                                  (gpointer *) &_focus_im_context);
    _focus_im_context = NULL;

    fcitxcontext->has_focus = false;

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxIMClientFocusOut(fcitxcontext->client);
    }

    fcitxcontext->cursor_pos = 0;
    if (fcitxcontext->preedit_string != NULL) {
        g_free(fcitxcontext->preedit_string);
        fcitxcontext->preedit_string = NULL;
        g_signal_emit(fcitxcontext, _signal_preedit_changed_id, 0);
        g_signal_emit(fcitxcontext, _signal_preedit_end_id, 0);
    }

    gtk_im_context_focus_out(fcitxcontext->slave);

    return;
}

///
static void
fcitx_im_context_set_cursor_location(GtkIMContext *context,
                                     GdkRectangle *area)
{
    FcitxLog(DEBUG, "fcitx_im_context_set_cursor_location %d %d %d %d", area->x, area->y, area->height, area->width);
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    if (fcitxcontext->area.x == area->x &&
            fcitxcontext->area.y == area->y &&
            fcitxcontext->area.width == area->width &&
            fcitxcontext->area.height == area->height) {
        return;
    }
    fcitxcontext->area = *area;

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        _set_cursor_location_internal(fcitxcontext);
    }
    gtk_im_context_set_cursor_location(fcitxcontext->slave, area);

    return;
}

static gboolean
_set_cursor_location_internal(FcitxIMContext *fcitxcontext)
{
    GdkRectangle area;

    if (fcitxcontext->client_window == NULL ||
            !IsFcitxIMClientValid(fcitxcontext->client)) {
        return FALSE;
    }

    area = fcitxcontext->area;
    if (area.x == -1 && area.y == -1 && area.width == 0 && area.height == 0) {
#if GTK_CHECK_VERSION (2, 91, 0)
        area.x = 0;
        area.y += gdk_window_get_height(fcitxcontext->client_window);
#else
        gint w, h;
        gdk_drawable_get_size(fcitxcontext->client_window, &w, &h);
        area.y += h;
        area.x = 0;
#endif
    }

#if GTK_CHECK_VERSION (2, 18, 0)
    gdk_window_get_root_coords(fcitxcontext->client_window,
                               area.x, area.y,
                               &area.x, &area.y);
#else
    {
        int rootx, rooty;
        gdk_window_get_origin(fcitxcontext->client_window, &rootx, &rooty);
        area.x = rootx + area.x;
        area.y = rooty + area.y;
    }
#endif

    FcitxIMClientSetCursorRect(fcitxcontext->client, area.x, area.y, area.width, area.height);
    return FALSE;
}

///
static void
fcitx_im_context_set_use_preedit(GtkIMContext *context,
                                 gboolean      use_preedit)
{
    FcitxLog(DEBUG, "fcitx_im_context_set_use_preedit");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    fcitxcontext->use_preedit = use_preedit;
    _fcitx_im_context_set_capacity(fcitxcontext, FALSE);

    gtk_im_context_set_use_preedit(fcitxcontext->slave, use_preedit);
}


static guint
get_selection_anchor_point (FcitxIMContext *fcitxcontext,
                            guint cursor_pos,
                            guint surrounding_text_len)
{
    GtkWidget *widget;
    if (fcitxcontext->client_window == NULL) {
        return cursor_pos;
    }
    gdk_window_get_user_data (fcitxcontext->client_window, (gpointer *)&widget);

    if (!GTK_IS_TEXT_VIEW (widget)){
        return cursor_pos;
    }

    GtkTextView *text_view = GTK_TEXT_VIEW (widget);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (text_view);

    if (!gtk_text_buffer_get_has_selection (buffer)) {
        return cursor_pos;
    }

    GtkTextIter start_iter, end_iter, cursor_iter;
    if (!gtk_text_buffer_get_selection_bounds (buffer, &start_iter, &end_iter)) {
        return cursor_pos;
    }

    gtk_text_buffer_get_iter_at_mark (buffer,
                                      &cursor_iter,
                                      gtk_text_buffer_get_insert (buffer));

    guint start_index = gtk_text_iter_get_offset (&start_iter);
    guint end_index = gtk_text_iter_get_offset (&end_iter);
    guint cursor_index = gtk_text_iter_get_offset (&cursor_iter);

    guint anchor;

    if (start_index == cursor_index) {
        anchor = end_index;
    } else if (end_index == cursor_index) {
        anchor = start_index;
    } else {
        return cursor_pos;
    }

    // Change absolute index to relative position.
    guint relative_origin = cursor_index - cursor_pos;

    if (anchor < relative_origin) {
        return cursor_pos;
    }
    anchor -= relative_origin;

    if (anchor > surrounding_text_len) {
        return cursor_pos;
    }

    return anchor;
}


static void
fcitx_im_context_set_surrounding (GtkIMContext *context,
                                  const gchar *text,
                                  gint l,
                                  gint cursor_index)
{
    g_return_if_fail (context != NULL);
    g_return_if_fail (FCITX_IS_IM_CONTEXT (context));
    g_return_if_fail (text != NULL);

    gint len = l;
    if (len < 0) {
        len = strlen(text);
    }

    g_return_if_fail (0 <= cursor_index && cursor_index <= len);

    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);

    if (IsFcitxIMClientValid(fcitxcontext->client) && !(fcitxcontext->capacity & CAPACITY_PASSWORD)) {
        guint cursor_pos;
        guint utf8_len;
        gchar *p;

        p = g_strndup (text, len);
        cursor_pos = g_utf8_strlen (p, cursor_index);
        utf8_len = g_utf8_strlen(p, len);

        guint anchor_pos = get_selection_anchor_point (fcitxcontext,
                                                       cursor_pos,
                                                       utf8_len);
        FcitxIMClientSetSurroundingText (fcitxcontext->client,
                                         p,
                                         cursor_pos,
                                         anchor_pos);
        g_free (p);
    }
    gtk_im_context_set_surrounding (fcitxcontext->slave,
                                    text,
                                    l,
                                    cursor_index);
}

void
_fcitx_im_context_set_capacity(FcitxIMContext* fcitxcontext, gboolean force)
{
    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxCapacityFlags flags = fcitxcontext->capacity & ~(CAPACITY_PREEDIT | CAPACITY_FORMATTED_PREEDIT | CAPACITY_PASSWORD);
        if (fcitxcontext->use_preedit)
            flags |= CAPACITY_PREEDIT | CAPACITY_FORMATTED_PREEDIT;

        if (fcitxcontext->client_window != NULL) {
            GtkWidget *widget;
            gdk_window_get_user_data (fcitxcontext->client_window,
                                    (gpointer *)&widget);
            if (GTK_IS_ENTRY (widget) &&
                !gtk_entry_get_visibility (GTK_ENTRY (widget))) {
                flags |= CAPACITY_PASSWORD;
            }
        }

        gboolean update = FALSE;
        if (G_UNLIKELY(fcitxcontext->capacity != flags)) {
            fcitxcontext->capacity = flags;
            update = TRUE;
        }
        if (G_UNLIKELY(update || force))
            FcitxIMClientSetCapacity(fcitxcontext->client, fcitxcontext->capacity);
    }
}

///
static void
fcitx_im_context_reset(GtkIMContext *context)
{
    FcitxLog(DEBUG, "fcitx_im_context_reset");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxIMClientReset(fcitxcontext->client);
    }

    gtk_im_context_reset(fcitxcontext->slave);
}

static void
fcitx_im_context_get_preedit_string(GtkIMContext   *context,
                                    gchar         **str,
                                    PangoAttrList **attrs,
                                    gint           *cursor_pos)
{
    FcitxLog(DEBUG, "fcitx_im_context_get_preedit_string");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        if (str) {
            if (fcitxcontext->preedit_string)
                *str = strdup(fcitxcontext->preedit_string);
            else
                *str = strdup("");
        }
        if (attrs) {
            if (fcitxcontext->attrlist == NULL) {
                *attrs = pango_attr_list_new();

                if (str) {
                    PangoAttribute *pango_attr;
                    pango_attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
                    pango_attr->start_index = 0;
                    pango_attr->end_index = strlen(*str);
                    pango_attr_list_insert(*attrs, pango_attr);
                }
            }
            else {
                *attrs = pango_attr_list_ref (fcitxcontext->attrlist);
            }
        }
        if (cursor_pos)
            *cursor_pos = fcitxcontext->cursor_pos;

    } else
        gtk_im_context_get_preedit_string(fcitxcontext->slave, str, attrs, cursor_pos);
    return ;
}

/* Callback functions for slave context */
static void
_slave_commit_cb(GtkIMContext *slave,
                 gchar *string,
                 FcitxIMContext *context)
{
    g_signal_emit(context, _signal_commit_id, 0, string);
}
static void
_slave_preedit_changed_cb(GtkIMContext *slave,
                          FcitxIMContext *context)
{
    if (context->client) {
        return;
    }

    g_signal_emit(context, _signal_preedit_changed_id, 0);
}
static void
_slave_preedit_start_cb(GtkIMContext *slave,
                        FcitxIMContext *context)
{
    if (context->client) {
        return;
    }

    g_signal_emit(context, _signal_preedit_start_id, 0);
}

static void
_slave_preedit_end_cb(GtkIMContext *slave,
                      FcitxIMContext *context)
{
    if (context->client) {
        return;
    }
    g_signal_emit(context, _signal_preedit_end_id, 0);
}

static gboolean
_slave_retrieve_surrounding_cb(GtkIMContext *slave,
                               FcitxIMContext *context)
{
    gboolean return_value;

    if (context->client) {
        return FALSE;
    }
    g_signal_emit(context, _signal_retrieve_surrounding_id, 0,
                  &return_value);
    return return_value;
}

static gboolean
_slave_delete_surrounding_cb(GtkIMContext *slave,
                             gint offset_from_cursor,
                             guint nchars,
                             FcitxIMContext *context)
{
    gboolean return_value;

    if (context->client) {
        return FALSE;
    }
    g_signal_emit(context, _signal_delete_surrounding_id, 0, offset_from_cursor, nchars, &return_value);
    return return_value;
}

void _fcitx_im_context_enable_im_cb(DBusGProxy* proxy, void* user_data)
{
    FcitxLog(DEBUG, "_fcitx_im_context_enable_im_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    FcitxIMClientSetEnabled(context->client, true);
}

void _fcitx_im_context_close_im_cb(DBusGProxy* proxy, void* user_data)
{
    FcitxLog(DEBUG, "_fcitx_im_context_close_im_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    FcitxIMClientSetEnabled(context->client, false);

    if (context->preedit_string != NULL)
        g_free(context->preedit_string);
    context->preedit_string = NULL;
    context->cursor_pos = 0;
    g_signal_emit(context, _signal_preedit_changed_id, 0);
    g_signal_emit(context, _signal_preedit_end_id, 0);
}

void _fcitx_im_context_commit_string_cb(DBusGProxy* proxy, char* str, void* user_data)
{
    FcitxLog(DEBUG, "_fcitx_im_context_commit_string_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    g_signal_emit(context, _signal_commit_id, 0, str);
}

void _fcitx_im_context_forward_key_cb(DBusGProxy* proxy, guint keyval, guint state, gint type, void* user_data)
{
    FcitxLog(DEBUG, "_fcitx_im_context_forward_key_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    FcitxKeyEventType tp = (FcitxKeyEventType) type;
    GdkEventKey* event = _create_gdk_event(context, keyval, state, tp);
    event->state |= FcitxKeyState_IgnoredMask;
    gdk_event_put((GdkEvent *)event);
    gdk_event_free((GdkEvent *)event);
}

static void
_fcitx_im_context_delete_surrounding_text_cb (DBusGProxy* proxy,
                                          gint offset_from_cursor,
                                          guint nchars,
                                          void* user_data)
{
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    gboolean return_value;
    g_signal_emit (context, _signal_delete_surrounding_id, 0, offset_from_cursor, nchars, &return_value);
}

/* Copy from gdk */
static GdkEventKey *
_create_gdk_event(FcitxIMContext *fcitxcontext,
                  guint keyval,
                  guint state,
                  FcitxKeyEventType type
                 )
{
    gunichar c = 0;
    gchar buf[8];

    GdkEventKey *event = (GdkEventKey *)gdk_event_new((type == FCITX_RELEASE_KEY) ? GDK_KEY_RELEASE : GDK_KEY_PRESS);

    if (fcitxcontext && fcitxcontext->client_window)
        event->window = g_object_ref(fcitxcontext->client_window);

    /* The time is copied the latest value from the previous
     * GdkKeyEvent in filter_keypress().
     *
     * We understand the best way would be to pass the all time value
     * to Fcitx functions process_key_event() and Fcitx DBus functions
     * ProcessKeyEvent() in IM clients and IM engines so that the
     * _create_gdk_event() could get the correct time values.
     * However it would causes to change many functions and the time value
     * would not provide the useful meanings for each Fcitx engines but just
     * pass the original value to ForwardKeyEvent().
     * We use the saved value at the moment.
     *
     * Another idea might be to have the time implementation in X servers
     * but some Xorg uses clock_gettime() and others use gettimeofday()
     * and the values would be different in each implementation and
     * locale/remote X server. So probably that idea would not work. */
    if (fcitxcontext) {
        event->time = fcitxcontext->time;
    } else {
        event->time = GDK_CURRENT_TIME;
    }

    event->send_event = FALSE;
    event->state = state;
    event->keyval = keyval;
    event->string = NULL;
    event->length = 0;
    event->hardware_keycode = 0;
    if (event->window) {
#ifndef NEW_GDK_WINDOW_GET_DISPLAY
        GdkDisplay      *display = gdk_display_get_default();
#else
        GdkDisplay      *display = gdk_window_get_display(event->window);
#endif
        GdkKeymap       *keymap  = gdk_keymap_get_for_display(display);
        GdkKeymapKey    *keys;
        gint             n_keys = 0;

        if (gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &n_keys)) {
            if (n_keys)
                event->hardware_keycode = keys[0].keycode;
            g_free(keys);
        }
    }

    event->group = 0;
    event->is_modifier = _key_is_modifier(keyval);

#ifdef DEPRECATED_GDK_KEYSYMS
    if (keyval != GDK_VoidSymbol)
#else
    if (keyval != GDK_KEY_VoidSymbol)
#endif
        c = gdk_keyval_to_unicode(keyval);

    if (c) {
        gsize bytes_written;
        gint len;

        /* Apply the control key - Taken from Xlib
        */
        if (event->state & GDK_CONTROL_MASK) {
            if ((c >= '@' && c < '\177') || c == ' ') c &= 0x1F;
            else if (c == '2') {
                event->string = g_memdup("\0\0", 2);
                event->length = 1;
                buf[0] = '\0';
                goto out;
            } else if (c >= '3' && c <= '7') c -= ('3' - '\033');
            else if (c == '8') c = '\177';
            else if (c == '/') c = '_' & 0x1F;
        }

        len = g_unichar_to_utf8(c, buf);
        buf[len] = '\0';

        event->string = g_locale_from_utf8(buf, len,
                                           NULL, &bytes_written,
                                           NULL);
        if (event->string)
            event->length = bytes_written;
#ifdef DEPRECATED_GDK_KEYSYMS
    } else if (keyval == GDK_Escape) {
#else
    } else if (keyval == GDK_KEY_Escape) {
#endif
        event->length = 1;
        event->string = g_strdup("\033");
    }
#ifdef DEPRECATED_GDK_KEYSYMS
    else if (keyval == GDK_Return ||
             keyval == GDK_KP_Enter) {
#else
    else if (keyval == GDK_KEY_Return ||
             keyval == GDK_KEY_KP_Enter) {
#endif
        event->length = 1;
        event->string = g_strdup("\r");
    }

    if (!event->string) {
        event->length = 0;
        event->string = g_strdup("");
    }
out:
    return event;
}


static gboolean
_key_is_modifier(guint keyval)
{
    /* See gdkkeys-x11.c:_gdk_keymap_key_is_modifier() for how this
    * really should be implemented */

    switch (keyval) {
#ifdef DEPRECATED_GDK_KEYSYMS
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_Control_L:
    case GDK_Control_R:
    case GDK_Caps_Lock:
    case GDK_Shift_Lock:
    case GDK_Meta_L:
    case GDK_Meta_R:
    case GDK_Alt_L:
    case GDK_Alt_R:
    case GDK_Super_L:
    case GDK_Super_R:
    case GDK_Hyper_L:
    case GDK_Hyper_R:
    case GDK_ISO_Lock:
    case GDK_ISO_Level2_Latch:
    case GDK_ISO_Level3_Shift:
    case GDK_ISO_Level3_Latch:
    case GDK_ISO_Level3_Lock:
    case GDK_ISO_Group_Shift:
    case GDK_ISO_Group_Latch:
    case GDK_ISO_Group_Lock:
        return TRUE;
#else
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    case GDK_KEY_Caps_Lock:
    case GDK_KEY_Shift_Lock:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
    case GDK_KEY_ISO_Lock:
    case GDK_KEY_ISO_Level2_Latch:
    case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_ISO_Level3_Latch:
    case GDK_KEY_ISO_Level3_Lock:
    case GDK_KEY_ISO_Level5_Shift:
    case GDK_KEY_ISO_Level5_Latch:
    case GDK_KEY_ISO_Level5_Lock:
    case GDK_KEY_ISO_Group_Shift:
    case GDK_KEY_ISO_Group_Latch:
    case GDK_KEY_ISO_Group_Lock:
        return TRUE;
#endif
    default:
        return FALSE;
    }
}

void _fcitx_im_context_connect_cb(FcitxIMClient* client, void* user_data)
{
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    if (IsFcitxIMClientValid(client)) {
        FcitxIMClientConnectSignal(client,
                                   G_CALLBACK(_fcitx_im_context_enable_im_cb),
                                   G_CALLBACK(_fcitx_im_context_close_im_cb),
                                   G_CALLBACK(_fcitx_im_context_commit_string_cb),
                                   G_CALLBACK(_fcitx_im_context_forward_key_cb),
                                   G_CALLBACK(_fcitx_im_context_update_preedit_cb),
                                   G_CALLBACK(_fcitx_im_context_update_formatted_preedit_cb),
                                   G_CALLBACK(_fcitx_im_context_delete_surrounding_text_cb),
                                   context,
                                   NULL);
        _fcitx_im_context_set_capacity(context, TRUE);

        if (context->has_focus && _focus_im_context == GTK_IM_CONTEXT(context))
            FcitxIMClientFocusIn(context->client);

        /* set_cursor_location_internal() will get origin from X server,
         * it blocks UI. So delay it to idle callback. */
        gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE,
                                  (GSourceFunc) _set_cursor_location_internal,
                                  g_object_ref(context),
                                  (GDestroyNotify) g_object_unref);

    }

}

void _fcitx_im_context_destroy_cb(FcitxIMClient* client, void* user_data)
{
    FcitxIMClientSetEnabled(client, false);
}

static void
_request_surrounding_text (FcitxIMContext **context)
{
    if (*context &&
        IsFcitxIMClientValid((*context)->client)) {
        gboolean return_value;
        FcitxLog(DEBUG, "requesting surrounding text");

        /* according to RH#859879, something bad could happen here. */
        g_object_add_weak_pointer ((GObject *) *context,
                                   (gpointer *) context);
        /* some unref can happen here */
        g_signal_emit (*context, _signal_retrieve_surrounding_id, 0,
                       &return_value);
        if (context)
            g_object_remove_weak_pointer ((GObject *) *context,
                                          (gpointer *) context);
        else
            return;
        if (return_value) {
            (*context)->capacity |= CAPACITY_SURROUNDING_TEXT;
            _fcitx_im_context_set_capacity (*context,
                                            FALSE);
        }
        else {
            (*context)->capacity &= ~CAPACITY_SURROUNDING_TEXT;
            _fcitx_im_context_set_capacity (*context,
                                            FALSE);
        }
    }
}


static gint
_key_snooper_cb (GtkWidget   *widget,
                 GdkEventKey *event,
                 gpointer     user_data)
{
    gboolean retval = FALSE;

    FcitxIMContext *fcitxcontext = (FcitxIMContext *) _focus_im_context;

    if (G_UNLIKELY(!_use_key_snooper))
        return FALSE;

    if (fcitxcontext == NULL || !fcitxcontext->has_focus)
        return FALSE;

    if (G_UNLIKELY (event->state & FcitxKeyState_HandledMask))
        return TRUE;

    if (G_UNLIKELY (event->state & FcitxKeyState_IgnoredMask))
        return FALSE;

    do {
        if (!IsFcitxIMClientValid(fcitxcontext->client)) {
            break;
        }

        _request_surrounding_text (&fcitxcontext);
        if (G_UNLIKELY(!fcitxcontext))
            return FALSE;
        fcitxcontext->time = event->time;

        if (_use_sync_mode) {

            int ret = FcitxIMClientProcessKeySync(fcitxcontext->client,
                                                    event->keyval,
                                                    event->hardware_keycode,
                                                    event->state,
                                                    (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                                    event->time);
            if (ret <= 0)
                retval = FALSE;
            else
                retval = TRUE;
        } else {
            ProcessKeyStruct* pks = g_malloc0(sizeof(ProcessKeyStruct));
            pks->context = g_object_ref(fcitxcontext);
            pks->event = (GdkEventKey *)  gdk_event_copy((GdkEvent *) event);

            FcitxIMClientProcessKey(fcitxcontext->client,
                                    _fcitx_im_context_process_key_cb,
                                    pks,
                                    process_key_data_free,
                                    event->keyval,
                                    event->hardware_keycode,
                                    event->state,
                                    (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                    event->time);
            retval = TRUE;
        }
    } while(0);

    if (!retval) {
        event->state |= FcitxKeyState_IgnoredMask;
        return FALSE;
    } else {
        event->state |= FcitxKeyState_HandledMask;
        return TRUE;
    }

    return retval;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
