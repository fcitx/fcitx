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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkwindow.h>
#include "fcitx/fcitx.h"
#include "fcitximcontext.h"
#include "fcitx-config/fcitx-config.h"
#include "client.h"
#include <fcitx-utils/log.h>

#define LOG_LEVEL INFO

#if !GTK_CHECK_VERSION (2, 91, 0)
# define DEPRECATED_GDK_KEYSYMS 1
#endif

struct _FcitxIMContext {
    GtkIMContext parent;

    GdkWindow *client_window;
    /* enable */
    boolean enable;
    GdkRectangle area;
    FcitxIMClient* client;
    GtkIMContext* slave;
    int has_focus;
};

struct _FcitxIMContextClass {
    GtkIMContextClass parent;
    /* klass members */
};

/* functions prototype */
static void     fcitx_im_context_class_init         (FcitxIMContextClass   *klass);
static void     fcitx_im_context_init               (FcitxIMContext        *im_context);
static void     fcitx_im_context_finalize           (GObject               *obj);
static void     fcitx_im_context_set_client_window  (GtkIMContext          *context,
        GdkWindow             *client_window);
static gboolean fcitx_im_context_filter_keypress    (GtkIMContext          *context,
        GdkEventKey           *key);
static void     fcitx_im_context_reset              (GtkIMContext          *context);
static void     fcitx_im_context_focus_in           (GtkIMContext          *context);
static void     fcitx_im_context_focus_out          (GtkIMContext          *context);
static void     fcitx_im_context_set_cursor_location (GtkIMContext          *context,
        GdkRectangle             *area);
static void     fcitx_im_context_set_use_preedit    (GtkIMContext          *context,
        gboolean               use_preedit);
static void     fcitx_im_context_get_preedit_string (GtkIMContext          *context,
        gchar                **str,
        PangoAttrList        **attrs,
        gint                  *cursor_pos);


static void
_set_cursor_location_internal (FcitxIMContext *fcitxcontext);
static void
_slave_commit_cb (GtkIMContext *slave,
                  gchar *string,
                  FcitxIMContext *context);
static void
_slave_preedit_changed_cb (GtkIMContext *slave,
                           FcitxIMContext *context);
static void
_slave_preedit_start_cb (GtkIMContext *slave,
                         FcitxIMContext *context);
static void
_slave_preedit_end_cb (GtkIMContext *slave,
                       FcitxIMContext *context);
static gboolean
_slave_retrieve_surrounding_cb (GtkIMContext *slave,
                                FcitxIMContext *context);
static gboolean
_slave_delete_surrounding_cb (GtkIMContext *slave,
                              gint offset_from_cursor,
                              guint nchars,
                              FcitxIMContext *context);

static GType _fcitx_type_im_context = 0;

static guint _signal_commit_id = 0;
static guint _signal_preedit_changed_id = 0;
static guint _signal_preedit_start_id = 0;
static guint _signal_preedit_end_id = 0;
static guint _signal_delete_surrounding_id = 0;
static guint _signal_retrieve_surrounding_id = 0;

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
fcitx_im_context_register_type (GTypeModule *type_module)
{
    static const GTypeInfo fcitx_im_context_info = {
        sizeof (FcitxIMContextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) fcitx_im_context_class_init,
        (GClassFinalizeFunc) NULL,
        NULL, /* klass data */
        sizeof (FcitxIMContext),
        0,
        (GInstanceInitFunc) fcitx_im_context_init,
    };

    if (!_fcitx_type_im_context) {
        if (type_module) {
            _fcitx_type_im_context =
                g_type_module_register_type (type_module,
                                             GTK_TYPE_IM_CONTEXT,
                                             "FcitxIMContext",
                                             &fcitx_im_context_info,
                                             (GTypeFlags)0);
        }
        else {
            _fcitx_type_im_context =
                g_type_register_static (GTK_TYPE_IM_CONTEXT,
                                        "FcitxIMContext",
                                        &fcitx_im_context_info,
                                        (GTypeFlags)0);
        }
    }
}

GType
fcitx_im_context_get_type (void)
{
    if (_fcitx_type_im_context == 0) {
        fcitx_im_context_register_type (NULL);
    }

    g_assert (_fcitx_type_im_context != 0);
    return _fcitx_type_im_context;
}

FcitxIMContext *
fcitx_im_context_new (void)
{
    GObject *obj = g_object_new (FCITX_TYPE_IM_CONTEXT, NULL);
    return FCITX_IM_CONTEXT (obj);
}

