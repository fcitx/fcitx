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
#include "module/spell/fcitx-spell.h"

#include "AutoEng.h"
#define case_autoeng_replace case ' ': case '\'': case '-': case '_':   \
case '/'
#define case_autoeng_exchange case ',': case '.': case ':': case ';':   \
case '?': case '!'

typedef enum {
    AECM_NONE,
    AECM_ALT,
    AECM_CTRL,
    AECM_SHIFT,
    _AECM_COUNT
} AutoEngChooseModifier;

typedef struct {
    FcitxGenericConfig gconfig;
    AutoEngChooseModifier chooseModifier;
    boolean disableSpell;
    int maxHintLength;
    boolean selectAddSpace;
    int maxKeep;
} FcitxAutoEngConfig;

typedef struct _FcitxAutoEngState {
    UT_array *autoEng;
    char *buf;
    int index;
    size_t buff_size;
    boolean active;
    boolean auto_space;
    FcitxInstance *owner;
    FcitxAutoEngConfig config;
    boolean cursor_moved;
    char *back_buff;
    size_t back_buff_size;
} FcitxAutoEngState;

static const unsigned int cmodtable[] = {
    FcitxKeyState_None,
    FcitxKeyState_Alt,
    FcitxKeyState_Ctrl,
    FcitxKeyState_Shift
};

static const UT_icd autoeng_icd = { sizeof(AUTO_ENG), NULL, NULL, NULL };

CONFIG_BINDING_BEGIN(FcitxAutoEngConfig);
CONFIG_BINDING_REGISTER("Auto English", "ChooseModifier", chooseModifier);
CONFIG_BINDING_REGISTER("Auto English", "DisableSpell", disableSpell);
CONFIG_BINDING_REGISTER("Auto English", "MaximumHintLength", maxHintLength);
CONFIG_BINDING_REGISTER("Auto English", "MaximumKeep", maxKeep);
CONFIG_BINDING_REGISTER("Auto English", "SelectAddSpace", selectAddSpace);
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
static void LoadAutoEng(FcitxAutoEngState *autoEngState);

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
                                      INPUT_RETURN_VALUE *retval);

static boolean PostInputProcessAutoEng(void* arg,
                                       FcitxKeySym sym,
                                       unsigned int state,
                                       INPUT_RETURN_VALUE *retval);

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
boolean SwitchToEng(FcitxAutoEngState *autoEngState, const char *str);

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

static void
AutoEngSetBuffLen(FcitxAutoEngState *autoEngState, size_t len)
{
    unsigned int size = fcitx_utils_align_to(len + 1, MAX_USER_INPUT);
    if (autoEngState->buff_size != size) {
        autoEngState->buf = realloc(autoEngState->buf, size);
        autoEngState->buff_size = size;
    }
    autoEngState->buf[len] = '\0';
    autoEngState->auto_space = false;
}

static void
AutoEngSetBuff(FcitxAutoEngState* autoEngState, const char *str, char extra)
{
    size_t len = str ? strlen(str) : 0;
    autoEngState->index = len + (extra ? 1 : 0);
    AutoEngSetBuffLen(autoEngState, autoEngState->index);
    if (len) {
        memcpy(autoEngState->buf, str, len);
    }
    if (extra) {
        autoEngState->buf[len] = extra;
    }
}

static void
AutoEngSwapBuff(FcitxAutoEngState* autoEngState)
{
    size_t tmp_size = autoEngState->buff_size;
    char *tmp_buf = autoEngState->buf;
    autoEngState->buf = autoEngState->back_buff;
    autoEngState->buff_size = autoEngState->back_buff_size;
    autoEngState->back_buff = tmp_buf;
    autoEngState->back_buff_size = tmp_size;
}

