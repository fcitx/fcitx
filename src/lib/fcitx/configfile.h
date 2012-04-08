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


/**
 * @addtogroup Fcitx
 * @{
 */

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
        SWITCHKEY_CTRL_BOTH = 6,
        SWITCHKEY_SHIFT_BOTH = 7,
        SWITCHKEY_None = 8
    } FcitxSwitchKey;

    typedef enum _FcitxIMSwitchKey {
        IMSWITCHKEY_CTRL_SHIFT = 0,
        IMSWITCHKEY_ALT_SHIFT = 1
    } FcitxIMSwitchKey;


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
     * struct opposite to ~/.config/fcitx/config
     **/
    typedef struct _FcitxConfig {
        /**
         * derives FcitxGenericConfig
         **/
        FcitxGenericConfig gconfig;
        /* program config */
        /**
         * delay start seconds
         **/
        int iDelayStart;
        /**
         * is the first run
         **/
        boolean bFirstRun;

        /* output config */
        /**
         * input eng punc after input number
         **/
        boolean bEngPuncAfterNumber;
        /**
         * enter key action
         **/
        FcitxEnterAcion enterToDo;
        /**
         * Remind mode can has multipage
         **/
        boolean bDisablePagingInRemind;
        /**
         * switch to english with switch key commit string or not
         **/
        boolean bSendTextWhenSwitchEng;
        /**
         * max candidate word number
         **/
        int iMaxCandWord;
        /**
         * phrase tips
         **/
        boolean bPhraseTips;

        /* appearance config */
        /**
         * show input window after trigger on
         **/
        boolean bShowInputWindowTriggering;
        /**
         * index number follow with a '.'
         **/
        boolean bPointAfterNumber;
        /**
         * show user input speed
         **/
        boolean bShowUserSpeed;
        /**
         * show fcitx version
         **/
        boolean bShowVersion;

        /* hotkey config */
        /**
         * trigger key
         **/
        FcitxHotkey hkTrigger[2];
        /**
         * switch key
         **/
        FcitxSwitchKey iSwitchKey;
        /**
         * hotkey format of switch key
         **/
        FcitxHotkey dummykey[2];
        /**
         * enable double press switch action
         **/
        boolean bDoubleSwitchKey;
        /**
         * key hit interval
         **/
        int iTimeInterval;
        /**
         * hotkey for switch VK
         **/
        FcitxHotkey hkVK[2];
        /**
         * hotkey for switch remind mode
         **/
        FcitxHotkey hkRemind[2];
        /**
         * hotkey for switch full width char
         **/
        FcitxHotkey hkFullWidthChar[2];
        /**
         * hotkey for switch punc
         **/
        FcitxHotkey hkPunc[2];
        /**
         * prev page
         **/
        FcitxHotkey hkPrevPage[2];
        /**
         * next page
         **/
        FcitxHotkey hkNextPage[2];
        /**
         * 2nd 3rd candidate select key
         **/
        FcitxHotkey str2nd3rdCand[2];
        /**
         * save all key
         **/
        FcitxHotkey hkSaveAll[2];

        /**
         * hotkey format for 2nd select key
         **/
        FcitxHotkey i2ndSelectKey[2];
        /**
         * hotkey format for 3rd select key
         **/
        FcitxHotkey i3rdSelectKey[2];

        /**
         * hide input window when there is only preedit string
         **/
        boolean bHideInputWindowWhenOnlyPreeditString;

        /**
         * hide input window when there is only one candidate word
         **/
        boolean bHideInputWindowWhenOnlyOneCandidate;

        /**
         * switch the preedit should show in client window or not
         **/
        FcitxHotkey hkSwitchEmbeddedPreedit[2];

        /**
         * Input method use global shared state
         **/
        FcitxShareState shareState;

        /**
         * Input method enable by default
         **/
        FcitxContextState defaultIMState;

        /**
         * Enable Left Ctrl + Left Shift to Switch Between Input Method
         **/
        boolean bIMSwitchKey;

        boolean firstAsInactive; /**< use first input method as inactive state */

        FcitxContextState _defaultIMState; /**< default input method state */

        boolean bDontCommitPreeditWhenUnfocus; /**< commit preedit when unfocus or not */

        int iIMSwitchKey;

        int padding[59]; /**< padding */
    } FcitxGlobalConfig;

    /**
     * load config
     *
     * @param fc config instance
     * @return boolean load success or not
     **/
    boolean FcitxGlobalConfigLoad(FcitxGlobalConfig* fc);
    /**
     * save config
     *
     * @param fc config instance
     * @return void
     **/
    void FcitxGlobalConfigSave(FcitxGlobalConfig* fc);

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
