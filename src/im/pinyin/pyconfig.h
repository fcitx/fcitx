/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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

#ifndef PYCONFIG_H
#define PYCONFIG_H
#include "fcitx-config/fcitx-config.h"
#include "fcitx/configfile.h"
#include "sp.h"

struct MHPY;

typedef enum _ADJUSTORDER {
    AD_NO = 0,
    AD_FAST = 1,
    AD_FREQ = 2
} ADJUSTORDER;

typedef enum _SHUANGPINSCHEME {
    SP_ZIRANMA,
    SP_MS,
    SP_ZIGUANG,
    SP_ABC,
    SP_ZHONGWENZHIXING,
    SP_PINYINJIAJIA,
    SP_XIAOHE,
    SP_USERDEFINE,
} SHUANGPINSCHEME;

typedef struct _PYMappedSplitData PYMappedSplitData;

typedef struct _FcitxPinyinConfig {
    FcitxGenericConfig gconfig;

    SHUANGPINSCHEME spscheme;
    boolean bFullPY;
    boolean bPYCreateAuto;
    boolean bPYSaveAutoAsPhrase;
    boolean bFixCursorAtHead;
    boolean bUseVForQuickPhrase;
    ADJUSTORDER baseOrder;
    ADJUSTORDER phraseOrder;
    ADJUSTORDER freqOrder;
    FcitxHotkey hkPYAddFreq[2];
    FcitxHotkey hkPYDelFreq[2];
    FcitxHotkey hkPYDelUserPhr[2];
    char* strPYGetWordFromPhrase;
    struct _MHPY *MHPY_C;
    struct _MHPY *MHPY_S;
    boolean bMisstypeNGGN;
    struct _PYTABLE *PYTable;
    char cNonS;
    SP_C SPMap_C[31];
    SP_S SPMap_S[4];

    PYMappedSplitData* splitData;
} FcitxPinyinConfig;

CONFIG_BINDING_DECLARE(FcitxPinyinConfig);
boolean LoadPYConfig(FcitxPinyinConfig *pyconfig);
void SavePYConfig(FcitxPinyinConfig *pyconfig);

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
