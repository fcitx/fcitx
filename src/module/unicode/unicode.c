/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#include <libintl.h>

#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"
#include "fcitx/context.h"
#include "fcitx/candidate.h"
#include <fcitx/keys.h>
#include "fcitx-config/xdg.h"
#include "charselectdata.h"

typedef struct _UnicodeModule {
    FcitxGenericConfig gconfig;
    FcitxHotkey key[2];
    boolean enable;
    CharSelectData* charselectdata;
    char buffer[MAX_USER_INPUT * UTF8_MAX_LENGTH + 1];
    FcitxInstance* owner;
    boolean loaded;
} UnicodeModule;

static void* UnicodeCreate(FcitxInstance* instance);
boolean UnicodePreFilter(void* arg, FcitxKeySym sym, unsigned int state,
                             INPUT_RETURN_VALUE *r);
void UnicodeReloadConfig(void* arg);
void UnicodeReset(void* arg);
INPUT_RETURN_VALUE UnicodeHotkey(void* arg);
INPUT_RETURN_VALUE UnicodeGetCandWords(UnicodeModule* uni);

FCITX_DEFINE_PLUGIN(fcitx_unicode, module, FcitxModule) = {
    UnicodeCreate,
    NULL,
    NULL,
    NULL,
    UnicodeReloadConfig
};

CONFIG_BINDING_BEGIN(UnicodeModule)
CONFIG_BINDING_REGISTER("Unicode", "Key", key)
CONFIG_BINDING_END()

CONFIG_DEFINE_LOAD_AND_SAVE(Unicode, UnicodeModule, "fcitx-unicode")

void* UnicodeCreate(FcitxInstance* instance)
{
    UnicodeModule* uni = fcitx_utils_new(UnicodeModule);
    uni->owner = instance;
    if (!UnicodeLoadConfig(uni)) {
        free(uni);
        return NULL;
    }

    FcitxIMEventHook imhk;
    imhk.arg = uni;
    imhk.func = UnicodeReset;
    FcitxInstanceRegisterResetInputHook(instance, imhk);

    FcitxKeyFilterHook kfhk;
    kfhk.arg = uni;
    kfhk.func = UnicodePreFilter;
    FcitxInstanceRegisterPreInputFilter(instance, kfhk);

    FcitxHotkeyHook hkhk;
    hkhk.arg = uni;
    hkhk.hotkey = uni->key;
    hkhk.hotkeyhandle = UnicodeHotkey;
    FcitxInstanceRegisterHotkeyFilter(instance, hkhk);

    return uni;
}

boolean UnicodePreFilter(void* arg, FcitxKeySym sym, unsigned int state,
                         INPUT_RETURN_VALUE *r)
{
    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    do {
        UnicodeModule *uni = arg;
        if (!uni->enable)
            break;
        FcitxInstance *instance = uni->owner;
        FcitxInputState *input = FcitxInstanceGetInputState(instance);
        FcitxGlobalConfig *fc = FcitxInstanceGetGlobalConfig(instance);
        FcitxCandidateWordList *candList;
        candList = FcitxInputStateGetCandidateList(input);

        FcitxCandidateWordSetPageSize(candList, 4);
        FcitxCandidateWordSetChooseAndModifier(candList, DIGIT_STR_CHOOSE,
                                               FcitxKeyState_Alt);
        if (FcitxHotkeyIsHotKey(sym, state,
                                FcitxConfigPrevPageKey(instance, fc))) {
            if (FcitxCandidateWordGoPrevPage(candList))
                retVal = IRV_DISPLAY_MESSAGE;
            else
                retVal = IRV_DO_NOTHING;
        } else if (FcitxHotkeyIsHotKey(sym, state,
                                       FcitxConfigNextPageKey(instance, fc))) {
            if (FcitxCandidateWordGoNextPage(candList))
                retVal = IRV_DISPLAY_MESSAGE;
            else
                retVal = IRV_DO_NOTHING;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
            size_t len = strlen(uni->buffer);
            if (len > 0)
                uni->buffer[--len] = '\0';
            if (len == 0) {
                retVal = IRV_CLEAN;
            } else {
                retVal = UnicodeGetCandWords(uni);
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
            retVal = IRV_CLEAN;
        }

        if (retVal == IRV_TO_PROCESS) {
            int index = FcitxCandidateWordCheckChooseKey(candList, sym, state);
            if (index >= 0)
                retVal = FcitxCandidateWordChooseByIndex(candList, index);
        }

        FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
        if (retVal == IRV_TO_PROCESS && FcitxHotkeyIsHotKeySimple(keymain, state)) {
            char buf[2];
            buf[0] = keymain;
            buf[1] = '\0';
            if (strlen(uni->buffer) < MAX_USER_INPUT)
                strcat(uni->buffer, buf);
            retVal = UnicodeGetCandWords(uni);
        }
    } while(0);

    *r = retVal;
    if (retVal == IRV_TO_PROCESS)
        return false;
    return true;
}

INPUT_RETURN_VALUE UnicodeGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    UnicodeModule* uni = arg;
    FcitxInputState *input = FcitxInstanceGetInputState(uni->owner);
    strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
    return IRV_COMMIT_STRING;
}

INPUT_RETURN_VALUE UnicodeGetCandWords(UnicodeModule* uni)
{
    FcitxInputState *input = FcitxInstanceGetInputState(uni->owner);
    FcitxInstanceCleanInputWindow(uni->owner);
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input),
                                         MSG_INPUT, uni->buffer);
    FcitxInputStateSetShowCursor(input, true);
    FcitxInputStateSetCursorPos(input, strlen(uni->buffer));

    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordSetLayoutHint(candList, CLH_Vertical);

    UT_array* result = CharSelectDataFind(uni->charselectdata, uni->buffer);
    utarray_foreach(c, result, uint16_t) {
        char* s = fcitx_utils_malloc0(sizeof(char) * (UTF8_MAX_LENGTH + 1));
        fcitx_ucs4_to_utf8(*c, s);
        FcitxCandidateWord candWord;
        candWord.callback = UnicodeGetCandWord;
        candWord.owner = uni;
        candWord.priv = NULL;
        candWord.extraType = MSG_OTHER;
        candWord.wordType = MSG_CODE;
        candWord.strWord = s;
        char* name = CharSelectDataName(uni->charselectdata, *c);
        fcitx_utils_alloc_cat_str(candWord.strExtra, " ", name);
        free(name);
        FcitxCandidateWordAppend(candList, &candWord);
    }
    utarray_free(result);
    return IRV_FLAG_UPDATE_INPUT_WINDOW;
}

void UnicodeReloadConfig(void* arg)
{
    UnicodeModule* uni = arg;
    UnicodeLoadConfig(uni);
}

void UnicodeReset(void* arg)
{
    UnicodeModule* uni = arg;
    uni->enable = false;
    uni->buffer[0] = '\0';
}

INPUT_RETURN_VALUE UnicodeHotkey(void* arg)
{
    UnicodeModule* uni = arg;

    if (!uni->loaded) {
        uni->charselectdata = CharSelectDataCreate();
        uni->loaded = true;
    }

    if (!uni->charselectdata)
        return IRV_TO_PROCESS;
    uni->enable = true;

    FcitxInstanceCleanInputWindow(uni->owner);
    FcitxInputState *input = FcitxInstanceGetInputState(uni->owner);
    FcitxInputStateSetShowCursor(input, false);
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input),
                                         MSG_TIPS, _("Search unicode"));
    return IRV_DISPLAY_MESSAGE;
}
