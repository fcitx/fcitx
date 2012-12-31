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

#define FCITX_USE_INTERNAL_PATH
#include "config.h"
#include <string.h>
#include <gtk/gtk.h>
#include <gtk/gtkimmodule.h>
#include "fcitx/fcitx.h"
#include "fcitximcontext.h"

static const GtkIMContextInfo fcitx_im_info = {
    "fcitx",
    "Fcitx (Flexible Input Method Framework)",
    "fcitx",
    LOCALEDIR,
    "ja:ko:zh:*"
};

static const GtkIMContextInfo *info_list[] = {
    &fcitx_im_info
};

FCITX_EXPORT_API G_MODULE_EXPORT const gchar*
g_module_check_init(GModule *module)
{
    FCITX_UNUSED(module);
    return glib_check_version(GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, 0);
}

FCITX_EXPORT_API
G_MODULE_EXPORT void
im_module_init(GTypeModule *type_module)
{
    /* make module resident */
    g_type_module_use(type_module);
    fcitx_im_context_register_type(type_module);
}

FCITX_EXPORT_API
G_MODULE_EXPORT void
im_module_exit(void)
{
}

FCITX_EXPORT_API
G_MODULE_EXPORT GtkIMContext *
im_module_create(const gchar *context_id)
{
    if (context_id != NULL && strcmp(context_id, "fcitx") == 0) {
        FcitxIMContext *context;
        context = fcitx_im_context_new();
        return (GtkIMContext *) context;
    }
    return NULL;
}

FCITX_EXPORT_API
G_MODULE_EXPORT void
im_module_list(const GtkIMContextInfo ***contexts,
               gint *n_contexts)
{
    *contexts = info_list;
    *n_contexts = G_N_ELEMENTS(info_list);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
