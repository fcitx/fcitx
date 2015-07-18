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
#include <xkbcommon/xkbcommon-compose.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/log.h"
#include "fcitx/frontend.h"

#include "fcitximcontext.h"
#include "fcitx-gclient/fcitxconnection.h"
#include "fcitx-gclient/fcitxclient.h"

#if !GTK_CHECK_VERSION (2, 91, 0)
# define DEPRECATED_GDK_KEYSYMS 1
#endif

#if GTK_CHECK_VERSION (2, 24, 0)
# define NEW_GDK_WINDOW_GET_DISPLAY
#endif

static const FcitxCapacityFlags purpose_related_capacity =
    CAPACITY_ALPHA |
    CAPACITY_DIGIT |
    CAPACITY_NUMBER |
    CAPACITY_DIALABLE |
    CAPACITY_URL |
    CAPACITY_EMAIL |
    CAPACITY_PASSWORD;

static const FcitxCapacityFlags hints_related_capacity =
    CAPACITY_SPELLCHECK |
    CAPACITY_NO_SPELLCHECK |
    CAPACITY_WORD_COMPLETION |
    CAPACITY_LOWERCASE |
    CAPACITY_UPPERCASE |
    CAPACITY_UPPERCASE_WORDS |
    CAPACITY_UPPERCASE_SENTENCES |
    CAPACITY_NO_ON_SCREEN_KEYBOARD;

struct _FcitxIMContext {
    GtkIMContext parent;

    GdkWindow *client_window;
    GdkRectangle area;
    FcitxClient* client;
    GtkIMContext* slave;
    int has_focus;
    guint32 time;
    gboolean use_preedit;
    gboolean support_surrounding_text;
    gboolean is_inpreedit;
    gchar* preedit_string;
    gchar* surrounding_text;
    int cursor_pos;
    FcitxCapacityFlags capacity_from_toolkit;
    FcitxCapacityFlags last_updated_capacity;
    PangoAttrList* attrlist;
    gint last_cursor_pos;
    gint last_anchor_pos;
    struct xkb_compose_state* xkbComposeState;
};

struct _FcitxIMContextClass {
    GtkIMContextClass parent;
    /* klass members */
};

/* functions prototype */
static void     fcitx_im_context_class_init(FcitxIMContextClass   *klass);
static void     fcitx_im_context_class_fini(FcitxIMContextClass *klass);
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
fcitx_im_context_set_surrounding(GtkIMContext *context, const gchar *text,
                                 gint len, gint cursor_index);
static void
fcitx_im_context_get_preedit_string(GtkIMContext *context, gchar **str,
                                    PangoAttrList **attrs, gint *cursor_pos);

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
_fcitx_im_context_enable_im_cb(FcitxClient* client, void* user_data);
static void
_fcitx_im_context_close_im_cb(FcitxClient* client, void* user_data);
static void
_fcitx_im_context_commit_string_cb(FcitxClient* client, char* str, void* user_data);
static void
_fcitx_im_context_forward_key_cb(FcitxClient* client, guint keyval, guint state, gint type, void* user_data);
static void
_fcitx_im_context_delete_surrounding_text_cb (FcitxClient* client, gint offset_from_cursor, guint nchars, void* user_data);
static void
_fcitx_im_context_connect_cb(FcitxClient* client, void* user_data);
static void
_fcitx_im_context_update_formatted_preedit_cb(FcitxClient* im, GPtrArray* array, int cursor_pos, void* user_data);
static void
_fcitx_im_context_process_key_cb(GObject* source_object, GAsyncResult* res, gpointer user_data);
static void
_fcitx_im_context_set_capacity(FcitxIMContext* fcitxcontext, gboolean force);

#if GTK_CHECK_VERSION(3, 6, 0)

static void
_fcitx_im_context_input_hints_changed_cb(GObject *gobject, GParamSpec *pspec,
                                         gpointer user_data);
static void
_fcitx_im_context_input_purpose_changed_cb(GObject *gobject, GParamSpec *pspec,
                                           gpointer user_data);
