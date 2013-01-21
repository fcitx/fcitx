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
#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#include "punc.h"
#include "fcitx/module.h"
#include "fcitx/fcitx.h"
#include "fcitx/hook.h"
#include "fcitx/ime.h"
#include "fcitx/keys.h"
#include "fcitx/frontend.h"
#include "fcitx/instance.h"
#include "fcitx/candidate.h"
#include "fcitx/context.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "fcitx-utils/bitset.h"

/**
 * @file punc.c
 * Trans full width punc for Fcitx
 */

#define PUNC_DICT_FILENAME  "punc.mb"
#define MAX_PUNC_NO     2
#define MAX_PUNC_LENGTH     2

struct _FcitxPuncState;
typedef struct _WidePunc {
    char ASCII;
    char strWidePunc[MAX_PUNC_NO][MAX_PUNC_LENGTH * UTF8_MAX_LENGTH + 1];
    unsigned iCount: 2;
} WidePunc;

typedef struct _PuncWhich {
    FcitxBitSet* bitset;
    WidePunc* lastPunc;
} PuncWhich;

typedef struct _FcitxPunc {
    char* langCode;
    WidePunc* curPunc;

    UT_hash_handle hh;
} FcitxPunc;

static boolean LoadPuncDict(struct _FcitxPuncState* puncState);
static FcitxPunc* LoadPuncFile(const char* filename);
static char *GetPunc(struct _FcitxPuncState* puncState, int iKey);
static void FreePunc(struct _FcitxPuncState* puncState);
static void* PuncCreate(FcitxInstance* instance);
static boolean PuncPreFilter(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal);
static boolean ProcessPunc(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal);
static void TogglePuncState(void *arg);
static boolean GetPuncState(void *arg);
static void ReloadPunc(void *arg);
static INPUT_RETURN_VALUE TogglePuncStateWithHotkey(void *arg);
static void ResetPunc(void *arg);
static void ResetPuncWhichStatus(void* arg);
static boolean IsHotKeyPunc(FcitxKeySym sym, unsigned int state);
static void PuncLanguageChanged(void* arg, const void* value);

static void* PuncWhichAlloc(void* arg);
static void* PuncWhichCopy(void* arg, void* data, void* src);
static void PuncWhichFree(void* arg, void* data);

DECLARE_ADDFUNCTIONS(Punc)

typedef struct _FcitxPuncState {
    char cLastIsAutoConvert;
    boolean bLastIsNumber;
    FcitxInstance* owner;
    FcitxPunc* puncSet;
    WidePunc* curPunc;
    int slot;
} FcitxPuncState;

FCITX_DEFINE_PLUGIN(fcitx_punc, module, FcitxModule) = {
    PuncCreate,
    NULL,
    NULL,
    NULL,
    ReloadPunc
};

void* PuncCreate(FcitxInstance* instance)
{
    FcitxPuncState* puncState = fcitx_utils_malloc0(sizeof(FcitxPuncState));
    puncState->owner = instance;
    LoadPuncDict(puncState);
    FcitxKeyFilterHook hk;
    hk.arg = puncState;
    hk.func = ProcessPunc;

    FcitxInstanceRegisterPostInputFilter(instance, hk);

    hk.func = PuncPreFilter;
    FcitxInstanceRegisterPreInputFilter(instance, hk);

    puncState->cLastIsAutoConvert = '\0';
    puncState->bLastIsNumber = false;

    FcitxHotkeyHook hotkey;
    hotkey.hotkey = FcitxInstanceGetGlobalConfig(instance)->hkPunc;
    hotkey.hotkeyhandle = TogglePuncStateWithHotkey;
    hotkey.arg = puncState;
    FcitxInstanceRegisterHotkeyFilter(instance, hotkey);

    FcitxIMEventHook hook;
    hook.arg = puncState;
    hook.func = ResetPunc;

    FcitxInstanceRegisterResetInputHook(instance, hook);

    hook.func = ResetPuncWhichStatus;

    FcitxInstanceRegisterInputUnFocusHook(instance, hook);

    FcitxInstanceWatchContext(instance, CONTEXT_IM_LANGUAGE,
                              PuncLanguageChanged, puncState);

    FcitxProfile* profile = FcitxInstanceGetProfile(instance);
    FcitxUIRegisterStatus(instance, puncState, "punc",
                          profile->bUseWidePunc ? _("Full width punct") :
                          _("Latin punct"),
                          _("Toggle Full Width Punctuation"), TogglePuncState,
                          GetPuncState);

    puncState->slot = FcitxInstanceAllocDataForIC(instance, PuncWhichAlloc,
                                                  PuncWhichCopy, PuncWhichFree,
                                                  puncState);

    FcitxPuncAddFunctions(instance);
    return puncState;
}

