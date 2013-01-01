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

#ifndef _FCITX_XKB_XKB_INTERNAL_H_
#define _FCITX_XKB_XKB_INTERNAL_H_

#include "rules.h"

#include <X11/Xlib.h>

typedef struct _FcitxXkbConfig {
    FcitxGenericConfig gconfig;
    boolean bOverrideSystemXKBSettings;
    boolean bIgnoreInputMethodLayoutRequest;
    char* xmodmapCommand;
    char* customXModmapScript;
} FcitxXkbConfig;

typedef struct _FcitxXkb
{
    Display* dpy;
    UT_array *defaultLayouts;
    UT_array *defaultModels;
    UT_array *defaultOptions;
    UT_array* defaultVariants;
    struct _FcitxInstance* owner;
    char *closeLayout;
    char *closeVariant;
    char *defaultXmodmapPath;
    FcitxXkbRules* rules;
    FcitxXkbConfig config;
    int xkbOpcode;
    struct _LayoutOverride* layoutOverride;
    unsigned long lastSerial;
} FcitxXkb;


CONFIG_BINDING_DECLARE(FcitxXkbConfig);

#endif
