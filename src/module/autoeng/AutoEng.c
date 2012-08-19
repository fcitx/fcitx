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
 * Auto Switch to English
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
#include "module/spell/spell.h"

#include "AutoEng.h"

typedef enum {
    AECM_NONE,
    AECM_ALT,
    AECM_CTRL,
    AECM_SHIFT,
} AutoEngChooseModifier;

typedef struct {
    FcitxGenericConfig gconfig;
    AutoEngChooseModifier chooseModifier;
} FcitxAutoEngConfig;

typedef struct _FcitxAutoEngState {
    UT_array* autoEng;
    char buf[MAX_USER_INPUT + 1];
    int index;
    boolean active;
    FcitxInstance *owner;
    FcitxAutoEngConfig config;
} FcitxAutoEngState;

static const unsigned int cmodtable[] = {
    FcitxKeyState_None,
    FcitxKeyState_Alt,
    FcitxKeyState_Ctrl,
    FcitxKeyState_Shift
};

static const UT_icd autoeng_icd = { sizeof(AUTO_ENG), 0, 0, 0 };

CONFIG_BINDING_BEGIN(FcitxAutoEngConfig);
CONFIG_BINDING_REGISTER("Auto English", "ChooseModifier", chooseModifier);
CONFIG_BINDING_END();

/**
 * Initialize for Auto English
 *
 * @return boolean
 **/
static void* AutoEngCreate(FcitxInstance *instance);

/**
 * Load AutoEng File
 *
 * @return void
 **/
static void            LoadAutoEng(FcitxAutoEngState* autoEngState);

/**
 * Cache the input key for autoeng, only simple key without combine key will be record
 *
 * @param sym keysym
 * @param state key state
 * @param retval Return state value
 * @return boolean
 **/
static boolean PreInputProcessAutoEng(void* arg,
                                      FcitxKeySym sym,
                                      unsigned int state,
                                      INPUT_RETURN_VALUE *retval
                                     );

static boolean PostInputProcessAutoEng(void* arg,
                                      FcitxKeySym sym,
                                      unsigned int state,
                                      INPUT_RETURN_VALUE *retval
                                     );

/**
 * clean the cache while reset input
 *
 * @return void
 **/
static void ResetAutoEng(void *arg);

/**
 * Free Auto Eng Data
 *
 * @return void
 **/
static void FreeAutoEng(void* arg);

static void ReloadAutoEng(void* arg);

/**
 * Check whether need to switch to English
 *
 * @param  str string
 * @return boolean
 **/
boolean            SwitchToEng(FcitxAutoEngState* autoEngState, char* str);


/**
 * Update message for Auto eng
 *
 * @return void
 **/
static void ShowAutoEngMessage(FcitxAutoEngState* autoEngState,
                               INPUT_RETURN_VALUE *retval);

FCITX_DEFINE_PLUGIN(fcitx_autoeng, module ,FcitxModule) = {
    AutoEngCreate,
    NULL,
    NULL,
    FreeAutoEng,
    ReloadAutoEng
};

void* AutoEngCreate(FcitxInstance *instance)
{
    FcitxAutoEngState* autoEngState = fcitx_utils_new(FcitxAutoEngState);
    autoEngState->owner = instance;
    LoadAutoEng(autoEngState);

    FcitxKeyFilterHook khk;
    khk.arg = autoEngState;
    khk.func = PreInputProcessAutoEng;

    FcitxInstanceRegisterPreInputFilter(instance, khk);

    khk.func = PostInputProcessAutoEng;
    FcitxInstanceRegisterPostInputFilter(instance, khk);

    FcitxIMEventHook rhk;
    rhk.arg = autoEngState;
    rhk.func = ResetAutoEng;
    FcitxInstanceRegisterResetInputHook(instance, rhk);

    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_DISABLE_AUTOENG, FCT_Boolean, FCF_ResetOnInputMethodChange);

    ResetAutoEng(autoEngState);
    return autoEngState;
}

