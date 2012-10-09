/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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
#ifndef _FCITX_MODULE_FCITX_CLIPBOARD_H
#define _FCITX_MODULE_FCITX_CLIPBOARD_H

#include <fcitx/addon.h>
#include <fcitx/module.h>
#include <stdint.h>
#include <module/clipboard/clipboard.h>

static inline FcitxAddon*
FcitxClipboardGetAddon(FcitxInstance *instance)
{
    static FcitxAddon *addon = NULL;
    if (!addon) {
        addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance),
                                          "fcitx-clipboard");
    }
    return addon;
}

static inline const char*
FcitxClipboardGetPrimarySelection(FcitxInstance *instance,
                                  unsigned int *arg0)
{
    MODULE_ARGS(args, (void*)(intptr_t)arg0);
    return (const char*)(intptr_t)FcitxModuleInvokeFunction(
        FcitxClipboardGetAddon(instance), 0, args);
}

static inline const char*
FcitxClipboardGetClipboardHistory(FcitxInstance *instance,
                                  unsigned int arg0, unsigned int *arg1)
{
    MODULE_ARGS(args, (void*)(intptr_t)arg0, (void*)(intptr_t)arg1);
    return (const char*)(intptr_t)FcitxModuleInvokeFunction(
        FcitxClipboardGetAddon(instance), 1, args);
}

#endif
