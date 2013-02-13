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
#include "config.h"

#include <libintl.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/utils.h"
#include "fcitx/ime.h"
#include "fcitx/keys.h"
#include "fcitx/ui.h"
#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "fcitx/context.h"
#include "fcitx/profile.h"
#include "fcitx/configfile.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/utf8.h"
#include "fcitx-utils/log.h"
#include "py.h"
#include "PYFA.h"
#include "pyParser.h"
#include "sp.h"
#include "pyconfig.h"
#include "spdata.h"
#include "module/quickphrase/fcitx-quickphrase.h"

#define PY_INDEX_MAGIC_NUMBER 0xf7462e34
#define PINYIN_TEMP_FILE "pinyin_XXXXXX"

typedef struct {
    PY_CAND_WORD_TYPE type;
    ADJUSTORDER order;
    FcitxPinyinState* pystate;
} PYCandWordSortContext;

static void LoadPYPhraseDict(FcitxPinyinState* pystate, FILE* fp,
                             boolean isSystem, boolean stripDup);
static void ReloadConfigPY(void* arg);
static void PinyinMigration();
static int PYCandWordCmp(const void* b, const void* a, void* arg);
static boolean PYGetPYMapByHZ(FcitxPinyinState*pystate, char *strHZ,
                              char* mapHint, char *strMap);

FCITX_DEFINE_PLUGIN(fcitx_pinyin, ime2, FcitxIMClass2) = {
    PYCreate,
    PYDestroy,
    ReloadConfigPY,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

DECLARE_ADDFUNCTIONS(Pinyin)

void *PYCreate(FcitxInstance* instance)
{
    FcitxPinyinState *pystate = fcitx_utils_new(FcitxPinyinState);
    InitMHPY(&pystate->pyconfig.MHPY_C, MHPY_C_TEMPLATE);
    InitMHPY(&pystate->pyconfig.MHPY_S, MHPY_S_TEMPLATE);
    InitPYTable(&pystate->pyconfig);
    InitPYSplitData(&pystate->pyconfig);
    if (!LoadPYConfig(&pystate->pyconfig)) {
        free(pystate->pyconfig.MHPY_C);
        free(pystate->pyconfig.MHPY_S);
        free(pystate->pyconfig.PYTable);
        FreePYSplitData(&pystate->pyconfig);
        free(pystate);
        return NULL;
    }

    PinyinMigration();

    pystate->pool = fcitx_memory_pool_create();

    FcitxInstanceRegisterIM(instance,
                            pystate,
                            "pinyin",
                            _("Pinyin"),
                            "pinyin",
                            PYInit,
                            ResetPYStatus,
                            DoPYInput,
                            PYGetCandWords,
                            NULL,
                            SavePY,
                            NULL,
                            NULL,
                            5,
                            "zh_CN");
    FcitxInstanceRegisterIM(instance,
                            pystate,
                            "shuangpin",
                            _("Shuangpin"),
                            "shuangpin",
                            SPInit,
                            ResetPYStatus,
                            DoPYInput,
                            PYGetCandWords,
                            NULL,
                            SavePY,
                            NULL,
                            NULL,
                            5,
                            "zh_CN");
    pystate->owner = instance;

    FcitxPinyinAddFunctions(instance);
    return pystate;
}

void PYDestroy(void* arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    free(pystate->pyconfig.MHPY_C);
    free(pystate->pyconfig.MHPY_S);
    free(pystate->pyconfig.PYTable);
    FreePYSplitData(&pystate->pyconfig);
    FcitxConfigFree(&pystate->pyconfig.gconfig);
    fcitx_memory_pool_destroy(pystate->pool);

    int i, j, k;
    PYFA *PYFAList = pystate->PYFAList;
    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            PyPhrase* phrase = USER_PHRASE_NEXT(PYFAList[i].pyBase[j].userPhrase);
            for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
                PyPhrase* cur = phrase;
                fcitx_utils_free(cur->strPhrase);
                fcitx_utils_free(cur->strMap);
                phrase = USER_PHRASE_NEXT(phrase);
                free(cur);
            }

            free(PYFAList[i].pyBase[j].userPhrase);
            fcitx_utils_free(PYFAList[i].pyBase[j].phrase);
        }
        free(PYFAList[i].pyBase);
    }
    free(PYFAList);

    while(pystate->pyFreq) {
        PyFreq* pCurFreq = pystate->pyFreq;
        pystate->pyFreq = pCurFreq->next;
        while (pCurFreq->HZList) {
            HZ* pHZ = pCurFreq->HZList;
            pCurFreq->HZList = pHZ->next;
            free(pHZ);
        }
        free(pCurFreq);
    }

    free(pystate);
}