void* PuncWhichAlloc(void *arg)
{
    FcitxPunc *puncState = arg;
    PuncWhich *which = fcitx_utils_new(PuncWhich);
    which->lastPunc = puncState->curPunc;
    which->bitset = fcitx_bitset_new(256);
    return which;
}

void* PuncWhichCopy(void* arg, void* data, void* src)
{
    FCITX_UNUSED(arg);
    PuncWhich *which = data;
    PuncWhich *whichsrc = src;
    which->lastPunc = whichsrc->lastPunc;
    memcpy(which->bitset, whichsrc->bitset, fcitx_bitset_size(256));
    return data;
}

void PuncWhichFree(void* arg, void* data)
{
    FCITX_UNUSED(arg);
    PuncWhich *which = data;
    free(which->bitset);
    free(data);
}

void PuncLanguageChanged(void* arg, const void* value)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    const char* lang = (const char*) value;
    FcitxPunc* punc = NULL;
    if (lang) {
        HASH_FIND_STR(puncState->puncSet, lang, punc);
        if (punc)
            puncState->curPunc = punc->curPunc;
        else
            puncState->curPunc = NULL;
    } else
        puncState->curPunc = NULL;

    FcitxUISetStatusVisable (puncState->owner, "punc",  puncState->curPunc != NULL) ;
}

static void
PuncGetPunc2(FcitxPuncState *puncState, int key, char **p1, char **p2)
{
    int iIndex = 0;
    WidePunc *curPunc = puncState->curPunc;

    if (!curPunc)
        return;

    while (curPunc[iIndex].ASCII) {
        if (curPunc[iIndex].ASCII == key) {
            if (p1)
                *p1 = curPunc[iIndex].strWidePunc[0];
            if (curPunc[iIndex].iCount > 1 && p2)
                *p2 = curPunc[iIndex].strWidePunc[1];
            break;
        }
        iIndex++;
    }
}

void ResetPunc(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    puncState->bLastIsNumber = false;
    puncState->cLastIsAutoConvert = '\0';
}

void ResetPuncWhichStatus(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    WidePunc       *curPunc = puncState->curPunc;

    if (!curPunc)
        return;

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(puncState->owner);
    if (!ic)
        return;
    PuncWhich* puncWhich = FcitxInstanceGetICData(puncState->owner, ic, puncState->slot);
    fcitx_bitset_clear(puncWhich->bitset);
}

boolean PuncPreFilter(void* arg, FcitxKeySym sym, unsigned int state,
                      INPUT_RETURN_VALUE* retVal)
{
    FCITX_UNUSED(retVal);
    FcitxPuncState *puncState = (FcitxPuncState*)arg;
    if (FcitxHotkeyIsHotKeySimple(sym, state) &&
        !FcitxHotkeyIsHotKeyDigit(sym, state) && !IsHotKeyPunc(sym, state))
        puncState->bLastIsNumber = false;
    return false;
}

