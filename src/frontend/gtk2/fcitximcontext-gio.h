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

#ifndef __FCITX_IM_CONTEXT_H_
#define __FCITX_IM_CONTEXT_H_

#include <gtk/gtk.h>

/*
* Type macros.
*/
#define FCITX_TYPE_IM_CONTEXT \
    (fcitx_im_context_get_type ())
#define FCITX_IM_CONTEXT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FCITX_TYPE_IM_CONTEXT, FcitxIMContext))
#define FCITX_IM_CONTEXT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FCITX_TYPE_IM_CONTEXT, FcitxIMContextClass))
#define FCITX_IS_IM_CONTEXT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FCITX_TYPE_IM_CONTEXT))
#define FCITX_IS_IM_CONTEXT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FCITX_TYPE_IM_CONTEXT))
#define FCITX_IM_CONTEXT_GET_CLASS(obj) \
    (G_TYPE_CHECK_GET_CLASS ((obj), FCITX_TYPE_IM_CONTEXT, FcitxIMContextClass))

G_BEGIN_DECLS

typedef struct _FcitxIMContext FcitxIMContext;
typedef struct _FcitxIMContextClass FcitxIMContextClass;

GType fcitx_im_context_get_type(void);
FcitxIMContext *fcitx_im_context_new(void);
void fcitx_im_context_register_type(GTypeModule *type_module);

G_END_DECLS
#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
