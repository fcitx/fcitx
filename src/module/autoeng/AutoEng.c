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
 * @file AutoEng.c
 * @brief Auto Switch to English
 */
#include <limits.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"
#include "fcitx/keys.h"
#include "fcitx/ui.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx-utils/utils.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/xdg.h"

#include "AutoEng.h"

typedef struct _FcitxAutoEngState {
    UT_array* autoEng;
    char buf[MAX_USER_INPUT + 1];
    int index;
    boolean active;
    FcitxInstance *owner;
} FcitxAutoEngState;

static const UT_icd autoeng_icd = { sizeof(AUTO_ENG), 0, 0, 0 };


/**
 * @brief Initialize for Auto English
 *
 * @return boolean
 **/
static void* AutoEngCreate(FcitxInstance *instance);

/**
 * @brief Load AutoEng File
 *
 * @return void
 **/
static void            LoadAutoEng(FcitxAutoEngState* autoEngState);

/**
 * @brief Cache the input key for autoeng, only simple key without combine key will be record
 *
 * @param sym keysym
 * @param state key state
 * @param retval Return state value
 * @return boolean
 **/
static boolean ProcessAutoEng(void* arg,
                              FcitxKeySym sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             );

/**
 * @brief clean the cache while reset input
 *
 * @return void
 **/
static void ResetAutoEng(void *arg);

/**
 * @brief Free Auto Eng Data
 *
 * @return void
 **/
static void FreeAutoEng(void* arg);

static void ReloadAutoEng(void* arg);

/**
 * @brief Check whether need to switch to English
 *
 * @param  str string
 * @return boolean
 **/
boolean            SwitchToEng(FcitxAutoEngState* autoEngState, char* str);


/**
 * @brief Update message for Auto eng
 *
 * @return void
 **/
void ShowAutoEngMessage(FcitxAutoEngState* autoEngState);

FCITX_EXPORT_API
FcitxModule module = {
    AutoEngCreate,
    NULL,
    NULL,
    FreeAutoEng,
    ReloadAutoEng
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* AutoEngCreate(FcitxInstance *instance)
{
    FcitxAutoEngState* autoEngState = fcitx_utils_malloc0(sizeof(FcitxAutoEngState));
    autoEngState->owner = instance;
    LoadAutoEng(autoEngState);

    FcitxKeyFilterHook khk;
    khk.arg = autoEngState;
    khk.func = ProcessAutoEng;

    FcitxIMEventHook rhk;
    rhk.arg = autoEngState;
    rhk.func = ResetAutoEng;

    FcitxInstanceRegisterPreInputFilter(instance, khk);
    FcitxInstanceRegisterResetInputHook(instance, rhk);
    
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_DISABLE_AUTOENG, FCT_Boolean, FCF_ResetOnInputMethodChange);

    ResetAutoEng(autoEngState);
    return autoEngState;
}

