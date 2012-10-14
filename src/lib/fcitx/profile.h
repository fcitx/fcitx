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

#ifndef _FCITX_PROFILE_H_
#define _FCITX_PROFILE_H_

#include <fcitx-config/fcitx-config.h>

#ifdef __cplusplus
extern "C" {
#endif
    struct _FcitxInstance;

    /**
     * @file profile.h
     *
     * define and function for ~/.config/fcitx/profile
     */

    /**
     * struct for ~/.config/fcitx/profile
     **/
    typedef struct _FcitxProfile {
        /**
         * derives from FcitxGenericConfig
         **/
        FcitxGenericConfig gconfig;
        /**
         * use remind mode
         **/
        boolean bUseRemind;

        /**
         * current im index
         **/
        char* imName;

        /**
         * use full width punc
         **/
        boolean bUseWidePunc;

        /**
         * use full width char
         **/
        boolean bUseFullWidthChar;

        /**
         * show preedit string in client or not
         **/
        boolean bUsePreedit;

        /**
         * enabled im list
         **/
        char* imList;

    } FcitxProfile;

    /**
     * load profile
     *
     * @param profile profile
     * @param instance instance
     *
     * @return boolean loading successful
     **/
    boolean FcitxProfileLoad(FcitxProfile* profile, struct _FcitxInstance* instance);
    /**
     * save profile
     *
     * @param profile profile instance
     * @return void
     **/
    void FcitxProfileSave(FcitxProfile* profile);

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
