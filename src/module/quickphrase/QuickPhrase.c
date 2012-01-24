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
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <libintl.h>

#include "fcitx/fcitx.h"

#include "QuickPhrase.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/utarray.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"
#include "fcitx/keys.h"
#include "fcitx/frontend.h"
#include "fcitx/candidate.h"
#include "fcitx/context.h"

static INPUT_RETURN_VALUE QuickPhraseGetCandWord(void* arg, FcitxCandidateWord* candWord);

typedef struct _QuickPhraseState {
    unsigned int uQuickPhraseCount;
    UT_array *quickPhrases ;
    int iFirstQuickPhrase;
    int iLastQuickPhrase;
    boolean enabled;
    FcitxInstance* owner;
} QuickPhraseState;

typedef struct _QuickPhraseCand {
    QUICK_PHRASE* cand;
} QuickPhraseCand;

static void * QuickPhraseCreate(FcitxInstance *instance);
static void LoadQuickPhrase(QuickPhraseState* qpstate);
static void FreeQuickPhrase(void* arg);
static void ReloadQuickPhrase(void* arg);
static INPUT_RETURN_VALUE QuickPhraseDoInput(void* arg, FcitxKeySym sym, int state);
static INPUT_RETURN_VALUE QuickPhraseGetCandWords(QuickPhraseState* qpstate);
static UT_icd qp_icd = {sizeof(QUICK_PHRASE), NULL, NULL, NULL};
static void ShowQuickPhraseMessage(QuickPhraseState *qpstate);

FCITX_EXPORT_API
FcitxModule module = {
    QuickPhraseCreate,
    NULL,
    NULL,
    FreeQuickPhrase,
    ReloadQuickPhrase
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static boolean QuickPhrasePostFilter(void* arg, FcitxKeySym sym,
                                     unsigned int state,
                                     INPUT_RETURN_VALUE *retval
                                    );
static boolean QuickPhrasePreFilter(void* arg, FcitxKeySym sym,
                                    unsigned int state,
                                    INPUT_RETURN_VALUE *retval
                                   );
static  void QuickPhraseReset(void* arg);
int PhraseCmp(const void* a, const void* b)
{
    return strcmp(((QUICK_PHRASE*)a)->strCode, ((QUICK_PHRASE*)b)->strCode);
}

int PhraseCmpA(const void* a, const void* b)
{
    int res, len;
    len = strlen(((QUICK_PHRASE*)a)->strCode);
    res = strncmp(((QUICK_PHRASE*)a)->strCode, ((QUICK_PHRASE*)b)->strCode, len);
    if (res)
        return res;
    else {
        return 1;
    }
}

void * QuickPhraseCreate(FcitxInstance *instance)
{
    QuickPhraseState *qpstate = fcitx_utils_malloc0(sizeof(QuickPhraseState));
    qpstate->iFirstQuickPhrase = -1;
    qpstate->owner = instance;
    qpstate->enabled = false;
    LoadQuickPhrase(qpstate);

    FcitxKeyFilterHook hk;
    hk.arg = qpstate ;
    hk.func = QuickPhrasePostFilter;
    FcitxInstanceRegisterPostInputFilter(instance, hk);

    hk.func = QuickPhrasePreFilter;
    FcitxInstanceRegisterPreInputFilter(instance, hk);

    FcitxIMEventHook resethk;
    resethk.arg = qpstate;
    resethk.func = QuickPhraseReset;
    FcitxInstanceRegisterResetInputHook(instance, resethk);
    
    FcitxInstanceRegisterWatchableContext(instance, CONTEXT_DISABLE_QUICKPHRASE, FCT_Boolean, FCF_ResetOnInputMethodChange);

    return qpstate;
}

/**
 * @brief 加载快速输入词典
 * @param void
 * @return void
 * @note 快速输入是在;的行为定义为2,并且输入;以后才可以使用的。
 * 加载快速输入词典.如：输入“zg”就直接出现“中华人民共和国”等等。
 * 文件中每一行数据的定义为：<字符组合> <短语>
 * 如：“zg 中华人民共和国”
 */
void LoadQuickPhrase(QuickPhraseState * qpstate)
{
    FILE *fp;
    char *buf = NULL;
    size_t len = 0;
    char *buf1 = NULL;

    QUICK_PHRASE tempQuickPhrase;

    qpstate->uQuickPhraseCount = 0;

    fp =  FcitxXDGGetFileWithPrefix("data", "QuickPhrase.mb", "rt", NULL);
    if (!fp)
        return;

    // 这儿的处理比较简单。因为是单索引对单值。
    // 应该注意的是，它在内存中是以单链表保存的。
    utarray_new(qpstate->quickPhrases, &qp_icd);
    while (getline(&buf, &len, fp) != -1) {
        if (buf1)
            free(buf1);
        buf1 = fcitx_utils_trim(buf);
        char *p = buf1;

        while (*p && !isspace(*p))
            p ++;

        if (p == '\0')
            continue;

        while (isspace(*p)) {
            *p = '\0';
            p ++;
        }

        if (strlen(buf1) >= QUICKPHRASE_CODE_LEN)
            continue;

        if (strlen(p) >= QUICKPHRASE_PHRASE_LEN * UTF8_MAX_LENGTH + 1)
            continue;

        strcpy(tempQuickPhrase.strCode, buf1);
        strcpy(tempQuickPhrase.strPhrase, p);

        utarray_push_back(qpstate->quickPhrases, &tempQuickPhrase);
    }

    strcpy(tempQuickPhrase.strCode, "\x47\x4e\x4f\x4d\x45");
    strcpy(tempQuickPhrase.strPhrase, "\x48\x6f\x77\x20\x63\x61\x6e\x20\x69\x74\x20\x66\x75\x63\x6b\x20\x66"
           "\x63\x69\x74\x78\x20\x73\x6f\x20\x6d\x61\x6e\x79\x20\x74\x69\x6d\x65\x73"
           "\x3f\x20\x49\x74\x20\x73\x75\x63\x6b\x73\x2e");
    utarray_push_back(qpstate->quickPhrases, &tempQuickPhrase);

    if (buf)
        free(buf);
    if (buf1)
        free(buf1);

    if (qpstate->quickPhrases) {
        utarray_sort(qpstate->quickPhrases, PhraseCmp);
    }

    fclose(fp);
}

void FreeQuickPhrase(void *arg)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    if (!qpstate->quickPhrases)
        return;

    utarray_free(qpstate->quickPhrases);
}