static boolean
AutoEngCheckAutoSpace(FcitxAutoEngState *autoEngState, char key)
{
    autoEngState->auto_space = false;
    if (autoEngState->buf[autoEngState->index - 1] != ' ') {
        return false;
    }
    switch (key) {
    case_autoeng_replace:
        autoEngState->buf[autoEngState->index - 1] = key;
        break;
    case_autoeng_exchange:
        autoEngState->buf[autoEngState->index - 1] = key;
        autoEngState->buf[autoEngState->index] = ' ';
        AutoEngSetBuffLen(autoEngState, ++autoEngState->index);
        autoEngState->auto_space = true;
        break;
    default:
        return false;
    }
    return true;
}

static INPUT_RETURN_VALUE
AutoEngPushKey(FcitxAutoEngState *autoEngState, char key)
{
    INPUT_RETURN_VALUE res = IRV_DISPLAY_MESSAGE;

    if (autoEngState->auto_space) {
        if (AutoEngCheckAutoSpace(autoEngState, key)) {
            return res;
        }
    }

    if (autoEngState->config.maxKeep == 0) {
        if (key == ' ') {
            FcitxInstance *instance = autoEngState->owner;
            FcitxInputContext *currentIC = FcitxInstanceGetCurrentIC(instance);
            FcitxInstanceCommitString(instance, currentIC, autoEngState->buf);
            FcitxInstanceCommitString(instance, currentIC, " ");
            ResetAutoEng(autoEngState);
            return res | IRV_CLEAN;
        }
    } else if (autoEngState->config.maxKeep > 0) {
        char *found = autoEngState->buf + autoEngState->index;
        int i = autoEngState->config.maxKeep;
        do {
            found = memrchr(autoEngState->buf, ' ', found - autoEngState->buf);
            if (!found) {
                break;
            }
        } while (--i > 0);
        if (found && found != autoEngState->buf) {
            FcitxInstance *instance = autoEngState->owner;
            FcitxInputContext *currentIC = FcitxInstanceGetCurrentIC(instance);
            *found = '\0';
            FcitxInstanceCommitString(instance, currentIC, autoEngState->buf);
            autoEngState->index = (autoEngState->buf + autoEngState->index
                                   - found);
            memmove(autoEngState->buf + 1, found + 1, autoEngState->index);
            *autoEngState->buf = ' ';
        }
    }
    autoEngState->buf[autoEngState->index++] = key;
    AutoEngSetBuffLen(autoEngState, autoEngState->index);
    return res;
}

static boolean
AutoEngCheckPreedit(FcitxAutoEngState *autoEngState)
{
    FcitxInputState *input;
    char *preedit;
    input = FcitxInstanceGetInputState(autoEngState->owner);
    preedit = FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
    boolean res = !(preedit && *fcitx_utils_get_ascii_end(preedit));
    free(preedit);
    return res;
}

