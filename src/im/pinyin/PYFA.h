/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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
#ifndef _PYFA_H
#define _PYFA_H

#include "fcitx-config/fcitx-config.h"
struct _FcitxPinyinConfig;

/**
 * ...
 **/
typedef struct _MHPY_TEMPLATE {
    char           strMap[3];
} MHPY_TEMPLATE;

typedef struct _MHPY {
    char           strMap[3];
    boolean           bMode;
} MHPY;

typedef enum _PYTABLE_CONTROL {
    PYTABLE_NONE,
    PYTABLE_NG_GN,
    PYTABLE_V_U,
    PYTABLE_AN_ANG, // 0
    PYTABLE_EN_ENG, // 1
    PYTABLE_IAN_IANG, // 2
    PYTABLE_IN_ING, // 3
    PYTABLE_U_OU, // 4
    PYTABLE_UAN_UANG, // 5
    PYTABLE_C_CH, // 0
    PYTABLE_F_H, // 1
    PYTABLE_L_N, // 2
    PYTABLE_S_SH, // 3
    PYTABLE_Z_ZH, // 4
    PYTABLE_AN_ANG_S //5
} PYTABLE_CONTROL;

typedef struct _PYTABLE_TEMPLATE {
    char            strPY[7];
    PYTABLE_CONTROL control;
} PYTABLE_TEMPLATE;

typedef struct _PYTABLE {
    char            strPY[7];
    boolean            *pMH;
} PYTABLE;

int GetMHIndex_C2(MHPY* MHPY_C, char map1, char map2);
//在输入词组时，比如，当用户输入“jiu's”时，应该可以出现“就是”这个词，而无论是否打开了模糊拼音
int GetMHIndex_S2(MHPY* MHPY_S, char map1, char map2, boolean bMode);
boolean     IsZ_C_S(char map);
void InitMHPY(MHPY** pMHPY, const MHPY_TEMPLATE* MHPYtemplate);
void InitPYTable(struct _FcitxPinyinConfig* pyconfig);

extern const PYTABLE_TEMPLATE  PYTable_template[];
extern const MHPY_TEMPLATE  MHPY_C_TEMPLATE[];
extern const MHPY_TEMPLATE MHPY_S_TEMPLATE[];
#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
