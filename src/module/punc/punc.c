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
#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#include "fcitx/module.h"
#include "fcitx/fcitx.h"
#include "fcitx/hook.h"
#include "punc.h"
#include "fcitx/ime.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "fcitx/keys.h"
#include "fcitx/frontend.h"
#include "fcitx/instance.h"
#include "fcitx/candidate.h"

/**
 * @file punc.c
 * @brief Trans full width punc for Fcitx
 */

#define PUNC_DICT_FILENAME  "punc.mb"
#define MAX_PUNC_NO     2
#define MAX_PUNC_LENGTH     2

struct _FcitxPuncState;
typedef struct _WidePunc {
    int             ASCII;
    char            strWidePunc[MAX_PUNC_NO][MAX_PUNC_LENGTH * UTF8_MAX_LENGTH + 1];
unsigned        iCount:
    2;
unsigned        iWhich:
    2;
} WidePunc;

static boolean LoadPuncDict (struct _FcitxPuncState* puncState);
static char *GetPunc (struct _FcitxPuncState* puncState, int iKey);
static void FreePunc (struct _FcitxPuncState* puncState);
static void* PuncCreate(FcitxInstance* instance);
static boolean ProcessPunc(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal);
static void* PuncGetPunc(void* a, FcitxModuleFunctionArg arg);
static void TogglePuncState(void *arg);
static boolean GetPuncState(void *arg);
static void ReloadPunc(void *arg);
static INPUT_RETURN_VALUE TogglePuncStateWithHotkey(void *arg);
static void ResetPunc(void *arg);
static boolean IsHotKeyPunc(FcitxKeySym sym, unsigned int state);


typedef struct _FcitxPuncState {
    char cLastIsAutoConvert;
    boolean bLastIsNumber;
    FcitxInstance* owner;
    WidePunc* chnPunc;
} FcitxPuncState;

FCITX_EXPORT_API
FcitxModule module = {
    PuncCreate,
    NULL,
    NULL,
    NULL,
    ReloadPunc
};

void* PuncCreate(FcitxInstance* instance)
{
    FcitxPuncState* puncState = fcitx_malloc0(sizeof(FcitxPuncState));
    FcitxAddon* puncaddon = GetAddonByName(&instance->addons, FCITX_PUNC_NAME);
    puncState->owner = instance;
    LoadPuncDict(puncState);
    KeyFilterHook hk;
    hk.arg = puncState;
    hk.func = ProcessPunc;

    RegisterPostInputFilter(instance, hk);

    puncState->cLastIsAutoConvert = '\0';
    puncState->bLastIsNumber = false;

    HotkeyHook hotkey;
    hotkey.hotkey = instance->config->hkPunc;
    hotkey.hotkeyhandle = TogglePuncStateWithHotkey;
    hotkey.arg = puncState;
    RegisterHotkeyFilter(instance, hotkey);

    FcitxIMEventHook hook;
    hook.arg = puncState;
    hook.func = ResetPunc;

    RegisterResetInputHook(instance, hook);

    RegisterStatus(instance, puncState, "punc", "Full Width Punctuation", "Full Width Punctuation", TogglePuncState, GetPuncState);

    AddFunction(puncaddon, PuncGetPunc);
    return puncState;
}

void* PuncGetPunc(void* a, FcitxModuleFunctionArg arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) a;
    int *key = arg.args[0];
    return GetPunc(puncState, *key);
}

void ResetPunc(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    puncState->bLastIsNumber = false;
    puncState->cLastIsAutoConvert = '\0';

}