void *AutoEngCreate(FcitxInstance *instance)
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
    FcitxInstance *instance = autoEngState->owner;
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(
        FcitxInstanceGetInputState(autoEngState->owner));
    if (!FcitxCandidateWordGetListSize(cand_list))
        return IRV_TO_PROCESS;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig *fc = FcitxInstanceGetGlobalConfig(instance);
    int key;
    FcitxCandidateWord *cand_word;
    if (FcitxHotkeyIsHotKey(sym, state, fc->nextWord)) {
        if (!autoEngState->cursor_moved) {
            cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
        } else {
            cand_word = FcitxCandidateWordGetFocus(cand_list, true);
            cand_word = FcitxCandidateWordGetNext(cand_list, cand_word);
            if (!cand_word) {
                FcitxCandidateWordSetPage(cand_list, 0);
            } else {
                FcitxCandidateWordSetFocus(
                    cand_list, FcitxCandidateWordGetIndex(cand_list,
                                                          cand_word));
            }
        }
    } else if (FcitxHotkeyIsHotKey(sym, state, fc->prevWord)) {
        if (!autoEngState->cursor_moved) {
            cand_word = FcitxCandidateWordGetByIndex(
                cand_list,
                FcitxCandidateWordGetCurrentWindowSize(cand_list) - 1);
        } else {
            cand_word = FcitxCandidateWordGetFocus(cand_list, true);
            cand_word = FcitxCandidateWordGetPrev(cand_list, cand_word);
            if (cand_word) {
                FcitxCandidateWordSetFocus(
                    cand_list, FcitxCandidateWordGetIndex(cand_list,
                                                          cand_word));
            }
        }
    } else if (FcitxHotkeyIsHotKey(sym, state,
                                   FcitxConfigPrevPageKey(instance, fc))) {
        boolean has_prev_page;
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        has_prev_page = FcitxCandidateWordGoPrevPage(cand_list);
        if (!autoEngState->cursor_moved) {
            cand_word = NULL;
        } else if (has_prev_page) {
            cand_word = FcitxCandidateWordGetCurrentWindow(cand_list) +
                FcitxCandidateWordGetCurrentWindowSize(cand_list) - 1;
        }
    } else if (FcitxHotkeyIsHotKey(sym, state,
                                   FcitxConfigNextPageKey(instance, fc))) {
        boolean has_next_page;
        cand_word = FcitxCandidateWordGetFocus(cand_list, true);
        has_next_page = FcitxCandidateWordGoNextPage(cand_list);
        if (!autoEngState->cursor_moved) {
            cand_word = NULL;
        } else if (has_next_page) {
            cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
        }
    } else if ((key = FcitxCandidateWordCheckChooseKey(cand_list,
                                                       sym, state)) >= 0) {
        return FcitxCandidateWordChooseByIndex(cand_list, key);
    } else {
        return IRV_TO_PROCESS;
    }
    if (cand_word) {
        FcitxCandidateWordSetType(cand_word, MSG_CANDIATE_CURSOR);
        if (!autoEngState->cursor_moved)
            AutoEngSwapBuff(autoEngState);
        AutoEngSetBuff(autoEngState, cand_word->strWord, '\0');
        autoEngState->cursor_moved = true;
    } else if (autoEngState->cursor_moved) {
        AutoEngSwapBuff(autoEngState);
        autoEngState->cursor_moved = false;
    } else {
        return IRV_FLAG_UPDATE_INPUT_WINDOW;
    }
    FcitxMessages *client_preedit = FcitxInputStateGetClientPreedit(input);
    FcitxMessages *preedit = FcitxInputStateGetPreedit(input);
    FcitxMessagesSetMessageCount(client_preedit, 0);
    FcitxMessagesSetMessageCount(preedit, 0);
    FcitxMessagesAddMessageStringsAtLast(client_preedit, MSG_INPUT,
                                         autoEngState->buf);
    FcitxMessagesAddMessageStringsAtLast(preedit, MSG_INPUT,
                                         autoEngState->buf);
    FcitxInputStateSetCursorPos(input, autoEngState->index);
    FcitxInputStateSetClientCursorPos(input, autoEngState->index);
    return IRV_FLAG_UPDATE_INPUT_WINDOW;
}

static void
AutoEngCommit(FcitxAutoEngState *autoEngState)
{
    FcitxInstance *instance = autoEngState->owner;
    FcitxInputContext *currentIC = FcitxInstanceGetCurrentIC(instance);
    FcitxInstanceCommitString(instance, currentIC, autoEngState->buf);
    AutoEngSetBuffLen(autoEngState, 0);
}

static void
AutoEngActivate(FcitxAutoEngState *autoEngState, FcitxInputState* input,
                INPUT_RETURN_VALUE *retval)
{
    FcitxInputStateSetShowCursor(input, false);
    *retval = IRV_DISPLAY_MESSAGE;
    autoEngState->active = true;
    autoEngState->cursor_moved = false;
    ShowAutoEngMessage(autoEngState, retval);
}