boolean PYInit(void *arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    boolean flag = true;
    FcitxInstanceSetContext(pystate->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    FcitxInstanceSetContext(pystate->owner, CONTEXT_SHOW_REMIND_STATUS, &flag);
    pystate->bSP = false;
    return true;
}

boolean SPInit(void *arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    boolean flag = true;
    FcitxInstanceSetContext(pystate->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    FcitxInstanceSetContext(pystate->owner, CONTEXT_SHOW_REMIND_STATUS, &flag);
    pystate->bSP = true;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;
    pyconfig->cNonS = 'o';
    memcpy(pyconfig->SPMap_S, SPMap_S_Ziranma, sizeof(SPMap_S_Ziranma));
    memcpy(pyconfig->SPMap_C, SPMap_C_Ziranma, sizeof(SPMap_C_Ziranma));

    LoadSPData(pystate);
    return true;
}

boolean LoadPYBaseDict(FcitxPinyinState *pystate)
{
    FILE *fp;
    int i, j;
    int32_t iLen;

    fp = FcitxXDGGetFileWithPrefix("pinyin", PY_BASE_FILE, "r", NULL);
    if (!fp)
        return false;

    fcitx_utils_read_int32(fp, &pystate->iPYFACount);
    pystate->PYFAList = (PYFA*)fcitx_utils_malloc0(sizeof(PYFA) * pystate->iPYFACount);
    PYFA *PYFAList = pystate->PYFAList;
    for (i = 0; i < pystate->iPYFACount; i++) {
        fread(PYFAList[i].strMap, sizeof(char) * 2, 1, fp);
        PYFAList[i].strMap[2] = '\0';

        fcitx_utils_read_int32(fp, &PYFAList[i].iBase);
        PYFAList[i].pyBase = (PyBase*)fcitx_utils_malloc0(sizeof(PyBase) * PYFAList[i].iBase);
        for (j = 0; j < PYFAList[i].iBase; j++) {
            int8_t len;
            fread(&len, sizeof(char), 1, fp);
            fread(PYFAList[i].pyBase[j].strHZ, sizeof(char) * len, 1, fp);
            PYFAList[i].pyBase[j].strHZ[len] = '\0';
            fcitx_utils_read_int32(fp, &iLen);
            PYFAList[i].pyBase[j].iIndex = iLen;
            PYFAList[i].pyBase[j].iHit = 0;
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            PYFAList[i].pyBase[j].iPhrase = 0;
            PYFAList[i].pyBase[j].iUserPhrase = 0;
            PYFAList[i].pyBase[j].userPhrase = fcitx_utils_new(PyUsrPhrase);
            PYFAList[i].pyBase[j].userPhrase->next = PYFAList[i].pyBase[j].userPhrase;
        }
    }

    fclose(fp);
    pystate->bPYBaseDictLoaded = true;

    pystate->iOrigCounter = pystate->iCounter;

    pystate->pyFreq = fcitx_utils_new(PyFreq);
    // pystate->pyFreq->next = NULL;

    return true;
}

void LoadPYPhraseDict(FcitxPinyinState* pystate, FILE *fp, boolean isSystem, boolean stripDup)
{
    int j, k;
    int32_t i, count, iLen;
    char strBase[UTF8_MAX_LENGTH + 1];
    PyPhrase *phrase = NULL, *temp;
    PYFA* PYFAList = pystate->PYFAList;
    while (!feof(fp)) {
        int8_t clen;
        if (!fcitx_utils_read_int32(fp, &i))
            break;
        if (!fread(&clen, sizeof(int8_t), 1, fp))
            break;
        if (clen <= 0 || clen > UTF8_MAX_LENGTH)
            break;
        if (!fread(strBase, sizeof(char) * clen, 1, fp))
            break;
        strBase[clen] = '\0';
        if (!fcitx_utils_read_int32(fp, &count))
            break;

        j = GetBaseIndex(pystate, i, strBase);
        if (j == -1)
            break;

        if (isSystem) {
            phrase = (PyPhrase*)fcitx_utils_malloc0(sizeof(PyPhrase) * count);
            temp = phrase;
        } else {
            PYFAList[i].pyBase[j].iUserPhrase = count;
            temp = &PYFAList[i].pyBase[j].userPhrase->phrase;
        }

        for (k = 0; k < count; k++) {
            if (!isSystem)
                phrase = (PyPhrase*)fcitx_utils_malloc0(sizeof(PyUsrPhrase));

            fcitx_utils_read_int32(fp, &iLen);

            if (isSystem) {
                phrase->strMap = (char*)fcitx_memory_pool_alloc(pystate->pool, sizeof(char) * (iLen + 1));
            } else {
                phrase->strMap = (char*)fcitx_utils_malloc0(sizeof(char) * (iLen + 1));
            }
            fread(phrase->strMap, sizeof(char) * iLen, 1, fp);
            phrase->strMap[iLen] = '\0';

            fcitx_utils_read_int32(fp, &iLen);

            if (isSystem) {
                phrase->strPhrase = (char*)fcitx_memory_pool_alloc(pystate->pool, sizeof(char) * (iLen + 1));
            } else {
                phrase->strPhrase = (char*)fcitx_utils_malloc0(sizeof(char) * (iLen + 1));
            }
            fread(phrase->strPhrase, sizeof(char) * iLen, 1, fp);
            phrase->strPhrase[iLen] = '\0';

            fcitx_utils_read_int32(fp, &iLen);
            phrase->iIndex = iLen;
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            if (isSystem) {
                phrase->iHit = 0;
                phrase ++;
            } else {
                fcitx_utils_read_int32(fp, &iLen);
                phrase->iHit = iLen;

                ((PyUsrPhrase*)phrase)->next = ((PyUsrPhrase*) temp)->next;
                ((PyUsrPhrase*)temp)->next = (PyUsrPhrase*) phrase;

                temp = phrase;
            }
        }

        if (isSystem) {
            if (PYFAList[i].pyBase[j].iPhrase == 0) {
                PYFAList[i].pyBase[j].iPhrase = count;
                PYFAList[i].pyBase[j].phrase = temp;
            } else {
                int m, n;
                boolean *flag = fcitx_utils_malloc0(sizeof(boolean) * count);
                int left = count;
                phrase = temp;
                if (stripDup) {
                    for (m = 0; m < count; m++) {
                        for (n = 0; n < PYFAList[i].pyBase[j].iPhrase; n++) {
                            int result = strcmp(PYFAList[i].pyBase[j].phrase[n].strMap, phrase[m].strMap);
                            if (result == 0) {
                                if (strcmp(PYFAList[i].pyBase[j].phrase[n].strPhrase, phrase[m].strPhrase) == 0)
                                    break;
                            }
                        }
                        if (n != PYFAList[i].pyBase[j].iPhrase) {
                            flag[m] = 1;
                            left -- ;
                        }
                    }
                }
                int orig = PYFAList[i].pyBase[j].iPhrase;
                if (left >= 0) {
                    PYFAList[i].pyBase[j].iPhrase += left;
                    PYFAList[i].pyBase[j].phrase = realloc(PYFAList[i].pyBase[j].phrase, sizeof(PyPhrase) * PYFAList[i].pyBase[j].iPhrase);
                    //memcpy(phrase, oldph, sizeof(PyPhrase) * old);
                }
                for (m = 0; m < count; m ++) {
                    if (!flag[m]) {
                        memcpy(&PYFAList[i].pyBase[j].phrase[orig], &phrase[m], sizeof(PyPhrase));
                        orig ++ ;
                    }
                    else {
                        // free(phrase[m].strMap);
                        // free(phrase[m].strPhrase);
                    }
                }
                assert(orig == PYFAList[i].pyBase[j].iPhrase);
                free(flag);
                free(phrase);
            }
        }
    }
}

boolean LoadPYOtherDict(FcitxPinyinState* pystate)
{
    //下面开始读系统词组
    FILE *fp;
    int32_t i, j, k, iLen;
    uint32_t iIndex;
    PyFreq *pyFreqTemp, *pPyFreq;
    HZ *HZTemp, *pHZ;
    PYFA* PYFAList = pystate->PYFAList;

    pystate->bPYOtherDictLoaded = true;

    fp = FcitxXDGGetFileWithPrefix("pinyin", PY_PHRASE_FILE, "r", NULL);
    if (!fp)
        FcitxLog(ERROR, _("Can not find System Database of Pinyin!"));
    else {
        LoadPYPhraseDict(pystate, fp, true, false);
        fclose(fp);
        FcitxStringHashSet *sset = FcitxXDGGetFiles("pinyin", NULL, ".mb");
        FcitxStringHashSet *curStr = sset;
        while (curStr) {
            char *filename;

            if (strcmp(curStr->name, PY_PHRASE_FILE) != 0
                && strcmp(curStr->name, PY_USERPHRASE_FILE) != 0
                && strcmp(curStr->name, PY_SYMBOL_FILE) != 0
                && strcmp(curStr->name, PY_BASE_FILE) != 0
                && strcmp(curStr->name, PY_FREQ_FILE) != 0) {

                fp = FcitxXDGGetFileWithPrefix("pinyin", curStr->name, "r", &filename);
                FcitxLog(INFO, _("Load extra dict: %s"), filename);
                free(filename);
                LoadPYPhraseDict(pystate, fp, true, true);
                fclose(fp);
            }
            curStr = curStr->hh.next;
        }

        fcitx_utils_free_string_hash_set(sset);

        pystate->iOrigCounter = pystate->iCounter;
    }

    //下面开始读取用户词库
    fp = FcitxXDGGetFileUserWithPrefix("pinyin", PY_USERPHRASE_FILE, "r", NULL);
    if (fp) {
        LoadPYPhraseDict(pystate, fp, false, false);
        fclose(fp);
    }
    //下面读取索引文件
    fp = FcitxXDGGetFileUserWithPrefix("pinyin", PY_INDEX_FILE, "r", NULL);
    if (fp) {
        uint32_t magic = 0;
        fcitx_utils_read_uint32(fp, &magic);
        if (magic == PY_INDEX_MAGIC_NUMBER) {
            fcitx_utils_read_int32(fp, &iLen);
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            while (!feof(fp)) {
                fcitx_utils_read_int32(fp, &i);
                fcitx_utils_read_int32(fp, &j);
                fcitx_utils_read_int32(fp, &k);
                fcitx_utils_read_uint32(fp, &iIndex);
                fcitx_utils_read_int32(fp, &iLen);

                if (i < pystate->iPYFACount) {
                    if (j < PYFAList[i].iBase) {
                        if (k < PYFAList[i].pyBase[j].iPhrase) {
                            if (k >= 0) {
                                PYFAList[i].pyBase[j].phrase[k].iIndex = iIndex;
                                PYFAList[i].pyBase[j].phrase[k].iHit = iLen;
                            } else {
                                PYFAList[i].pyBase[j].iIndex = iIndex;
                                PYFAList[i].pyBase[j].iHit = iLen;
                            }
                        }
                    }
                }
            }
        } else {
            FcitxLog(WARNING, _("Pinyin Index Magic Number Doesn't match"));
        }

        fclose(fp);
    }
    //下面读取常用词表
    fp = FcitxXDGGetFileUserWithPrefix("pinyin", PY_FREQ_FILE, "r", NULL);
    if (fp) {
        pPyFreq = pystate->pyFreq;

        fcitx_utils_read_uint32(fp, &pystate->iPYFreqCount);

        for (i = 0; i < pystate->iPYFreqCount; i++) {
            pyFreqTemp = fcitx_utils_new(PyFreq);
            // pyFreqTemp->next = NULL;

            fread(pyFreqTemp->strPY, sizeof(char) * 11, 1, fp);
            fcitx_utils_read_uint32(fp, &pyFreqTemp->iCount);

            pyFreqTemp->HZList = fcitx_utils_new(HZ);
            // pyFreqTemp->HZList->next = NULL;
            pHZ = pyFreqTemp->HZList;

            for (k = 0; k < pyFreqTemp->iCount; k++) {
                int8_t slen;
                HZTemp = fcitx_utils_new(HZ);
                fread(&slen, sizeof(int8_t), 1, fp);
                fread(HZTemp->strHZ, sizeof(char) * slen, 1, fp);
                HZTemp->strHZ[slen] = '\0';
                fcitx_utils_read_int32(fp, &HZTemp->iPYFA);
                fcitx_utils_read_uint32(fp, &HZTemp->iHit);
                fcitx_utils_read_uint32(fp, &HZTemp->iIndex);
                pHZ->next = HZTemp;
                pHZ = HZTemp;
            }

            pPyFreq->next = pyFreqTemp;
            pPyFreq = pyFreqTemp;
        }

        fclose(fp);
    }
    return true;
}

void ResetPYStatus(void* arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    pystate->iPYInsertPoint = 0;
    pystate->iPYSelected = 0;
    pystate->strFindString[0] = '\0';
    pystate->strPYAuto[0] = '\0';

    pystate->bIsPYAddFreq = false;
    pystate->bIsPYDelFreq = false;
    pystate->bIsPYDelUserPhr = false;

    pystate->findMap.iMode = PARSE_SINGLEHZ;     //只要不是PARSE_ERROR就可以
}

int GetBaseIndex(FcitxPinyinState* pystate, int32_t iPYFA, char *strBase)
{
    int i;

    if (iPYFA < pystate->iPYFACount) {
        for (i = 0; i < pystate->PYFAList[iPYFA].iBase; i++) {
            if (!strcmp(strBase, pystate->PYFAList[iPYFA].pyBase[i].strHZ))
                return i;
        }
    }

    return -1;
}

INPUT_RETURN_VALUE DoPYInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*) arg;
    FcitxInputState *input = FcitxInstanceGetInputState(pystate->owner);
    int i = 0;
    int val;
    INPUT_RETURN_VALUE retVal;

    if (sym == 0 && state == 0)
        sym = -1;

    if (!pystate->bPYBaseDictLoaded)
        LoadPYBaseDict(pystate);
    if (!pystate->bPYOtherDictLoaded)
        LoadPYOtherDict(pystate);

    retVal = IRV_TO_PROCESS;

    /* is not in py special state */
    if (!pystate->bIsPYAddFreq && !pystate->bIsPYDelFreq && !pystate->bIsPYDelUserPhr) {
        if ((FcitxHotkeyIsHotKeyLAZ(sym, state)
             || FcitxHotkeyIsHotKey(sym, state, FCITX_SEPARATOR)
             || (pystate->bSP && FcitxInputStateGetRawInputBufferSize(input) > 0 && pystate->bSP_UseSemicolon && FcitxHotkeyIsHotKey(sym, state, FCITX_SEMICOLON)))) {
            FcitxInputStateSetIsInRemind(input, false);
            FcitxInputStateSetShowCursor(input, true);

            /* we cannot insert seperator in the first, nor there is a existing separator */
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_SEPARATOR)) {
                if (!pystate->iPYInsertPoint)
                    return IRV_TO_PROCESS;
                if (pystate->strFindString[pystate->iPYInsertPoint - 1] == PY_SEPARATOR)
                    return IRV_DO_NOTHING;
            }

            val = i = strlen(pystate->strFindString);

            if (!pystate->bSP &&
                pystate->pyconfig.bUseVForQuickPhrase && i == 0 &&
                FcitxHotkeyIsKey(sym, state, FcitxKey_v, FcitxKeyState_None)) {
                int key = sym;
                boolean useDup = false;
                boolean append = true;
                if (FcitxQuickPhraseLaunch(pystate->owner, &key,
                                           &useDup, &append))
                    return IRV_DISPLAY_MESSAGE;
            }

            /* do the insert */
            for (; i > pystate->iPYInsertPoint; i--)
                pystate->strFindString[i] = pystate->strFindString[i - 1];

            pystate->strFindString[pystate->iPYInsertPoint++] = sym;
            pystate->strFindString[val + 1] = '\0';
            ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);

            val = 0;
            for (i = 0; i < pystate->iPYSelected; i++)
                val += fcitx_utf8_strlen(pystate->pySelected[i].strHZ);

            retVal = IRV_DISPLAY_CANDWORDS;
            if (pystate->findMap.iHZCount > (MAX_WORDS_USER_INPUT - val)) {
                UpdateFindString(pystate, val);
                ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                retVal = IRV_DO_NOTHING;
            }

        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
            if (pystate->iPYSelected) {
                char strTemp[MAX_USER_INPUT + 1];

                val = strlen(pystate->strFindString);
                strcpy(strTemp, pystate->pySelected[pystate->iPYSelected - 1].strPY);
                strcat(strTemp, pystate->strFindString);
                strcpy(pystate->strFindString, strTemp);
                pystate->iPYInsertPoint += strlen(pystate->strFindString) - val;
                pystate->iPYSelected--;
                ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);

                retVal = IRV_DISPLAY_CANDWORDS;
            } else if (pystate->iPYInsertPoint) {
                char *move_src = (pystate->strFindString
                                  + pystate->iPYInsertPoint);
                /* we cannot delete it if cursor is at the first */
                val = ((pystate->iPYInsertPoint > 1)
                       && (move_src[-2] == PY_SEPARATOR)) ? 2 : 1;
                memmove(move_src - val, move_src, strlen(move_src) + 1);
                ParsePY(&pystate->pyconfig, pystate->strFindString,
                        &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                pystate->iPYInsertPoint -= val;

                if (!strlen(pystate->strFindString))
                    return IRV_CLEAN;
                retVal = IRV_DISPLAY_CANDWORDS;
            } else {
                if (!strlen(pystate->strFindString))
                    return IRV_TO_PROCESS;
                else
                    return IRV_DO_NOTHING;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_DELETE)) {
            if (FcitxInputStateGetRawInputBufferSize(input)) {
                if (pystate->iPYInsertPoint == strlen(pystate->strFindString))
                    return IRV_DO_NOTHING;
                char *move_dst = (pystate->strFindString
                                  + pystate->iPYInsertPoint);
                val = (move_dst[1] == PY_SEPARATOR) ? 2 : 1;
                memmove(move_dst, move_dst + val, strlen(move_dst + val) + 1);

                ParsePY(&pystate->pyconfig, pystate->strFindString,
                        &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                if (!strlen(pystate->strFindString))
                    return IRV_CLEAN;
                retVal = IRV_DISPLAY_CANDWORDS;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_HOME)) {
            if (FcitxInputStateGetRawInputBufferSize(input) == 0)
                return IRV_TO_PROCESS;
            pystate->iPYInsertPoint = 0;
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_END)) {
            if (FcitxInputStateGetRawInputBufferSize(input) == 0)
                return IRV_TO_PROCESS;
            pystate->iPYInsertPoint = strlen(pystate->strFindString);
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT)) {
            if (!FcitxInputStateGetRawInputBufferSize(input))
                return IRV_TO_PROCESS;
            if (pystate->iPYInsertPoint == strlen(pystate->strFindString))
                return IRV_DO_NOTHING;
            pystate->iPYInsertPoint++;
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT)) {
            if (!FcitxInputStateGetRawInputBufferSize(input))
                return IRV_TO_PROCESS;
            if (pystate->iPYInsertPoint <= 0) {
                if (pystate->iPYSelected) {
                    char strTemp[MAX_USER_INPUT + 1];

                    val = strlen(pystate->strFindString);
                    strcpy(strTemp, pystate->pySelected[pystate->iPYSelected - 1].strPY);
                    strcat(strTemp, pystate->strFindString);
                    strcpy(pystate->strFindString, strTemp);
                    pystate->iPYInsertPoint = strlen(pystate->strFindString) - val;
                    pystate->iPYSelected--;
                    ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);

                    retVal = IRV_DISPLAY_CANDWORDS;
                } else
                    retVal = IRV_DO_NOTHING;
            } else {
                pystate->iPYInsertPoint--;
                retVal = IRV_DISPLAY_CANDWORDS;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
            FcitxCandidateWordList *candList;
            candList = FcitxInputStateGetCandidateList(input);
            if (FcitxCandidateWordPageCount(candList) == 0) {
                if (FcitxInputStateGetRawInputBufferSize(input) == 0)
                    return IRV_TO_PROCESS;
                else
                    return IRV_DO_NOTHING;
            }

            retVal = FcitxCandidateWordChooseByIndex(candList, 0);
        } else if (FcitxHotkeyIsHotKey(sym, state, pystate->pyconfig.hkPYDelUserPhr)) {
            if (!pystate->bIsPYDelUserPhr) {
                int i;
                FcitxCandidateWordList *candList;
                FcitxCandidateWord* candWord = NULL;
                candList = FcitxInputStateGetCandidateList(input);
                for (i = 0;
                     (candWord = FcitxCandidateWordGetByIndex(candList, i));
                     i++) {
                    if (candWord->owner == pystate) {
                        PYCandWord* pycandWord = candWord->priv;
                        if (pycandWord->iWhich == PY_CAND_USERPHRASE)
                            break;
                    }
                }

                if (!candWord)
                    return IRV_TO_PROCESS;

                pystate->bIsPYDelUserPhr = true;
                FcitxInputStateSetIsDoInputOnly(input, true);

                FcitxInstanceCleanInputWindowUp(pystate->owner);
                FcitxMessagesAddMessageStringsAtLast(
                    FcitxInputStateGetAuxUp(input), MSG_TIPS,
                    _("Press index to delete user phrase (ESC for cancel)"));
                FcitxInputStateSetShowCursor(input, false);

                return IRV_DISPLAY_MESSAGE;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, pystate->pyconfig.hkPYAddFreq)) {
            if (!pystate->bIsPYAddFreq && pystate->findMap.iHZCount == 1 && FcitxInputStateGetRawInputBufferSize(input)) {
                pystate->bIsPYAddFreq = true;
                FcitxInputStateSetIsDoInputOnly(input, true);

                FcitxInstanceCleanInputWindowUp(pystate->owner);
                FcitxMessagesAddMessageStringsAtLast(
                    FcitxInputStateGetAuxUp(input), MSG_TIPS,
                    _("Press number to make word in frequent list"));
                FcitxInputStateSetShowCursor(input, false);

                return IRV_DISPLAY_MESSAGE;
            }
        } else if (FcitxHotkeyIsHotKey(sym, state, pystate->pyconfig.hkPYDelFreq)) {
            if (!pystate->bIsPYDelFreq) {
                val = 0;
                int i;
                FcitxCandidateWord* candWord = NULL;
                FcitxCandidateWordList *candList;
                candList = FcitxInputStateGetCandidateList(input);
                for (i = 0;
                     (candWord = FcitxCandidateWordGetByIndex(candList, i));
                     i++) {
                    if (candWord->owner == pystate) {
                        PYCandWord* pycandWord = candWord->priv;
                        if (pycandWord->iWhich == PY_CAND_FREQ) {
                            val = i + 1;
                        }
                    }
                }

                if (val == 0)
                    return IRV_DO_NOTHING;

                FcitxInstanceCleanInputWindowUp(pystate->owner);
                if (val == 1) {
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Press 1 to delete %s in frequent list (ESC for cancel)"), pystate->strFindString);
                } else {
                    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Press 1-%d to delete %s in frequent list (ESC for cancel)"), val, pystate->strFindString);
                }

                pystate->bIsPYDelFreq = true;
                FcitxInputStateSetIsDoInputOnly(input, true);

                FcitxInputStateSetShowCursor(input, false);

                return IRV_DISPLAY_MESSAGE;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS) {
        FcitxCandidateWordList *candList;
        int iKey;
        candList = FcitxInputStateGetCandidateList(input);
        iKey = FcitxCandidateWordCheckChooseKey(candList, sym, state);
        if (iKey >= 0) {
            FcitxCandidateWord *candWord;
            candWord = FcitxCandidateWordGetByIndex(candList, iKey);
            if (!FcitxInputStateGetIsInRemind(input)) {
                if (!FcitxInputStateGetRawInputBufferSize(input))
                    return IRV_TO_PROCESS;
                else if (candWord == NULL)
                    return IRV_DO_NOTHING;
                else {
                    if (candWord->owner == pystate && (pystate->bIsPYAddFreq || pystate->bIsPYDelFreq || pystate->bIsPYDelUserPhr)) {
                        PYCandWord* pycandWord = candWord->priv;
                        if (pystate->bIsPYAddFreq) {
                            PYAddFreq(pystate, pycandWord);
                            pystate->bIsPYAddFreq = false;
                        } else if (pystate->bIsPYDelFreq) {
                            PYDelFreq(pystate, pycandWord);
                            pystate->bIsPYDelFreq = false;
                        } else {
                            if (pycandWord->iWhich == PY_CAND_USERPHRASE)
                                PYDelUserPhrase(pystate, pycandWord->cand.phrase.iPYFA,
                                                pycandWord->cand.phrase.iBase, (PyUsrPhrase*) pycandWord->cand.phrase.phrase);
                            pystate->bIsPYDelUserPhr = false;
                        }
                        FcitxInputStateSetIsDoInputOnly(input, false);
                        FcitxInputStateSetShowCursor(input, true);

                        retVal = IRV_DISPLAY_CANDWORDS;
                    }
                }
            }
        } else if (sym == -1) {
            ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
            pystate->iPYInsertPoint = 0;
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
            return IRV_TO_PROCESS;
        } else if (pystate->bIsPYAddFreq || pystate->bIsPYDelFreq ||
                   pystate->bIsPYDelUserPhr) {
            return IRV_DO_NOTHING;
        }
    }

    if (!FcitxInputStateGetIsInRemind(input)) {
        UpdateCodeInputPY(pystate);
        CalculateCursorPosition(pystate);
    }

    return retVal;
}

