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

#ifndef FCITX_KEYBOARD_RULES_H
#define FCITX_KEYBOARD_RULES_H

#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>

typedef struct _FcitxXkbRules {
    UT_array* layoutInfos;
    UT_array* modelInfos;
    UT_array* optionGroupInfos;
    char* version;
} FcitxXkbRules;

typedef struct _FcitxXkbRulesHandler {
    UT_array* path;
    FcitxXkbRules* rules;
    boolean fromExtra;
} FcitxXkbRulesHandler;

typedef struct _FcitxXkbLayoutInfo {
    UT_array* variantInfos;
    char* name;
    char* description;
    UT_array* languages;
} FcitxXkbLayoutInfo;

typedef struct _FcitxXkbVariantInfo {
    char* name;
    char* description;
    UT_array* languages;
} FcitxXkbVariantInfo;

typedef struct _FcitxXkbModelInfo {
    char* name;
    char* description;
    char* vendor;
} FcitxXkbModelInfo;

typedef struct _FcitxXkbOptionGroupInfo {
    UT_array* optionInfos;
    char* name;
    char* description;
    boolean exclusive;
} FcitxXkbOptionGroupInfo;

typedef struct _FcitxXkbOptionInfo {
    char* name;
    char* description;
} FcitxXkbOptionInfo;

FcitxXkbRules* FcitxXkbReadRules(const char* file);
void FcitxXkbRulesFree(FcitxXkbRules* rules);
char* FcitxXkbRulesToReadableString(FcitxXkbRules* rules);
boolean StringEndsWith(const char* str, const char* suffix);

#endif
