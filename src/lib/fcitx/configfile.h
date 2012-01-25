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
#ifndef _FCITX_CONFIGFILE_H_
#define _FCITX_CONFIGFILE_H_

#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/frontend.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum _FcitxSwitchKey {
        SWITCHKEY_R_CTRL = 0,
        SWITCHKEY_R_SHIFT = 1,
        SWITCHKEY_L_SHIFT = 2,
        SWITCHKEY_L_CTRL = 3,
        SWITCHKEY_ALT_L_SHIFT = 4,
        SWITCHKEY_ALT_R_SHIFT = 5,
        SWITCHKEY_None = 6
    } FcitxSwitchKey;

    typedef enum _FcitxEnterAcion {
        K_ENTER_NOTHING = 0,
        K_ENTER_CLEAN = 1,
        K_ENTER_SEND = 2
    } FcitxEnterAcion;

    typedef enum _FcitxShareState {
        ShareState_None = 0,
        ShareState_All = 1,
        ShareState_PerProgram = 2
    } FcitxShareState;

    /**
     * @brief struct opposite to ~/.config/fcitx/config
     **/
    typedef struct _FcitxConfig {
        /**
         * @brief derives FcitxGenericConfig
         **/
        FcitxGenericConfig gconfig;
        /* program config */
        /**
         * @brief delay start seconds
         **/
        int iDelayStart;
        /**
         * @brief is the first run
         **/
        boolean bFirstRun;

        /* output config */
        /**
         * @brief input eng punc after input number
         **/
        boolean bEngPuncAfterNumber;
        /**
         * @brief enter key action
         **/
        FcitxEnterAcion enterToDo;
        /**
         * @brief Remind mode can has multipage
         **/
        boolean bDisablePagingInRemind;
        /**
         * @brief switch to english with switch key commit string or not
         **/
        boolean bSendTextWhenSwitchEng;
        /**
         * @brief max candidate word number
         **/
        int iMaxCandWord;
        /**
         * @brief phrase tips
         **/
        boolean bPhraseTips;

        /* appearance config */
        /**
         * @brief show input window after trigger on
         **/
        boolean bShowInputWindowTriggering;
        /**
         * @brief index number follow with a '.'
         **/
        boolean bPointAfterNumber;
        /**
         * @brief show user input speed
         **/
        boolean bShowUserSpeed;
        /**
         * @brief show fcitx version
         **/
        boolean bShowVersion;

        /* hotkey config */
        /**
         * @brief trigger key
         **/
        FcitxHotkey hkTrigger[2];
        /**
         * @brief switch key
         **/
        FcitxSwitchKey iSwitchKey;
        /**
         * @brief hotkey format of switch key
         **/
        FcitxHotkey switchKey[2];
        /**
         * @brief enable double press switch action
         **/
        boolean bDoubleSwitchKey;
        /**
         * @brief key hit interval
         **/
        int iTimeInterval;
        /**
         * @brief hotkey for switch VK
         **/
        FcitxHotkey hkVK[2];
        /**
         * @brief hotkey for switch remind mode
         **/
        FcitxHotkey hkRemind[2];
        /**
         * @brief hotkey for switch full width char
         **/
        FcitxHotkey hkFullWidthChar[2];
        /**
         * @brief hotkey for switch punc
         **/
        FcitxHotkey hkPunc[2];
        /**
         * @brief prev page
         **/
        FcitxHotkey hkPrevPage[2];
        /**
         * @brief next page
         **/
        FcitxHotkey hkNextPage[2];
        /**
         * @brief 2nd 3rd candidate select key
         **/
        FcitxHotkey str2nd3rdCand[2];
        /**
         * @brief save all key
         **/
        FcitxHotkey hkSaveAll[2];

        /**
         * @brief hotkey format for 2nd select key
         **/
        FcitxHotkey i2ndSelectKey[2];
        /**
         * @brief hotkey format for 3rd select key
         **/
        FcitxHotkey i3rdSelectKey[2];

        /**
         * @brief hide input window when there is only preedit string
         **/
        boolean bHideInputWindowWhenOnlyPreeditString;

        /**
         * @brief hide input window when there is only one candidate word
         **/
        boolean bHideInputWindowWhenOnlyOneCandidate;

        /**
         * @brief switch the preedit should show in client window or not
         **/
        FcitxHotkey hkSwitchEmbeddedPreedit[2];

        /**
         * @brief Input method use global shared state
         **/
        FcitxShareState shareState;

        /**
         * @brief Input method enable by default
         **/
        FcitxContextState defaultIMState;

        /**
         * @brief Enable Left Ctrl + Left Shift to Switch Between Input Method
         **/
        boolean bIMSwitchKey;
        
        boolean firstAsInactive;

        FcitxContextState _defaultIMState;
        int padding[61];
    } FcitxGlobalConfig;

    /**
     * @brief load config
     *
     * @param fc config instance
     * @return boolean load success or not
     **/
    boolean FcitxGlobalConfigLoad(FcitxGlobalConfig* fc);
    /**
     * @brief save config
     *
     * @param fc config instance
     * @return void
     **/
    void FcitxGlobalConfigSave(FcitxGlobalConfig* fc);

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
