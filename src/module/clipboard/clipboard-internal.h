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
#ifndef _FCITX_MODULE_CLIPBOARD_INTERNAL_H
#define _FCITX_MODULE_CLIPBOARD_INTERNAL_H

#include "config.h"
#include "fcitx/instance.h"
#include "fcitx-config/fcitx-config.h"

#include "clipboard.h"

typedef struct {
    FcitxGenericConfig gconfig;
    boolean save_history;
} FcitxClipboardConfig;

typedef struct {
    FcitxInstance *owner;
    FcitxClipboardConfig config;
#ifdef ENABLE_X11
    unsigned int x11_primary_notify_id;
    unsigned int x11_clipboard_notify_id;
#endif
} FcitxClipboard;

#ifdef __cplusplus
extern "C" {
#endif
    CONFIG_BINDING_DECLARE(FcitxClipboardConfig);
#ifdef __cplusplus
}
#endif
#endif
