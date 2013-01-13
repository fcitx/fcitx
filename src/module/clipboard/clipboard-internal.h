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
#include <stdint.h>
#include <fcitx/module.h>

#include "clipboard.h"

#define CLIPBOARD_MAX_LEN 16

typedef enum {
    CBCM_NONE,
    CBCM_ALT,
    CBCM_CTRL,
    CBCM_SHIFT,
    _CBCM_COUNT
} ClipboardChooseModifier;

typedef struct {
    FcitxGenericConfig gconfig;
    boolean save_history;
    int history_len;
    int cand_max_len;
    FcitxHotkey trigger_key[2];
    ClipboardChooseModifier choose_modifier;
    boolean use_primary;
    boolean ignore_blank;
} FcitxClipboardConfig;

typedef struct {
    uint32_t len;
    char *str;
} ClipboardSelectionStr;

typedef struct {
    FcitxInstance *owner;
    FcitxClipboardConfig config;
    boolean active;
    int cand_half_len;
    ClipboardSelectionStr primary;
    uint32_t clp_hist_len;
    ClipboardSelectionStr clp_hist_lst[CLIPBOARD_MAX_LEN];
// #ifdef ENABLE_X11
    FcitxAddon *x11;
    unsigned int x11_primary_notify_id;
    unsigned int x11_clipboard_notify_id;
// #endif
} FcitxClipboard;

void ClipboardSetPrimary(FcitxClipboard *clipboard,
                         uint32_t len, const char *str);
void ClipboardPushClipboard(FcitxClipboard *clipboard,
                            uint32_t len, const char *str);
CONFIG_BINDING_DECLARE(FcitxClipboardConfig);

#endif