/*
 * 本函数根据当前插入点计算光标的实际位置
 */
void CalculateCursorPosition(FcitxPinyinState* pystate)
{
    int i;
    int iTemp;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);

    int iCursorPos = 0;
    int hzLen = 0;

    for (i = 0; i < pystate->iPYSelected; i++)
        iCursorPos += strlen(pystate->pySelected[i].strHZ);

    hzLen = iCursorPos;

    if (pystate->iPYInsertPoint > strlen(pystate->strFindString))
        pystate->iPYInsertPoint = strlen(pystate->strFindString);
    iTemp = pystate->iPYInsertPoint;

    for (i = 0; i < pystate->findMap.iHZCount; i++) {
        if (strlen(pystate->findMap.strPYParsed[i]) >= iTemp) {
            iCursorPos += iTemp;
            break;
        }
        iCursorPos += strlen(pystate->findMap.strPYParsed[i]);

        iCursorPos++;
        iTemp -= strlen(pystate->findMap.strPYParsed[i]);
    }
    FcitxInputStateSetCursorPos(input, iCursorPos);

    if (pystate->pyconfig.bFixCursorAtHead)
        FcitxInputStateSetClientCursorPos(input, 0);
    else
        FcitxInputStateSetClientCursorPos(input, hzLen);
}

/*
 * 由于拼音的编辑功能修改了strFindString，必须保证FcitxInputStateGetRawInputBuffer(input)与用户的输入一致
 */
void UpdateCodeInputPY(FcitxPinyinState* pystate)
{
    int i;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);
    char* strCodeInput = FcitxInputStateGetRawInputBuffer(input);

    strCodeInput[0] = '\0';
    for (i = 0; i < pystate->iPYSelected; i++)
        strcat(strCodeInput, pystate->pySelected[i].strPY);
    strcat(strCodeInput, pystate->strFindString);
    FcitxInputStateSetRawInputBufferSize(input, strlen(strCodeInput));
}