boolean ProcessPunc(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retVal)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    FcitxInstance* instance = puncState->owner;
    FcitxInputState* input = &puncState->owner->input;
    char *pPunc = NULL;

    if (*retVal != IRV_TO_PROCESS)
        return false;

    FcitxKeySym origsym = sym;
    sym = KeyPadToMain(sym);
    if (instance->profile->bUseWidePunc) {

        if (puncState->bLastIsNumber && instance->config->bEngPuncAfterNumber
                && (IsHotKey(origsym, state, FCITX_PERIOD)
                    || IsHotKey(origsym, state, FCITX_SEMICOLON)
                    || IsHotKey(origsym, state, FCITX_COMMA)))
        {
            puncState->cLastIsAutoConvert = origsym;
            puncState->bLastIsNumber = false;
            *retVal = IRV_DONOT_PROCESS;
            return true;
        }
        if (IsHotKeySimple(sym, state))
            pPunc = GetPunc(puncState, origsym);
    }

    /*
     * 在有候选词未输入的情况下，选择第一个候选词并输入标点
     */
    if (IsHotKeyPunc(sym, state)) {
        GetOutputString(input)[0] = '\0';
        INPUT_RETURN_VALUE ret = IRV_TO_PROCESS;
        if (!puncState->owner->input.bIsInRemind)
            ret = CandidateWordChooseByIndex(input->candList, 0);

        /* if there is nothing to commit */
        if (ret == IRV_TO_PROCESS)
        {
            if (pPunc)
            {
                strcat(GetOutputString(input), pPunc);
                *retVal = IRV_PUNC;
                CleanInputWindow(instance);
                return true;
            }
            else
                return false;
        }
        else
        {
            if (pPunc)
                strcat(GetOutputString(input), pPunc);
            else
            {
                char buf[2] = { sym, 0 };
                strcat(GetOutputString(input), buf);
            }

            CleanInputWindow(instance);
            *retVal = IRV_PUNC;
            return true;
        }

        return false;
    }

    if (instance->profile->bUseWidePunc)
    {
        if (IsHotKey(sym, state, FCITX_BACKSPACE)
                 && puncState->cLastIsAutoConvert) {
            char *pPunc;

            ForwardKey(puncState->owner, GetCurrentIC(instance), FCITX_PRESS_KEY, sym, state);
            pPunc = GetPunc(puncState, puncState->cLastIsAutoConvert);
            if (pPunc)
                CommitString(puncState->owner, GetCurrentIC(instance), pPunc);

            puncState->cLastIsAutoConvert = 0;
            *retVal = IRV_DO_NOTHING;
            return true;
        } else if (IsHotKeySimple(sym, state)) {
            if (IsHotKeyDigit(sym, state))
                puncState->bLastIsNumber = true;
            else {
                puncState->bLastIsNumber = false;
            }
        }
    }
    puncState->cLastIsAutoConvert = 0;
    return false;
}

/**
 * @brief 加载标点词典
 * @param void
 * @return void
 * @note 文件中数据的格式为： 对应的英文符号 中文标点 <中文标点>
 * 加载标点词典。标点词典定义了一组标点转换，如输入‘.’就直接转换成‘。’
 */