#endif


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
static FcitxConnection* _connection = NULL;
static struct xkb_context* xkbContext = NULL;
static struct xkb_compose_table* xkbComposeTable = NULL;

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
fcitx_im_context_class_fini(FcitxIMContextClass *klass)
{
    FCITX_UNUSED(klass);
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
    context->last_anchor_pos = -1;
    context->last_cursor_pos = -1;
    context->preedit_string = NULL;
    context->attrlist = NULL;
    context->last_updated_capacity = CAPACITY_SURROUNDING_TEXT;

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

#if GTK_CHECK_VERSION(3, 6, 0)
    g_signal_connect(context, "notify::input-hints", G_CALLBACK(_fcitx_im_context_input_hints_changed_cb), NULL);
    g_signal_connect(context, "notify::input-purpose", G_CALLBACK(_fcitx_im_context_input_purpose_changed_cb), NULL);
#endif

    context->time = GDK_CURRENT_TIME;

    static gsize has_info = 0;
    if (g_once_init_enter(&has_info)) {
        _connection = fcitx_connection_new();
        g_object_ref_sink(_connection);

        xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

        if (xkbContext) {
            xkb_context_set_log_level(xkbContext, XKB_LOG_LEVEL_CRITICAL);
        }

        const char* locale = getenv("LC_ALL");
        if (!locale)
            locale = getenv("LC_CTYPE");
        if (!locale)
            locale = getenv("LANG");
        if (!locale)
            locale = "C";

        xkbComposeTable = xkbContext ? xkb_compose_table_new_from_locale(xkbContext, locale, XKB_COMPOSE_COMPILE_NO_FLAGS) : NULL;

        g_once_init_leave(&has_info, 1);
    }

    context->client = fcitx_client_new_with_connection(_connection);
    g_signal_connect(context->client, "connected", G_CALLBACK(_fcitx_im_context_connect_cb), context);
    g_signal_connect(context->client, "enable-im", G_CALLBACK(_fcitx_im_context_enable_im_cb), context);
    g_signal_connect(context->client, "close-im", G_CALLBACK(_fcitx_im_context_close_im_cb), context);
    g_signal_connect(context->client, "forward-key", G_CALLBACK(_fcitx_im_context_forward_key_cb), context);
    g_signal_connect(context->client, "commit-string", G_CALLBACK(_fcitx_im_context_commit_string_cb), context);
    g_signal_connect(context->client, "delete-surrounding-text", G_CALLBACK(_fcitx_im_context_delete_surrounding_text_cb), context);
    g_signal_connect(context->client, "update-formatted-preedit", G_CALLBACK(_fcitx_im_context_update_formatted_preedit_cb), context);

    context->xkbComposeState = xkbComposeTable ? xkb_compose_state_new(xkbComposeTable, XKB_COMPOSE_STATE_NO_FLAGS) : NULL;
}

static void
fcitx_im_context_finalize(GObject *obj)
{
    FcitxLog(DEBUG, "fcitx_im_context_finalize");
    FcitxIMContext *context = FCITX_IM_CONTEXT(obj);

    fcitx_im_context_set_client_window(GTK_IM_CONTEXT(context), NULL);

#ifndef g_signal_handlers_disconnect_by_data
#define g_signal_handlers_disconnect_by_data(instance, data) \
    g_signal_handlers_disconnect_matched ((instance), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (data))
#endif

    if (context->xkbComposeState) {
        xkb_compose_state_unref(context->xkbComposeState);
        context->xkbComposeState = NULL;
    }

    if (context->client) {
        g_signal_handlers_disconnect_by_data(context->client, context);
        g_object_unref(context->client);
        context->client = NULL;
    }

    if (context->slave) {
        g_signal_handlers_disconnect_by_data(context->slave, context);
        g_object_unref(context->slave);
        context->slave = NULL;
    }

    g_free(context->preedit_string);
    context->preedit_string = NULL;

    g_free(context->surrounding_text);
    context->surrounding_text = NULL;

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

static gboolean
fcitx_im_context_filter_keypress_fallback(FcitxIMContext *context, GdkEventKey *event)
{
    if (!context->xkbComposeState || event->type == GDK_KEY_RELEASE) {
        return gtk_im_context_filter_keypress(context->slave, event);;
    }

    struct xkb_compose_state* xkbComposeState = context->xkbComposeState;

    enum xkb_compose_feed_result result = xkb_compose_state_feed(xkbComposeState, event->keyval);
    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return gtk_im_context_filter_keypress(context->slave, event);
    }

    enum xkb_compose_status status = xkb_compose_state_get_status(xkbComposeState);
    if (status == XKB_COMPOSE_NOTHING) {
        return gtk_im_context_filter_keypress(context->slave, event);
    } else if (status == XKB_COMPOSE_COMPOSED) {
        char buffer[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        int length = xkb_compose_state_get_utf8(xkbComposeState, buffer, sizeof(buffer));
        xkb_compose_state_reset(xkbComposeState);
        if (length != 0) {
            g_signal_emit(context, _signal_commit_id, 0, buffer);
        }

    } else if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(xkbComposeState);
    }

    return TRUE;
}

