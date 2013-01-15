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

    /** switch key for active/inactive */
    typedef enum _FcitxSwitchKey {
        SWITCHKEY_R_CTRL = 0,
        SWITCHKEY_R_SHIFT = 1,
        SWITCHKEY_L_SHIFT = 2,
        SWITCHKEY_L_CTRL = 3,
        SWITCHKEY_ALT_L_SHIFT = 4,
        SWITCHKEY_ALT_R_SHIFT = 5,
        SWITCHKEY_CTRL_BOTH = 6,
        SWITCHKEY_SHIFT_BOTH = 7,
        SWITCHKEY_LALT = 8,
        SWITCHKEY_RALT = 9,
        SWITCHKEY_ALT_BOTH = 10,
        SWITCHKEY_None = 11
    } FcitxSwitchKey;

    /** switch key for change current input method */
    typedef enum _FcitxIMSwitchKey {
        IMSWITCHKEY_CTRL_SHIFT = 0,
        IMSWITCHKEY_ALT_SHIFT = 1
    } FcitxIMSwitchKey;

    /** action after press enter */
    typedef enum _FcitxEnterAcion {
        K_ENTER_NOTHING = 0,
        K_ENTER_CLEAN = 1,
        K_ENTER_SEND = 2
    } FcitxEnterAcion;

    /** policy of how input method state is shared */
    typedef enum _FcitxShareState {
        ShareState_None = 0,
        ShareState_All = 1,
        ShareState_PerProgram = 2
    } FcitxShareState;

    /**
     * struct opposite to ~/.config/fcitx/config
     **/
    typedef struct _FcitxConfig {
        FcitxGenericConfig gconfig;        /**< derives FcitxGenericConfig */

        int iDelayStart;        /**< delay start seconds*/
        boolean dummy3; /**< dummy */
        boolean bEngPuncAfterNumber;  /**< input eng punc after input number */
        FcitxEnterAcion enterToDo;        /**< enter key action */
        boolean bDisablePagingInRemind;        /**< Remind mode can has multipage */
        boolean bSendTextWhenSwitchEng;   /**< switch to english with switch key commit string or not */
        int iMaxCandWord;/**< max candidate word number */
        boolean bPhraseTips;/**< phrase tips*/
        boolean bShowInputWindowTriggering;/**< show input window after trigger on*/
        boolean bPointAfterNumber;/**< index number follow with a '.'*/
        boolean bShowUserSpeed;/**< show user input speed*/
        boolean bShowVersion;/**< show fcitx version*/
        FcitxHotkey hkTrigger[2];/**< trigger key*/
        FcitxSwitchKey iSwitchKey;/**< switch key*/
        FcitxHotkey dummykey[2];/**< hotkey format of switch key*/
        boolean bDoubleSwitchKey;/**< enable double press switch action*/
        int iTimeInterval;/**< key hit interval*/
        FcitxHotkey hkVK[2];/**< hotkey for switch VK*/
        FcitxHotkey hkRemind[2];/**< hotkey for switch remind mode*/
        FcitxHotkey hkFullWidthChar[2];/**< hotkey for switch full width char*/
        FcitxHotkey hkPunc[2];/**< hotkey for switch punc*/
        FcitxHotkey hkPrevPage[2];/**< prev page*/
        FcitxHotkey hkNextPage[2];/**< next page*/
        FcitxHotkey str2nd3rdCand[2];/**< 2nd 3rd candidate select key*/
        FcitxHotkey hkSaveAll[2];/**< save all key*/
        FcitxHotkey i2ndSelectKey[2];/**< hotkey format for 2nd select key*/
        FcitxHotkey i3rdSelectKey[2];/**< hotkey format for 3rd select key*/
        boolean bHideInputWindowWhenOnlyPreeditString;/**< hide input window when there is only preedit string*/
        boolean bHideInputWindowWhenOnlyOneCandidate;/**< hide input window when there is only one candidate word*/
        FcitxHotkey hkSwitchEmbeddedPreedit[2];/**< switch the preedit should show in client window or not*/
        FcitxShareState shareState;        /**< Input method use global shared state*/
        FcitxContextState defaultIMState;        /**< Input method enable by default */
        boolean bIMSwitchKey; /**< Enable Left Ctrl + Left Shift to Switch Between Input Method */
        boolean dummy; /**< dummy */
        int _defaultIMState; /**< default input method state */
        boolean bDontCommitPreeditWhenUnfocus; /**< commit preedit when unfocus or not */
        int iIMSwitchKey; /**< the type of input method switch key */

        union {
            FcitxHotkey hkActivate[2];
            int dummy1[8];
        };

        union {
            FcitxHotkey hkInactivate[2];
            int dummy2[8];
        };

        boolean bUseExtraTriggerKeyOnlyWhenUseItToInactivate;

        boolean bShowInputWindowWhenFocusIn;

        boolean bShowInputWindowOnlyWhenActive;

        boolean bIMSwitchIncludeInactive;
        union {
            FcitxHotkey prevWord[2];
            int _dummy4[8];
        };
        union {
            FcitxHotkey nextWord[2];
            int _dummy5[8];
        };
        union {
            FcitxHotkey hkReloadConfig[2];
            int _dummy6[8];
        };
        int padding[15]; /**< padding */
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
