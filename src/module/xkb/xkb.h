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

#ifndef FCITX_XKB_H
#define FCITX_XKB_H

#include <X11/Xlib.h>

#include "fcitx/instance.h"
#include "rules.h"

typedef struct _FcitxXkbConfig {
    FcitxGenericConfig gconfig;
    boolean bOverrideSystemXKBSettings;
    boolean bIgnoreInputMethodLayoutRequest;
} FcitxXkbConfig;

typedef struct _FcitxXkb
{
    Display* dpy;
    UT_array *defaultLayouts;
    UT_array *defaultModels;
    UT_array *defaultOptions;
    UT_array* defaultVariants;
    FcitxInstance* owner;
    int closeGroup;
    FcitxXkbRules* rules;
    FcitxXkbConfig config;
    int xkbOpcode;
} FcitxXkb;

#define FCITX_XKB_NAME "fcitx-xkb"
#define FCITX_XKB_GETRULES 0
#define FCITX_XKB_GETRULES_RETURNTYPE FcitxXkbRules*
#define FCITX_XKB_GETCURRENTLAYOUT 1
#define FCITX_XKB_GETCURRENTLAYOUT_RETURNTYPE void
#define FCITX_XKB_LAYOUTEXISTS 2
#define FCITX_XKB_LAYOUTEXISTS_RETURNTYPE void

CONFIG_BINDING_DECLARE(FcitxXkbConfig);

#endif
