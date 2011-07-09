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
#include "fcitx/backend.h"

typedef struct QuickPhraseState {
    uint uQuickPhraseCount;
    UT_array *quickPhrases ;
    int iFirstQuickPhrase;
    int iLastQuickPhrase;
    QUICK_PHRASE *quickPhraseCandWords[MAX_CAND_WORD];
    boolean enabled;
    FcitxInstance* owner;
} QuickPhraseState;


static void * QuickPhraseCreate (FcitxInstance *instance);
static void LoadQuickPhrase(QuickPhraseState* qpstate);
static void FreeQuickPhrase(void* arg);
static void ReloadQuickPhrase(void* arg);
static INPUT_RETURN_VALUE QuickPhraseDoInput (void* arg, FcitxKeySym sym, int state);
static INPUT_RETURN_VALUE QuickPhraseGetCandWords ();
static UT_icd qp_icd = {sizeof(QUICK_PHRASE), NULL, NULL, NULL};
static void ShowQuickPhraseMessage(QuickPhraseState *qpstate);

FCITX_EXPORT_API
FcitxModule module =
{
    QuickPhraseCreate,
    NULL,
    FreeQuickPhrase,
    ReloadQuickPhrase
};

static boolean QuickPhrasePostFilter(void* arg, long unsigned int sym,
                                     unsigned int state,
                                     INPUT_RETURN_VALUE *retval
                                    );
static boolean QuickPhrasePreFilter(void* arg, long unsigned int sym,
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
    int res,len;
    len = strlen(((QUICK_PHRASE*)a)->strCode);
    res = strncmp(((QUICK_PHRASE*)a)->strCode, ((QUICK_PHRASE*)b)->strCode, len);
    if (res)
        return res;
    else
    {
        return 1;
    }
}

void * QuickPhraseCreate (FcitxInstance *instance)
{
    QuickPhraseState *qpstate = fcitx_malloc0(sizeof(QuickPhraseState));
    qpstate->iFirstQuickPhrase = -1;
    qpstate->owner = instance;
    qpstate->enabled = false;
    LoadQuickPhrase(qpstate);

    KeyFilterHook hk;
    hk.arg = qpstate ;
    hk.func = QuickPhrasePostFilter;
    RegisterPostInputFilter(instance, hk);

    hk.func = QuickPhrasePreFilter;
    RegisterPreInputFilter(instance, hk);

    FcitxIMEventHook resethk;
    resethk.arg = qpstate;
    resethk.func = QuickPhraseReset;
    RegisterResetInputHook(instance, resethk);

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

    qpstate->uQuickPhraseCount=0;

    fp =  GetXDGFileWithPrefix("data", "QuickPhrase.mb", "rt", NULL);
    if (!fp)
        return;

    // 这儿的处理比较简单。因为是单索引对单值。
    // 应该注意的是，它在内存中是以单链表保存的。
    utarray_new(qpstate->quickPhrases, &qp_icd);
    while (getline(&buf, &len, fp) != -1)
    {
        if (buf1)
            free(buf1);
        buf1 = fcitx_trim(buf);
        char *p = buf1;

        while (*p && !isspace(*p))
            p ++;

        if (p == '\0')
            continue;

        while (isspace(*p))
        {
            *p = '\0';
            p ++;
        }

        if (strlen(buf1) >= QUICKPHRASE_CODE_LEN)
            continue;

        if (strlen(p) >= QUICKPHRASE_PHRASE_LEN * UTF8_MAX_LENGTH + 1)
            continue;

        strcpy(tempQuickPhrase.strCode,buf1);
        strcpy(tempQuickPhrase.strPhrase, p);

        utarray_push_back(qpstate->quickPhrases, &tempQuickPhrase);
    }

    if (buf)
        free(buf);
    if (buf1)
        free(buf1);

    if (qpstate->quickPhrases)
    {
        utarray_sort(qpstate->quickPhrases, PhraseCmp);
    }

    fclose(fp);
}

void FreeQuickPhrase(void *arg)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    if ( !qpstate->quickPhrases )
        return;

    utarray_free(qpstate->quickPhrases);
}

void ShowQuickPhraseMessage(QuickPhraseState *qpstate)
{
    FcitxInputState *input = &qpstate->owner->input;
    input->iCursorPos = strlen(input->strCodeInput);
    SetMessageCount(GetMessageUp(qpstate->owner), 0);
    AddMessageAtLast(GetMessageUp(qpstate->owner), MSG_TIPS, _("Quick Phrase: "));
    AddMessageAtLast(GetMessageUp(qpstate->owner), MSG_INPUT, input->strCodeInput);
    input->iCursorPos += strlen(_("Quick Phrase: "));
}

