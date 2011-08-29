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

    /**
     * @brief struct opposite to ~/.config/fcitx/config
     **/
    typedef struct _FcitxConfig
    {
        /**
         * @brief derives GenericConfig
         **/
        GenericConfig gconfig;
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
        ENTER_TO_DO enterToDo;
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
        HOTKEYS hkTrigger[2];
        /**
         * @brief switch key
         **/
        SWITCHKEY iSwitchKey;
        /**
         * @brief hotkey format of switch key
         **/
        HOTKEYS switchKey[2];
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
        HOTKEYS hkVK[2];
        /**
         * @brief hotkey for switch remind mode
         **/
        HOTKEYS hkRemind[2];
        /**
         * @brief hotkey for switch full width char
         **/
        HOTKEYS hkFullWidthChar[2];
        /**
         * @brief hotkey for switch punc
         **/
        HOTKEYS hkPunc[2];
        /**
         * @brief prev page
         **/
        HOTKEYS hkPrevPage[2];
        /**
         * @brief next page
         **/
        HOTKEYS hkNextPage[2];
        /**
         * @brief 2nd 3rd candidate select key
         **/
        HOTKEYS str2nd3rdCand[2];
        /**
         * @brief save all key
         **/
        HOTKEYS hkSaveAll[2];

        /**
         * @brief hotkey format for 2nd select key
         **/
        HOTKEYS i2ndSelectKey[2];
        /**
         * @brief hotkey format for 3rd select key
         **/
        HOTKEYS i3rdSelectKey[2];

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
        HOTKEYS hkSwitchEmbeddedPreedit[2];
    } FcitxConfig;

    /**
     * @brief load config
     *
     * @param fc config instance
     * @return boolean load success or not
     **/
    boolean LoadConfig(FcitxConfig* fc);
    /**
     * @brief save config
     *
     * @param fc config instance
     * @return void
     **/
    void SaveConfig(FcitxConfig* fc);
    /**
     * @brief get max candidate word number
     *
     * @param fc config instance
     * @return int number
     **/
    int ConfigGetMaxCandWord(FcitxConfig* fc);
    /**
     * @brief get bPointAfterNumber
     *
     * @param fc config instance
     * @return boolean bPointAfterNumber
     **/
    boolean ConfigGetPointAfterNumber(FcitxConfig* fc);

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
