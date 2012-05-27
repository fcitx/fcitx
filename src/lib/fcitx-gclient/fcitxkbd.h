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

#ifndef _FCITX_KBD_H_
#define _FCITX_KBD_H_

#include <gio/gio.h>

/*
 * Type macros
 */

/* define GOBJECT macros */
#define FCITX_TYPE_KBD          (fcitx_kbd_get_type ())
#define FCITX_KBD(object) \
        (G_TYPE_CHECK_INSTANCE_CAST ((object), FCITX_TYPE_KBD, FcitxKbd))
#define FCITX_IS_KBD(object) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((object), FCITX_TYPE_KBD))
#define FCITX_KBD_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), FCITX_TYPE_KBD, FcitxKbdClass))
#define FCITX_KBD_GET_CLASS(object)\
        (G_TYPE_INSTANCE_GET_CLASS ((object), FCITX_TYPE_KBD, FcitxKbdClass))

G_BEGIN_DECLS

typedef struct _FcitxKbd FcitxKbd;
typedef struct _FcitxKbdClass FcitxKbdClass;
typedef struct _FcitxLayoutItem FcitxLayoutItem;

/**
 * FcitxKbd:
 *
 * A FcitxKbd allow you to control fcitx via DBus.
 */
struct _FcitxKbd {
    GDBusProxy parent;
    /* instance members */
};

struct _FcitxKbdClass {
    GDBusProxyClass parent;
    /* signals */

    /*< private >*/
    /* padding */
};

struct _FcitxLayoutItem {
    gchar* layout;
    gchar* variant;
    gchar* name;
    gchar* langcode;
};

GType        fcitx_kbd_get_type(void) G_GNUC_CONST;

/**
 * fcitx_kbd_new
 * @bus_type: #GBusType
 * @flags:  #GDBusProxyFlags
 * @display_number: display_number
 * @cancellable: A #GCancellable or %NULL
 * @error: Error or %NULL
 *
 * @returns: A newly allocated FcitxKbd.
 *
 * New a FcitxKbd.
 */
FcitxKbd*
fcitx_kbd_new(GBusType             bus_type,
              GDBusProxyFlags      flags,
              gint                 display_number,
              GCancellable        *cancellable,
              GError             **error);

/**
 * fcitx_kbd_get_layouts:
 *
 * @kbd: A FcitxKbd
 * @returns: (transfer full) (element-type FcitxLayoutItem): A FcitxLayoutItem List
 *
 * Get Fcitx all im list
 **/
GPtrArray*   fcitx_kbd_get_layouts(FcitxKbd* kbd);

/**
 * fcitx_kbd_get_layout_for_im:
 *
 * @kbd: A FcitxKbd
 * @imname: input method name
 * @layout: return'd layout
 * @variant: return'd variant
 *
 * Get a layout binding with input method
 **/
void        fcitx_kbd_get_layout_for_im(FcitxKbd* kbd, const gchar* imname, gchar** layout, gchar** variant);

/**
 * fcitx_kbd_get_layout_for_im:
 *
 * @kbd: A FcitxKbd
 * @imname: input method name
 *
 * Get a layout binding with input method
 **/
void         fcitx_kbd_set_layout_for_im(FcitxKbd* kbd, const gchar* imname, const gchar* layout, const gchar* variant);


/**
 * fcitx_layout_item_free
 *
 * @data: A FcitxLayoutItem
 *
 * free an im_item
 **/
void fcitx_layout_item_free(gpointer data);

G_END_DECLS

#endif