void ShowQuickPhraseMessage(QuickPhraseState *qpstate)
{
    FcitxInputState *input = FcitxInstanceGetInputState(qpstate->owner);
    FcitxInputStateSetCursorPos(input, strlen(FcitxInputStateGetRawInputBuffer(input)));
    FcitxInstanceCleanInputWindowUp(qpstate->owner);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, "%s", _("Quick Phrase: "));
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));
}

boolean QuickPhrasePreFilter(void* arg, FcitxKeySym sym,
                             unsigned int state,
                             INPUT_RETURN_VALUE *retval
                            )
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FcitxInputState *input = FcitxInstanceGetInputState(qpstate->owner);
    if (qpstate->enabled) {
        FcitxKeySym keymain = FcitxHotkeyPadToMain(sym);
        if (FcitxHotkeyIsHotKeySimple(keymain, state)) {
            *retval = QuickPhraseDoInput(qpstate, keymain, state);
            if (*retval == IRV_TO_PROCESS) {
                if (strlen(FcitxInputStateGetRawInputBuffer(input)) == 0
                        && (FcitxHotkeyIsHotKey(keymain, state, FCITX_SEMICOLON) || FcitxHotkeyIsHotKey(keymain, state, FCITX_SPACE))) {
                    strcpy(FcitxInputStateGetOutputString(input), "；");
                    *retval = IRV_COMMIT_STRING;
                } else {
                    char buf[2];
                    buf[0] = keymain;
                    buf[1] = '\0';
                    if (strlen(FcitxInputStateGetRawInputBuffer(input)) < MAX_USER_INPUT)
                        strcat(FcitxInputStateGetRawInputBuffer(input), buf);
                    ShowQuickPhraseMessage(qpstate);
                    *retval = QuickPhraseGetCandWords(qpstate);
                }
            } else
                return true;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
            size_t len = strlen(FcitxInputStateGetRawInputBuffer(input));
            if (len > 0)
                FcitxInputStateGetRawInputBuffer(input)[--len] = '\0';
            if (len == 0) {
                *retval = IRV_CLEAN;
            } else {
                ShowQuickPhraseMessage(qpstate);
                *retval = QuickPhraseGetCandWords(qpstate);
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {

            if (strlen(FcitxInputStateGetRawInputBuffer(input)) > 0) {
                strcpy(FcitxInputStateGetOutputString(input), FcitxInputStateGetRawInputBuffer(input));
                QuickPhraseReset(qpstate);
                *retval = IRV_COMMIT_STRING;
            } else {
                strcpy(FcitxInputStateGetOutputString(input), ";");
                *retval = IRV_COMMIT_STRING;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
            *retval = IRV_CLEAN;
        } else if (FcitxHotkeyIsHotKeyModifierCombine(sym, state)) {
            return false;
        } else
            *retval = IRV_DO_NOTHING;
        if (*retval == IRV_DISPLAY_MESSAGE) {
            FcitxMessagesSetMessageCount(FcitxInputStateGetAuxDown(input), 0);
            if (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input)) == 0)
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, "%s", _("Press Enter to input text"));
        }
        return true;
    }
    return false;
}
boolean QuickPhrasePostFilter(void* arg, FcitxKeySym sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             )
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FcitxInputState *input = FcitxInstanceGetInputState(qpstate->owner);
    boolean disableQuickPhrase = FcitxInstanceGetContextBoolean(qpstate->owner, CONTEXT_DISABLE_QUICKPHRASE);

    if (*retval != IRV_TO_PROCESS)
        return false;

    if (!disableQuickPhrase 
        && !qpstate->enabled
        && FcitxInputStateGetRawInputBufferSize(input) == 0
        && FcitxHotkeyIsHotKey(sym, state, FCITX_SEMICOLON)) {
        FcitxInstanceCleanInputWindow(qpstate->owner);
        FcitxInputStateSetShowCursor(input, true);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, "%s", _("Quick Phrase: "));
        FcitxInputStateSetCursorPos(input, 0);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, "%s", _("Spcae for ； Enter for;"));

        qpstate->enabled = true;
        *retval = IRV_DISPLAY_MESSAGE;

        return true;
    }
    return false;
}

