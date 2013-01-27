/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#ifndef FCITX_KEYBOARD_H
#define FCITX_KEYBOARD_H

#include "config.h"

#include <iconv.h>

#include <fcitx/ime.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include "module/xkb/fcitx-xkb.h"

#define FCITX_KEYBOARD_MAX_BUFFER 20
#define FCITX_MAX_COMPOSE_LEN 7

typedef enum _ChooseModifier {
    CM_NONE,
    CM_ALT,
    CM_CTRL,
    CM_SHIFT,
    _CM_COUNT
} ChooseModifier;

typedef struct _FcitxKeyboardConfig {
    FcitxGenericConfig gconfig;
    boolean bCommitWithExtraSpace;
    boolean bUseEnterToCommit;
    FcitxHotkey hkToggleWordHint[2];
    FcitxHotkey hkAddToUserDict[2];
    int minimumHintLength;
    int maximumHintLength;
    ChooseModifier chooseModifier;
} FcitxKeyboardConfig;

typedef struct _FcitxKeyboard {
    struct _FcitxInstance* owner;
    char dictLang[6];
    FcitxKeyboardConfig config;
    FcitxXkbRules* rules;
    iconv_t iconv;
    char *initialLayout;
    char *initialVariant;
    char buffer[2][FCITX_KEYBOARD_MAX_BUFFER + UTF8_MAX_LENGTH + 1];
    int cursorPos;
    uint composeBuffer[FCITX_MAX_COMPOSE_LEN + 1];
    int n_compose;
    char *tempBuffer;
    int lastLength;
    int dataSlot;
    int enUSRegistered;
    boolean cursor_moved;
} FcitxKeyboard;

typedef struct _FcitxKeyboardLayout {
    FcitxKeyboard *owner;
    char *layoutString;
    char *variantString;
} FcitxKeyboardLayout;

CONFIG_BINDING_DECLARE(FcitxKeyboardConfig);

#endif