void UpdateFindString(FcitxPinyinState* pystate, int val)
{
    int i;

    pystate->strFindString[0] = '\0';
    for (i = 0; i < pystate->findMap.iHZCount; i++) {
        if (i >= MAX_WORDS_USER_INPUT - val)
            break;
        strcat(pystate->strFindString, pystate->findMap.strPYParsed[i]);
    }
    if (pystate->iPYInsertPoint > strlen(pystate->strFindString))
        pystate->iPYInsertPoint = strlen(pystate->strFindString);
}

INPUT_RETURN_VALUE PYGetCandWords(void* arg)
{
    int iVal;
    FcitxPinyinState *pystate = (FcitxPinyinState*) arg;
    FcitxInputState *input = FcitxInstanceGetInputState(pystate->owner);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(pystate->owner);
    FcitxMessages* msgPreedit = FcitxInputStateGetPreedit(input);
    FcitxMessages* msgClientPreedit = FcitxInputStateGetClientPreedit(input);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);

    FcitxCandidateWordSetPageSize(candList, config->iMaxCandWord);
    FcitxCandidateWordSetChoose(candList, DIGIT_STR_CHOOSE);

    /* update preedit string */
    int i;
    FcitxMessagesSetMessageCount(msgPreedit, 0);
    FcitxMessagesSetMessageCount(msgClientPreedit, 0);
    if (pystate->iPYSelected) {
        FcitxMessagesAddMessageStringsAtLast(msgPreedit, MSG_OTHER, "");
        FcitxMessagesAddMessageStringsAtLast(msgClientPreedit, MSG_OTHER, "");
        for (i = 0; i < pystate->iPYSelected; i++) {
            FcitxMessagesMessageConcat(msgPreedit, FcitxMessagesGetMessageCount(msgPreedit) - 1, pystate->pySelected[i].strHZ);
            FcitxMessagesMessageConcat(msgClientPreedit, FcitxMessagesGetMessageCount(msgClientPreedit) - 1, pystate->pySelected[i].strHZ);
        }
    }

    for (i = 0; i < pystate->findMap.iHZCount; i++) {
        FcitxMessagesAddMessageStringsAtLast(msgPreedit, MSG_CODE,
                                             pystate->findMap.strPYParsed[i]);
        if (i < pystate->findMap.iHZCount - 1)
            FcitxMessagesMessageConcat(msgPreedit, FcitxMessagesGetMessageCount(msgPreedit) - 1, " ");
    }

    if (pystate->findMap.iMode == PARSE_ERROR) {
        for (i = 0; i < pystate->findMap.iHZCount; i++) {
            FcitxMessagesAddMessageStringsAtLast(
                msgClientPreedit, MSG_CODE, pystate->findMap.strPYParsed[i]);
        }
        char* errorAuto = FcitxUIMessagesToCString(msgClientPreedit);
        FcitxInstanceCleanInputWindowDown(pystate->owner);

        FcitxCandidateWord candWord;
        candWord.owner = pystate;
        candWord.callback = PYGetCandWord;
        candWord.priv = NULL;
        candWord.strWord = strdup(errorAuto);
        candWord.strExtra = NULL;
        candWord.wordType = MSG_OTHER;
        FcitxCandidateWordAppend(candList, &candWord);
        return IRV_DISPLAY_CANDWORDS;
    }

    if (FcitxInputStateGetIsInRemind(input))
        return PYGetRemindCandWords(pystate);

    //判断是不是要输入常用字或符号
    PyFreq* pCurFreq = pystate->pyFreq->next;
    for (iVal = 0; iVal < pystate->iPYFreqCount; iVal++) {
        if (!strcmp(pystate->strFindString, pCurFreq->strPY))
            break;
        pCurFreq = pCurFreq->next;
    }

    if (pystate->pyconfig.bPYCreateAuto)
        PYCreateAuto(pystate);

    if (pystate->strPYAuto[0]) {
        FcitxCandidateWord candWord;
        PYCandWord* pycandword = fcitx_utils_new(PYCandWord);
        pycandword->iWhich = PY_CAND_AUTO;
        candWord.owner = pystate;
        candWord.callback = PYGetCandWord;
        candWord.priv = pycandword;
        candWord.strWord = strdup(pystate->strPYAuto);
        candWord.strExtra = NULL;
        candWord.wordType = MSG_OTHER;
        FcitxCandidateWordAppend(candList, &candWord);
    }

    PYGetPhraseCandWords(pystate);
    if (pCurFreq)
        PYGetFreqCandWords(pystate, pCurFreq);
    PYGetBaseCandWords(pystate, pCurFreq);

    if (FcitxCandidateWordPageCount(candList) != 0) {
        FcitxCandidateWord *candWord = FcitxCandidateWordGetCurrentWindow(candList);
        FcitxMessagesAddMessageStringsAtLast(msgClientPreedit, MSG_INPUT,
                                             candWord->strWord);
    }

    return IRV_DISPLAY_CANDWORDS;
}

/*
 * 根据用户的录入自动生成一个汉字组合
 * 此处采用的策略是按照使用频率最高的字/词
 */
void PYCreateAuto(FcitxPinyinState* pystate)
{
    PYCandIndex candPos;
    int val;
    int iMatchedLength;
    char str[3];
    PyPhrase *phrase;
    PyPhrase *phraseSelected = NULL;
    PyBase *baseSelected = NULL;
    PYFA *pPYFA = NULL;
    char strMap[MAX_WORDS_USER_INPUT * 2 + 1];
    int iCount;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;

    pystate->strPYAuto[0] = '\0';
    pystate->strPYAutoMap[0] = '\0';
    str[2] = '\0';

    if (pystate->findMap.iHZCount == 1)
        return;

    while (fcitx_utf8_strlen(pystate->strPYAuto) < pystate->findMap.iHZCount) {
        phraseSelected = NULL;
        baseSelected = NULL;

        iCount = fcitx_utf8_strlen(pystate->strPYAuto);
        str[0] = pystate->findMap.strMap[iCount][0];
        str[1] = pystate->findMap.strMap[iCount][1];

        strMap[0] = '\0';

        for (val = iCount + 1; val < pystate->findMap.iHZCount; val++)
            strcat(strMap, pystate->findMap.strMap[val]);

        candPos.iPYFA = 0;
        candPos.iBase = 0;
        candPos.iPhrase = 0;
        if ((pystate->findMap.iHZCount - iCount) > 1) {
            for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
                if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
                    for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                        phrase = USER_PHRASE_NEXT(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].userPhrase);
                        for (candPos.iPhrase = 0;
                                candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iUserPhrase; candPos.iPhrase++) {
                            val = CmpMap(pyconfig, phrase->strMap, strMap, &iMatchedLength, pystate->bSP);
                            if (!val && iMatchedLength == (pystate->findMap.iHZCount - 1) * 2)
                                return;
                            if (!val || (val && (strlen(phrase->strMap) == iMatchedLength))) {
                                if (!phraseSelected) {
                                    baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                                    pPYFA = &PYFAList[candPos.iPYFA];
                                    phraseSelected = phrase;
                                } else if (strlen(phrase->strMap) <= (pystate->findMap.iHZCount - 1) * 2) {
                                    if (strlen(phrase->strMap) == strlen(phraseSelected->strMap)) {
                                        //先看词频，如果词频一样，再最近优先
                                        if ((phrase->iHit > phraseSelected->iHit)
                                                || ((phrase->iHit == phraseSelected->iHit)
                                                    && (phrase->iIndex > phraseSelected->iIndex))) {
                                            baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                                            pPYFA = &PYFAList[candPos.iPYFA];
                                            phraseSelected = phrase;
                                        }
                                    } else if (strlen(phrase->strMap) > strlen(phraseSelected->strMap)) {
                                        baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                                        pPYFA = &PYFAList[candPos.iPYFA];
                                        phraseSelected = phrase;
                                    }
                                }
                            }
                            phrase = USER_PHRASE_NEXT(phrase);
                        }
                    }
                }
            }

            for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
                if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
                    for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                        for (candPos.iPhrase = 0;
                                candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iPhrase; candPos.iPhrase++) {
                            val =
                                CmpMap(pyconfig, PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap,
                                       strMap, &iMatchedLength, pystate->bSP);
                            if (!val && iMatchedLength == (pystate->findMap.iHZCount - 1) * 2)
                                return;
                            if (!val || (val && (strlen(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap)
                                                 == iMatchedLength))) {
                                if (!phraseSelected) {
                                    baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                                    pPYFA = &PYFAList[candPos.iPYFA];
                                    phraseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase]);
                                } else if (strlen(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap)
                                           <= (pystate->findMap.iHZCount - 1) * 2) {
                                    if (strlen(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap)
                                            == strlen(phraseSelected->strMap)) {
                                        //先看词频，如果词频一样，再最近优先
                                        if ((PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].iHit >
                                                phraseSelected->iHit)
                                                ||
                                                ((PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].iHit ==
                                                  phraseSelected->iHit)
                                                 &&
                                                 (PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].iIndex >
                                                  phraseSelected->iIndex))) {
                                            baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                                            pPYFA = &PYFAList[candPos.iPYFA];
                                            phraseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase]);
                                        }
                                    } else if (strlen(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap)
                                               > strlen(phraseSelected->strMap)) {
                                        baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                                        pPYFA = &PYFAList[candPos.iPYFA];
                                        phraseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase]);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (baseSelected) {
                strcat(pystate->strPYAuto, baseSelected->strHZ);
                strcat(pystate->strPYAutoMap, pPYFA->strMap);
                strcat(pystate->strPYAuto, phraseSelected->strPhrase);
                strcat(pystate->strPYAutoMap, phraseSelected->strMap);
            }
        }

        if (!baseSelected) {
            val = -1;
            baseSelected = NULL;
            for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
                if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
                    for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                        if ((int)
                                (PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iHit) > val) {
                            val = PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iHit;
                            baseSelected = &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase]);
                            pPYFA = &PYFAList[candPos.iPYFA];
                        }
                    }
                }
            }

            if (baseSelected) {
                strcat(pystate->strPYAuto, baseSelected->strHZ);
                strcat(pystate->strPYAutoMap, pPYFA->strMap);
            } else {            //出错了
                pystate->strPYAuto[0] = '\0';
                return;
            }
        }
    }
}