boolean ProcessPunc(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    FcitxInstance* instance = puncState->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(puncState->owner);
    FcitxProfile* profile = FcitxInstanceGetProfile(instance);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(instance);

    char *pPunc = NULL;

    if (*retVal != IRV_TO_PROCESS)
        return false;

    FcitxCandidateWordList *candList = FcitxInputStateGetCandidateList(input);
    if (FcitxCandidateWordGetListSize(candList) != 0) {
        if (FcitxCandidateWordGetHasGoneToNextPage(candList) &&
            FcitxHotkeyIsHotKey(sym, state,
                                FcitxConfigPrevPageKey(instance, config))) {
            return false;
        }
        /*
         * comparing with upper case, if paging is occupied,
         * punc will not let next page pass
         */
        if (FcitxHotkeyIsHotKey(sym, state,
                                FcitxConfigNextPageKey(instance, config))) {
            return false;
        }
    }

    FcitxKeySym origsym = sym;
    sym = FcitxHotkeyPadToMain(sym);
    if (profile->bUseWidePunc) {
        if (puncState->bLastIsNumber && config->bEngPuncAfterNumber
                && (FcitxHotkeyIsHotKey(origsym, state, FCITX_PERIOD)
                    || FcitxHotkeyIsHotKey(origsym, state, FCITX_SEMICOLON)
                    || FcitxHotkeyIsHotKey(origsym, state, FCITX_COMMA))) {
            puncState->cLastIsAutoConvert = origsym;
            puncState->bLastIsNumber = false;
            *retVal = IRV_DONOT_PROCESS;
            return true;
        }
        if (FcitxHotkeyIsHotKeySimple(sym, state))
            pPunc = GetPunc(puncState, origsym);
    }

    /*
     * 在有候选词未输入的情况下，选择第一个候选词并输入标点
     */
    if (IsHotKeyPunc(sym, state)) {
        FcitxInputStateGetOutputString(input)[0] = '\0';
        INPUT_RETURN_VALUE ret = IRV_TO_PROCESS;
        if (!FcitxInputStateGetIsInRemind(input))
            ret = FcitxCandidateWordChooseByTotalIndex(FcitxInputStateGetCandidateList(input), 0);

        /* if there is nothing to commit */
        if (ret == IRV_TO_PROCESS) {
            if (pPunc) {
                strcat(FcitxInputStateGetOutputString(input), pPunc);
                *retVal = IRV_PUNC;
                FcitxInstanceCleanInputWindow(instance);
                return true;
            } else
                return false;
        } else {
            if (pPunc)
                strcat(FcitxInputStateGetOutputString(input), pPunc);
            else {
                char buf[2] = { sym, 0 };
                strcat(FcitxInputStateGetOutputString(input), buf);
            }

            FcitxInstanceCleanInputWindow(instance);
            *retVal = IRV_PUNC;
            return true;
        }

        return false;
    }

    if (profile->bUseWidePunc) {
        if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)
                && puncState->cLastIsAutoConvert) {
            char *pPunc;

            FcitxInstanceForwardKey(puncState->owner, FcitxInstanceGetCurrentIC(instance), FCITX_PRESS_KEY, sym, state);
            pPunc = GetPunc(puncState, puncState->cLastIsAutoConvert);
            if (pPunc)
                FcitxInstanceCommitString(puncState->owner, FcitxInstanceGetCurrentIC(instance), pPunc);

            puncState->cLastIsAutoConvert = 0;
            *retVal = IRV_DO_NOTHING;
            return true;
        } else if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
            puncState->bLastIsNumber = true;
        } else {
            puncState->bLastIsNumber = false;
        }
    }
    puncState->cLastIsAutoConvert = 0;
    return false;
}

/**
 * 加载标点词典
 * @param void
 * @return void
 * @note 文件中数据的格式为： 对应的英文符号 中文标点 <中文标点>
 * 加载标点词典。标点词典定义了一组标点转换，如输入‘.’就直接转换成‘。’
 */
boolean LoadPuncDict(FcitxPuncState* puncState)
{
    FcitxStringHashSet* puncfiles = FcitxXDGGetFiles("data", PUNC_DICT_FILENAME "." , NULL);
    FcitxStringHashSet *curpuncfile = puncfiles;
    FcitxPunc* punc;
    while (curpuncfile) {
        punc = LoadPuncFile(curpuncfile->name);
        if (punc)
            HASH_ADD_KEYPTR(hh, puncState->puncSet, punc->langCode, strlen(punc->langCode), punc);
        curpuncfile = curpuncfile->hh.next;
    }

    fcitx_utils_free_string_hash_set(puncfiles);
    return true;
}