void QuickPhraseReset(void* arg)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    qpstate->enabled = false;
}
INPUT_RETURN_VALUE QuickPhraseDoInput(void* arg, FcitxKeySym sym, int state)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FcitxInputState *input = FcitxInstanceGetInputState(qpstate->owner);
    FcitxGlobalConfig* fc = FcitxInstanceGetGlobalConfig(qpstate->owner);
    int retVal = IRV_TO_PROCESS;
    
    const FcitxHotkey* hkPrevPage = FcitxInstanceGetContextHotkey(qpstate->owner, CONTEXT_ALTERNATIVE_PREVPAGE_KEY);
    if (hkPrevPage == NULL)
        hkPrevPage = fc->hkPrevPage;
    
    const FcitxHotkey* hkNextPage = FcitxInstanceGetContextHotkey(qpstate->owner, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY);
    if (hkNextPage == NULL)
        hkNextPage = fc->hkNextPage;

    if (FcitxHotkeyIsHotKey(sym, state, hkPrevPage)) {
        if (FcitxCandidateWordGoPrevPage(FcitxInputStateGetCandidateList(input)))
            retVal = IRV_DISPLAY_MESSAGE;
    } else if (FcitxHotkeyIsHotKey(sym, state, hkNextPage)) {
        if (FcitxCandidateWordGoNextPage(FcitxInputStateGetCandidateList(input)))
            retVal = IRV_DISPLAY_MESSAGE;
    } else if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
        int iKey = FcitxHotkeyCheckChooseKey(sym, state, DIGIT_STR_CHOOSE);
        if (iKey >= 0)
            retVal = FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), iKey);
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
        if (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0)
            retVal = FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
    }

    return retVal;
}

