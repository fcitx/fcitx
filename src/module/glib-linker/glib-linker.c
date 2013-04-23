/***************************************************************************
 *   Copyright (C) 2013~2013 by CS Slayer                                  *
 *   wengxt@gmail.com                                                      *
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
#include "fcitx/instance.h"
#include "fcitx/module.h"

#include <glib-object.h>

static void* GLibLinkerCreate(FcitxInstance* instance);
static void GlibLinkerDestroy(void* arg);

FCITX_DEFINE_PLUGIN(fcitx_glib_linker, module, FcitxModule) = {
    GLibLinkerCreate,
    NULL,
    NULL,
    GlibLinkerDestroy,
    NULL
};

void* GLibLinkerCreate(FcitxInstance* instance)
{
#if !GLIB_CHECK_VERSION(2, 35, 3)
    g_type_init();
#endif
    return fcitx_utils_new(int);
}

void GlibLinkerDestroy(void* arg) {
    free(arg);
}