///
static void
fcitx_im_context_class_init (FcitxIMContextClass *klass)
{
    GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS (klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    im_context_class->set_client_window = fcitx_im_context_set_client_window;
    im_context_class->filter_keypress = fcitx_im_context_filter_keypress;
    im_context_class->reset = fcitx_im_context_reset;
    im_context_class->get_preedit_string = fcitx_im_context_get_preedit_string;
    im_context_class->focus_in = fcitx_im_context_focus_in;
    im_context_class->focus_out = fcitx_im_context_focus_out;
    im_context_class->set_cursor_location = fcitx_im_context_set_cursor_location;
    im_context_class->set_use_preedit = fcitx_im_context_set_use_preedit;
    gobject_class->finalize = fcitx_im_context_finalize;
    
    _signal_commit_id =
        g_signal_lookup ("commit", G_TYPE_FROM_CLASS (klass));
    g_assert (_signal_commit_id != 0);

    _signal_preedit_changed_id =
        g_signal_lookup ("preedit-changed", G_TYPE_FROM_CLASS (klass));
    g_assert (_signal_preedit_changed_id != 0);

    _signal_preedit_start_id =
        g_signal_lookup ("preedit-start", G_TYPE_FROM_CLASS (klass));
    g_assert (_signal_preedit_start_id != 0);

    _signal_preedit_end_id =
        g_signal_lookup ("preedit-end", G_TYPE_FROM_CLASS (klass));
    g_assert (_signal_preedit_end_id != 0);

    _signal_delete_surrounding_id =
        g_signal_lookup ("delete-surrounding", G_TYPE_FROM_CLASS (klass));
    g_assert (_signal_delete_surrounding_id != 0);

    _signal_retrieve_surrounding_id =
        g_signal_lookup ("retrieve-surrounding", G_TYPE_FROM_CLASS (klass));
    g_assert (_signal_retrieve_surrounding_id != 0);
}


static void
fcitx_im_context_init (FcitxIMContext *context)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_init");
    context->client = NULL;
    context->area.x = -1;
    context->area.y = -1;
    context->area.width = 0;
    context->area.height = 0;
    
    context->slave = gtk_im_context_simple_new ();
    gtk_im_context_simple_add_table (GTK_IM_CONTEXT_SIMPLE (context->slave),
                                     cedilla_compose_seqs,
                                     4,
                                     G_N_ELEMENTS (cedilla_compose_seqs) / (4 + 2));

    g_signal_connect (context->slave,
                      "commit",
                      G_CALLBACK (_slave_commit_cb),
                      context);
    g_signal_connect (context->slave,
                      "preedit-start",
                      G_CALLBACK (_slave_preedit_start_cb),
                      context);
    g_signal_connect (context->slave,
                      "preedit-end",
                      G_CALLBACK (_slave_preedit_end_cb),
                      context);
    g_signal_connect (context->slave,
                      "preedit-changed",
                      G_CALLBACK (_slave_preedit_changed_cb),
                      context);
    g_signal_connect (context->slave,
                      "retrieve-surrounding",
                      G_CALLBACK (_slave_retrieve_surrounding_cb),
                      context);
    g_signal_connect (context->slave,
                      "delete-surrounding",
                      G_CALLBACK (_slave_delete_surrounding_cb),
                      context);
    
    context->client = FcitxIMClientOpen();
}

static void
fcitx_im_context_finalize (GObject *obj)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_finalize");
    FcitxIMContext *context = FCITX_IM_CONTEXT (obj);

    if (IsFcitxIMClientValid(context->client)) {
        FcitxIMClientClose(context->client);
        context->client = NULL;
    }
    
    if (context->slave)
    {
        g_object_unref(context->slave);
        context->slave = NULL;
    }
}


static void
set_ic_client_window (FcitxIMContext *context,
                      GdkWindow       *client_window)
{
    if (!client_window)
        return;

    if (context->client_window) {
        g_object_unref (context->client_window);
        context->client_window = NULL;
    }

    if (client_window != NULL)
        context->client_window = g_object_ref (client_window);

    if (context->slave)
        gtk_im_context_set_client_window (context->slave, client_window);
}


///
static void
fcitx_im_context_set_client_window (GtkIMContext          *context,
                                    GdkWindow             *client_window)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_set_client_window");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    set_ic_client_window (fcitxcontext, client_window);
}

///
static gboolean
fcitx_im_context_filter_keypress (GtkIMContext *context,
                                  GdkEventKey  *event)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_filter_keypress");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    if (IsFcitxIMClientValid(fcitxcontext->client))
    {
        int ret = FcitxIMClientProcessKey(fcitxcontext->client,
                                          event->keyval,
                                          event->hardware_keycode,
                                          event->state,
                                          (event->type == GDK_KEY_PRESS)?(FCITX_PRESS_KEY):(FCITX_RELEASE_KEY),
                                          event->time);
        if (ret <= 0)
        {
            gtk_im_context_filter_keypress(fcitxcontext->slave, event);
        }
    }
    else
    {
        gtk_im_context_filter_keypress(fcitxcontext->slave, event);
    }
    return FALSE;
}

