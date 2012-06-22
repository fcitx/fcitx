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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
/**
 * @file   ime-internal.h
 * @date   2008-1-16
 *
 *  Private Header for Input Method
 *
 */

#ifndef _FCITX_IME_INTERNAL_H_
#define _FCITX_IME_INTERNAL_H_

#include "fcitx-config/hotkey.h"
#include "ime.h"
#include "frontend.h"
#include "fcitx-utils/utarray.h"

struct _FcitxInputContext;
struct _FcitxInstance;

typedef enum _KEY_RELEASED {
    KR_OTHER = 0,
    KR_SWITCH,
    KR_2ND_SELECTKEY,
    KR_3RD_SELECTKEY,
    KR_SWITCH_IM,
    KR_SWITCH_IM_REVERSE
} KEY_RELEASED;

struct _FcitxInputState {
    long unsigned int lastKeyPressedTime;
    boolean bIsDoInputOnly;
    KEY_RELEASED keyReleased;
    int iCodeInputCount;
    char strCodeInput[MAX_USER_INPUT + 1];
    char strStringGet[MAX_USER_INPUT + 1];
    char strLastCommit[MAX_USER_INPUT + 1];
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
    struct _FcitxCandidateWordList* candList;
    FcitxMessages* msgPreedit;
    FcitxMessages* msgAuxUp;
    FcitxMessages* msgAuxDown;
    FcitxMessages* msgClientPreedit;

    uint32_t keycode;
    uint32_t keysym;
    uint32_t keystate;

    int padding[60];
};

struct _FcitxIMEntry {
    FcitxGenericConfig config;
    char* uniqueName;
    char* name;
    char* iconName;
    int priority;
    char* langCode;
    char* parent;
};

typedef struct _FcitxIMEntry FcitxIMEntry;

void FcitxInstanceInitIM(struct _FcitxInstance* instance);

void FcitxInstanceInitBuiltInHotkey(struct _FcitxInstance* instance);

void FcitxInstanceDoPhraseTips(struct _FcitxInstance* instance);

boolean FcitxInstanceLoadAllIM(struct _FcitxInstance* instance);

void FcitxInstanceInitIMMenu(struct _FcitxInstance* instance);

void FcitxInstanceShowInputSpeed(struct _FcitxInstance* instance);

INPUT_RETURN_VALUE ImProcessEnter(void *arg);

INPUT_RETURN_VALUE ImProcessEscape(void *arg);

INPUT_RETURN_VALUE ImProcessRemind(void *arg);

INPUT_RETURN_VALUE ImProcessReload(void *arg);

INPUT_RETURN_VALUE ImProcessSaveAll(void *arg);

INPUT_RETURN_VALUE ImSwitchEmbeddedPreedit(void *arg);

boolean IMIsInIMNameList(UT_array* imList, FcitxIM* ime);

void FcitxInstanceLoadIM(struct _FcitxInstance* instance, FcitxAddon* addon);

void FcitxInstanceSwitchIMInternal(struct _FcitxInstance* instance, int index, boolean skipZero, boolean updateGlobal);

FcitxConfigFileDesc* GetIMConfigDesc();

int IMPriorityCmp(const void *a, const void *b);

boolean FcitxInstanceUpdateCurrentIM(struct _FcitxInstance* instance, boolean force);

void HideInputSpeed(void* arg);

boolean FcitxInstanceCheckICFromSameApplication (struct  _FcitxInstance* instance, FcitxInputContext* rec, FcitxInputContext* ic);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
