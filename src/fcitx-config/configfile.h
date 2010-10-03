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
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <X11/Xlib.h>
#include "fcitx-config/fcitx-config.h"
#define INPUT_METHODS	5	//标示输入法的类别数量

#ifndef MHPY_DEFINED
#define MHPY_DEFINED
typedef struct MH_PY MHPY;
#endif

typedef enum _HIDE_MAINWINDOW {
    HM_SHOW = 0,
    HM_AUTO = 1,
    HM_HIDE = 2
} HIDE_MAINWINDOW;

typedef enum ADJUSTORDER {
    AD_NO = 0,
    AD_FAST = 1,
    AD_FREQ = 2
} ADJUSTORDER;

typedef enum SWITCHKEY {
    S_R_CTRL = 0,
    S_R_SHIFT = 1,
    S_L_SHIFT = 2,
    S_R_SUPER = 3,
    S_L_SUPER = 4,
    S_L_CTRL = 5
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

typedef struct FcitxConfig
{
    GenericConfig gconfig;
    /* program config */
    char *fontEn;
    char *fontZh;
    char *strUserLocale;
    char *strRecordingPath;
    Bool bUseTrayIcon;
    Bool bUseTrayIcon_;
    Bool bUseDBus;
    Bool bUseDBus_;
    Bool bEnableAddons;
    /* output config */
    Bool bEngPuncAfterNumber;
    ENTER_TO_DO enterToDo;
    SEMICOLON_TO_DO semicolonToDo;
    Bool bEngAfterCap;
    Bool bConvertPunc;
    Bool bDisablePagingInLegend;
    Bool bSendTextWhenSwitchEng;

    /* appearance config */
    int iMaxCandWord;
    HIDE_MAINWINDOW hideMainWindow;
    Bool bCenterInputWindow;
    Bool bShowInputWindowTriggering;
    Bool bPointAfterNumber;
    Bool bShowUserSpeed;
    Bool bShowVersion;
    Bool bHintWindow;
    char* skinType;

    /* hotkey config */
    HOTKEYS hkTrigger[2];
    SWITCHKEY iSwitchKey;
    KEY_CODE switchKey;
    Bool bDoubleSwitchKey;
    int iTimeInterval;
    HOTKEYS hkTrack[2];
    HOTKEYS hkHideMainWindow[2];
    HOTKEYS hkVK[2];
    HOTKEYS hkGBT[2];
    HOTKEYS hkLegend[2];
    HOTKEYS hkGetPY[2];
    HOTKEYS hkCorner[2];
    HOTKEYS hkPunc[2];
    HOTKEYS hkPrevPage[2];
    HOTKEYS hkNextPage[2];
    HOTKEYS str2nd3rdCand[2];
    HOTKEYS hkSaveAll[2];
    HOTKEYS hkRecording[2];
    HOTKEYS hkResetRecording[2];

    /* im config */
    int inputMethods[INPUT_METHODS];
    char *strDefaultSP;
    Bool bPhraseTips;

    /* py config */
    Bool bFullPY;
    Bool bPYCreateAuto;
    Bool bPYSaveAutoAsPhrase;
    HOTKEYS hkPYAddFreq[2];
    HOTKEYS hkPYDelFreq[2];
    HOTKEYS hkPYDelUserPhr[2];
    char* strPYGetWordFromPhrase;
    ADJUSTORDER baseOrder;
    ADJUSTORDER phraseOrder;
    ADJUSTORDER freqOrder;
    MHPY *MHPY_C;
    MHPY *MHPY_S;
    int i2ndSelectKey;
    int i3rdSelectKey;
    char cPYYCDZ[3];
} FcitxConfig;

#ifdef _FCITX_H_
extern FcitxConfig fc;
void LoadConfig();
void SaveConfig();
#endif

#endif