static INPUT_RETURN_VALUE
AutoEngCheckSelect(FcitxAutoEngState *autoEngState,
                   FcitxKeySym sym, unsigned int state)
{
    FcitxCandidateWordList *candList = FcitxInputStateGetCandidateList(
        FcitxInstanceGetInputState(autoEngState->owner));
    int iKey = FcitxCandidateWordCheckChooseKey(candList, sym, state);
    if (iKey >= 0)
        return FcitxCandidateWordChooseByIndex(candList, iKey);
    return 0;
}

static boolean PreInputProcessAutoEng(void* arg, FcitxKeySym sym,
                                      unsigned int state,
                                      INPUT_RETURN_VALUE *retval)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*)arg;
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);
    boolean disableCheckUAZ = FcitxInstanceGetContextBoolean(autoEngState->owner, CONTEXT_DISABLE_AUTOENG);
    if (disableCheckUAZ)
        return false;

    if (autoEngState->active) {
        FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
        if ((*retval = AutoEngCheckSelect(autoEngState, sym, state))) {
            return true;
        } else if (FcitxHotkeyIsHotKeySimple(keymain, state)) {
            if (autoEngState->index < MAX_USER_INPUT) {
                autoEngState->buf[autoEngState->index] = keymain;
                autoEngState->index++;
                autoEngState->buf[autoEngState->index] = '\0';
                *retval = IRV_DISPLAY_MESSAGE;
            } else {
                *retval = IRV_DO_NOTHING;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
            autoEngState->index--;
            autoEngState->buf[autoEngState->index] = '\0';
            if (autoEngState->index == 0) {
                ResetAutoEng(autoEngState);
                *retval = IRV_CLEAN;
            } else {
                *retval = IRV_DISPLAY_MESSAGE;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {
            strcpy(FcitxInputStateGetOutputString(input), autoEngState->buf);
            ResetAutoEng(autoEngState);
            *retval = IRV_COMMIT_STRING;
        } else if (FcitxHotkeyIsHotkeyCursorMove(sym, state)) {
            *retval = IRV_DO_NOTHING;
            return true;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
            *retval = IRV_CLEAN;
            return true;
        }
        ShowAutoEngMessage(autoEngState, retval);
        return true;
    }
    if (FcitxHotkeyIsHotKeySimple(sym, state)) {
        strncpy(autoEngState->buf, FcitxInputStateGetRawInputBuffer(input), MAX_USER_INPUT);
        if (strlen(autoEngState->buf) >= MAX_USER_INPUT - 1)
            return false;

        autoEngState->index = strlen(autoEngState->buf);
        autoEngState->buf[autoEngState->index++] = sym;
        autoEngState->buf[autoEngState->index] = '\0';

        if (SwitchToEng(autoEngState, autoEngState->buf)) {
            *retval = IRV_DISPLAY_MESSAGE;
            FcitxInputStateSetShowCursor(input, false);
            autoEngState->index = strlen(autoEngState->buf);
            autoEngState->active = true;
            ShowAutoEngMessage(autoEngState, retval);
            return true;
        }
    }
    return false;
}

boolean PostInputProcessAutoEng(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);
    boolean disableCheckUAZ = FcitxInstanceGetContextBoolean(autoEngState->owner, CONTEXT_DISABLE_AUTOENG);
    if (disableCheckUAZ)
        return false;
    if (FcitxHotkeyIsHotKeyUAZ(sym, state)
        && (FcitxInputStateGetRawInputBufferSize(input) != 0
        || (FcitxInputStateGetKeyState(input) & FcitxKeyState_CapsLock) == 0)) {
        *retval = IRV_DISPLAY_MESSAGE;
        FcitxInputStateSetShowCursor(input, false);
        strncpy(autoEngState->buf, FcitxInputStateGetRawInputBuffer(input), MAX_USER_INPUT);
        autoEngState->index = strlen(autoEngState->buf);
        autoEngState->buf[autoEngState->index++] = sym;
        autoEngState->buf[autoEngState->index] = '\0';
        autoEngState->active = true;
        ShowAutoEngMessage(autoEngState, retval);
        return true;
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

static CONFIG_DESC_DEFINE(GetAutoEngConfigDesc, "fcitx-autoeng.desc");

static void
SaveAutoEngConfig(FcitxAutoEngConfig* fs)
{
    FcitxConfigFileDesc *configDesc = GetAutoEngConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-autoeng.config",
                                             "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &fs->gconfig, configDesc);
    if (fp)
        fclose(fp);
}

static boolean
LoadAutoEngConfig(FcitxAutoEngConfig *config)
{
    FcitxConfigFileDesc *configDesc = GetAutoEngConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-autoeng.config",
                                             "r", NULL);

    if (!fp) {
        if (errno == ENOENT)
            SaveAutoEngConfig(config);
    }
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);
    FcitxAutoEngConfigConfigBind(config, cfile, configDesc);
    FcitxConfigBindSync(&config->gconfig);
    if (config->chooseModifier > AECM_CTRL)
        config->chooseModifier = AECM_CTRL;

    if (fp)
        fclose(fp);

    return true;
}

void LoadAutoEng(FcitxAutoEngState* autoEngState)
{
    FILE    *fp;
    char    *buf = NULL;
    size_t   length = 0;

    LoadAutoEngConfig(&autoEngState->config);
    fp = FcitxXDGGetFileWithPrefix("data", "AutoEng.dat", "r", NULL);
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

static void
ShowAutoEngMessage(FcitxAutoEngState *autoEngState, INPUT_RETURN_VALUE *retval)
{
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);
    FcitxGlobalConfig* config;
    FcitxInstanceCleanInputWindow(autoEngState->owner);
    if (autoEngState->buf[0] == '\0')
        return;

    config = FcitxInstanceGetGlobalConfig(autoEngState->owner);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input),
                                  MSG_INPUT, "%s", autoEngState->buf);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input),
                                  MSG_INPUT, "%s", autoEngState->buf);
    strcpy(FcitxInputStateGetRawInputBuffer(input), autoEngState->buf);
    FcitxInputStateSetRawInputBufferSize(input, strlen(autoEngState->buf));
    FcitxInputStateSetCursorPos(
        input, FcitxInputStateGetRawInputBufferSize(input));
    FcitxInputStateSetClientCursorPos(
        input, FcitxInputStateGetRawInputBufferSize(input));
    FcitxInputStateSetShowCursor(input, true);

    FcitxModuleFunctionArg func_arg;
    func_arg.args[0] = NULL;
    func_arg.args[1] = autoEngState->buf;
    func_arg.args[2] = NULL;
    func_arg.args[3] = (void*)(long)config->iMaxCandWord;
    func_arg.args[4] = "en";
    func_arg.args[5] = "cus";
    func_arg.args[6] = NULL;
    func_arg.args[7] = NULL;
    FcitxCandidateWordList *candList =
        InvokeFunction(autoEngState->owner, FCITX_SPELL,
                       GET_CANDWORDS, func_arg);
    if (candList) {
        FcitxCandidateWordList *iList = FcitxInputStateGetCandidateList(input);
        FcitxCandidateWordSetChooseAndModifier(
            iList, DIGIT_STR_CHOOSE,
            cmodtable[autoEngState->config.chooseModifier]);
        FcitxCandidateWordMerge(iList, candList, -1);
        FcitxCandidateWordFreeList(candList);
    }
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input),
                                  MSG_TIPS, _("Press Enter to input text"));
    *retval |= IRV_FLAG_UPDATE_INPUT_WINDOW;
}

void ReloadAutoEng(void* arg)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    FreeAutoEng(autoEngState);
    LoadAutoEng(autoEngState);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
