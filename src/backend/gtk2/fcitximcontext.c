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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkwindow.h>
#include "fcitx/fcitx.h"
#include "fcitximcontext.h"
#include "fcitx-config/fcitx-config.h"
#include "client.h"
#include <stdlib.h>

#if !GTK_CHECK_VERSION (2, 91, 0)
# define DEPRECATED_GDK_KEYSYMS 1
#endif

struct _FcitxIMContext {
    GtkIMContext parent;

    GdkWindow *client_window;
    /* enabled */
    boolean enabled;
    gint cursor_x;
    gint cursor_y;
    int timeout_handle;
    int pe_attN;
    char* pe_att;
    char* pe_str;
    int pe_cursor;
    int pe_started;
    int fcitx_client;
    FcitxIMClient* client;
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

static GType _fcitx_type_im_context = 0;

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

static void
get_im (FcitxIMContext *context_xim)
{
    GdkWindow *client_window = context_xim->client_window;
    if (!context_xim->client) {
        if (!(context_xim->client = FcitxIMClientOpen()))
            perror("cannot open client");
        context_xim->timeout_handle = 0;
        context_xim->pe_attN = 0;
        context_xim->pe_att = NULL;
        context_xim->pe_str = NULL;
        context_xim->pe_cursor = 0;
        context_xim->pe_started = FALSE;

    }
}

///
static void
fcitx_im_context_class_init (FcitxIMContextClass *klass)
{
    GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS (klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

//  parent_class = g_type_class_peek_parent (class);
    im_context_class->set_client_window = fcitx_im_context_set_client_window;
    im_context_class->filter_keypress = fcitx_im_context_filter_keypress;
    im_context_class->reset = fcitx_im_context_reset;
    im_context_class->get_preedit_string = fcitx_im_context_get_preedit_string;
    im_context_class->focus_in = fcitx_im_context_focus_in;
    im_context_class->focus_out = fcitx_im_context_focus_out;
    im_context_class->set_cursor_location = fcitx_im_context_set_cursor_location;
    im_context_class->set_use_preedit = fcitx_im_context_set_use_preedit;
    gobject_class->finalize = fcitx_im_context_finalize;
}


static void
fcitx_im_context_init (FcitxIMContext *im_context)
{
    im_context->timeout_handle = 0;
    im_context->pe_attN = 0;
    im_context->pe_att = NULL;
    im_context->pe_str = NULL;
    im_context->pe_cursor = 0;
    im_context->client = NULL;
}


void clear_preedit(FcitxIMContext *im_context)
{
    if (!im_context)
        return;

    if (im_context->pe_str) {
        free(im_context->pe_str);
        im_context->pe_str = NULL;
    }

    if (im_context->pe_att) {
        free(im_context->pe_att);
        im_context->pe_att = NULL;
        im_context->pe_attN = 0;
    }

    im_context->pe_cursor = 0;
}


static void
fcitx_im_context_finalize (GObject *obj)
{
    FcitxIMContext *context_xim = FCITX_IM_CONTEXT (obj);
    clear_preedit(context_xim);

    if (context_xim->client) {
        FcitxIMClientClose(context_xim->client);
        context_xim->client = NULL;
    }
}


static void
set_ic_client_window (FcitxIMContext *context_xim,
                      GdkWindow       *client_window)
{
    if (!client_window)
        return;

    context_xim->client_window = client_window;

    if (context_xim->client_window) {
        get_im (context_xim);
        if (context_xim->client) {
            /* TODO */
        }
    }
}


///
static void
fcitx_im_context_set_client_window (GtkIMContext          *context,
                                    GdkWindow             *client_window)
{
    FcitxIMContext *context_xim = FCITX_IM_CONTEXT (context);
    set_ic_client_window (context_xim, client_window);
}

///
static gboolean
fcitx_im_context_filter_keypress (GtkIMContext *context,
                                  GdkEventKey  *event)
{
    FcitxIMContext *context_xim = FCITX_IM_CONTEXT (context);
    return FALSE;
}

///
static void
fcitx_im_context_focus_in (GtkIMContext *context)
{
    FcitxIMContext *context_xim = FCITX_IM_CONTEXT (context);
    if (context_xim->client) {
        FcitxIMClientFoucsIn(context_xim->client);
    }

    return;
}

static void
fcitx_im_context_focus_out (GtkIMContext *context)
{
    FcitxIMContext *context_xim = FCITX_IM_CONTEXT (context);
//  printf("fcitx_im_context_focus_out\n");

    if (context_xim->client) {
        FcitxIMClientFoucsOut(context_xim->client);
    }

    return;
}

///
static void
fcitx_im_context_set_cursor_location (GtkIMContext *context,
                                      GdkRectangle *area)
{
#if DBG
    printf("fcitx_im_context_set_cursor_location %d\n", area->x);
#endif
    FcitxIMContext *context_xim = FCITX_IM_CONTEXT (context);

    if (context_xim->client) {
        FcitxIMClientSetCursorLocation(context_xim->client, area->x, area->y + area->height);
    }

    return;
}

///
static void
fcitx_im_context_set_use_preedit (GtkIMContext *context,
                                  gboolean      use_preedit)
{
    FcitxIMContext *im_context = FCITX_IM_CONTEXT (context);
    if (!im_context->client)
        return;
    int ret;
}


///
static void
fcitx_im_context_reset (GtkIMContext *context)
{
    FcitxIMContext *im_context = FCITX_IM_CONTEXT (context);

    im_context->pe_started = FALSE;
    if (im_context->client) {
        FcitxIMClientReset(im_context->client);
        if (im_context->pe_str && im_context->pe_str[0]) {
            clear_preedit(im_context);
            g_signal_emit_by_name(context, "preedit_changed");
        }
    }
}

static void
fcitx_im_context_get_preedit_string (GtkIMContext   *context,
                                     gchar         **str,
                                     PangoAttrList **attrs,
                                     gint           *cursor_pos)
{
    FcitxIMContext *im_context = FCITX_IM_CONTEXT (context);
}


/**
 * fcitx_im_context_shutdown:
 *
 * Destroys all the status windows that are kept by the Fcitx contexts.  This
 * function should only be called by the Fcitx module exit routine.
 **/
void
fcitx_im_context_shutdown (void)
{
}