boolean QuickPhrasePreFilter(void* arg, long unsigned int sym,
                             unsigned int state,
                             INPUT_RETURN_VALUE *retval
                            )
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FcitxInputState *input = &qpstate->owner->input;
    FcitxConfig *fc = &qpstate->owner->config;
    if (qpstate->enabled)
    {
        if (IsHotKey(sym, state, fc->hkPrevPage))
            *retval = QuickPhraseGetCandWords(qpstate, SM_PREV);
        else if (IsHotKey(sym, state, fc->hkNextPage))
            *retval = QuickPhraseGetCandWords(qpstate, SM_NEXT);
        else if (IsHotKeySimple(sym, state))
        {
            *retval = QuickPhraseDoInput(qpstate, sym, state);
            if (*retval == IRV_TO_PROCESS)
            {
                if (strlen(input->strCodeInput) == 0
                    && (IsHotKey(sym, state, FCITX_SEMICOLON) || IsHotKey(sym, state, FCITX_SPACE)))
                {
                        strcpy(GetOutputString(input), "；");
                        *retval = IRV_GET_CANDWORDS;
                }
                else {
                    char buf[2];
                    buf[0] = sym;
                    buf[1] = '\0';
                    if (strlen(input->strCodeInput) < MAX_USER_INPUT)
                        strcat(input->strCodeInput, buf);
                    ShowQuickPhraseMessage(qpstate);
                    *retval = QuickPhraseGetCandWords(qpstate, SM_FIRST);
                    if (*retval == IRV_DISPLAY_MESSAGE)
                    {
                        SetMessageCount(GetMessageDown(qpstate->owner), 0);
                        AddMessageAtLast(GetMessageDown(qpstate->owner), MSG_OTHER, _("Press enter to input text"));
                    }
                }
            }
            else
                return true;
        }
        else if (IsHotKey(sym, state, FCITX_BACKSPACE))
        {
            size_t len = strlen(input->strCodeInput);
            if (len > 0)
                input->strCodeInput[--len] = '\0';
            if (len == 0)
            {
                *retval = IRV_CLEAN;
            }
            else
            {
                ShowQuickPhraseMessage(qpstate);
                *retval = QuickPhraseGetCandWords(qpstate, SM_FIRST);
            }
        }
        else if (IsHotKey(sym, state, FCITX_ENTER))
        {
            
            if (strlen(input->strCodeInput) > 0)
            {
                strcpy(GetOutputString(input), input->strCodeInput);
                QuickPhraseReset(qpstate);
                *retval = IRV_GET_CANDWORDS;
            }
            else 
            {
                strcpy(GetOutputString(input), ";");
                *retval = IRV_GET_CANDWORDS;
            }
        }
        else if (IsHotKey(sym, state, FCITX_ESCAPE))
        {
            *retval = IRV_CLEAN;
        }
        else if (IsHotKeyModifierCombine(sym, state))
        {
            return false;
        }
        else
            *retval = IRV_DO_NOTHING;
        return true;
    }
    return false;
}
boolean QuickPhrasePostFilter(void* arg, long unsigned int sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             )
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FcitxInputState *input = &qpstate->owner->input;
    if (!qpstate->enabled
            && input->iCodeInputCount == 0
            && IsHotKey(sym, state, FCITX_SEMICOLON))
    {
        SetMessageCount(GetMessageUp(qpstate->owner), 0);
        AddMessageAtLast(GetMessageUp(qpstate->owner), MSG_TIPS, _("Quick Phrase: "));
        input->iCursorPos += strlen(_("Quick Phrase: "));
        SetMessageCount(GetMessageDown(qpstate->owner), 0);
        AddMessageAtLast(GetMessageDown(qpstate->owner), MSG_TIPS, _("Spcae for ； Enter for;"));

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
INPUT_RETURN_VALUE QuickPhraseDoInput (void* arg, FcitxKeySym sym, int state)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FcitxInputState *input = &qpstate->owner->input;
    FcitxInstance *instance = qpstate->owner;
    int retVal;
    int iKey;

    iKey = sym;

    if (iKey >= '0' && iKey <= '9') {
        if (!input->iCandWordCount)
            return IRV_TO_PROCESS;

        iKey -= '0';
        if (iKey == 0)
            iKey = 10;

        if (iKey > input->iCandWordCount)
            retVal = IRV_DO_NOTHING;
        else {
            strcpy (GetOutputString(input), qpstate->quickPhraseCandWords[iKey-1]->strPhrase);
            retVal = IRV_GET_CANDWORDS;
            SetMessageCount(GetMessageDown(instance), 0);
        }
    }
    else if (iKey==' ') {
        if (!input->iCandWordCount)
            retVal = IRV_TO_PROCESS;
        else {
            strcpy (GetOutputString(input), qpstate->quickPhraseCandWords[0]->strPhrase);
            retVal = IRV_GET_CANDWORDS;
            SetMessageCount(GetMessageDown(instance), 0);
        }
    }
    else
        retVal = IRV_TO_PROCESS;

    return retVal;
}