FcitxPunc* LoadPuncFile(const char* filename)
{
    FILE           *fpDict;             // 词典文件指针
    int             iRecordNo;
    char            strText[4 + MAX_PUNC_LENGTH * UTF8_MAX_LENGTH];
    char           *pstr;               // 临时指针
    int             i;
    fpDict = FcitxXDGGetFileWithPrefix("data", filename, "r", NULL);

    if (strlen(filename) < strlen(PUNC_DICT_FILENAME))
        return NULL;

    if (!fpDict) {
        FcitxLog(WARNING, _("Can't open punc file."));
        return NULL;
    }

    /* 计算词典里面有多少的数据
     * 这个函数非常简单，就是计算该文件有多少行（包含空行）。
     * 因为空行，在下面会略去，所以，这儿存在内存的浪费现象。
     * 没有一个空行就是浪费sizeof (WidePunc)字节内存*/
    iRecordNo = fcitx_utils_calculate_record_number(fpDict);
    // 申请空间，用来存放这些数据。这儿没有检查是否申请到内存，严格说有小隐患
    WidePunc* punc = (WidePunc *) fcitx_utils_malloc0(sizeof(WidePunc) * (iRecordNo + 1));

    iRecordNo = 0;

    // 下面这个循环，就是一行一行的读入词典文件的数据。并将其放入到curPunc里面去。
    for (;;) {
        if (!fgets(strText, (MAX_PUNC_LENGTH * UTF8_MAX_LENGTH + 3), fpDict))
            break;
        i = strlen(strText) - 1;

        // 先找到最后一个字符
        while ((strText[i] == '\n') || (strText[i] == ' ')) {
            if (!i)
                break;
            i--;
        }

        // 如果找到，进行出入。当是空行时，肯定找不到。所以，也就略过了空行的处理
        if (i) {
            strText[i + 1] = '\0';              // 在字符串的最后加个封口
            pstr = strText;                     // 将pstr指向第一个非空字符
            while (*pstr == ' ')
                pstr++;
            punc[iRecordNo].ASCII = *pstr++; // 这个就是中文符号所对应的ASCII码值
            while (*pstr == ' ')                // 然后，将pstr指向下一个非空字符
                pstr++;

            punc[iRecordNo].iCount = 0;      // 该符号有几个转化，比如英文"就可以转换成“和”
            // 依次将该ASCII码所对应的符号放入到结构中
            while (*pstr) {
                i = 0;
                // 因为中文符号都是多字节（这里读取并不像其他地方是固定两个，所以没有问题）的，所以，要一直往后读，知道空格或者字符串的末尾
                while (*pstr != ' ' && *pstr) {
                    punc[iRecordNo].strWidePunc[punc[iRecordNo].iCount][i] = *pstr;
                    i++;
                    pstr++;
                }

                // 每个中文符号用'\0'隔开
                punc[iRecordNo].strWidePunc[punc[iRecordNo].iCount][i] = '\0';
                while (*pstr == ' ')
                    pstr++;
                punc[iRecordNo].iCount++;
            }

            iRecordNo++;
        }
    }

    punc[iRecordNo].ASCII = '\0';
    fclose(fpDict);

    FcitxPunc* p = fcitx_utils_malloc0(sizeof(FcitxPunc));
    p->langCode = "";

    const char* langcode = filename + strlen(PUNC_DICT_FILENAME);
    if (*langcode == '\0')
        p->langCode = strdup("C");
    else
        p->langCode = strdup(langcode + 1);

    p->curPunc = punc;

    return p;
}