INPUT_RETURN_VALUE PYGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);

    if (candWord->priv == NULL) {
        strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
        return IRV_COMMIT_STRING;
    }

    char *pBase = NULL, *pPhrase = NULL;
    char *pBaseMap = NULL, *pPhraseMap = NULL;
    uint *pIndex = NULL;
    boolean bAddNewPhrase = true;
    int i;
    char strHZString[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    int iLen;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxInstance* instance = pystate->owner;
    PYCandWord* pycandWord = candWord->priv;
    FcitxProfile* profile = FcitxInstanceGetProfile(pystate->owner);

    switch (pycandWord->iWhich) {
    case PY_CAND_AUTO:
        pBase = pystate->strPYAuto;
        pBaseMap = pystate->strPYAutoMap;
        bAddNewPhrase = pystate->pyconfig.bPYSaveAutoAsPhrase;
        break;
    case PY_CAND_BASE:         //是系统单字
        pBase = PYFAList[pycandWord->cand.base.iPYFA].pyBase[pycandWord->cand.base.iBase].strHZ;
        pBaseMap = PYFAList[pycandWord->cand.base.iPYFA].strMap;
        pIndex = &(PYFAList[pycandWord->cand.base.iPYFA].pyBase[pycandWord->cand.base.iBase].iIndex);
        PYFAList[pycandWord->cand.base.iPYFA].pyBase[pycandWord->cand.base.iBase].iHit++;
        pystate->iOrderCount++;
        break;
    case PY_CAND_USERPHRASE:   //是用户词组
        pystate->iNewPYPhraseCount++; // fall through
    case PY_CAND_SYSPHRASE:    //是系统词组
        pBase = PYFAList[pycandWord->cand.phrase.iPYFA].pyBase[pycandWord->cand.phrase.iBase].strHZ;
        pBaseMap = PYFAList[pycandWord->cand.phrase.iPYFA].strMap;
        pPhrase = pycandWord->cand.phrase.phrase->strPhrase;
        pPhraseMap = pycandWord->cand.phrase.phrase->strMap;
        pIndex = &(pycandWord->cand.phrase.phrase->iIndex);
        pycandWord->cand.phrase.phrase->iHit++;
        pystate->iOrderCount++;
        break;
    case PY_CAND_FREQ:         //是常用字
        pBase = pycandWord->cand.freq.hz->strHZ;
        pBaseMap = PYFAList[pycandWord->cand.freq.hz->iPYFA].strMap;
        pycandWord->cand.freq.hz->iHit++;
        pIndex = &(pycandWord->cand.freq.hz->iIndex);
        pystate->iNewFreqCount++;
        break;
    case PY_CAND_REMIND: {
        strcpy(pystate->strPYRemindSource, pycandWord->cand.remind.phrase->strPhrase + pycandWord->cand.remind.iLength);
        strcpy(pystate->strPYRemindMap, pycandWord->cand.remind.phrase->strMap + pycandWord->cand.remind.iLength);
        pBase = pystate->strPYRemindSource;
        strcpy(FcitxInputStateGetOutputString(input), pBase);
        FcitxCandidateWordReset(FcitxInputStateGetCandidateList(input));
        INPUT_RETURN_VALUE retVal = PYGetRemindCandWords(pystate);
        if (retVal == IRV_DISPLAY_CANDWORDS)
            return IRV_COMMIT_STRING_REMIND;
        else
            return IRV_COMMIT_STRING;
    }
    }

    if (pIndex && (*pIndex != pystate->iCounter))
        *pIndex = ++pystate->iCounter;
    if (pystate->iOrderCount >= AUTOSAVE_ORDER_COUNT) {
        SavePYIndex(pystate);
    }
    if (pystate->iNewFreqCount >= AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
    }

    strcpy(strHZString, pBase);
    if (pPhrase)
        strcat(strHZString, pPhrase);
    iLen = fcitx_utf8_strlen(strHZString);
    if (iLen == pystate->findMap.iHZCount) {
        pystate->strPYAuto[0] = '\0';
        for (iLen = 0; iLen < pystate->iPYSelected; iLen++)
            strcat(pystate->strPYAuto, pystate->pySelected[iLen].strHZ);
        strcat(pystate->strPYAuto, strHZString);
        ParsePY(&pystate->pyconfig, FcitxInputStateGetRawInputBuffer(input), &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
        strHZString[0] = '\0';
        for (i = 0; i < pystate->iPYSelected; i++)
            strcat(strHZString, pystate->pySelected[i].strMap);
        if (pBaseMap)
            strcat(strHZString, pBaseMap);
        if (pPhraseMap)
            strcat(strHZString, pPhraseMap);
        if (bAddNewPhrase && (fcitx_utf8_strlen(pystate->strPYAuto) <= (MAX_PY_PHRASE_LENGTH)))
            PYAddUserPhrase(pystate, pystate->strPYAuto, strHZString, false);
        FcitxInstanceCleanInputWindow(instance);
        strcpy(FcitxInputStateGetOutputString(input), pystate->strPYAuto);
        if (profile->bUseRemind) {
            strcpy(pystate->strPYRemindSource, pystate->strPYAuto);
            strcpy(pystate->strPYRemindMap, strHZString);
            PYGetRemindCandWords(pystate);
            pystate->iPYInsertPoint = 0;
            pystate->strFindString[0] = '\0';
            return IRV_COMMIT_STRING_REMIND;
        }

        return IRV_COMMIT_STRING;
    }
    //此时进入自造词状态
    pystate->pySelected[pystate->iPYSelected].strPY[0] = '\0';
    pystate->pySelected[pystate->iPYSelected].strMap[0] = '\0';
    for (i = 0; i < iLen; i++)
        strcat(pystate->pySelected[pystate->iPYSelected].strPY, pystate->findMap.strPYParsed[i]);
    if (pBaseMap)
        strcat(pystate->pySelected[pystate->iPYSelected].strMap, pBaseMap);
    if (pPhraseMap)
        strcat(pystate->pySelected[pystate->iPYSelected].strMap, pPhraseMap);
    strcpy(pystate->pySelected[pystate->iPYSelected].strHZ, strHZString);
    pystate->iPYSelected++;
    pystate->strFindString[0] = '\0';
    for (; i < pystate->findMap.iHZCount; i++)
        strcat(pystate->strFindString, pystate->findMap.strPYParsed[i]);
    DoPYInput(pystate, 0, 0);
    pystate->iPYInsertPoint = strlen(pystate->strFindString);

    CalculateCursorPosition(pystate);
    return IRV_DISPLAY_CANDWORDS;
}

void PYGetPhraseCandWords(FcitxPinyinState* pystate)
{
    PYCandIndex candPos;
    char str[3];
    int val, iMatchedLength;
    char strMap[MAX_WORDS_USER_INPUT * 2 + 1];
    PyPhrase *phrase;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);

    if (pystate->findMap.iHZCount == 1)
        return;

    UT_array candtemp;
    utarray_init(&candtemp, fcitx_ptr_icd);

    str[0] = pystate->findMap.strMap[0][0];
    str[1] = pystate->findMap.strMap[0][1];
    str[2] = '\0';
    strMap[0] = '\0';
    for (val = 1; val < pystate->findMap.iHZCount; val++)
        strcat(strMap, pystate->findMap.strMap[val]);
    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                phrase = USER_PHRASE_NEXT(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].userPhrase);
                for (candPos.iPhrase = 0;
                        candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iUserPhrase; candPos.iPhrase++) {
                    val = CmpMap(pyconfig, phrase->strMap, strMap, &iMatchedLength, pystate->bSP);
                    if (!val || (val && (strlen(phrase->strMap) == iMatchedLength))) {
                        PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
                        PYAddPhraseCandWord(pystate, candPos, phrase, false, pycandWord);
                        utarray_push_back(&candtemp, &pycandWord);
                    }

                    phrase = USER_PHRASE_NEXT(phrase);
                }
            }
        }
    }

    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                for (candPos.iPhrase = 0; candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iPhrase; candPos.iPhrase++) {
                    val = CmpMap(
                              pyconfig,
                              PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap,
                              strMap,
                              &iMatchedLength,
                              pystate->bSP);
                    if (!val ||
                            (val && (strlen(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap) == iMatchedLength))) {
                        PYCandWord* pycandWord = fcitx_utils_new(PYCandWord);
                        PYAddPhraseCandWord(pystate, candPos, &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase]), true, pycandWord);
                        utarray_push_back(&candtemp, &pycandWord);
                    }
                }
            }
        }
    }

    PYCandWordSortContext context;
    context.order = pystate->pyconfig.phraseOrder;
    context.type = PY_CAND_SYSPHRASE;
    context.pystate = pystate;
    if (context.order != AD_NO)
        utarray_msort_r(&candtemp, PYCandWordCmp, &context);

    PYCandWord** pcand = NULL;
    for (pcand = (PYCandWord**) utarray_front(&candtemp);
            pcand != NULL;
            pcand = (PYCandWord**) utarray_next(&candtemp, pcand)) {
        FcitxCandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = NULL;
        if ((*pcand)->iWhich == PY_CAND_USERPHRASE)
            candWord.wordType = MSG_USERPHR;
        else
            candWord.wordType = MSG_OTHER;
        const char* pBase = PYFAList[(*pcand)->cand.phrase.iPYFA].pyBase[(*pcand)->cand.phrase.iBase].strHZ;
        const char* pPhrase = (*pcand)->cand.phrase.phrase->strPhrase;
        fcitx_utils_alloc_cat_str(candWord.strWord, pBase, pPhrase);
        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input),
                                 &candWord);
    }

    utarray_done(&candtemp);
}

/*
 * 将一个词加入到候选列表的合适位置中
 * b = true 表示是系统词组，false表示是用户词组
 */
boolean PYAddPhraseCandWord(FcitxPinyinState* pystate, PYCandIndex pos, PyPhrase * phrase, boolean b, PYCandWord* pycandword)
{
    PYFA* PYFAList = pystate->PYFAList;
    char str[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];

    strcpy(str, PYFAList[pos.iPYFA].pyBase[pos.iBase].strHZ);
    strcat(str, phrase->strPhrase);
    if (pystate->strPYAuto[0]) {
        if (strcmp(pystate->strPYAuto, str) == 0) {
            return false;
        }
    }

    pycandword->iWhich = (b) ? PY_CAND_SYSPHRASE : PY_CAND_USERPHRASE;
    pycandword->cand.phrase.phrase = phrase;
    pycandword->cand.phrase.iPYFA = pos.iPYFA;
    pycandword->cand.phrase.iBase = pos.iBase;
    return true;
}

//*****************************************************

