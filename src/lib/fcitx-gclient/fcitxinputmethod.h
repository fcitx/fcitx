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

G_BEGIN_DECLS

typedef struct _FcitxInputMethod        FcitxInputMethod;
typedef struct _FcitxInputMethodClass   FcitxInputMethodClass;

#define FCITX_TYPE_INPUT_METHOD         (fcitx_inputmethod_get_type ())
#define FCITX_INPUTMETHOD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), FCITX_TYPE_INPUT_METHOD, FcitxInputMethod))
#define FCITX_INPUTMETHOD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), FCITX_TYPE_INPUT_METHOD, FcitxInputMethodClass))
#define FCITX_INPUTMETHOD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), FCITX_TYPE_INPUT_METHOD, FcitxInputMethodClass))

struct _FcitxInputMethod {
    GDBusProxy parent_instance;
    void (*imlist_changed)(FcitxInputMethod* im);
};

struct _FcitxInputMethodClass {
    GDBusProxyClass parent_class;
};

typedef struct _FcitxIMItem {
    gchar* name;
    gchar* unique_name;
    gchar* langcode;
    gboolean enable;
} FcitxIMItem;

FcitxInputMethod* fcitx_inputmethod_new(GBusType             bus_type,
                                        GDBusProxyFlags      flags,
                                        int                  display_number,
                                        GCancellable        *cancellable,
                                        GError             **error);

GType        fcitx_inputmethod_get_type(void) G_GNUC_CONST;

GPtrArray*   fcitx_inputmethod_get_imlist(FcitxInputMethod* im);

void         fcitx_inputmethod_set_imlist(FcitxInputMethod* im, GPtrArray* array);

void         fcitx_inputmethod_exit(FcitxInputMethod* im);

void         fcitx_inputmethod_item_free(gpointer data);

G_END_DECLS

#endif
