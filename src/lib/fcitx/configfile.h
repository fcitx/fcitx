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
#ifndef _FCITX_CONFIGFILE_H_
#define _FCITX_CONFIGFILE_H_

#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/hotkey.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ADJUSTORDER {
    AD_NO = 0,
    AD_FAST = 1,
    AD_FREQ = 2
} ADJUSTORDER;

typedef enum _SWITCHKEY {
    S_R_CTRL = 0,
    S_R_SHIFT = 1,
    S_L_SHIFT = 2,
    S_L_CTRL = 3
} SWITCHKEY;

typedef enum _ENTER_TO_DO {
    K_ENTER_NOTHING = 0,
    K_ENTER_CLEAN = 1,
    K_ENTER_SEND = 2
} ENTER_TO_DO;

typedef enum _SEMICOLON_TO_DO {
    K_SEMICOLON_NOCHANGE = 0,
    K_SEMICOLON_ENG = 1,
    K_SEMICOLON_QUICKPHRASE = 2
} SEMICOLON_TO_DO;

typedef struct _FcitxConfig
{
    GenericConfig gconfig;
    /* program config */
    int iDelayStart;
    boolean bFirstRun;

    /* output config */
    boolean bEngPuncAfterNumber;
    ENTER_TO_DO enterToDo;
    boolean bDisablePagingInRemind;
    boolean bSendTextWhenSwitchEng;
    int iMaxCandWord;
    boolean bPhraseTips;

    /* appearance config */
    boolean bShowInputWindowTriggering;
    boolean bPointAfterNumber;
    boolean bShowUserSpeed;
    boolean bShowVersion;

    /* hotkey config */
    HOTKEYS hkTrigger[2];
    SWITCHKEY iSwitchKey;
    HOTKEYS switchKey[2];
    boolean bDoubleSwitchKey;
    int iTimeInterval;
    HOTKEYS hkVK[2];
    HOTKEYS hkRemind[2];
    HOTKEYS hkFullWidthChar[2];
    HOTKEYS hkPunc[2];
    HOTKEYS hkPrevPage[2];
    HOTKEYS hkNextPage[2];
    HOTKEYS str2nd3rdCand[2];
    HOTKEYS hkSaveAll[2];
    
    HOTKEYS i2ndSelectKey[2];
    HOTKEYS i3rdSelectKey[2];
} FcitxConfig;

void LoadConfig(FcitxConfig* fc);
void SaveConfig(FcitxConfig* fc);
int ConfigGetMaxCandWord(FcitxConfig* fc);
boolean ConfigGetPointAfterNumber(FcitxConfig* fc);

#ifdef __cplusplus
}
#endif

#endif