INPUT_RETURN_VALUE QuickPhraseGetCandWords(QuickPhraseState* qpstate)
{
    int iInputLen;
    QUICK_PHRASE searchKey, *pKey, *currentQuickPhrase, *lastQuickPhrase;
    FcitxInputState *input = FcitxInstanceGetInputState(qpstate->owner);
    FcitxInstance *instance = qpstate->owner;
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(instance);
    
    FcitxInstanceCleanInputWindowDown(qpstate->owner);

    FcitxCandidateWordSetPageSize(FcitxInputStateGetCandidateList(input), config->iMaxCandWord);
    FcitxCandidateWordSetChoose(FcitxInputStateGetCandidateList(input), DIGIT_STR_CHOOSE);

    pKey = &searchKey;

    if (!qpstate->quickPhrases)
        return IRV_DISPLAY_MESSAGE;

    iInputLen = strlen(FcitxInputStateGetRawInputBuffer(input));
    if (iInputLen > QUICKPHRASE_CODE_LEN)
        return IRV_DISPLAY_MESSAGE;

    strcpy(searchKey.strCode, FcitxInputStateGetRawInputBuffer(input));

    FcitxCandidateWordReset(FcitxInputStateGetCandidateList(input));

    currentQuickPhrase = utarray_custom_bsearch(pKey, qpstate->quickPhrases, false, PhraseCmp);
    qpstate->iFirstQuickPhrase = utarray_eltidx(qpstate->quickPhrases, currentQuickPhrase);
    lastQuickPhrase = utarray_custom_bsearch(pKey, qpstate->quickPhrases, false, PhraseCmpA);
    qpstate->iLastQuickPhrase = utarray_eltidx(qpstate->quickPhrases, lastQuickPhrase);
    if (qpstate->iLastQuickPhrase < 0)
        qpstate->iLastQuickPhrase = utarray_len(qpstate->quickPhrases);
    if (!currentQuickPhrase || strncmp(FcitxInputStateGetRawInputBuffer(input), currentQuickPhrase->strCode, iInputLen)) {
        FcitxInstanceCleanInputWindowDown(instance);
        currentQuickPhrase = NULL;
        return IRV_DISPLAY_MESSAGE;
    }

    for (currentQuickPhrase = (QUICK_PHRASE*) utarray_eltptr(qpstate->quickPhrases,
                              qpstate->iFirstQuickPhrase);
            currentQuickPhrase != NULL;
            currentQuickPhrase = (QUICK_PHRASE*) utarray_next(qpstate->quickPhrases, currentQuickPhrase)) {
        if (!strncmp(FcitxInputStateGetRawInputBuffer(input), currentQuickPhrase->strCode, iInputLen)) {
            QuickPhraseCand* qpcand = fcitx_utils_malloc0(sizeof(QuickPhraseCand));
            qpcand->cand = currentQuickPhrase;
            FcitxCandidateWord candWord;
            candWord.callback = QuickPhraseGetCandWord;
            candWord.owner = qpstate;
            candWord.priv = qpcand;
            candWord.strExtra = strdup(currentQuickPhrase->strCode + iInputLen);
            candWord.strWord = strdup(currentQuickPhrase->strPhrase);
            candWord.wordType = MSG_OTHER;
            candWord.extraType = MSG_CODE;
            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
        }
    }

    return IRV_DISPLAY_MESSAGE;
}

void ReloadQuickPhrase(void* arg)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FreeQuickPhrase(arg);
    LoadQuickPhrase(qpstate);
}

INPUT_RETURN_VALUE QuickPhraseGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    QuickPhraseCand* qpcand = candWord->priv;
    FcitxInputState *input = FcitxInstanceGetInputState(qpstate->owner);
    strcpy(FcitxInputStateGetOutputString(input), qpcand->cand->strPhrase);
    return IRV_COMMIT_STRING;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
