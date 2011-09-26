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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @file   ime-internal.h
 * @date   2008-1-16
 *
 * @brief  Private Header for Input Method
 *
 */

#ifndef _FCITX_IME_INTERNAL_H_
#define _FCITX_IME_INTERNAL_H_

#include "fcitx-config/hotkey.h"
#include "ime.h"
#include "fcitx-utils/utarray.h"

struct _FcitxInputContext;
struct _FcitxInstance;

struct _FcitxInputState {
    long unsigned int lastKeyPressedTime;
    boolean bIsDoInputOnly;
    KEY_RELEASED keyReleased;
    int iCodeInputCount;
    char strCodeInput[MAX_USER_INPUT + 1];
    char strStringGet[MAX_USER_INPUT + 1];  //保存输入法返回的需要送到客户程序中的字串
    boolean bIsInRemind;

    time_t dummy;
    int iCursorPos;
    int iClientCursorPos;
    boolean bShowCursor;
    int dummy2;
    int lastIsSingleHZ;
    boolean bLastIsNumber;
    boolean dummy3;

    /* the ui message part, if there is something in it, then it will be shown */
    struct _CandidateWordList* candList;
    Messages* msgPreedit;
    Messages* msgAuxUp;
    Messages* msgAuxDown;
    Messages* msgClientPreedit;

    int padding[63];
};

/**
 * @brief init fcitx im array
 *
 * @param instance instance
 * @return void
 **/
void InitFcitxIM(struct _FcitxInstance* instance);

/**
 * @brief init builtin hotkey (ESC, ENTER)
 *
 * @param instance instance
 * @return void
 **/
void InitBuiltInHotkey(struct _FcitxInstance* instance);

/**
 * @brief generat phrase tips
 *
 * @param instance fcitx instance
 * @return void
 **/
void DoPhraseTips(struct _FcitxInstance* instance);

/**
 * @brief unload all input method
 *
 * @param ims im array
 * @return void
 **/
void UnloadAllIM(UT_array* ims);

/**
 * @brief load all im from addons
 *
 * @param instance instance
 * @return boolean
 **/
boolean LoadAllIM(struct _FcitxInstance* instance);

/**
 * @brief init builtin im menu
 *
 * @param instance instance
 * @return void
 **/
void InitIMMenu(struct _FcitxInstance* instance);

/**
 * @brief show input speed
 *
 * @param instance instance
 * @return void
 **/
void ShowInputSpeed(struct _FcitxInstance* instance);

/**
 * @brief process enter action
 *
 * @param arg instance
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE ImProcessEnter(void *arg);

/**
 * @brief process escape action
 *
 * @param arg instance
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE ImProcessEscape(void *arg);

/**
 * @brief process hkRemind
 *
 * @param arg instance
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE ImProcessRemind(void *arg);

/**
 * @brief process reload key
 *
 * @param arg instance
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE ImProcessReload(void *arg);

/**
 * @brief process hkSaveAll
 *
 * @param arg instance
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE ImProcessSaveAll(void *arg);

/**
 * @brief switch between "on the spot" and "over the spot"
 *
 * @param arg instance
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE ImSwitchEmbeddedPreedit(void *arg);

int GetIMIndexByName(struct _FcitxInstance* instance, char* imName);

boolean IMIsInIMNameList(UT_array* imList, FcitxIM* ime);


#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
