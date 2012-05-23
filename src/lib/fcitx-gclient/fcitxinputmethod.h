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

#ifndef _FCITX_INPUTMETHOD_H_
#define _FCITX_INPUTMETHOD_H_

#include <gio/gio.h>

/*
 * Type macros
 */

/* define GOBJECT macros */
#define FCITX_TYPE_INPUT_METHOD          (fcitx_input_method_get_type ())
#define FCITX_INPUT_METHOD(object) \
        (G_TYPE_CHECK_INSTANCE_CAST ((object), FCITX_TYPE_INPUT_METHOD, FcitxInputMethod))
#define FCITX_IS_INPUT_METHOD(object) \
        (G_TYPE_CHECK_INSTANCE_TYPE ((object), KB_TYPE_BIBTEX))
#define FCITX_INPUT_METHOD_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), FCITX_TYPE_INPUT_METHOD, FcitxInputMethodClass))
#define FCITX_INPUT_METHOD_GET_CLASS(object)\
        (G_TYPE_INSTANCE_GET_CLASS ((object), FCITX_TYPE_INPUT_METHOD, FcitxInputMethodClass))

G_BEGIN_DECLS

typedef struct _FcitxInputMethod FcitxInputMethod;
typedef struct _FcitxInputMethodClass FcitxInputMethodClass;
typedef struct _FcitxIMItem FcitxIMItem;

/**
 * FcitxInputMethod:
 *
 * A FcitxInputMethod allow you to control fcitx via DBus.
 */
struct _FcitxInputMethod {
    GDBusProxy parent;
    /* instance members */
};

struct _FcitxInputMethodClass {
    GDBusProxyClass parent;
    /* signals */

    /*< private >*/
    /* padding */
};

struct _FcitxIMItem {
    gchar* name;
    gchar* unique_name;
    gchar* langcode;
    gboolean enable;
};

GType        fcitx_input_method_get_type(void) G_GNUC_CONST;

/**
 * fcitx_input_method_new
 * @bus_type: #GBusType
 * @flags:  #GDBusProxyFlags
 * @display_number: display_number
 * @cancellable: A #GCancellable or %NULL
 * @error: Error or %NULL
 *
 * @returns: A newly allocated FcitxInputMethod.
 *
 * New a FcitxInputMethod.
 */
FcitxInputMethod*
fcitx_input_method_new(GBusType             bus_type,
                       GDBusProxyFlags      flags,
                       gint                 display_number,
                       GCancellable        *cancellable,
                       GError             **error);

/**
 * fcitx_input_method_get_imlist:
 *
 * @im: A FcitxInputMethod
 * @returns: (transfer full) (element-type FcitxIMItem): An FcitxInputMethod List
 *
 * Get Fcitx all im list
 **/
GPtrArray*   fcitx_input_method_get_imlist(FcitxInputMethod* im);

/**
 * fcitx_input_method_set_imlist:
 *
 * @im: A FcitxInputMethod
 * @array: An FcitxInputMethod List
 *
 * Set Fcitx all im list
 **/
void         fcitx_input_method_set_imlist(FcitxInputMethod* im, GPtrArray* array);

/**
 * fcitx_input_method_exit:
 *
 * @im: A FcitxInputMethod
 *
 * Send exit command to fcitx
 **/
void         fcitx_input_method_exit(FcitxInputMethod* im);


/**
 * fcitx_im_item_free
 *
 * @data: A FcitxIMItem
 *
 * free an im_item
 **/
void fcitx_im_item_free(gpointer data);

G_END_DECLS

#endif
