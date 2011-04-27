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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef PYCONFIG_H
#define PYCONFIG_H
#include "fcitx-config/fcitx-config.h"
#include <utils/configfile.h>

struct MHPY;

typedef struct FcitxPinyinConfig
{
    GenericConfig gconfig;
    /* py config */
    char *strDefaultSP;
    boolean bFullPY;
    boolean bPYCreateAuto;
    boolean bPYSaveAutoAsPhrase;
    ADJUSTORDER baseOrder;
    ADJUSTORDER phraseOrder;
    ADJUSTORDER freqOrder;
    HOTKEYS hkPYAddFreq[2];
    HOTKEYS hkPYDelFreq[2];
    HOTKEYS hkPYDelUserPhr[2];
    char* strPYGetWordFromPhrase;
    char cPYYCDZ[3];
    struct MHPY *MHPY_C;
    struct MHPY *MHPY_S;
    boolean bMisstype;
} FcitxPinyinConfig;

CONFIG_BINDING_DECLARE(FcitxPinyinConfig);

extern FcitxPinyinConfig pyconfig;

#endif