void PYGetBaseCandWords(FcitxPinyinState* pystate, PyFreq* pCurFreq)
{
    PYCandIndex candPos = {
        0, 0, 0
    };
    char str[3];
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);
    UT_array candtemp;
    utarray_init(&candtemp, fcitx_ptr_icd);

    str[0] = pystate->findMap.strMap[0][0];
    str[1] = pystate->findMap.strMap[0][1];
    str[2] = '\0';
    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                if (!PYIsInFreq(pCurFreq, PYFAList[candPos.iPYFA].pyBase[candPos.iBase].strHZ)) {
                    PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
                    PYAddBaseCandWord(candPos, pycandWord);
                    utarray_push_back(&candtemp, &pycandWord);
                }
            }
        }
    }

    PYCandWordSortContext context;
    context.order = pystate->pyconfig.baseOrder;
    context.type = PY_CAND_BASE;
    context.pystate = pystate;
    if (context.order != AD_NO)
        utarray_msort_r(&candtemp, PYCandWordCmp, &context);

    PYCandWord** pcand = NULL;
    for (pcand = (PYCandWord**) utarray_front(&candtemp);
            pcand != NULL;
            pcand = (PYCandWord**) utarray_next(&candtemp, pcand)) {
        FcitxCandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(PYFAList[(*pcand)->cand.base.iPYFA].pyBase[(*pcand)->cand.base.iBase].strHZ);
        candWord.wordType = MSG_OTHER;

        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }

    utarray_done(&candtemp);
}

/*
 * 将一个字加入到候选列表的合适位置中
 */
void PYAddBaseCandWord(PYCandIndex pos, PYCandWord* pycandWord)
{
    pycandWord->iWhich = PY_CAND_BASE;
    pycandWord->cand.base.iPYFA = pos.iPYFA;
    pycandWord->cand.base.iBase = pos.iBase;
}

void PYGetFreqCandWords(FcitxPinyinState* pystate, PyFreq* pCurFreq)
{
    int i;
    HZ *hz;
    UT_array candtemp;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);
    utarray_init(&candtemp, fcitx_ptr_icd);

    if (pCurFreq) {
        hz = pCurFreq->HZList->next;
        for (i = 0; i < pCurFreq->iCount; i++) {
            PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
            PYAddFreqCandWord(pCurFreq, hz, pCurFreq->strPY, pycandWord);
            utarray_push_back(&candtemp, &pycandWord);
            hz = hz->next;
        }
    }

    PYCandWordSortContext context;
    context.order = pystate->pyconfig.freqOrder;
    context.type = PY_CAND_FREQ;
    context.pystate = pystate;
    if (context.order != AD_NO)
        utarray_msort_r(&candtemp, PYCandWordCmp, &context);

    PYCandWord** pcand = NULL;
    for (pcand = (PYCandWord**) utarray_front(&candtemp);
            pcand != NULL;
            pcand = (PYCandWord**) utarray_next(&candtemp, pcand)) {
        FcitxCandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup((*pcand)->cand.freq.hz->strHZ);
        candWord.wordType = MSG_USERPHR;

        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }

    utarray_done(&candtemp);
}

/*
 * 将一个常用字加入到候选列表的合适位置中
 */
void PYAddFreqCandWord(PyFreq* pyFreq, HZ * hz, char *strPY, PYCandWord* pycandWord)
{
    pycandWord->iWhich = PY_CAND_FREQ;
    pycandWord->cand.freq.hz = hz;
    pycandWord->cand.freq.strPY = strPY;
    pycandWord->cand.freq.pyFreq = pyFreq;
}

/*
 * 将一个词组保存到用户词组库中
 * 返回true表示是新词组
 */
boolean PYAddUserPhrase(FcitxPinyinState* pystate, const char *phrase, const char *map, boolean incHit)
{
    PyUsrPhrase *userPhrase, *newPhrase, *temp;
    char str[UTF8_MAX_LENGTH + 1];
    int i, j, k, iTemp;
    int clen;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;

    //如果短于两个汉字，则不能组成词组
    if (fcitx_utf8_strlen(phrase) < 2)
        return false;
    str[0] = map[0];
    str[1] = map[1];
    str[2] = '\0';
    i = GetBaseMapIndex(pystate, str);

    clen = fcitx_utf8_char_len(phrase);
    strncpy(str, phrase, clen);
    str[clen] = '\0';
    j = GetBaseIndex(pystate, i, str);;
    //判断该词组是否已经在库中
    //首先，看它是不是在用户词组库中
    userPhrase = PYFAList[i].pyBase[j].userPhrase->next;
    for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
        if (!strcmp(map + 2, userPhrase->phrase.strMap)
                && !strcmp(phrase + clen, userPhrase->phrase.strPhrase)) {
            if (incHit) {
                userPhrase->phrase.iHit ++;
                userPhrase->phrase.iIndex = ++pystate->iCounter;
            }
            return false;
        }
        userPhrase = userPhrase->next;
    }

    //然后，看它是不是在系统词组库中
    for (k = 0; k < PYFAList[i].pyBase[j].iPhrase; k++)
        if (!strcmp(map + 2, PYFAList[i].pyBase[j].phrase[k].strMap)
                && !strcmp(phrase + clen, PYFAList[i].pyBase[j].phrase[k].strPhrase)) {
            if (incHit) {
                PYFAList[i].pyBase[j].phrase[k].iHit ++;
                PYFAList[i].pyBase[j].phrase[k].iIndex = ++pystate->iCounter;
            }
            return false;
        }
    //下面将词组添加到列表中
    newPhrase = fcitx_utils_new(PyUsrPhrase);
    newPhrase->phrase.strMap = (char*)fcitx_utils_malloc0(sizeof(char) * strlen(map + 2) + 1);
    newPhrase->phrase.strPhrase = (char*)fcitx_utils_malloc0(sizeof(char) * strlen(phrase + clen) + 1);
    strcpy(newPhrase->phrase.strMap, map + 2);
    strcpy(newPhrase->phrase.strPhrase, phrase + clen);
    newPhrase->phrase.iIndex = ++pystate->iCounter;
    newPhrase->phrase.iHit = 1;
    temp = PYFAList[i].pyBase[j].userPhrase;
    userPhrase = PYFAList[i].pyBase[j].userPhrase->next;
    for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
        if (CmpMap(pyconfig, map + 2, userPhrase->phrase.strMap, &iTemp, pystate->bSP) > 0)
            break;
        temp = userPhrase;
        userPhrase = userPhrase->next;
    }

    newPhrase->next = temp->next;
    temp->next = newPhrase;
    PYFAList[i].pyBase[j].iUserPhrase++;
    pystate->iNewPYPhraseCount++;
    if (pystate->iNewPYPhraseCount >= AUTOSAVE_PHRASE_COUNT) {
        SavePYUserPhrase(pystate);
    }

    return true;
}

void PYDelUserPhrase(FcitxPinyinState* pystate, int32_t iPYFA, int iBase, PyUsrPhrase * phrase)
{
    PyUsrPhrase *temp;
    PYFA* PYFAList = pystate->PYFAList;

    //首先定位该词组
    temp = PYFAList[iPYFA].pyBase[iBase].userPhrase;
    while (temp) {
        if (temp->next == phrase)
            break;
        temp = temp->next;
    }
    if (!temp)
        return;
    temp->next = phrase->next;
    free(phrase->phrase.strPhrase);
    free(phrase->phrase.strMap);
    free(phrase);
    PYFAList[iPYFA].pyBase[iBase].iUserPhrase--;
    pystate->iNewPYPhraseCount++;
    if (pystate->iNewPYPhraseCount >= AUTOSAVE_PHRASE_COUNT) {
        SavePYUserPhrase(pystate);
    }
}

int GetBaseMapIndex(FcitxPinyinState* pystate, char *strMap)
{
    int i;

    for (i = 0; i < pystate->iPYFACount; i++) {
        if (!strcmp(strMap, pystate->PYFAList[i].strMap))
            return i;
    }
    return -1;
}

/*
 * 保存用户词库
 */
void SavePYUserPhrase(FcitxPinyinState* pystate)
{
    int j, k;
    int32_t i, iTemp;
    char *tempfile, *pstr;
    FILE *fp;
    PyPhrase *phrase;
    PYFA* PYFAList = pystate->PYFAList;
    int fd;

    FcitxXDGGetFileUserWithPrefix("pinyin", "", "w", NULL);
    FcitxXDGGetFileUserWithPrefix("pinyin", PINYIN_TEMP_FILE, NULL, &tempfile);
    fd = mkstemp(tempfile);
    fp = NULL;

    if (fd > 0)
        fp = fdopen(fd, "w");

    if (!fp) {
        FcitxLog(ERROR, _("Cannot Save User Pinyin Database: %s"), tempfile);
        free(tempfile);
        return;
    }

    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            iTemp = PYFAList[i].pyBase[j].iUserPhrase;
            if (iTemp) {
                char clen;
                fcitx_utils_write_int32(fp, i);
                clen = strlen(PYFAList[i].pyBase[j].strHZ);
                fwrite(&clen, sizeof(char), 1, fp);
                fwrite(PYFAList[i].pyBase[j].strHZ, sizeof(char) * clen, 1, fp);
                fcitx_utils_write_int32(fp, iTemp);
                phrase = USER_PHRASE_NEXT(PYFAList[i].pyBase[j].userPhrase);
                for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
                    iTemp = strlen(phrase->strMap);
                    fcitx_utils_write_int32(fp, iTemp);
                    fwrite(phrase->strMap, sizeof(char) * iTemp, 1, fp);

                    iTemp = strlen(phrase->strPhrase);
                    fcitx_utils_write_int32(fp, iTemp);
                    fwrite(phrase->strPhrase, sizeof(char) * iTemp, 1, fp);

                    fcitx_utils_write_uint32(fp, phrase->iIndex);
                    fcitx_utils_write_uint32(fp, phrase->iHit);
                    phrase = USER_PHRASE_NEXT(phrase);
                }
            }
        }
    }

    fclose(fp);
    FcitxXDGGetFileUserWithPrefix("pinyin", PY_USERPHRASE_FILE, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(tempfile, pstr);
    free(pstr);
    free(tempfile);
    pystate->iNewPYPhraseCount = 0;
}