static boolean ProcessAutoEng(void* arg, FcitxKeySym sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             )
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);
    boolean disableCheckUAZ = FcitxInstanceGetContextBoolean(autoEngState->owner, CONTEXT_DISABLE_AUTOENG);
    if (disableCheckUAZ)
        return false;
    
    if (autoEngState->active) {
        FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
        if (FcitxHotkeyIsHotKeySimple(keymain, state)) {
            if (autoEngState->index < MAX_USER_INPUT) {
                autoEngState->buf[autoEngState->index] = keymain;
                autoEngState->index++;
                autoEngState->buf[autoEngState->index] = '\0';
                *retval = IRV_DISPLAY_MESSAGE;
            } else
                *retval = IRV_DO_NOTHING;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
            autoEngState->index -- ;
            autoEngState->buf[autoEngState->index] = '\0';
            if (autoEngState->index == 0) {
                ResetAutoEng(autoEngState);
                *retval = IRV_CLEAN;
            } else
                *retval = IRV_DISPLAY_MESSAGE;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {
            strcpy(FcitxInputStateGetOutputString(input), autoEngState->buf);
            ResetAutoEng(autoEngState);
            *retval = IRV_COMMIT_STRING;
        }
        ShowAutoEngMessage(autoEngState);
        return true;
    }
    if (FcitxHotkeyIsHotKeySimple(sym, state)) {
        if (FcitxInputStateGetRawInputBufferSize(input) == 0 && FcitxHotkeyIsHotKeyUAZ(sym, state)) {
            autoEngState->index = 1;
            autoEngState->buf[0] = sym;
            autoEngState->buf[1] = '\0';
            *retval = IRV_DISPLAY_MESSAGE;
            FcitxInputStateSetShowCursor(input, false);
            autoEngState->index = strlen(autoEngState->buf);
            autoEngState->active = true;
            ShowAutoEngMessage(autoEngState);
            return true;
        }

        strncpy(autoEngState->buf, FcitxInputStateGetRawInputBuffer(input), MAX_USER_INPUT);
        if (strlen(autoEngState->buf) >= MAX_USER_INPUT - 1)
            return false;

        autoEngState->index = strlen(autoEngState->buf);
        autoEngState->buf[autoEngState->index ++ ] = sym;
        autoEngState->buf[autoEngState->index] = '\0';

        if (SwitchToEng(autoEngState, autoEngState->buf)) {
            *retval = IRV_DISPLAY_MESSAGE;
            FcitxInputStateSetShowCursor(input, false);
            autoEngState->index = strlen(autoEngState->buf);
            autoEngState->active = true;
            ShowAutoEngMessage(autoEngState);
            return true;
        }
    }
    return false;
}

void ResetAutoEng(void* arg)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    autoEngState->index = 0;
    autoEngState->buf[autoEngState->index] = '\0';
    autoEngState->active = false;
}

void LoadAutoEng(FcitxAutoEngState* autoEngState)
{
    FILE    *fp;
    char    *buf = NULL;
    size_t   length = 0;

    fp = FcitxXDGGetFileWithPrefix("data", "AutoEng.dat", "rt", NULL);
    if (!fp)
        return;

    utarray_new(autoEngState->autoEng, &autoeng_icd);
    AUTO_ENG autoeng;

    while (getline(&buf, &length, fp) != -1) {
        char* line = fcitx_utils_trim(buf);
        if (strlen(line) > MAX_AUTO_TO_ENG)
            FcitxLog(WARNING, _("Too long item for AutoEng"));
        strncpy(autoeng.str, line, MAX_AUTO_TO_ENG);
        free(line);
        autoeng.str[MAX_AUTO_TO_ENG] = '\0';
        utarray_push_back(autoEngState->autoEng, &autoeng);
    }

    free(buf);

    fclose(fp);
}

void FreeAutoEng(void* arg)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    if (autoEngState->autoEng) {
        utarray_free(autoEngState->autoEng);
        autoEngState->autoEng = NULL;
    }
}

boolean SwitchToEng(FcitxAutoEngState* autoEngState, char *str)
{
    AUTO_ENG*       autoeng;
    for (autoeng = (AUTO_ENG *) utarray_front(autoEngState->autoEng);
            autoeng != NULL;
            autoeng = (AUTO_ENG *) utarray_next(autoEngState->autoEng, autoeng))
        if (!strcmp(str, autoeng->str))
            return true;

    return false;
}

void ShowAutoEngMessage(FcitxAutoEngState* autoEngState)
{
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);

    FcitxInstanceCleanInputWindow(autoEngState->owner);

    if (autoEngState->buf[0] == '\0')
        return;

    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", autoEngState->buf);
    strcpy(FcitxInputStateGetRawInputBuffer(input), autoEngState->buf);
    FcitxInputStateSetRawInputBufferSize(input, strlen(autoEngState->buf));
    FcitxInputStateSetCursorPos(input, FcitxInputStateGetRawInputBufferSize(input));
    FcitxInputStateSetShowCursor(input, true);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, _("Press Enter to input text"));
}

void ReloadAutoEng(void* arg)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    FreeAutoEng(autoEngState);
    LoadAutoEng(autoEngState);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