INPUT_RETURN_VALUE QuickPhraseGetCandWords (QuickPhraseState* qpstate, SEARCH_MODE mode)
{
    int i, iInputLen;
    QUICK_PHRASE searchKey, *pKey, *currentQuickPhrase, *lastQuickPhrase;
    char strTemp[2];
    FcitxInputState *input = &qpstate->owner->input;
    FcitxInstance *instance = qpstate->owner;

    pKey = &searchKey;

    if ( !qpstate->quickPhrases )
        return IRV_DISPLAY_MESSAGE;

    iInputLen = strlen(input->strCodeInput);
    if (iInputLen > QUICKPHRASE_CODE_LEN)
        return IRV_DISPLAY_MESSAGE;

    strcpy(searchKey.strCode, input->strCodeInput);

    if (mode==SM_FIRST) {
        input->iCandPageCount=0;
        input->iCurrentCandPage=0;
        input->iCandWordCount=0;
        currentQuickPhrase = utarray_custom_bsearch(pKey, qpstate->quickPhrases, false, PhraseCmp);
        qpstate->iFirstQuickPhrase = utarray_eltidx(qpstate->quickPhrases, currentQuickPhrase);
        lastQuickPhrase = utarray_custom_bsearch(pKey, qpstate->quickPhrases, false, PhraseCmpA);
        qpstate->iLastQuickPhrase = utarray_eltidx(qpstate->quickPhrases, lastQuickPhrase);
        if (qpstate->iLastQuickPhrase < 0)
            qpstate->iLastQuickPhrase = utarray_len(qpstate->quickPhrases);
        input->iCandPageCount = (qpstate->iLastQuickPhrase - qpstate->iFirstQuickPhrase + ConfigGetMaxCandWord(&instance->config) - 1) / ConfigGetMaxCandWord(&instance->config) - 1;
        if ( !currentQuickPhrase || strncmp(input->strCodeInput,currentQuickPhrase->strCode,iInputLen) ) {
            SetMessageCount(GetMessageDown(instance), 0);
            currentQuickPhrase = NULL;
            return IRV_DISPLAY_MESSAGE;
        }
    }
    else if (mode==SM_NEXT) {
        if (input->iCurrentCandPage >= input->iCandPageCount)
            return IRV_DO_NOTHING;
        input->iCandWordCount=0;
        input->iCurrentCandPage++;
    }
    else {
        if (input->iCurrentCandPage <= 0)
            return IRV_DO_NOTHING;
        input->iCandWordCount=0;
        input->iCurrentCandPage--;
    }

    for ( currentQuickPhrase = (QUICK_PHRASE*) utarray_eltptr(qpstate->quickPhrases,
                               qpstate->iFirstQuickPhrase
                               + input->iCurrentCandPage * ConfigGetMaxCandWord(&instance->config));
            currentQuickPhrase != NULL;
            currentQuickPhrase = (QUICK_PHRASE*) utarray_next(qpstate->quickPhrases, currentQuickPhrase))
    {
        if (!strncmp(input->strCodeInput,currentQuickPhrase->strCode,iInputLen)) {
            qpstate->quickPhraseCandWords[input->iCandWordCount++]=currentQuickPhrase;
            if (input->iCandWordCount==ConfigGetMaxCandWord(&instance->config)) {
                break;
            }
        }
    }

    if (!input->iCandWordCount)
        return IRV_DISPLAY_MESSAGE;

    SetMessageCount(GetMessageDown(instance), 0);
    strTemp[1]='\0';

    for (i = 0; i < input->iCandWordCount; i++) {
        strTemp[0] = i + 1 + '0';
        if (i == 9)
            strTemp[0] = '0';
        AddMessageAtLast(GetMessageDown(instance), MSG_INDEX, "%s", strTemp);
        AddMessageAtLast(GetMessageDown(instance), ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER), "%s", qpstate->quickPhraseCandWords[i]->strPhrase);

        //编码提示
        AddMessageAtLast(GetMessageDown(instance), MSG_CODE, "%s", qpstate->quickPhraseCandWords[i]->strCode + iInputLen);
        if (i != (input->iCandWordCount - 1))
            MessageConcatLast(GetMessageDown(instance), " ");
    }

    return IRV_DISPLAY_CANDWORDS;
}

void ReloadQuickPhrase(void* arg)
{
    QuickPhraseState *qpstate = (QuickPhraseState*) arg;
    FreeQuickPhrase(arg);
    LoadQuickPhrase(qpstate);
}