///
static void
fcitx_im_context_focus_in (GtkIMContext *context)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_focus_in");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    
    if (fcitxcontext->has_focus)
        return;
    
    fcitxcontext->has_focus = true;
    
    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxIMClientFocusIn(fcitxcontext->client);
    }
    
    gtk_im_context_focus_in (fcitxcontext->slave);

    return;
}

static void
fcitx_im_context_focus_out (GtkIMContext *context)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_focus_out");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    
    if (!fcitxcontext->has_focus)
    {
        return;
    }
    
    fcitxcontext->has_focus = false;

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxIMClientFocusOut(fcitxcontext->client);
    }
    
    gtk_im_context_focus_out(fcitxcontext->slave);

    return;
}

///
static void
fcitx_im_context_set_cursor_location (GtkIMContext *context,
                                      GdkRectangle *area)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_set_cursor_location");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    
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

static void
_set_cursor_location_internal (FcitxIMContext *fcitxcontext)
{
    GdkRectangle area;

    if(fcitxcontext->client_window == NULL ||
       !IsFcitxIMClientValid(fcitxcontext->client)) {
        return;
    }

    area = fcitxcontext->area;
    if (area.x == -1 && area.y == -1 && area.width == 0 && area.height == 0) {
#if GTK_CHECK_VERSION (2, 91, 0)
        area.x = 0;
        area.y += gdk_window_get_height (fcitxcontext->client_window);
#else
        gint w, h;
        gdk_drawable_get_size (fcitxcontext->client_window, &w, &h);
        area.y += h;
        area.x = 0;
#endif
    }

    gdk_window_get_root_coords (fcitxcontext->client_window,
                                area.x, area.y,
                                &area.x, &area.y);
    
    FcitxIMClientSetCursorLocation(fcitxcontext->client, area.x, area.y + area.height);
    return;
}

///
static void
fcitx_im_context_set_use_preedit (GtkIMContext *context,
                                  gboolean      use_preedit)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_set_use_preedit");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    if (!IsFcitxIMClientValid(fcitxcontext->client))
    {
    }
    
    gtk_im_context_set_use_preedit(fcitxcontext->slave, use_preedit);
}


///
static void
fcitx_im_context_reset (GtkIMContext *context)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_reset");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);

    if (IsFcitxIMClientValid(fcitxcontext->client)) {
        FcitxIMClientReset(fcitxcontext->client);
    }
    
    gtk_im_context_reset(fcitxcontext->slave);
}

static void
fcitx_im_context_get_preedit_string (GtkIMContext   *context,
                                     gchar         **str,
                                     PangoAttrList **attrs,
                                     gint           *cursor_pos)
{
    FcitxLog(LOG_LEVEL, "fcitx_im_context_get_preedit_string");
    FcitxIMContext *fcitxcontext = FCITX_IM_CONTEXT (context);
    if (fcitxcontext->enable)
    {
    }
    else
    {
        gtk_im_context_get_preedit_string (fcitxcontext->slave, str, attrs, cursor_pos);
    }
    return ;
}

/* Callback functions for slave context */
static void
_slave_commit_cb (GtkIMContext *slave,
                  gchar *string,
                  FcitxIMContext *context)
{
    g_signal_emit (context, _signal_commit_id, 0, string);
}
static void
_slave_preedit_changed_cb (GtkIMContext *slave,
                           FcitxIMContext *context)
{
    if (context->enable && context->client) {
        return;
    }

    g_signal_emit (context, _signal_preedit_changed_id, 0);
}
static void
_slave_preedit_start_cb (GtkIMContext *slave,
                         FcitxIMContext *context)
{
    if (context->enable && context->client) {
        return;
    }

    g_signal_emit (context, _signal_preedit_start_id, 0);
}

static void
_slave_preedit_end_cb (GtkIMContext *slave,
                       FcitxIMContext *context)
{
    if (context->enable && context->client) {
        return;
    }
    g_signal_emit (context, _signal_preedit_end_id, 0);
}

static gboolean
_slave_retrieve_surrounding_cb (GtkIMContext *slave,
                                FcitxIMContext *context)
{
    gboolean return_value;

    if (context->enable && context->client) {
        return FALSE;
    }
    g_signal_emit (context, _signal_retrieve_surrounding_id, 0,
                   &return_value);
    return return_value;
}

static gboolean
_slave_delete_surrounding_cb (GtkIMContext *slave,
                              gint offset_from_cursor,
                              guint nchars,
                              FcitxIMContext *context)
{
    gboolean return_value;

    if (context->enable && context->client) {
        return FALSE;
    }
    g_signal_emit (context, _signal_delete_surrounding_id, 0, offset_from_cursor, nchars, &return_value);
    return return_value;
}