///
static gboolean
fcitx_im_context_filter_keypress(GtkIMContext *context,
                                 GdkEventKey  *event)
{
    FcitxLog(DEBUG, "fcitx_im_context_filter_keypress");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    /* check this first, since we use key snooper, most key will be handled. */
    if (fcitx_client_is_valid(fcitxcontext->client) ) {
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
        return fcitx_im_context_filter_keypress_fallback(fcitxcontext, event);

    if (fcitx_client_is_valid(fcitxcontext->client) && fcitxcontext->has_focus) {
        _request_surrounding_text (&fcitxcontext);
        if (G_UNLIKELY(!fcitxcontext))
            return FALSE;

        fcitxcontext->time = event->time;

        if (_use_sync_mode) {
            int ret = fcitx_client_process_key_sync(fcitxcontext->client,
                                                    event->keyval,
                                                    event->hardware_keycode,
                                                    event->state,
                                                    (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                                    event->time);
            if (ret <= 0) {
                event->state |= FcitxKeyState_IgnoredMask;
                return fcitx_im_context_filter_keypress_fallback(fcitxcontext, event);
            } else {
                event->state |= FcitxKeyState_HandledMask;
                return TRUE;
            }
        } else {
            fcitx_client_process_key_async(fcitxcontext->client,
                                           event->keyval,
                                           event->hardware_keycode,
                                           event->state,
                                           (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                           event->time,
                                           -1,
                                           NULL,
                                           _fcitx_im_context_process_key_cb,
                                           gdk_event_copy((GdkEvent *) event)
                                          );
            event->state |= FcitxKeyState_HandledMask;
            return TRUE;
        }
    } else {
        return fcitx_im_context_filter_keypress_fallback(fcitxcontext, event);
    }
    return FALSE;
}

static void
_fcitx_im_context_process_key_cb (GObject *source_object,
                                  GAsyncResult *res,
                                  gpointer user_data)
{
    GdkEventKey* event = user_data;
    int ret = fcitx_client_process_key_finish(FCITX_CLIENT(source_object), res);
    if (ret <= 0) {
        event->state |= FcitxKeyState_IgnoredMask;
        gdk_event_put((GdkEvent *)event);
    }
    gdk_event_free((GdkEvent *)event);
}

static void
_fcitx_im_context_update_formatted_preedit_cb(FcitxClient* im, GPtrArray* array,
                                              int cursor_pos, void* user_data)
{
    FCITX_UNUSED(im);
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

    unsigned int i = 0;
    for (i = 0;i < array->len;i++) {
        size_t bytelen = strlen(gstr->str);
        FcitxPreeditItem *preedit = g_ptr_array_index(array, i);
        const gchar *s = preedit->string;
        gint type = preedit->type;

        PangoAttribute *pango_attr = NULL;
        if ((type & MSG_NOUNDERLINE) == 0) {
            pango_attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
            pango_attr->start_index = bytelen;
            pango_attr->end_index = bytelen + strlen(s);
            pango_attr_list_insert(context->attrlist, pango_attr);
        }

        if (type & MSG_HIGHLIGHT) {
            gboolean hasColor = false;
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
#if GTK_CHECK_VERSION (3, 0, 0)
                    GtkStyleContext* styleContext = gtk_widget_get_style_context(widget);
                    GdkRGBA* fg_rgba = NULL;
                    GdkRGBA* bg_rgba = NULL;
                    gtk_style_context_get(styleContext, GTK_STATE_FLAG_SELECTED,
                                          "background-color", &bg_rgba,
                                          "color", &fg_rgba,
                                          NULL
                                         );

                    fg.pixel = 0;
                    fg.red = CLAMP ((guint) (fg_rgba->red * 65535), 0, 65535);
                    fg.green = CLAMP ((guint) (fg_rgba->green * 65535), 0, 65535);
                    fg.blue = CLAMP ((guint) (fg_rgba->blue * 65535), 0, 65535);
                    bg.pixel = 0;
                    bg.red = CLAMP ((guint) (bg_rgba->red * 65535), 0, 65535);
                    bg.green = CLAMP ((guint) (bg_rgba->green * 65535), 0, 65535);
                    bg.blue = CLAMP ((guint) (bg_rgba->blue * 65535), 0, 65535);
                    gdk_rgba_free (fg_rgba);
                    gdk_rgba_free (bg_rgba);
#else
                    GtkStyle* style = gtk_widget_get_style(widget);
                    fg = style->text[GTK_STATE_SELECTED];
                    bg = style->bg[GTK_STATE_SELECTED];
#endif
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

    gchar *str = g_string_free(gstr, FALSE);

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

    /*
     * Do not call gtk_im_context_focus_out() here.
     * This might workaround some chrome issue
     */
#if 0
    if (_focus_im_context != NULL) {
        g_assert (_focus_im_context != context);
        gtk_im_context_focus_out (_focus_im_context);
        g_assert (_focus_im_context == NULL);
    }
#endif

    if (fcitx_client_is_valid(fcitxcontext->client)) {
        fcitx_client_focus_in(fcitxcontext->client);
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

#if 0
    g_assert (context == _focus_im_context);
#endif
    g_object_remove_weak_pointer ((GObject *) context,
                                  (gpointer *) &_focus_im_context);
    _focus_im_context = NULL;

    fcitxcontext->has_focus = false;

    if (fcitx_client_is_valid(fcitxcontext->client)) {
        fcitx_client_focus_out(fcitxcontext->client);
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

    if (fcitx_client_is_valid(fcitxcontext->client)) {
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
            !fcitx_client_is_valid(fcitxcontext->client)) {
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
        area.x += rootx;
        area.y += rooty;
    }
#endif
    int scale = 1;
#if GTK_CHECK_VERSION (3, 10, 0)
    scale = gdk_window_get_scale_factor(fcitxcontext->client_window);
#endif
    area.x *= scale;
    area.y *= scale;
    area.width *= scale;
    area.height *= scale;

    fcitx_client_set_cusor_rect(fcitxcontext->client, area.x, area.y, area.width, area.height);
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

    if (!GTK_IS_TEXT_VIEW (widget)) {
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

    if (fcitx_client_is_valid(fcitxcontext->client) &&
        !(fcitxcontext->last_updated_capacity & CAPACITY_PASSWORD)) {
        gint cursor_pos;
        guint utf8_len;
        gchar *p;

        p = g_strndup(text, len);
        cursor_pos = g_utf8_strlen(p, cursor_index);
        utf8_len = g_utf8_strlen(p, len);

        gint anchor_pos = get_selection_anchor_point(fcitxcontext,
                                                     cursor_pos,
                                                     utf8_len);
        if (g_strcmp0(fcitxcontext->surrounding_text, p) == 0) {
            g_free(p);
            p = NULL;
        } else {
            g_free(fcitxcontext->surrounding_text);
            fcitxcontext->surrounding_text = p;
        }

        if (p || fcitxcontext->last_cursor_pos != cursor_pos ||
            fcitxcontext->last_anchor_pos != anchor_pos) {
            fcitxcontext->last_cursor_pos = cursor_pos;
            fcitxcontext->last_anchor_pos = anchor_pos;
            fcitx_client_set_surrounding_text(fcitxcontext->client,
                                              p, cursor_pos, anchor_pos);
        }
    }
    gtk_im_context_set_surrounding(fcitxcontext->slave,
                                   text, l, cursor_index);
}

void
_fcitx_im_context_set_capacity(FcitxIMContext* fcitxcontext, gboolean force)
{
    if (fcitx_client_is_valid(fcitxcontext->client)) {
        FcitxCapacityFlags flags = fcitxcontext->capacity_from_toolkit;
        // toolkit hint always not have preedit / surrounding hint
        // no need to check them
        if (fcitxcontext->use_preedit) {
            flags |= CAPACITY_PREEDIT | CAPACITY_FORMATTED_PREEDIT;
        }
        if (fcitxcontext->support_surrounding_text) {
            flags |= CAPACITY_SURROUNDING_TEXT;
        }

        // always run this code against all gtk version
        // seems visibility != PASSWORD hint
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
        if (G_UNLIKELY(fcitxcontext->last_updated_capacity != flags)) {
            fcitxcontext->last_updated_capacity = flags;
            update = TRUE;
        }
        if (G_UNLIKELY(update || force))
            fcitx_client_set_capacity(fcitxcontext->client, fcitxcontext->last_updated_capacity);
    }
}

///
static void
fcitx_im_context_reset(GtkIMContext *context)
{
    FcitxLog(DEBUG, "fcitx_im_context_reset");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT(context);

    if (fcitx_client_is_valid(fcitxcontext->client)) {
        fcitx_client_reset(fcitxcontext->client);
    }

    if (fcitxcontext->xkbComposeState) {
        xkb_compose_state_reset(fcitxcontext->xkbComposeState);
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

    if (fcitx_client_is_valid(fcitxcontext->client)) {
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
    FCITX_UNUSED(slave);
    g_signal_emit(context, _signal_commit_id, 0, string);
}
static void
_slave_preedit_changed_cb(GtkIMContext *slave,
                          FcitxIMContext *context)
{
    FCITX_UNUSED(slave);
    if (context->client) {
        return;
    }

    g_signal_emit(context, _signal_preedit_changed_id, 0);
}
static void
_slave_preedit_start_cb(GtkIMContext *slave,
                        FcitxIMContext *context)
{
    FCITX_UNUSED(slave);
    if (context->client) {
        return;
    }

    g_signal_emit(context, _signal_preedit_start_id, 0);
}

static void
_slave_preedit_end_cb(GtkIMContext *slave,
                      FcitxIMContext *context)
{
    FCITX_UNUSED(slave);
    if (context->client) {
        return;
    }
    g_signal_emit(context, _signal_preedit_end_id, 0);
}

static gboolean
_slave_retrieve_surrounding_cb(GtkIMContext *slave,
                               FcitxIMContext *context)
{
    FCITX_UNUSED(slave);
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
    FCITX_UNUSED(slave);
    gboolean return_value;

    if (context->client) {
        return FALSE;
    }
    g_signal_emit(context, _signal_delete_surrounding_id, 0,
                  offset_from_cursor, nchars, &return_value);
    return return_value;
}

void _fcitx_im_context_enable_im_cb(FcitxClient* im, void* user_data)
{
    FCITX_UNUSED(im);
    FCITX_UNUSED(user_data);
}

void _fcitx_im_context_close_im_cb(FcitxClient* im, void* user_data)
{
    FCITX_UNUSED(im);
    FcitxLog(DEBUG, "_fcitx_im_context_close_im_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);

    if (context->preedit_string != NULL)
        g_free(context->preedit_string);
    context->preedit_string = NULL;
    context->cursor_pos = 0;
    g_signal_emit(context, _signal_preedit_changed_id, 0);
    g_signal_emit(context, _signal_preedit_end_id, 0);
}

void _fcitx_im_context_commit_string_cb(FcitxClient* im, char* str, void* user_data)
{
    FCITX_UNUSED(im);
    FcitxLog(DEBUG, "_fcitx_im_context_commit_string_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    g_signal_emit(context, _signal_commit_id, 0, str);
}

void _fcitx_im_context_forward_key_cb(FcitxClient* im, guint keyval, guint state, gint type, void* user_data)
{
    FCITX_UNUSED(im);
    FcitxLog(DEBUG, "_fcitx_im_context_forward_key_cb");
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    FcitxKeyEventType tp = (FcitxKeyEventType) type;
    GdkEventKey* event = _create_gdk_event(context, keyval, state, tp);
    event->state |= FcitxKeyState_IgnoredMask;
    gdk_event_put((GdkEvent *)event);
    gdk_event_free((GdkEvent *)event);
}

static void
_fcitx_im_context_delete_surrounding_text_cb (FcitxClient *im,
                                              gint offset_from_cursor,
                                              guint nchars,
                                              void *user_data)
{
    FCITX_UNUSED(im);
    FcitxIMContext *context =  FCITX_IM_CONTEXT(user_data);
    gboolean return_value;
    g_signal_emit(context, _signal_delete_surrounding_id, 0,
                  offset_from_cursor, nchars, &return_value);
}

/* Copy from gdk */
static GdkEventKey *
_create_gdk_event(FcitxIMContext *fcitxcontext, guint keyval, guint state,
                  FcitxKeyEventType type)
{
    gunichar c = 0;
    gchar buf[8];

    GdkEventKey *event = (GdkEventKey*)gdk_event_new((type == FCITX_RELEASE_KEY) ? GDK_KEY_RELEASE : GDK_KEY_PRESS);

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

void _fcitx_im_context_connect_cb(FcitxClient* im, void* user_data)
{
    FCITX_UNUSED(im);
    FcitxIMContext* context =  FCITX_IM_CONTEXT(user_data);
    _fcitx_im_context_set_capacity(context, TRUE);
    if (context->has_focus && _focus_im_context == (GtkIMContext*) context && fcitx_client_is_valid(context->client))
        fcitx_client_focus_in(context->client);
    /* set_cursor_location_internal() will get origin from X server,
     * it blocks UI. So delay it to idle callback. */
    gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE,
                              (GSourceFunc) _set_cursor_location_internal,
                              g_object_ref(context),
                              (GDestroyNotify) g_object_unref);
}


static void
_request_surrounding_text (FcitxIMContext **context)
{
    if (*context && fcitx_client_is_valid((*context)->client)) {
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
            (*context)->support_surrounding_text = TRUE;
            _fcitx_im_context_set_capacity (*context,
                                            FALSE);
        }
        else {
            (*context)->support_surrounding_text = FALSE;
            _fcitx_im_context_set_capacity (*context,
                                            FALSE);
        }
    }
}


static gint
_key_snooper_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    FCITX_UNUSED(widget);
    FCITX_UNUSED(user_data);
    gboolean retval = FALSE;

    FcitxIMContext *fcitxcontext = (FcitxIMContext*)_focus_im_context;

    if (G_UNLIKELY(!_use_key_snooper))
        return FALSE;

    if (fcitxcontext == NULL || !fcitxcontext->has_focus)
        return FALSE;

    if (G_UNLIKELY (event->state & FcitxKeyState_HandledMask))
        return TRUE;

    if (G_UNLIKELY (event->state & FcitxKeyState_IgnoredMask))
        return FALSE;

    do {
        if (!fcitx_client_is_valid(fcitxcontext->client)) {
            break;
        }

        _request_surrounding_text (&fcitxcontext);
        if (G_UNLIKELY(!fcitxcontext))
            return FALSE;
        fcitxcontext->time = event->time;

        if (_use_sync_mode) {

            int ret = fcitx_client_process_key_sync(fcitxcontext->client,
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
            fcitx_client_process_key_async(fcitxcontext->client,
                                           event->keyval,
                                           event->hardware_keycode,
                                           event->state,
                                           (event->type == GDK_KEY_PRESS) ? (FCITX_PRESS_KEY) : (FCITX_RELEASE_KEY),
                                           event->time,
                                           -1,
                                           NULL,
                                           _fcitx_im_context_process_key_cb,
                                           gdk_event_copy((GdkEvent *) event)
                                          );
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

#if GTK_CHECK_VERSION(3, 6, 0)

void
_fcitx_im_context_input_purpose_changed_cb(GObject* gobject, GParamSpec* pspec,
                                           gpointer user_data)
{
    FCITX_UNUSED(pspec);
    FCITX_UNUSED(user_data);
    FcitxIMContext* fcitxcontext = FCITX_IM_CONTEXT(gobject);

    GtkInputPurpose purpose;
    g_object_get(gobject, "input-purpose", &purpose, NULL);


    fcitxcontext->capacity_from_toolkit &= ~purpose_related_capacity;

#define CASE_PURPOSE(_PURPOSE, _CAPACITY) \
    case _PURPOSE: \
        fcitxcontext->capacity_from_toolkit |= _CAPACITY; \
        break;

    switch(purpose) {
        CASE_PURPOSE(GTK_INPUT_PURPOSE_ALPHA, CAPACITY_ALPHA)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_DIGITS, CAPACITY_DIGIT);
        CASE_PURPOSE(GTK_INPUT_PURPOSE_NUMBER, CAPACITY_NUMBER)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_PHONE, CAPACITY_DIALABLE)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_URL, CAPACITY_URL)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_EMAIL, CAPACITY_EMAIL)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_NAME, CAPACITY_NAME)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_PASSWORD, CAPACITY_PASSWORD)
        CASE_PURPOSE(GTK_INPUT_PURPOSE_PIN, CAPACITY_PASSWORD | CAPACITY_DIGIT)
        case GTK_INPUT_PURPOSE_FREE_FORM:
        default:
            break;
    }

    _fcitx_im_context_set_capacity(fcitxcontext, FALSE);
}

void
_fcitx_im_context_input_hints_changed_cb(GObject* gobject, GParamSpec *pspec,
                                         gpointer user_data)
{
    FCITX_UNUSED(pspec);
    FCITX_UNUSED(user_data);
    FcitxIMContext* fcitxcontext = FCITX_IM_CONTEXT(gobject);

    GtkInputHints hints;
    g_object_get(gobject, "input-hints", &hints, NULL);

    fcitxcontext->capacity_from_toolkit &= ~hints_related_capacity;

#define CHECK_HINTS(_HINTS, _CAPACITY) \
    if (hints & _HINTS) \
        fcitxcontext->capacity_from_toolkit |= _CAPACITY;

    CHECK_HINTS(GTK_INPUT_HINT_SPELLCHECK, CAPACITY_SPELLCHECK)
    CHECK_HINTS(GTK_INPUT_HINT_NO_SPELLCHECK, CAPACITY_NO_SPELLCHECK);
    CHECK_HINTS(GTK_INPUT_HINT_WORD_COMPLETION, CAPACITY_WORD_COMPLETION)
    CHECK_HINTS(GTK_INPUT_HINT_LOWERCASE, CAPACITY_LOWERCASE)
    CHECK_HINTS(GTK_INPUT_HINT_UPPERCASE_CHARS, CAPACITY_UPPERCASE)
    CHECK_HINTS(GTK_INPUT_HINT_UPPERCASE_WORDS, CAPACITY_UPPERCASE_WORDS)
    CHECK_HINTS(GTK_INPUT_HINT_UPPERCASE_SENTENCES, CAPACITY_UPPERCASE_SENTENCES)
    CHECK_HINTS(GTK_INPUT_HINT_INHIBIT_OSK, CAPACITY_NO_ON_SCREEN_KEYBOARD)

    _fcitx_im_context_set_capacity(fcitxcontext, FALSE);
}

#endif

// kate: indent-mode cstyle; replace-tabs on;