static boolean PreInputProcessAutoEng(void* arg, FcitxKeySym sym,
                                      unsigned int state,
                                      INPUT_RETURN_VALUE *retval)
{
    FcitxAutoEngState *autoEngState = (FcitxAutoEngState*)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(autoEngState->owner);
    boolean disableCheckUAZ = FcitxInstanceGetContextBoolean(
        autoEngState->owner, CONTEXT_DISABLE_AUTOENG);
    if (disableCheckUAZ)
        return false;

    FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
    if (!autoEngState->active) {
        if (FcitxHotkeyIsHotKeySimple(sym, state)) {
            AutoEngSetBuff(autoEngState,
                           FcitxInputStateGetRawInputBuffer(input), keymain);
            if (SwitchToEng(autoEngState, autoEngState->buf)) {
                AutoEngActivate(autoEngState, input, retval);
                return true;
            }
        }
        return false;
    }
    if ((*retval = AutoEngCheckSelect(autoEngState, sym, state))) {
        return true;
    } else if (FcitxHotkeyIsHotKeySimple(keymain, state)) {
        *retval = AutoEngPushKey(autoEngState, keymain);
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
        AutoEngSetBuffLen(autoEngState, --autoEngState->index);
        if (autoEngState->index == 0) {
            ResetAutoEng(autoEngState);
            *retval = IRV_CLEAN;
        } else {
            *retval = IRV_DISPLAY_MESSAGE;
        }
    } else if (FcitxHotkeyIsHotkeyCursorMove(sym, state)) {
        *retval = IRV_DO_NOTHING;
        return true;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {
        AutoEngCommit(autoEngState);
        ResetAutoEng(autoEngState);
        *retval = IRV_FLAG_UPDATE_INPUT_WINDOW | IRV_FLAG_RESET_INPUT;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
        *retval = IRV_CLEAN;
        return true;
    }
    ShowAutoEngMessage(autoEngState, retval);
    return true;
}

boolean PostInputProcessAutoEng(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);
    boolean disableCheckUAZ = FcitxInstanceGetContextBoolean(autoEngState->owner, CONTEXT_DISABLE_AUTOENG);
    if (disableCheckUAZ)
        return false;
    if (FcitxHotkeyIsHotKeyUAZ(sym, state) &&
        (FcitxInputStateGetRawInputBufferSize(input) != 0 ||
         (FcitxInputStateGetKeyState(input) & FcitxKeyState_CapsLock) == 0) &&
        AutoEngCheckPreedit(autoEngState)) {
        AutoEngSetBuff(autoEngState, FcitxInputStateGetRawInputBuffer(input),
                       FcitxHotkeyPadToMain(sym));
        AutoEngActivate(autoEngState, input, retval);
        return true;
    }

    return false;
}