boolean LoadPuncDict (FcitxPuncState* puncState)
{
    FILE           *fpDict;             // 词典文件指针
    int             iRecordNo;
    char            strText[4 + MAX_PUNC_LENGTH * UTF8_MAX_LENGTH];
    char           *pstr;               // 临时指针
    int             i;

    fpDict = GetXDGFileWithPrefix("data", PUNC_DICT_FILENAME, "rt", NULL);

    if (!fpDict) {
        FcitxLog(WARNING, _("Can't open Chinese punc file."));
        return false;
    }

    /* 计算词典里面有多少的数据
     * 这个函数非常简单，就是计算该文件有多少行（包含空行）。
     * 因为空行，在下面会略去，所以，这儿存在内存的浪费现象。
     * 没有一个空行就是浪费sizeof (WidePunc)字节内存*/
    iRecordNo = CalculateRecordNumber (fpDict);
    // 申请空间，用来存放这些数据。这儿没有检查是否申请到内存，严格说有小隐患
    puncState->chnPunc = (WidePunc *) malloc (sizeof (WidePunc) * (iRecordNo + 1));

    iRecordNo = 0;

    // 下面这个循环，就是一行一行的读入词典文件的数据。并将其放入到chnPunc里面去。
    for (;;) {
        if (!fgets (strText, (MAX_PUNC_LENGTH * UTF8_MAX_LENGTH + 3), fpDict))
            break;
        i = strlen (strText) - 1;

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
            puncState->chnPunc[iRecordNo].ASCII = *pstr++; // 这个就是中文符号所对应的ASCII码值
            while (*pstr == ' ')                // 然后，将pstr指向下一个非空字符
                pstr++;

            puncState->chnPunc[iRecordNo].iCount = 0;      // 该符号有几个转化，比如英文"就可以转换成“和”
            puncState->chnPunc[iRecordNo].iWhich = 0;      // 标示该符号的输入状态，即处于第几个转换。如"，iWhich标示是转换成“还是”
            // 依次将该ASCII码所对应的符号放入到结构中
            while (*pstr) {
                i = 0;
                // 因为中文符号都是多字节（这里读取并不像其他地方是固定两个，所以没有问题）的，所以，要一直往后读，知道空格或者字符串的末尾
                while (*pstr != ' ' && *pstr) {
                    puncState->chnPunc[iRecordNo].strWidePunc[puncState->chnPunc[iRecordNo].iCount][i] = *pstr;
                    i++;
                    pstr++;
                }

                // 每个中文符号用'\0'隔开
                puncState->chnPunc[iRecordNo].strWidePunc[puncState->chnPunc[iRecordNo].iCount][i] = '\0';
                while (*pstr == ' ')
                    pstr++;
                puncState->chnPunc[iRecordNo].iCount++;
            }

            iRecordNo++;
        }
    }

    puncState->chnPunc[iRecordNo].ASCII = '\0';
    fclose (fpDict);

    return true;
}

void FreePunc (FcitxPuncState* puncState)
{
    if (!puncState->chnPunc)
        return;

    free (puncState->chnPunc);
    puncState->chnPunc = (WidePunc *) NULL;
}

/*
 * 根据字符得到相应的标点符号
 * 如果该字符不在标点符号集中，则返回NULL
 */
char           *GetPunc (FcitxPuncState* puncState, int iKey)
{
    int             iIndex = 0;
    char           *pPunc;
    WidePunc       *chnPunc = puncState->chnPunc;

    if (!chnPunc)
        return (char *) NULL;

    while (chnPunc[iIndex].ASCII) {
        if (chnPunc[iIndex].ASCII == iKey) {
            pPunc = chnPunc[iIndex].strWidePunc[chnPunc[iIndex].iWhich];
            chnPunc[iIndex].iWhich++;
            if (chnPunc[iIndex].iWhich >= chnPunc[iIndex].iCount)
                chnPunc[iIndex].iWhich = 0;
            return pPunc;
        }
        iIndex++;
    }

    return (char *) NULL;
}

void TogglePuncState(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState* )arg;
    FcitxInstance* instance = puncState->owner;
    instance->profile->bUseWidePunc = !instance->profile->bUseWidePunc;
    SaveProfile(instance->profile);
    ResetInput(puncState->owner);
}

INPUT_RETURN_VALUE TogglePuncStateWithHotkey(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState* )arg;
    UpdateStatus(puncState->owner, "punc");
    return IRV_DO_NOTHING;
}

boolean GetPuncState(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    FcitxInstance* instance = puncState->owner;
    return instance->profile->bUseWidePunc;
}

void ReloadPunc(void* arg)
{
    FcitxPuncState* puncState = (FcitxPuncState*) arg;
    FreePunc(puncState);
    LoadPuncDict(puncState);
}

boolean IsHotKeyPunc(FcitxKeySym sym, unsigned int state)
{
    if (IsHotKeySimple(sym, state)
        && !IsHotKeyDigit(sym, state)
        && !IsHotKeyLAZ(sym, state)
        && !IsHotKeyUAZ(sym, state)
        && !IsHotKey(sym, state, FCITX_SPACE))
        return true;

    return false;
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