void SavePYFreq(FcitxPinyinState *pystate)
{
    int32_t i;
    int k;
    char *pstr;
    char *tempfile;
    FILE *fp;
    PyFreq *pPyFreq;
    HZ *hz;
    int fd;

    FcitxXDGGetFileUserWithPrefix("pinyin", "", "w", NULL);
    FcitxXDGGetFileUserWithPrefix("pinyin", PINYIN_TEMP_FILE, NULL, &tempfile);
    fd = mkstemp(tempfile);
    fp = NULL;

    if (fd > 0)
        fp = fdopen(fd, "w");

    if (!fp) {
        FcitxLog(ERROR, _("Cannot Save Frequent word: %s"), tempfile);
        free(tempfile);
        return;
    }

    i = 0;
    pPyFreq = pystate->pyFreq->next;
    while (pPyFreq) {
        i++;
        pPyFreq = pPyFreq->next;
    }
    fcitx_utils_write_int32(fp, i);
    pPyFreq = pystate->pyFreq->next;
    while (pPyFreq) {
        fwrite(pPyFreq->strPY, sizeof(char) * 11, 1, fp);
        fcitx_utils_write_int32(fp, pPyFreq->iCount);
        hz = pPyFreq->HZList->next;
        for (k = 0; k < pPyFreq->iCount; k++) {
            char slen = strlen(hz->strHZ);
            fwrite(&slen, sizeof(char), 1, fp);
            fwrite(hz->strHZ, sizeof(char) * slen, 1, fp);
            fcitx_utils_write_int32(fp, hz->iPYFA);
            fcitx_utils_write_uint32(fp, hz->iHit);
            fcitx_utils_write_int32(fp, hz->iIndex);

            hz = hz->next;
        }
        pPyFreq = pPyFreq->next;
    }

    fclose(fp);

    FcitxXDGGetFileUserWithPrefix("pinyin", PY_FREQ_FILE, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(tempfile, pstr);

    free(pstr);
    free(tempfile);
    pystate->iNewFreqCount = 0;
}

/*
 * 保存索引文件
 */
void SavePYIndex(FcitxPinyinState *pystate)
{
    int32_t i, j, k;
    char *pstr;
    char *tempfile;
    FILE *fp;
    PYFA* PYFAList = pystate->PYFAList;
    int fd;

    FcitxXDGGetFileUserWithPrefix("pinyin", "", "w", NULL);
    FcitxXDGGetFileUserWithPrefix("pinyin", PINYIN_TEMP_FILE, NULL, &tempfile);
    fd = mkstemp(tempfile);
    fp = NULL;

    if (fd > 0)
        fp = fdopen(fd, "w");

    if (!fp) {
        FcitxLog(ERROR, _("Cannot Save Pinyin Index: %s"), tempfile);
        free(tempfile);
        return;
    }

    fcitx_utils_write_uint32(fp, PY_INDEX_MAGIC_NUMBER);

    //Save Counter
    fcitx_utils_write_uint32(fp, pystate->iCounter);
    //先保存索引不为0的单字
    k = -1;
    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            if (PYFAList[i].pyBase[j].iIndex > pystate->iOrigCounter) {
                fcitx_utils_write_int32(fp, i);
                fcitx_utils_write_int32(fp, j);
                fcitx_utils_write_int32(fp, k);
                fcitx_utils_write_uint32(fp, PYFAList[i].pyBase[j].iIndex);
                fcitx_utils_write_uint32(fp, PYFAList[i].pyBase[j].iHit);
            }
        }
    }

    //再保存索引不为0的系统词组
    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            for (k = 0; k < PYFAList[i].pyBase[j].iPhrase; k++) {
                if (PYFAList[i].pyBase[j].phrase[k].iIndex
                    > pystate->iOrigCounter) {
                    fcitx_utils_write_int32(fp, i);
                    fcitx_utils_write_int32(fp, j);
                    fcitx_utils_write_int32(fp, k);
                    fcitx_utils_write_uint32(
                        fp, PYFAList[i].pyBase[j].phrase[k].iIndex);
                    fcitx_utils_write_uint32(
                        fp, PYFAList[i].pyBase[j].phrase[k].iHit);
                }
            }
        }
    }

    fclose(fp);

    FcitxXDGGetFileUserWithPrefix("pinyin", PY_INDEX_FILE, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(tempfile, pstr);

    free(pstr);
    free(tempfile);
    pystate->iOrderCount = 0;
}

/*
 * 设置拼音的常用字表
 * 只有以下情形才能设置
 *  当用户输入单字时
 * 至于常用词的问题暂时不考虑
 */
void PYAddFreq(FcitxPinyinState* pystate, PYCandWord* pycandWord)
{
    int i;
    HZ *HZTemp;
    PyFreq *freq;
    HZ *hz;
    PYFA* PYFAList = pystate->PYFAList;
    PyFreq* pCurFreq = pystate->pyFreq->next;
    for (i = 0; i < pystate->iPYFreqCount; i++) {
        if (!strcmp(pystate->strFindString, pCurFreq->strPY))
            break;
        pCurFreq = pCurFreq->next;
    }

    //能到这儿来，就说明候选列表中都是单字
    //首先，看这个字是不是已经在常用字表中
    i = 1;
    if (pCurFreq) {
        i = -1;
        if (pycandWord->iWhich != PY_CAND_FREQ) {
            //说明该字是系统单字
            HZTemp = pCurFreq->HZList->next;
            for (i = 0; i < pCurFreq->iCount; i++) {
                if (!strcmp(PYFAList[pycandWord->cand.base.iPYFA].pyBase[pycandWord->cand.base.iBase].strHZ, HZTemp->strHZ)) {
                    i = -1;
                    break;
                }
                HZTemp = HZTemp->next;
            }
        }
    }
    //借用i来指示是否需要添加新的常用字
    if (i < 0)
        return;
    //需要添加该字，此时该字必然是系统单字
    if (!pCurFreq) {
        freq = fcitx_utils_new(PyFreq);
        freq->HZList = fcitx_utils_new(HZ);
        freq->HZList->next = NULL;
        strcpy(freq->strPY, pystate->strFindString);
        freq->next = NULL;
        freq->iCount = 0;
        pCurFreq = pystate->pyFreq;
        for (i = 0; i < pystate->iPYFreqCount; i++)
            pCurFreq = pCurFreq->next;
        pCurFreq->next = freq;
        pystate->iPYFreqCount++;
        pCurFreq = freq;
    }

    HZTemp = fcitx_utils_new(HZ);
    strcpy(HZTemp->strHZ, PYFAList[pycandWord->cand.base.iPYFA].pyBase[pycandWord->cand.base.iBase].strHZ);
    HZTemp->iPYFA = pycandWord->cand.base.iPYFA;
    HZTemp->iHit = 0;
    HZTemp->iIndex = 0;
    HZTemp->next = NULL;
    //将HZTemp加到链表尾部
    hz = pCurFreq->HZList;
    for (i = 0; i < pCurFreq->iCount; i++)
        hz = hz->next;
    hz->next = HZTemp;
    pCurFreq->iCount++;
    pystate->iNewFreqCount++;
    if (pystate->iNewFreqCount >= AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
    }
}

/*
 * 删除拼音常用字表中的某个字
 */
void PYDelFreq(FcitxPinyinState *pystate, PYCandWord* pycandWord)
{
    HZ *hz;

    //能到这儿来，就说明候选列表中都是单字
    //首先，看这个字是不是已经在常用字表中
    if (pycandWord->iWhich != PY_CAND_FREQ)
        return;
    //先找到需要删除单字的位置
    hz = pycandWord->cand.freq.pyFreq->HZList;
    while (hz->next != pycandWord->cand.freq.hz)
        hz = hz->next;
    hz->next = pycandWord->cand.freq.hz->next;
    free(pycandWord->cand.freq.hz);
    pycandWord->cand.freq.pyFreq->iCount--;
    pystate->iNewFreqCount++;
    if (pystate->iNewFreqCount >= AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
    }
}

/*
 * 判断一个字是否已经是常用字
 */
boolean PYIsInFreq(PyFreq* pCurFreq, char *strHZ)
{
    HZ *hz;
    int i;

    if (!pCurFreq)
        return false;
    hz = pCurFreq->HZList->next;
    for (i = 0; i < pCurFreq->iCount; i++) {
        if (!strcmp(strHZ, hz->strHZ))
            return true;
        hz = hz->next;
    }

    return false;
}

/*
 * 取得拼音的联想字串
 *  按照频率来定排列顺序
 */