void ResetAutoEng(void *arg)
{
    FcitxAutoEngState *autoEngState = (FcitxAutoEngState*)arg;
    autoEngState->index = 0;
    AutoEngSetBuffLen(autoEngState, 0);
    autoEngState->active = false;
    autoEngState->cursor_moved = false;
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
    if (fcitx_unlikely(config->chooseModifier >= _AECM_COUNT))
        config->chooseModifier = _AECM_COUNT - 1;

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

static void
AutoEngFreeList(FcitxAutoEngState* autoEngState)
{
    if (autoEngState->autoEng) {
        utarray_free(autoEngState->autoEng);
        autoEngState->autoEng = NULL;
    }
}

void FreeAutoEng(void* arg)
{
    FcitxAutoEngState *autoEngState = (FcitxAutoEngState*)arg;
    AutoEngFreeList(autoEngState);
    fcitx_utils_free(autoEngState->buf);
    fcitx_utils_free(autoEngState->back_buff);
}

boolean SwitchToEng(FcitxAutoEngState *autoEngState, const char *str)
{
    AUTO_ENG *autoeng;
    if (!AutoEngCheckPreedit(autoEngState))
        return false;
    for (autoeng = (AUTO_ENG*)utarray_front(autoEngState->autoEng);
         autoeng != NULL;
         autoeng = (AUTO_ENG*)utarray_next(autoEngState->autoEng, autoeng))
        if (!strcmp(str, autoeng->str))
            return true;

    return false;
}

static INPUT_RETURN_VALUE
AutoEngGetCandWordCb(void *arg, const char *commit)
{
    FcitxAutoEngState *autoEngState = arg;
    INPUT_RETURN_VALUE res = IRV_DO_NOTHING;
    if (autoEngState->config.maxKeep == 0 &&
        !autoEngState->config.selectAddSpace) {
        return IRV_TO_PROCESS;
    }
    AutoEngSetBuff(autoEngState, commit, '\0');
    if (autoEngState->config.selectAddSpace) {
        autoEngState->auto_space = false;
        res |= AutoEngPushKey(autoEngState, ' ');
        if (!(res & IRV_FLAG_RESET_INPUT)) {
            autoEngState->auto_space = true;
        }
    }
    if (!(res & IRV_FLAG_RESET_INPUT)) {
        ShowAutoEngMessage(autoEngState, &res);
    }
    return res;
}

static void
AutoEngGetSpellHint(FcitxAutoEngState *autoEngState)
{
    FcitxCandidateWordList *candList;
    if (autoEngState->config.disableSpell)
        return;
    candList = FcitxSpellGetCandWords(autoEngState->owner, NULL,
                                      autoEngState->buf, NULL,
                                      autoEngState->config.maxHintLength,
                                      "en", "cus", AutoEngGetCandWordCb,
                                      autoEngState);
    if (candList) {
        FcitxInputState *input = FcitxInstanceGetInputState(autoEngState->owner);
        FcitxCandidateWordList *iList = FcitxInputStateGetCandidateList(input);
        FcitxCandidateWordSetOverrideDefaultHighlight(iList, false);
        FcitxCandidateWordSetChooseAndModifier(
            iList, DIGIT_STR_CHOOSE,
            cmodtable[autoEngState->config.chooseModifier]);
        FcitxCandidateWordMerge(iList, candList, -1);
        FcitxCandidateWordFreeList(candList);
    }
}

#define AUTOENG_MAX_PREEDIT 100

static void
ShowAutoEngMessage(FcitxAutoEngState *autoEngState, INPUT_RETURN_VALUE *retval)
{
    FcitxInputState* input = FcitxInstanceGetInputState(autoEngState->owner);
    char *raw_buff;
    int buff_len;
    FcitxInstanceCleanInputWindow(autoEngState->owner);
    if (autoEngState->buf[0] == '\0')
        return;

    raw_buff = FcitxInputStateGetRawInputBuffer(input);
    buff_len = strlen(autoEngState->buf);
    strncpy(raw_buff, autoEngState->buf, MAX_USER_INPUT);
    if (buff_len > MAX_USER_INPUT) {
        raw_buff[MAX_USER_INPUT] = '\0';
        FcitxInputStateSetRawInputBufferSize(input, MAX_USER_INPUT);
    } else {
        FcitxInputStateSetRawInputBufferSize(input, buff_len);
    }
    if (buff_len > AUTOENG_MAX_PREEDIT) {
        FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input),
                                             MSG_INPUT, autoEngState->buf +
                                             buff_len - AUTOENG_MAX_PREEDIT);
        FcitxInputStateSetCursorPos(input, AUTOENG_MAX_PREEDIT);
    } else {
        FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input),
                                             MSG_INPUT, autoEngState->buf);
        FcitxInputStateSetCursorPos(input, autoEngState->index);
    }
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetClientPreedit(input),
                                         MSG_INPUT, autoEngState->buf);

    FcitxInputStateSetClientCursorPos(input, autoEngState->index);
    FcitxInputStateSetShowCursor(input, true);

    AutoEngGetSpellHint(autoEngState);
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxDown(input),
                                         MSG_TIPS,
                                         _("Press Enter to input text"));
    *retval |= IRV_FLAG_UPDATE_INPUT_WINDOW;
}

void ReloadAutoEng(void* arg)
{
    FcitxAutoEngState *autoEngState = (FcitxAutoEngState*)arg;
    AutoEngFreeList(autoEngState);
    LoadAutoEng(autoEngState);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