void FreePunc(FcitxPuncState* puncState)
{
    puncState->curPunc = NULL;
    FcitxPunc* cur;
    while (puncState->puncSet) {
        cur = puncState->puncSet;
        HASH_DEL(puncState->puncSet, cur);
        free(cur->langCode);
        free(cur->curPunc);
        free(cur);
    }
}

static inline int GetPuncWhich(FcitxPuncState* puncState, WidePunc* punc)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(puncState->owner);
    if (!ic)
        return 0;
    PuncWhich* puncWhich = FcitxInstanceGetICData(puncState->owner, ic, puncState->slot);
    if (puncWhich->lastPunc != puncState->curPunc) {
        fcitx_bitset_clear(puncWhich->bitset);
        puncWhich->lastPunc = puncState->curPunc;
    }
    int result = fcitx_bitset_isset(puncWhich->bitset, punc->ASCII) ? 1 : 0;
    if (result >= punc->iCount)
        result = 0;
    return result;
}

static inline void SetPuncWhich(FcitxPuncState* puncState, WidePunc* punc)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(puncState->owner);
    if (!ic)
        return;
    PuncWhich* puncWhich = FcitxInstanceGetICData(puncState->owner, ic, puncState->slot);
    FcitxBitSet* bitset = puncWhich->bitset;
    if (punc->iCount == 1)
        fcitx_bitset_unset(bitset, punc->ASCII);
    else {
        if (fcitx_bitset_isset(bitset, punc->ASCII))
            fcitx_bitset_unset(bitset, punc->ASCII);
        else
            fcitx_bitset_set(bitset, punc->ASCII);
    }
}

/*
 * 根据字符得到相应的标点符号
 * 如果该字符不在标点符号集中，则返回NULL
 */
static char*
GetPunc(FcitxPuncState *puncState, int iKey)
{
    int iIndex = 0;
    char *pPunc;
    WidePunc *curPunc = puncState->curPunc;

    if (!curPunc)
        return NULL;

    while (curPunc[iIndex].ASCII) {
        if (curPunc[iIndex].ASCII == iKey) {
            pPunc = curPunc[iIndex].strWidePunc[GetPuncWhich(puncState,
                                                             &curPunc[iIndex])];
            SetPuncWhich(puncState, &curPunc[iIndex]);
            return pPunc;
        }
        iIndex++;
    }

    return NULL;
}

void TogglePuncState(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*)arg;
    FcitxInstance* instance = puncState->owner;
    FcitxProfile* profile = FcitxInstanceGetProfile(instance);
    profile->bUseWidePunc = !profile->bUseWidePunc;

    FcitxUISetStatusString(puncState->owner, "punc",
                           profile->bUseWidePunc ? _("Full width punct") :
                           _("Latin punct"),
                           _("Toggle Full Width Punctuation"));
    FcitxProfileSave(profile);
}

INPUT_RETURN_VALUE TogglePuncStateWithHotkey(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*)arg;

    FcitxUIStatus *status = FcitxUIGetStatusByName(puncState->owner, "punc");
    if (status->visible){
        FcitxUIUpdateStatus(puncState->owner, "punc");
        return IRV_DO_NOTHING;
    }
    else
        return IRV_TO_PROCESS;
}

boolean GetPuncState(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    FcitxInstance* instance = puncState->owner;
    FcitxProfile* profile = FcitxInstanceGetProfile(instance);
    return profile->bUseWidePunc;
}

void ReloadPunc(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    FreePunc(puncState);
    LoadPuncDict(puncState);

    PuncLanguageChanged(puncState, FcitxInstanceGetContextString(puncState->owner, CONTEXT_IM_LANGUAGE));
}

boolean IsHotKeyPunc(FcitxKeySym sym, unsigned int state)
{
    if (FcitxHotkeyIsHotKeySimple(sym, state)
            && !FcitxHotkeyIsHotKeyDigit(sym, state)
            && !FcitxHotkeyIsHotKeyLAZ(sym, state)
            && !FcitxHotkeyIsHotKeyUAZ(sym, state)
            && !FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE))
        return true;

    return false;
}
#include "fcitx-punc-addfunctions.h"