INPUT_RETURN_VALUE PYGetRemindCandWords(void *arg)
{
    int i, j;
    PyPhrase *phrase;
    FcitxPinyinState* pystate = (FcitxPinyinState*) arg;
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(pystate->owner);
    boolean bDisablePagingInRemind = config->bDisablePagingInRemind;
    FcitxInputState *input = FcitxInstanceGetInputState(pystate->owner);
    PYFA* PYFAList = pystate->PYFAList;

    if (!pystate->strPYRemindSource[0])
        return IRV_TO_PROCESS;

    PyBase* pyBaseForRemind = NULL;
    for (i = 0; i < pystate->iPYFACount; i++) {
        if (!strncmp(pystate->strPYRemindMap, PYFAList[i].strMap, 2)) {
            for (j = 0; j < PYFAList[i].iBase; j++) {
                if (!fcitx_utf8_strncmp(pystate->strPYRemindSource, PYFAList[i].pyBase[j].strHZ, 1)) {
                    pyBaseForRemind = &(PYFAList[i].pyBase[j]);
                    goto _HIT;
                }
            }
        }
    }

_HIT:
    if (!pyBaseForRemind)
        return IRV_TO_PROCESS;

    UT_array candtemp;
    utarray_init(&candtemp, fcitx_ptr_icd);

    for (i = 0; i < pyBaseForRemind->iPhrase; i++) {

        if (bDisablePagingInRemind && utarray_len(&candtemp) >= FcitxCandidateWordGetPageSize(FcitxInputStateGetCandidateList(input)))
            break;

        if (fcitx_utf8_strlen(pystate->strPYRemindSource) == 1) {
            if (fcitx_utf8_strlen(pyBaseForRemind->phrase[i].strPhrase) == 1) {
                PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
                PYAddRemindCandWord(pystate, &pyBaseForRemind->phrase[i], pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        } else if (strlen(pyBaseForRemind->phrase[i].strPhrase) == strlen(pystate->strPYRemindSource)) {
            if (!strncmp
                    (pystate->strPYRemindSource + fcitx_utf8_char_len(pystate->strPYRemindSource),
                     pyBaseForRemind->phrase[i].strPhrase, strlen(pystate->strPYRemindSource + fcitx_utf8_char_len(pystate->strPYRemindSource)))
               ) {
                PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
                PYAddRemindCandWord(pystate, &pyBaseForRemind->phrase[i], pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        }
    }

    phrase = &pyBaseForRemind->userPhrase->next->phrase;
    for (i = 0; i < pyBaseForRemind->iUserPhrase; i++) {
        if (bDisablePagingInRemind && utarray_len(&candtemp) >= FcitxCandidateWordGetPageSize(FcitxInputStateGetCandidateList(input)))
            break;

        if (fcitx_utf8_strlen(pystate->strPYRemindSource) == 1) {
            if (fcitx_utf8_strlen(phrase->strPhrase) == 1) {
                PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
                PYAddRemindCandWord(pystate, phrase, pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        } else if (strlen(phrase->strPhrase) == strlen(pystate->strPYRemindSource)) {
            if (!strncmp
                    (pystate->strPYRemindSource + fcitx_utf8_char_len(pystate->strPYRemindSource),
                     phrase->strPhrase, strlen(pystate->strPYRemindSource + fcitx_utf8_char_len(pystate->strPYRemindSource)))) {
                PYCandWord *pycandWord = fcitx_utils_new(PYCandWord);
                PYAddRemindCandWord(pystate, phrase, pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        }

        phrase = USER_PHRASE_NEXT(phrase);
    }

    FcitxMessages *msg_up = FcitxInputStateGetAuxUp(input);
    FcitxMessagesSetMessageCount(msg_up, 0);
    FcitxMessagesAddMessageStringsAtLast(msg_up, MSG_TIPS, _("Remind: "));
    FcitxMessagesAddMessageStringsAtLast(msg_up, MSG_INPUT,
                                         pystate->strPYRemindSource);

    PYCandWord** pcand = NULL;
    for (pcand = (PYCandWord**) utarray_front(&candtemp);
            pcand != NULL;
            pcand = (PYCandWord**) utarray_next(&candtemp, pcand)) {
        FcitxCandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup((*pcand)->cand.remind.phrase->strPhrase + (*pcand)->cand.remind.iLength);
        candWord.wordType = MSG_OTHER;

        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }

    utarray_done(&candtemp);

    FcitxInputStateSetIsInRemind(input, (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0));
    return IRV_DISPLAY_CANDWORDS;
}

void PYAddRemindCandWord(FcitxPinyinState* pystate, PyPhrase * phrase, PYCandWord* pycandWord)
{
    PYRemindCandWord* pyRemindCandWords = &pycandWord->cand.remind;

    pycandWord->iWhich = PY_CAND_REMIND;
    pyRemindCandWords->phrase = phrase;
    pyRemindCandWords->iLength = strlen(pystate->strPYRemindSource) - fcitx_utf8_char_len(pystate->strPYRemindSource);
}

void PYGetPYByHZ(FcitxPinyinState*pystate, const char *strHZ, char *strPY)
{
    int i, j;
    char str_PY[MAX_PY_LENGTH + 1];
    PYFA* PYFAList = pystate->PYFAList;

    strPY[0] = '\0';
    for (i = pystate->iPYFACount - 1; i >= 0; i--) {
        if (MapToPY(PYFAList[i].strMap, str_PY)) {
            for (j = 0; j < PYFAList[i].iBase; j++) {
                if (!strcmp(PYFAList[i].pyBase[j].strHZ, strHZ)) {
                    if (strPY[0])
                        strcat(strPY, " ");
                    strcat(strPY, str_PY);
                }
            }
        }
    }
}

void SavePY(void *arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    if (pystate->iNewPYPhraseCount)
        SavePYUserPhrase(pystate);
    if (pystate->iOrderCount)
        SavePYIndex(pystate);
    if (pystate->iNewFreqCount)
        SavePYFreq(pystate);
}

void ReloadConfigPY(void* arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;

    LoadPYConfig(&pystate->pyconfig);
}

void PinyinMigration()
{
    char* olduserphrase, *oldpyindex, *newuserphrase, *newpyindex;
    FcitxXDGGetFileUserWithPrefix("", PY_USERPHRASE_FILE, NULL, &olduserphrase);
    FcitxXDGGetFileUserWithPrefix("", PY_INDEX_FILE, NULL, &oldpyindex);
    FcitxXDGGetFileUserWithPrefix("pinyin", PY_USERPHRASE_FILE, NULL, &newuserphrase);
    FcitxXDGGetFileUserWithPrefix("pinyin", PY_INDEX_FILE, NULL, &newpyindex);

    struct stat olduserphrasestat, oldpyindexstat, newuserphrasestat, newpyindexstat;

    /* check old file are all not exist */
    if (stat(newpyindex, &newpyindexstat) == -1 && stat(newuserphrase, &newuserphrasestat) == -1) {
        if (stat(oldpyindex, &oldpyindexstat) == 0 || stat(olduserphrase, &olduserphrasestat) == 0) {
            FcitxLog(INFO, _("Migrate the old file path to the new one"));
            /* there might be a very very rare case, that ~/.config/fcitx/pinyin
             * and ~/.config/fcitx in different filesystem, who the fucking guy
             * do this meaningless go die */
            link(oldpyindex, newpyindex);
            link(olduserphrase, newuserphrase);
        }
    }


    free(oldpyindex);
    free(olduserphrase);
    free(newpyindex);
    free(newuserphrase);
}

/* decend sort */
int PYCandWordCmp(const void* b, const void *a, void* arg)
{
    const PYCandWord* canda = *(PYCandWord**)a;
    const PYCandWord* candb = *(PYCandWord**)b;
    PYCandWordSortContext *context = arg;

    switch (context->type) {
    case PY_CAND_BASE: {
        switch (context->order) {
        case AD_NO:
            return 0;
        case AD_FAST: {
            int delta = context->pystate->PYFAList[canda->cand.base.iPYFA].pyBase[canda->cand.base.iBase].iIndex
                        - context->pystate->PYFAList[candb->cand.base.iPYFA].pyBase[candb->cand.base.iBase].iIndex;
            if (delta != 0)
                return delta;

            delta = context->pystate->PYFAList[canda->cand.base.iPYFA].pyBase[canda->cand.base.iBase].iHit
                    - context->pystate->PYFAList[candb->cand.base.iPYFA].pyBase[candb->cand.base.iBase].iHit;
            return delta;
        }
        break;
        case AD_FREQ: {
            int delta = context->pystate->PYFAList[canda->cand.base.iPYFA].pyBase[canda->cand.base.iBase].iHit
                        - context->pystate->PYFAList[candb->cand.base.iPYFA].pyBase[candb->cand.base.iBase].iHit;
            if (delta != 0)
                return delta;

            delta = context->pystate->PYFAList[canda->cand.base.iPYFA].pyBase[canda->cand.base.iBase].iIndex
                    - context->pystate->PYFAList[candb->cand.base.iPYFA].pyBase[candb->cand.base.iBase].iIndex;
            return delta;
        }
        break;
        }
    }
    break;
    case PY_CAND_SYSPHRASE:
    case PY_CAND_USERPHRASE: {
        switch (context->order) {
        case AD_NO:
            return strlen(canda->cand.phrase.phrase->strPhrase) - strlen(candb->cand.phrase.phrase->strPhrase);
            break;
        case AD_FAST: {
            int size = strlen(canda->cand.phrase.phrase->strPhrase) - strlen(candb->cand.phrase.phrase->strPhrase);
            if (size != 0)
                return size;

            if (canda->cand.phrase.phrase->iIndex - candb->cand.phrase.phrase->iIndex != 0)
                return canda->cand.phrase.phrase->iIndex - candb->cand.phrase.phrase->iIndex;

            return canda->cand.phrase.phrase->iHit - candb->cand.phrase.phrase->iHit;
        }
        break;
        case AD_FREQ: {
            int size = strlen(canda->cand.phrase.phrase->strPhrase) - strlen(candb->cand.phrase.phrase->strPhrase);
            if (size != 0)
                return size;

            if (canda->cand.phrase.phrase->iHit - candb->cand.phrase.phrase->iHit != 0)
                return canda->cand.phrase.phrase->iHit - candb->cand.phrase.phrase->iHit;

            return canda->cand.phrase.phrase->iIndex - candb->cand.phrase.phrase->iIndex;
        }
        break;
        }
    }
    break;
    case PY_CAND_FREQ: {
        switch (context->order) {
        case AD_NO:
            return 0;
        case AD_FAST:
            return canda->cand.freq.hz->iIndex - candb->cand.freq.hz->iIndex;
        case AD_FREQ:
            return canda->cand.freq.hz->iHit - candb->cand.freq.hz->iHit;
        }
    }
    break;
    case PY_CAND_REMIND:
        return canda->cand.remind.phrase->iHit - canda->cand.remind.phrase->iHit;
    default:
        return 0;
    }

    return 0;
}

static char*
PYSP2QP(FcitxPinyinState *pystate, const char* strSP)
{
    char strQP[MAX_PY_LENGTH + 1] = "";
    SP2QP(&pystate->pyconfig, strSP, strQP);
    return strdup(strQP);
}

static boolean
PYGetPYMapByHZ(FcitxPinyinState* pystate, char* strHZ,
               char* mapHint, char* strMap)
{
    int i, j;
    PYFA* PYFAList = pystate->PYFAList;

    strMap[0] = '\0';
    for (i = pystate->iPYFACount - 1; i >= 0; i--) {
        if (!Cmp2Map(&pystate->pyconfig, PYFAList[i].strMap, mapHint, false)) {
            for (j = 0; j < PYFAList[i].iBase; j++) {
                if (!strcmp(PYFAList[i].pyBase[j].strHZ, strHZ)) {
                    strcpy(strMap, PYFAList[i].strMap);
                    return true;
                }
            }
        }
    }
    return false;
}

static void
PYAddUserPhraseFromCString(FcitxPinyinState *pystate, const char *strHZ)
{
    const char *pivot;
    char *sp;
    char singleHZ[UTF8_MAX_LENGTH + 1];
    char strMap[3];
    if (!fcitx_utf8_check_string(strHZ))
        return;

    pivot = strHZ;
    size_t hzCount = fcitx_utf8_strlen(strHZ);
    size_t hzCountLocal = 0;

    if (pystate->iPYSelected) {
        int i;
        for (i = 0 ; i < pystate->iPYSelected; i ++) {
            hzCountLocal += strlen(pystate->pySelected[i].strMap) / 2;
        }
    }
    hzCountLocal += pystate->findMap.iHZCount;

    /* in order not to get a wrong one, use strict check */
    if (hzCountLocal != hzCount || hzCount > MAX_PY_PHRASE_LENGTH)
        return;
    char *totalMap = fcitx_utils_malloc0(1 + 2 * hzCount);

    if (pystate->iPYSelected) {
        int i;
        for (i = 0 ; i < pystate->iPYSelected; i ++)
            strcat(totalMap, pystate->pySelected[i].strMap);
        strHZ = fcitx_utf8_get_nth_char(strHZ, strlen(totalMap) / 2);
    }

    int i = 0;
    while (*strHZ) {
        uint32_t chr;

        sp = fcitx_utf8_get_char(strHZ, &chr);
        size_t len = sp - strHZ;
        strncpy(singleHZ, strHZ, len);
        singleHZ[len] = '\0';

        if (!PYGetPYMapByHZ(pystate, singleHZ, pystate->findMap.strMap[i],
                            strMap)) {
            free(totalMap);
            return;
        }

        strncat(totalMap, strMap, 2);
        strHZ = sp;
        i ++;
    }

    PYAddUserPhrase(pystate, pivot, totalMap, true);
    free(totalMap);
}

#include "fcitx-pinyin-addfunctions.h"
