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
#ifdef FCITX_HAVE_CONFIG_H
#include "config.h"
#endif

#include <libintl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#ifdef HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#endif

#include "fcitx/fcitx.h"

#include "fcitx-utils/utils.h"
#include "fcitx/ime.h"
#include "fcitx/keys.h"
#include "fcitx/ui.h"
#include "py.h"
#include "PYFA.h"
#include "pyParser.h"
#include "sp.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/profile.h"
#include "fcitx/configfile.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/xdg.h"
#include "pyconfig.h"
#include "fcitx/instance.h"
#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "fcitx/candidate.h"

#define TEMP_FILE       "FCITX_DICT_TEMP"
#define PY_INDEX_MAGIC_NUMBER 0xf7462e34

FCITX_EXPORT_API
FcitxIMClass ime = {
    PYCreate,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

const UT_icd pycand_icd = {sizeof(PYCandWord*) , NULL, NULL, NULL };

typedef struct _PYCandWordSortContext {
    PY_CAND_WORD_TYPE type;
    ADJUSTORDER order;
    FcitxPinyinState* pystate;
} PYCandWordSortContext;

static void LoadPYPhraseDict(FcitxPinyinState* pystate, FILE* fp, boolean isSystem, boolean stripDup);
static void * LoadPYBaseDictWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetPYByHZWrapper(void* arg, FcitxModuleFunctionArg args);
static void * DoPYInputWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetCandWordsWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetFindStringWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYResetWrapper(void* arg, FcitxModuleFunctionArg args);
static void ReloadConfigPY(void* arg);
static void PinyinMigration();
static int PYCandWordCmp(const void* b, const void* a, void* arg);
static void* PYSP2QP(void* arg, FcitxModuleFunctionArg args);
static boolean PYGetPYMapByHZ(FcitxPinyinState*pystate, char *strHZ, char* mapHint, char *strMap);
static void PYAddUserPhraseFromCString(void* arg, FcitxModuleFunctionArg args);

void *PYCreate(FcitxInstance* instance)
{
    FcitxPinyinState *pystate = fcitx_malloc0(sizeof(FcitxPinyinState));
    FcitxAddon* pyaddon = GetAddonByName(FcitxInstanceGetAddons(instance), FCITX_PINYIN_NAME);
    InitMHPY(&pystate->pyconfig.MHPY_C, MHPY_C_TEMPLATE);
    InitMHPY(&pystate->pyconfig.MHPY_S, MHPY_S_TEMPLATE);
    InitPYTable(&pystate->pyconfig);
    if (!LoadPYConfig(&pystate->pyconfig)) {
        free(pystate->pyconfig.MHPY_C);
        free(pystate->pyconfig.MHPY_S);
        free(pystate->pyconfig.PYTable);
        free(pystate);
        return NULL;
    }

    PinyinMigration();

    FcitxRegisterIMv2(instance,
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
                      ReloadConfigPY,
                      NULL,
                      pystate->pyconfig.iPinyinPriority,
                      "zh_CN"
                     );
    FcitxRegisterIMv2(instance,
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
                      ReloadConfigPY,
                      NULL,
                      pystate->pyconfig.iShuangpinPriority,
                      "zh_CN"
                     );
    pystate->owner = instance;

    /* ensure the order! */
    AddFunction(pyaddon, LoadPYBaseDictWrapper); // 0
    AddFunction(pyaddon, PYGetPYByHZWrapper); // 1
    AddFunction(pyaddon, DoPYInputWrapper); // 2
    AddFunction(pyaddon, PYGetCandWordsWrapper); // 3
    AddFunction(pyaddon, PYGetFindStringWrapper); // 4
    AddFunction(pyaddon, PYResetWrapper); // 5
    AddFunction(pyaddon, PYSP2QP); // 6
    AddFunction(pyaddon, PYAddUserPhraseFromCString); // 6
    return pystate;
}

boolean PYInit(void *arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    pystate->bSP = false;
    return true;
}

boolean LoadPYBaseDict(FcitxPinyinState *pystate)
{
    FILE *fp;
    int i, j, iLen;

    fp = GetXDGFileWithPrefix("pinyin", PY_BASE_FILE, "r", NULL);
    if (!fp)
        return false;

    fread(&pystate->iPYFACount, sizeof(int), 1, fp);
    pystate->PYFAList = (PYFA *) fcitx_malloc0(sizeof(PYFA) * pystate->iPYFACount);
    PYFA* PYFAList = pystate->PYFAList;
    for (i = 0; i < pystate->iPYFACount; i++) {
        fread(PYFAList[i].strMap, sizeof(char) * 2, 1, fp);
        PYFAList[i].strMap[2] = '\0';

        fread(&(PYFAList[i].iBase), sizeof(int), 1, fp);
        PYFAList[i].pyBase = (PyBase *) fcitx_malloc0(sizeof(PyBase) * PYFAList[i].iBase);
        for (j = 0; j < PYFAList[i].iBase; j++) {
            int8_t len;
            fread(&len, sizeof(char), 1, fp);
            fread(PYFAList[i].pyBase[j].strHZ, sizeof(char) * len, 1, fp);
            PYFAList[i].pyBase[j].strHZ[len] = '\0';
            fread(&iLen, sizeof(int), 1, fp);
            PYFAList[i].pyBase[j].iIndex = iLen;
            PYFAList[i].pyBase[j].iHit = 0;
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            PYFAList[i].pyBase[j].iPhrase = 0;
            PYFAList[i].pyBase[j].iUserPhrase = 0;
            PYFAList[i].pyBase[j].userPhrase = (PyUsrPhrase *) fcitx_malloc0(sizeof(PyUsrPhrase));
            PYFAList[i].pyBase[j].userPhrase->next = PYFAList[i].pyBase[j].userPhrase;
        }
    }

    fclose(fp);
    pystate->bPYBaseDictLoaded = true;

    pystate->iOrigCounter = pystate->iCounter;

    pystate->pyFreq = (PyFreq *) fcitx_malloc0(sizeof(PyFreq));
    pystate->pyFreq->next = NULL;

    return true;
}

StringHashSet *GetPYPhraseFiles()
{
    char **pinyinPath;
    size_t len;
    char pathBuf[PATH_MAX];
    int i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;

    StringHashSet* sset = NULL;

    pinyinPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/pinyin" , DATADIR, PACKAGE "/pinyin");

    for (i = 0; i < len; i++) {
        snprintf(pathBuf, sizeof(pathBuf), "%s", pinyinPath[i]);
        pathBuf[sizeof(pathBuf) - 1] = '\0';

        dir = opendir(pathBuf);
        if (dir == NULL)
            continue;

        /* collect all *.conf files */
        while ((drt = readdir(dir)) != NULL) {
            size_t nameLen = strlen(drt->d_name);
            if (nameLen <= strlen(".mb"))
                continue;
            memset(pathBuf, 0, sizeof(pathBuf));

            if (strcmp(drt->d_name + nameLen - strlen(".mb"), ".mb") != 0)
                continue;
            if (strcmp(drt->d_name, PY_PHRASE_FILE) == 0)
                continue;
            if (strcmp(drt->d_name, PY_USERPHRASE_FILE) == 0)
                continue;
            if (strcmp(drt->d_name, PY_SYMBOL_FILE) == 0)
                continue;
            if (strcmp(drt->d_name, PY_BASE_FILE) == 0)
                continue;
            if (strcmp(drt->d_name, PY_FREQ_FILE) == 0)
                continue;
            FcitxLog(INFO, "Try %s", drt->d_name);
            snprintf(pathBuf, sizeof(pathBuf), "%s/%s", pinyinPath[i], drt->d_name);

            if (stat(pathBuf, &fileStat) == -1)
                continue;

            if (fileStat.st_mode & S_IFREG) {
                StringHashSet *string;
                HASH_FIND_STR(sset, drt->d_name, string);
                if (!string) {
                    char *bStr = strdup(drt->d_name);
                    string = fcitx_malloc0(sizeof(StringHashSet));
                    memset(string, 0, sizeof(StringHashSet));
                    string->name = bStr;
                    HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
                }
            }
        }

        closedir(dir);
    }

    FreeXDGPath(pinyinPath);

    return sset;
}

void LoadPYPhraseDict(FcitxPinyinState* pystate, FILE *fp, boolean isSystem, boolean stripDup)
{
    int i, j , k, count, iLen;
    char strBase[UTF8_MAX_LENGTH + 1];
    PyPhrase *phrase = NULL, *temp;
    PYFA* PYFAList = pystate->PYFAList;
    while (!feof(fp)) {
        int8_t clen;
        if (!fread(&i, sizeof(int), 1, fp))
            break;
        if (!fread(&clen, sizeof(int8_t), 1, fp))
            break;
        if (clen <= 0 || clen > UTF8_MAX_LENGTH)
            break;
        if (!fread(strBase, sizeof(char) * clen, 1, fp))
            break;
        strBase[clen] = '\0';
        if (!fread(&count, sizeof(int), 1, fp))
            break;

        j = GetBaseIndex(pystate, i, strBase);
        if (j == -1)
            break;

        if (isSystem) {
            phrase = (PyPhrase *) fcitx_malloc0(sizeof(PyPhrase) * count);
            temp = phrase;
        } else {
            PYFAList[i].pyBase[j].iUserPhrase = count;
            temp = &PYFAList[i].pyBase[j].userPhrase->phrase;
        }

        for (k = 0; k < count; k++) {
            if (!isSystem)
                phrase = (PyPhrase *) fcitx_malloc0(sizeof(PyUsrPhrase));
            fread(&iLen, sizeof(int), 1, fp);
            phrase->strMap = (char *) fcitx_malloc0(sizeof(char) * (iLen + 1));
            fread(phrase->strMap, sizeof(char) * iLen, 1, fp);
            phrase->strMap[iLen] = '\0';
            fread(&iLen, sizeof(int), 1, fp);
            phrase->strPhrase = (char *) fcitx_malloc0(sizeof(char) * (iLen + 1));
            fread(phrase->strPhrase, sizeof(char) * iLen, 1, fp);
            phrase->strPhrase[iLen] = '\0';
            fread(&iLen, sizeof(unsigned int), 1, fp);
            phrase->iIndex = iLen;
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            if (isSystem) {
                phrase->iHit = 0;
                phrase ++;
            } else {
                fread(&iLen, sizeof(int), 1, fp);
                phrase->iHit = iLen;

                ((PyUsrPhrase*) phrase)->next = ((PyUsrPhrase*) temp)->next;
                ((PyUsrPhrase*) temp)->next = (PyUsrPhrase*) phrase;

                temp = phrase;
            }
        }

        if (isSystem) {
            if (PYFAList[i].pyBase[j].iPhrase == 0) {
                PYFAList[i].pyBase[j].iPhrase = count;
                PYFAList[i].pyBase[j].phrase = temp;
            } else {
                int m, n;
                boolean *flag = fcitx_malloc0(sizeof(boolean) * count);
                memset(flag, 0, sizeof(boolean) * count);
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
                }
                for (m = 0; m < count; m ++) {
                    if (flag[m]) {
                        free(phrase[m].strMap);
                        free(phrase[m].strPhrase);
                    } else {
                        memcpy(&PYFAList[i].pyBase[j].phrase[orig], &phrase[m], sizeof(PyPhrase));
                        orig ++ ;
                    }
                }
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
    int i, j, k, iLen;
    uint iIndex;
    PyFreq *pyFreqTemp, *pPyFreq;
    HZ *HZTemp, *pHZ;
    PYFA* PYFAList = pystate->PYFAList;

    pystate->bPYOtherDictLoaded = true;

    fp = GetXDGFileWithPrefix("pinyin", PY_PHRASE_FILE, "r", NULL);
    if (!fp)
        FcitxLog(ERROR, _("Can not find System Database of Pinyin!"));
    else {
        LoadPYPhraseDict(pystate, fp, true, false);
        fclose(fp);
        StringHashSet *sset = GetPYPhraseFiles();
        StringHashSet *curStr;
        while (sset) {
            curStr = sset;
            HASH_DEL(sset, curStr);
            char *filename;
            fp = GetXDGFileWithPrefix("pinyin", curStr->name, "r", &filename);
            FcitxLog(INFO, _("Load extra dict: %s"), filename);
            free(filename);
            LoadPYPhraseDict(pystate, fp, true, true);
            fclose(fp);
            free(curStr->name);
            free(curStr);
        }

        pystate->iOrigCounter = pystate->iCounter;
    }

    //下面开始读取用户词库
    fp = GetXDGFileWithPrefix("pinyin", PY_USERPHRASE_FILE, "rb", NULL);
    if (fp) {
        LoadPYPhraseDict(pystate, fp, false, false);
        fclose(fp);
    }
    //下面读取索引文件
    fp = GetXDGFileWithPrefix("pinyin", PY_INDEX_FILE, "rb", NULL);
    if (fp) {
        uint32_t magic = 0;
        fread(&magic, sizeof(uint32_t), 1, fp);
        if (magic == PY_INDEX_MAGIC_NUMBER) {
            fread(&iLen, sizeof(uint), 1, fp);
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            while (!feof(fp)) {
                fread(&i, sizeof(int), 1, fp);
                fread(&j, sizeof(int), 1, fp);
                fread(&k, sizeof(int), 1, fp);
                fread(&iIndex, sizeof(uint), 1, fp);
                fread(&iLen, sizeof(uint), 1, fp);

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
    fp = GetXDGFileWithPrefix("pinyin", PY_FREQ_FILE, "rb", NULL);
    if (fp) {
        pPyFreq = pystate->pyFreq;

        fread(&pystate->iPYFreqCount, sizeof(uint), 1, fp);

        for (i = 0; i < pystate->iPYFreqCount; i++) {
            pyFreqTemp = (PyFreq *) fcitx_malloc0(sizeof(PyFreq));
            pyFreqTemp->next = NULL;
            pyFreqTemp->bIsSym = false;

            fread(pyFreqTemp->strPY, sizeof(char) * 11, 1, fp);
            fread(&j, sizeof(int), 1, fp);
            pyFreqTemp->iCount = j;

            pyFreqTemp->HZList = (HZ *) fcitx_malloc0(sizeof(HZ));
            pyFreqTemp->HZList->next = NULL;
            pHZ = pyFreqTemp->HZList;

            for (k = 0; k < pyFreqTemp->iCount; k++) {
                int8_t slen;
                HZTemp = (HZ *) fcitx_malloc0(sizeof(HZ));
                fread(&slen, sizeof(int8_t), 1, fp);
                fread(HZTemp->strHZ, sizeof(char) * slen, 1, fp);
                HZTemp->strHZ[slen] = '\0';
                fread(&j, sizeof(int), 1, fp);
                HZTemp->iPYFA = j;
                fread(&j, sizeof(int), 1, fp);
                HZTemp->iHit = j;
                fread(&j, sizeof(int), 1, fp);
                HZTemp->iIndex = j;
                pHZ->next = HZTemp;
                pHZ = HZTemp;
            }

            pPyFreq->next = pyFreqTemp;
            pPyFreq = pyFreqTemp;
        }

        fclose(fp);
    }
    //下面读取特殊符号表
    fp = GetXDGFileWithPrefix("pinyin", PY_SYMBOL_FILE, "rb", NULL);
    if (fp) {
        char strTxt[256];
        char str1[MAX_PY_PHRASE_LENGTH * MAX_PY_LENGTH + 1], str2[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
        char *str;

        for (;;) {
            if (!fgets(strTxt, 255, fp))
                break;
            i = strlen(strTxt) - 1;
            if (strTxt[0] == '#')
                continue;
            if (strTxt[i] == '\n')
                strTxt[i] = '\0';
            str = strTxt;
            while (*str == ' ' || *str == '\t')
                str++;
            if (!strlen(str))
                continue;
            sscanf(str, "%s %s", str1, str2);

            //首先看看str1是否已经在列表中
            pyFreqTemp = pystate->pyFreq->next;
            pPyFreq = pystate->pyFreq;
            while (pyFreqTemp) {
                if (!strcmp(pyFreqTemp->strPY, str1))
                    break;
                pPyFreq = pPyFreq->next;
                pyFreqTemp = pyFreqTemp->next;
            }

            if (!pyFreqTemp) {
                pyFreqTemp = (PyFreq *) fcitx_malloc0(sizeof(PyFreq));
                strcpy(pyFreqTemp->strPY, str1);
                pyFreqTemp->next = NULL;
                pyFreqTemp->iCount = 0;
                pyFreqTemp->bIsSym = true;
                pyFreqTemp->HZList = (HZ *) fcitx_malloc0(sizeof(HZ));
                pyFreqTemp->HZList->next = NULL;
                pPyFreq->next = pyFreqTemp;
                pystate->iPYFreqCount++;
            } else {
                if (!pyFreqTemp->bIsSym)        //不能与常用字的编码相同
                    continue;
            }

            HZTemp = (HZ *) fcitx_malloc0(sizeof(HZ));
            strcpy(HZTemp->strHZ, str2);
            HZTemp->next = NULL;
            pyFreqTemp->iCount++;

            pHZ = pyFreqTemp->HZList;
            while (pHZ->next)
                pHZ = pHZ->next;

            pHZ->next = HZTemp;
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

int GetBaseIndex(FcitxPinyinState* pystate, int iPYFA, char *strBase)
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
    char strTemp[MAX_USER_INPUT + 1];

    if (sym == 0 && state == 0)
        sym = -1;

    if (!pystate->bPYBaseDictLoaded)
        LoadPYBaseDict(pystate);
    if (!pystate->bPYOtherDictLoaded)
        LoadPYOtherDict(pystate);

    retVal = IRV_TO_PROCESS;

    /* is not in py special state */
    if (!pystate->bIsPYAddFreq && !pystate->bIsPYDelFreq && !pystate->bIsPYDelUserPhr) {
        if ((IsHotKeyLAZ(sym, state)
                || IsHotKey(sym, state, FCITX_SEPARATOR)
                || (pystate->bSP && FcitxInputStateGetRawInputBufferSize(input) > 0 && pystate->bSP_UseSemicolon && IsHotKey(sym, state, FCITX_SEMICOLON)))) {
            FcitxInputStateSetIsInRemind(input, false);
            FcitxInputStateSetShowCursor(input, true);

            /* we cannot insert seperator in the first, nor there is a existing separator */
            if (IsHotKey(sym, state, FCITX_SEPARATOR)) {
                if (!pystate->iPYInsertPoint)
                    return IRV_TO_PROCESS;
                if (pystate->strFindString[pystate->iPYInsertPoint - 1] == PY_SEPARATOR)
                    return IRV_DO_NOTHING;
            }

            val = i = strlen(pystate->strFindString);

            /* do the insert */
            for (; i > pystate->iPYInsertPoint; i--)
                pystate->strFindString[i] = pystate->strFindString[i - 1];

            pystate->strFindString[pystate->iPYInsertPoint++] = sym;
            pystate->strFindString[val + 1] = '\0';
            ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);

            val = 0;
            for (i = 0; i < pystate->iPYSelected; i++)
                val += utf8_strlen(pystate->pySelected[i].strHZ);

            if (pystate->findMap.iHZCount > (MAX_WORDS_USER_INPUT - val)) {
                UpdateFindString(pystate, val);
                ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
            }

            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_BACKSPACE)) {
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
                /* we cannot delete it if cursor is at the first */
                val = ((pystate->iPYInsertPoint > 1)
                       && (pystate->strFindString[pystate->iPYInsertPoint - 2] == PY_SEPARATOR)) ? 2 : 1;
                int len = strlen(pystate->strFindString + pystate->iPYInsertPoint), i = 0;
                /* 这里使用<=而不是<是因为还有'\0'需要拷贝 */
                for (i = 0; i <= len; i++)
                    pystate->strFindString[i + pystate->iPYInsertPoint - val] = pystate->strFindString[i + pystate->iPYInsertPoint];
                ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                pystate->iPYInsertPoint -= val;

                if (!strlen(pystate->strFindString)) {
                    if (!pystate->iPYSelected)
                        return IRV_CLEAN;

                    val = strlen(pystate->strFindString);
                    strcpy(strTemp, pystate->pySelected[pystate->iPYSelected - 1].strPY);
                    strcat(strTemp, pystate->strFindString);
                    strcpy(pystate->strFindString, strTemp);
                    pystate->iPYInsertPoint = strlen(pystate->strFindString) - val;
                    pystate->iPYSelected--;
                    ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                }

                retVal = IRV_DISPLAY_CANDWORDS;
            } else {
                if (strlen(pystate->strFindString) == 0)
                    return IRV_TO_PROCESS;
                else
                    return IRV_DO_NOTHING;
            }
        } else if (IsHotKey(sym, state, FCITX_DELETE)) {
            if (FcitxInputStateGetRawInputBufferSize(input)) {
                if (pystate->iPYInsertPoint == strlen(pystate->strFindString))
                    return IRV_DO_NOTHING;
                val = (pystate->strFindString[pystate->iPYInsertPoint + 1] == PY_SEPARATOR) ? 2 : 1;
                int len = strlen(pystate->strFindString + pystate->iPYInsertPoint + val), i = 0;
                /* 这里使用<=而不是<是因为还有'\0'需要拷贝 */
                for (i = 0; i <= len; i++)
                    pystate->strFindString[i + pystate->iPYInsertPoint] = pystate->strFindString[i + pystate->iPYInsertPoint + val];

                ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                if (!strlen(pystate->strFindString))
                    return IRV_CLEAN;
                retVal = IRV_DISPLAY_CANDWORDS;
            }
        } else if (IsHotKey(sym, state, FCITX_HOME)) {
            if (FcitxInputStateGetRawInputBufferSize(input) == 0)
                return IRV_DONOT_PROCESS;
            pystate->iPYInsertPoint = 0;
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_END)) {
            if (FcitxInputStateGetRawInputBufferSize(input) == 0)
                return IRV_DONOT_PROCESS;
            pystate->iPYInsertPoint = strlen(pystate->strFindString);
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_RIGHT)) {
            if (!FcitxInputStateGetRawInputBufferSize(input))
                return IRV_TO_PROCESS;
            if (pystate->iPYInsertPoint == strlen(pystate->strFindString))
                return IRV_DO_NOTHING;
            pystate->iPYInsertPoint++;
            retVal = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_LEFT)) {
            if (!FcitxInputStateGetRawInputBufferSize(input))
                return IRV_TO_PROCESS;
            if (pystate->iPYInsertPoint < 2) {
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
                } else if (pystate->iPYInsertPoint) {
                    pystate->iPYInsertPoint--;
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else
                    retVal = IRV_DO_NOTHING;
            } else {
                pystate->iPYInsertPoint--;
                retVal = IRV_DISPLAY_CANDWORDS;
            }
        } else if (IsHotKey(sym, state, FCITX_SPACE)) {
            if (pystate->findMap.iMode == PARSE_ERROR)
                return IRV_DO_NOTHING;

            if (CandidateWordPageCount(FcitxInputStateGetCandidateList(input)) == 0) {
                if (FcitxInputStateGetRawInputBufferSize(input) == 0)
                    return IRV_TO_PROCESS;
                else
                    return IRV_DO_NOTHING;
            }

            retVal = CandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
        } else if (IsHotKey(sym, state, pystate->pyconfig.hkPYDelUserPhr)) {
            if (!pystate->bIsPYDelUserPhr) {
                CandidateWord* candWord = NULL;
                for (candWord = CandidateWordGetCurrentWindow(FcitxInputStateGetCandidateList(input));
                        candWord != NULL;
                        candWord = CandidateWordGetCurrentWindowNext(FcitxInputStateGetCandidateList(input), candWord)) {
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

                CleanInputWindowUp(pystate->owner);
                AddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Press index to delete user phrase (ESC for cancel)"));
                FcitxInputStateSetShowCursor(input, false);

                return IRV_DISPLAY_MESSAGE;
            }
        } else if (IsHotKey(sym, state, pystate->pyconfig.hkPYAddFreq)) {
            if (!pystate->bIsPYAddFreq && pystate->findMap.iHZCount == 1 && FcitxInputStateGetRawInputBufferSize(input)) {
                pystate->bIsPYAddFreq = true;
                FcitxInputStateSetIsDoInputOnly(input, true);

                CleanInputWindowUp(pystate->owner);
                AddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Press number to make word in frequent list"), pystate->strFindString);
                FcitxInputStateSetShowCursor(input, false);

                return IRV_DISPLAY_MESSAGE;
            }
        } else if (IsHotKey(sym, state, pystate->pyconfig.hkPYDelFreq)) {
            if (!pystate->bIsPYDelFreq) {
                val = 0;
                int index = 0;
                CandidateWord* candWord = NULL;
                for (candWord = CandidateWordGetCurrentWindow(FcitxInputStateGetCandidateList(input));
                        candWord != NULL;
                        candWord = CandidateWordGetCurrentWindowNext(FcitxInputStateGetCandidateList(input), candWord)) {
                    index ++ ;
                    if (candWord->owner == pystate) {
                        PYCandWord* pycandWord = candWord->priv;
                        if (pycandWord->iWhich == PY_CAND_FREQ) {
                            val = index;
                        }
                    }
                }

                if (val == 0)
                    return IRV_DO_NOTHING;

                CleanInputWindowUp(pystate->owner);
                if (val == 1)
                    AddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Press 1 to delete %s in frequent list (ESC for cancel)"), pystate->strFindString);
                else
                    AddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Press 1-%d to delete %s in frequent list (ESC for cancel)"), val, pystate->strFindString);

                pystate->bIsPYDelFreq = true;
                FcitxInputStateSetIsDoInputOnly(input, true);

                FcitxInputStateSetShowCursor(input, false);

                return IRV_DISPLAY_MESSAGE;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS) {
        if (IsHotKeyDigit(sym, state)) {
            int iKey = sym - Key_0;
            if (iKey == 0)
                iKey = 10;

            CandidateWord* candWord = CandidateWordGetByIndex(FcitxInputStateGetCandidateList(input), iKey - 1);
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
        } else if (IsHotKey(sym, state, FCITX_ESCAPE))
            return IRV_TO_PROCESS;
        else {
            if (pystate->bIsPYAddFreq || pystate->bIsPYDelFreq || pystate->bIsPYDelUserPhr)
                return IRV_DO_NOTHING;

            //下面实现以词定字
            if (CandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0) {
                if (state == KEY_NONE && (sym == pystate->pyconfig.cPYYCDZ[0] || sym == pystate->pyconfig.cPYYCDZ[1])) {
                    CandidateWord* candWord = CandidateWordGetByIndex(FcitxInputStateGetCandidateList(input), pystate->iYCDZ);
                    if (candWord->owner == pystate) {
                        PYCandWord* pycandWord = candWord->priv;
                        if (pycandWord->iWhich == PY_CAND_USERPHRASE || pycandWord->iWhich == PY_CAND_SYSPHRASE) {
                            char *pBase, *pPhrase;

                            pBase = pystate->PYFAList[pycandWord->cand.phrase.iPYFA].pyBase[pycandWord->cand.phrase.iBase].strHZ;
                            pPhrase = pycandWord->cand.phrase.phrase->strPhrase;

                            if (sym == pystate->pyconfig.cPYYCDZ[0])
                                strcpy(GetOutputString(input), pBase);
                            else {
                                int8_t clen;
                                clen = utf8_char_len(pPhrase);
                                strncpy(GetOutputString(input), pPhrase, clen);
                                GetOutputString(input)[clen] = '\0';
                            }
                            SetMessageCount(FcitxInputStateGetAuxDown(input), 0);
                            return IRV_COMMIT_STRING;
                        }
                    }
                } else if (!FcitxInputStateGetIsInRemind(input)) {
                    val = -1;
                    switch (sym) {
                    case Key_parenright:
                        val++;
                    case Key_parenleft:
                        val++;
                    case Key_asterisk:
                        val++;
                    case Key_ampersand:
                        val++;
                    case Key_asciicircum:
                        val++;
                    case Key_percent:
                        val++;
                    case Key_dollar:
                        val++;
                    case Key_numbersign:
                        val++;
                    case Key_at:
                        val++;
                    case Key_exclam:
                        val++;
                    default:
                        break;
                    }

                    if (val != -1 && CandidateWordGetByIndex(FcitxInputStateGetCandidateList(input), val)) {
                        pystate->iYCDZ = val;
                        return IRV_DISPLAY_CANDWORDS;
                    }

                    return IRV_TO_PROCESS;
                }
            } else
                return IRV_TO_PROCESS;
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

    for (i = 0; i < pystate->iPYSelected; i++)
        iCursorPos += strlen(pystate->pySelected[i].strHZ);

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
    FcitxInputStateSetClientCursorPos(input, 0);
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
    FcitxConfig* config = FcitxInstanceGetConfig(pystate->owner);
    Messages* msgPreedit = FcitxInputStateGetPreedit(input);
    Messages* msgClientPreedit = FcitxInputStateGetClientPreedit(input);
    struct _CandidateWordList* candList = FcitxInputStateGetCandidateList(input);

    /* update preedit string */
    int i;
    SetMessageCount(msgPreedit, 0);
    SetMessageCount(msgClientPreedit, 0);
    if (pystate->iPYSelected) {
        AddMessageAtLast(msgPreedit, MSG_OTHER, "");
        for (i = 0; i < pystate->iPYSelected; i++) {
            MessageConcat(msgPreedit, GetMessageCount(msgPreedit) - 1, pystate->pySelected[i].strHZ);
            MessageConcat(msgClientPreedit, GetMessageCount(msgPreedit) - 1, pystate->pySelected[i].strHZ);
        }
    }

    for (i = 0; i < pystate->findMap.iHZCount; i++) {
        AddMessageAtLast(msgPreedit, MSG_CODE, "%s", pystate->findMap.strPYParsed[i]);
        if (i < pystate->findMap.iHZCount - 1)
            MessageConcat(msgPreedit, GetMessageCount(msgPreedit) - 1, " ");
    }

    if (pystate->findMap.iMode == PARSE_ERROR) {
        CleanInputWindowDown(pystate->owner);
        return IRV_DISPLAY_MESSAGE;
    }

    if (FcitxInputStateGetIsInRemind(input))
        return PYGetRemindCandWords(pystate);

    CandidateWordSetPageSize(candList, config->iMaxCandWord);
    CandidateWordSetChoose(candList, DIGIT_STR_CHOOSE);

    pystate->iYCDZ = 0;

    //判断是不是要输入常用字或符号
    PyFreq* pCurFreq = pystate->pyFreq->next;
    for (iVal = 0; iVal < pystate->iPYFreqCount; iVal++) {
        if (!strcmp(pystate->strFindString, pCurFreq->strPY))
            break;
        pCurFreq = pCurFreq->next;
    }

    /* if it is symbol mode, all word will be take care */
    if (!(pCurFreq && pCurFreq->bIsSym)) {

        if (pystate->pyconfig.bPYCreateAuto)
            PYCreateAuto(pystate);

        if (pystate->strPYAuto[0]) {
            CandidateWord candWord;
            PYCandWord* pycandword = fcitx_malloc0(sizeof(PYCandWord));
            pycandword->iWhich = PY_CAND_AUTO;
            candWord.owner = pystate;
            candWord.callback = PYGetCandWord;
            candWord.priv = pycandword;
            candWord.strWord = strdup(pystate->strPYAuto);
            candWord.strExtra = NULL;
            CandidateWordAppend(candList, &candWord);
        }

        PYGetPhraseCandWords(pystate);
        if (pCurFreq)
            PYGetFreqCandWords(pystate, pCurFreq);
        PYGetBaseCandWords(pystate, pCurFreq);
    } else {
        PYGetSymCandWords(pystate, pCurFreq);
    }

    if (CandidateWordPageCount(candList) != 0) {
        CandidateWord* candWord = CandidateWordGetCurrentWindow(candList);
        AddMessageAtLast(msgClientPreedit, MSG_INPUT, "%s", candWord->strWord);
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

    while (utf8_strlen(pystate->strPYAuto) < pystate->findMap.iHZCount) {
        phraseSelected = NULL;
        baseSelected = NULL;

        iCount = utf8_strlen(pystate->strPYAuto);
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

INPUT_RETURN_VALUE PYGetCandWord(void* arg, CandidateWord* candWord)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    char *pBase = NULL, *pPhrase = NULL;
    char *pBaseMap = NULL, *pPhraseMap = NULL;
    uint *pIndex = NULL;
    boolean bAddNewPhrase = true;
    int i;
    char strHZString[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    int iLen;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);
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
    case PY_CAND_SYSPHRASE:    //是系统词组
    case PY_CAND_USERPHRASE:   //是用户词组
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
    case PY_CAND_SYMBOL:       //是特殊符号
        pBase = pycandWord->cand.freq.hz->strHZ;
        bAddNewPhrase = false;
        break;
    case PY_CAND_REMIND: {
        strcpy(pystate->strPYRemindSource, pycandWord->cand.remind.phrase->strPhrase + pycandWord->cand.remind.iLength);
        strcpy(pystate->strPYRemindMap, pycandWord->cand.remind.phrase->strMap + pycandWord->cand.remind.iLength);
        pBase = pystate->strPYRemindSource;
        strcpy(GetOutputString(input), pBase);
        CandidateWordReset(FcitxInputStateGetCandidateList(input));
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
        pystate->iOrderCount = 0;
    }
    if (pystate->iNewFreqCount >= AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
        pystate->iNewFreqCount = 0;
    }

    strcpy(strHZString, pBase);
    if (pPhrase)
        strcat(strHZString, pPhrase);
    iLen = utf8_strlen(strHZString);
    if (iLen == pystate->findMap.iHZCount || pycandWord->iWhich == PY_CAND_SYMBOL) {
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
        if (bAddNewPhrase && (utf8_strlen(pystate->strPYAuto) <= (MAX_PY_PHRASE_LENGTH)))
            PYAddUserPhrase(pystate, pystate->strPYAuto, strHZString, false);
        CleanInputWindow(instance);
        strcpy(GetOutputString(input), pystate->strPYAuto);
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
    utarray_init(&candtemp, &pycand_icd);

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
                        PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
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
                        PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
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
        CandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = NULL;
        const char* pBase = PYFAList[(*pcand)->cand.phrase.iPYFA].pyBase[(*pcand)->cand.phrase.iBase].strHZ;
        const char* pPhrase = (*pcand)->cand.phrase.phrase->strPhrase;
        asprintf(&candWord.strWord, "%s%s", pBase, pPhrase);
        CandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
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

//********************************************
void PYGetSymCandWords(FcitxPinyinState* pystate, PyFreq* pCurFreq)
{
    int i;
    HZ *hz;
    FcitxInputState* input = FcitxInstanceGetInputState(pystate->owner);

    if (pCurFreq && pCurFreq->bIsSym) {
        hz = pCurFreq->HZList->next;
        for (i = 0; i < pCurFreq->iCount; i++) {
            PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
            PYAddSymCandWord(hz, pycandWord);
            CandidateWord candWord;
            candWord.callback = PYGetCandWord;
            candWord.owner = pystate;
            candWord.priv = pycandWord;
            candWord.strExtra = NULL;
            candWord.strWord = strdup(hz->strHZ);
            CandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
            hz = hz->next;
        }
    }
}

/*
 * 将一个符号加入到候选列表的合适位置中
 * 符号不需进行频率调整
 */
void PYAddSymCandWord(HZ * hz, PYCandWord* pycandWord)
{
    pycandWord->iWhich = PY_CAND_SYMBOL;
    pycandWord->cand.sym.hz = hz;
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
    utarray_init(&candtemp, &pycand_icd);

    str[0] = pystate->findMap.strMap[0][0];
    str[1] = pystate->findMap.strMap[0][1];
    str[2] = '\0';
    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                if (!PYIsInFreq(pCurFreq, PYFAList[candPos.iPYFA].pyBase[candPos.iBase].strHZ)) {
                    PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
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
        CandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(PYFAList[(*pcand)->cand.base.iPYFA].pyBase[(*pcand)->cand.base.iBase].strHZ);

        CandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
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
    utarray_init(&candtemp, &pycand_icd);

    if (pCurFreq && !pCurFreq->bIsSym) {
        hz = pCurFreq->HZList->next;
        for (i = 0; i < pCurFreq->iCount; i++) {
            PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
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
        CandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup((*pcand)->cand.freq.hz->strHZ);

        CandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
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
boolean PYAddUserPhrase(FcitxPinyinState* pystate, char *phrase, char *map, boolean incHit)
{
    PyUsrPhrase *userPhrase, *newPhrase, *temp;
    char str[UTF8_MAX_LENGTH + 1];
    int i, j, k, iTemp;
    int clen;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;

    //如果短于两个汉字，则不能组成词组
    if (utf8_strlen(phrase) < 2)
        return false;
    str[0] = map[0];
    str[1] = map[1];
    str[2] = '\0';
    i = GetBaseMapIndex(pystate, str);

    clen = utf8_char_len(phrase);
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
    newPhrase = (PyUsrPhrase *) fcitx_malloc0(sizeof(PyUsrPhrase));
    newPhrase->phrase.strMap = (char *) fcitx_malloc0(sizeof(char) * strlen(map + 2) + 1);
    newPhrase->phrase.strPhrase = (char *) fcitx_malloc0(sizeof(char) * strlen(phrase + clen) + 1);
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
    if (pystate->iNewPYPhraseCount == AUTOSAVE_PHRASE_COUNT) {
        SavePYUserPhrase(pystate);
        pystate->iNewPYPhraseCount = 0;
    }

    return true;
}

void PYDelUserPhrase(FcitxPinyinState* pystate, int iPYFA, int iBase, PyUsrPhrase * phrase)
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
    if (pystate->iNewPYPhraseCount == AUTOSAVE_PHRASE_COUNT) {
        SavePYUserPhrase(pystate);
        pystate->iNewPYPhraseCount = 0;
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
    int i, j, k;
    int iTemp;
    char *pstr;
    char strPathTemp[PATH_MAX];
    FILE *fp;
    PyPhrase *phrase;
    PYFA* PYFAList = pystate->PYFAList;

    fp = GetXDGFileWithPrefix("pinyin", TEMP_FILE, "wb", &pstr);
    if (!fp) {
        FcitxLog(ERROR, _("Cannot Save User Pinyin Database: %s"), pstr);
        return;
    }
    strcpy(strPathTemp, pstr);
    free(pstr);

    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            iTemp = PYFAList[i].pyBase[j].iUserPhrase;
            if (iTemp) {
                char clen;
                fwrite(&i, sizeof(int), 1, fp);
                clen = strlen(PYFAList[i].pyBase[j].strHZ);
                fwrite(&clen, sizeof(char), 1, fp);
                fwrite(PYFAList[i].pyBase[j].strHZ, sizeof(char) * clen, 1, fp);
                fwrite(&iTemp, sizeof(int), 1, fp);
                phrase = USER_PHRASE_NEXT(PYFAList[i].pyBase[j].userPhrase);
                for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
                    iTemp = strlen(phrase->strMap);
                    fwrite(&iTemp, sizeof(int), 1, fp);
                    fwrite(phrase->strMap, sizeof(char) * iTemp, 1, fp);

                    iTemp = strlen(phrase->strPhrase);
                    fwrite(&iTemp, sizeof(int), 1, fp);
                    fwrite(phrase->strPhrase, sizeof(char) * iTemp, 1, fp);

                    iTemp = phrase->iIndex;
                    fwrite(&iTemp, sizeof(int), 1, fp);
                    iTemp = phrase->iHit;
                    fwrite(&iTemp, sizeof(int), 1, fp);
                    phrase = USER_PHRASE_NEXT(phrase);
                }
            }
        }
    }

    fclose(fp);
    fp = GetXDGFileWithPrefix("pinyin", PY_USERPHRASE_FILE, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(strPathTemp, pstr);
    free(pstr);
}

void SavePYFreq(FcitxPinyinState *pystate)
{
    int i, j, k;
    char *pstr;
    char strPathTemp[PATH_MAX];
    FILE *fp;
    PyFreq *pPyFreq;
    HZ *hz;

    fp = GetXDGFileWithPrefix("pinyin", TEMP_FILE, "wb", &pstr);
    if (!fp) {
        FcitxLog(ERROR, _("Cannot Save Frequent word: %s"), pstr);
        return;
    }
    strcpy(strPathTemp, pstr);
    free(pstr);

    i = 0;
    pPyFreq = pystate->pyFreq->next;
    while (pPyFreq) {
        if (!pPyFreq->bIsSym)
            i++;
        pPyFreq = pPyFreq->next;
    }
    fwrite(&i, sizeof(uint), 1, fp);
    pPyFreq = pystate->pyFreq->next;
    while (pPyFreq) {
        if (!pPyFreq->bIsSym) {
            fwrite(pPyFreq->strPY, sizeof(char) * 11, 1, fp);
            j = pPyFreq->iCount;
            fwrite(&j, sizeof(int), 1, fp);
            hz = pPyFreq->HZList->next;
            for (k = 0; k < pPyFreq->iCount; k++) {

                char slen = strlen(hz->strHZ);
                fwrite(&slen, sizeof(char), 1, fp);
                fwrite(hz->strHZ, sizeof(char) * slen, 1, fp);

                j = hz->iPYFA;
                fwrite(&j, sizeof(int), 1, fp);

                j = hz->iHit;
                fwrite(&j, sizeof(int), 1, fp);

                j = hz->iIndex;
                fwrite(&j, sizeof(int), 1, fp);

                hz = hz->next;
            }
        }
        pPyFreq = pPyFreq->next;
    }

    fclose(fp);

    fp = GetXDGFileWithPrefix("pinyin", PY_FREQ_FILE, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(strPathTemp, pstr);
    free(pstr);
}

/*
 * 保存索引文件
 */
void SavePYIndex(FcitxPinyinState *pystate)
{
    int i, j, k, l;
    char *pstr;
    char strPathTemp[PATH_MAX];
    FILE *fp;
    PYFA* PYFAList = pystate->PYFAList;

    fp = GetXDGFileWithPrefix("pinyin", TEMP_FILE, "wb", &pstr);
    if (!fp) {
        FcitxLog(ERROR, _("Cannot Save Pinyin Index: %s"), pstr);
        return;
    }
    strcpy(strPathTemp, pstr);
    free(pstr);

    uint32_t magic = PY_INDEX_MAGIC_NUMBER;
    fwrite(&magic, sizeof(uint32_t), 1, fp);

    //保存计数器
    fwrite(&pystate->iCounter, sizeof(uint), 1, fp);
    //先保存索引不为0的单字
    k = -1;
    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            if (PYFAList[i].pyBase[j].iIndex > pystate->iOrigCounter) {
                fwrite(&i, sizeof(int), 1, fp);
                fwrite(&j, sizeof(int), 1, fp);
                fwrite(&k, sizeof(int), 1, fp);
                l = PYFAList[i].pyBase[j].iIndex;
                fwrite(&l, sizeof(uint), 1, fp);
                l = PYFAList[i].pyBase[j].iHit;
                fwrite(&l, sizeof(uint), 1, fp);
            }
        }
    }

    //再保存索引不为0的系统词组
    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            for (k = 0; k < PYFAList[i].pyBase[j].iPhrase; k++) {
                if (PYFAList[i].pyBase[j].phrase[k].iIndex > pystate->iOrigCounter) {
                    fwrite(&i, sizeof(int), 1, fp);
                    fwrite(&j, sizeof(int), 1, fp);
                    fwrite(&k, sizeof(int), 1, fp);
                    l = PYFAList[i].pyBase[j].phrase[k].iIndex;
                    fwrite(&l, sizeof(uint), 1, fp);
                    l = PYFAList[i].pyBase[j].phrase[k].iHit;
                    fwrite(&l, sizeof(uint), 1, fp);
                }
            }
        }
    }

    fclose(fp);

    fp = GetXDGFileWithPrefix("pinyin", PY_INDEX_FILE, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(strPathTemp, pstr);

    free(pstr);
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
        freq = (PyFreq *) fcitx_malloc0(sizeof(PyFreq));
        freq->HZList = (HZ *) fcitx_malloc0(sizeof(HZ));
        freq->HZList->next = NULL;
        strcpy(freq->strPY, pystate->strFindString);
        freq->next = NULL;
        freq->iCount = 0;
        freq->bIsSym = false;
        pCurFreq = pystate->pyFreq;
        for (i = 0; i < pystate->iPYFreqCount; i++)
            pCurFreq = pCurFreq->next;
        pCurFreq->next = freq;
        pystate->iPYFreqCount++;
        pCurFreq = freq;
    }

    HZTemp = (HZ *) fcitx_malloc0(sizeof(HZ));
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
    if (pystate->iNewFreqCount == AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
        pystate->iNewFreqCount = 0;
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
    if (pystate->iNewFreqCount == AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
        pystate->iNewFreqCount = 0;
    }
}

/*
 * 判断一个字是否已经是常用字
 */
boolean PYIsInFreq(PyFreq* pCurFreq, char *strHZ)
{
    HZ *hz;
    int i;

    if (!pCurFreq || pCurFreq->bIsSym)
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
    FcitxConfig* config = FcitxInstanceGetConfig(pystate->owner);
    boolean bDisablePagingInRemind = config->bDisablePagingInRemind;
    FcitxInputState *input = FcitxInstanceGetInputState(pystate->owner);
    PYFA* PYFAList = pystate->PYFAList;

    if (!pystate->strPYRemindSource[0])
        return IRV_TO_PROCESS;

    PyBase* pyBaseForRemind = NULL;
    for (i = 0; i < pystate->iPYFACount; i++) {
        if (!strncmp(pystate->strPYRemindMap, PYFAList[i].strMap, 2)) {
            for (j = 0; j < PYFAList[i].iBase; j++) {
                if (!utf8_strncmp(pystate->strPYRemindSource, PYFAList[i].pyBase[j].strHZ, 1)) {
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
    utarray_init(&candtemp, &pycand_icd);

    for (i = 0; i < pyBaseForRemind->iPhrase; i++) {

        if (bDisablePagingInRemind && utarray_len(&candtemp) >= CandidateWordGetPageSize(FcitxInputStateGetCandidateList(input)))
            break;

        if (utf8_strlen(pystate->strPYRemindSource) == 1) {
            if (utf8_strlen(pyBaseForRemind->phrase[i].strPhrase) == 1) {
                PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
                PYAddRemindCandWord(pystate, &pyBaseForRemind->phrase[i], pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        } else if (strlen(pyBaseForRemind->phrase[i].strPhrase) == strlen(pystate->strPYRemindSource)) {
            if (!strncmp
                    (pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource),
                     pyBaseForRemind->phrase[i].strPhrase, strlen(pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource)))
               ) {
                PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
                PYAddRemindCandWord(pystate, &pyBaseForRemind->phrase[i], pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        }
    }

    phrase = &pyBaseForRemind->userPhrase->next->phrase;
    for (i = 0; i < pyBaseForRemind->iUserPhrase; i++) {
        if (bDisablePagingInRemind && utarray_len(&candtemp) >= CandidateWordGetPageSize(FcitxInputStateGetCandidateList(input)))
            break;

        if (utf8_strlen(pystate->strPYRemindSource) == 1) {
            if (utf8_strlen(phrase->strPhrase) == 1) {
                PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
                PYAddRemindCandWord(pystate, phrase, pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        } else if (strlen(phrase->strPhrase) == strlen(pystate->strPYRemindSource)) {
            if (!strncmp
                    (pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource),
                     phrase->strPhrase, strlen(pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource)))) {
                PYCandWord* pycandWord = fcitx_malloc0(sizeof(PYCandWord));
                PYAddRemindCandWord(pystate, phrase, pycandWord);
                utarray_push_back(&candtemp, &pycandWord);
            }
        }

        phrase = USER_PHRASE_NEXT(phrase);
    }

    SetMessageCount(FcitxInputStateGetAuxUp(input), 0);
    AddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Remind: "));
    AddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_INPUT, "%s", pystate->strPYRemindSource);

    PYCandWord** pcand = NULL;
    for (pcand = (PYCandWord**) utarray_front(&candtemp);
            pcand != NULL;
            pcand = (PYCandWord**) utarray_next(&candtemp, pcand)) {
        CandidateWord candWord;
        candWord.callback = PYGetCandWord;
        candWord.owner = pystate;
        candWord.priv = *pcand;
        candWord.strExtra = NULL;
        candWord.strWord = strdup((*pcand)->cand.remind.phrase->strPhrase + (*pcand)->cand.remind.iLength);

        CandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }

    utarray_done(&candtemp);

    FcitxInputStateSetIsInRemind(input, (CandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0));
    return IRV_DISPLAY_CANDWORDS;
}

void PYAddRemindCandWord(FcitxPinyinState* pystate, PyPhrase * phrase, PYCandWord* pycandWord)
{
    PYRemindCandWord* pyRemindCandWords = &pycandWord->cand.remind;

    pycandWord->iWhich = PY_CAND_REMIND;
    pyRemindCandWords->phrase = phrase;
    pyRemindCandWords->iLength = strlen(pystate->strPYRemindSource) - utf8_char_len(pystate->strPYRemindSource);
}

void PYGetPYByHZ(FcitxPinyinState*pystate, char *strHZ, char *strPY)
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

void* LoadPYBaseDictWrapper(void * arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    if (!pystate->bPYBaseDictLoaded)
        LoadPYBaseDict(pystate);
    return NULL;
}

void* PYGetPYByHZWrapper(void * arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    char *a = args.args[0];
    char *b = args.args[1];
    PYGetPYByHZ(pystate, a, b);
    return NULL;

}
void* DoPYInputWrapper(void * arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    FcitxKeySym *a = args.args[0];
    unsigned int *b = args.args[1];
    DoPYInput(pystate, *a, *b);
    return NULL;

}
void* PYGetCandWordsWrapper(void * arg, FcitxModuleFunctionArg args)
{
    PYGetCandWords(arg);
    return NULL;
}

void* PYGetFindStringWrapper(void * arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    return pystate->strFindString;

}
void* PYResetWrapper(void * arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;

    pystate->bSP = false;
    pystate->strPYAuto[0] = '\0';
    ResetPYStatus(pystate);

    return NULL;
}

void ReloadConfigPY(void* arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;

    LoadPYConfig(&pystate->pyconfig);
}

void PinyinMigration()
{
    char* olduserphrase, *oldpyindex, *newuserphrase, *newpyindex;
    GetXDGFileUserWithPrefix("", PY_USERPHRASE_FILE, NULL, &olduserphrase);
    GetXDGFileUserWithPrefix("", PY_INDEX_FILE, NULL, &oldpyindex);
    GetXDGFileUserWithPrefix("pinyin", PY_USERPHRASE_FILE, NULL, &newuserphrase);
    GetXDGFileUserWithPrefix("pinyin", PY_INDEX_FILE, NULL, &newpyindex);

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


void* PYSP2QP(void* arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    char* strSP = args.args[0];
    char strQP[MAX_PY_LENGTH + 1];
    strQP[0] = 0;

    SP2QP(&pystate->pyconfig, strSP, strQP);

    return strdup(strQP);
}

boolean PYGetPYMapByHZ(FcitxPinyinState* pystate, char* strHZ, char* mapHint, char* strMap)
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

void PYAddUserPhraseFromCString(void* arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    char* strHZ = args.args[0], *sp, *pivot;
    char singleHZ[UTF8_MAX_LENGTH + 1];
    char strMap[3];
    if (!utf8_check_string(strHZ))
        return;

    pivot = strHZ;
    size_t hzCount = utf8_strlen(strHZ);
    size_t hzCountLocal = 0;

    if (pystate->iPYSelected) {
        int i = 0;
        for (i = 0 ; i < pystate->iPYSelected; i ++)
            hzCountLocal += strlen(pystate->pySelected[i].strMap) / 2;
    }
    hzCountLocal += pystate->findMap.iHZCount;

    /* in order not to get a wrong one, use strict check */
    if (hzCountLocal != hzCount || hzCount > MAX_PY_PHRASE_LENGTH)
        return;
    char* totalMap = fcitx_malloc0(sizeof(char) * (1 + 2 * hzCount));

    if (pystate->iPYSelected) {
        int i = 0;
        for (i = 0 ; i < pystate->iPYSelected; i ++)
            strcat(totalMap, pystate->pySelected[i].strMap);
        strHZ = utf8_get_nth_char(strHZ, strlen(totalMap) / 2);
    }

    int i = 0;
    while (*strHZ) {
        int chr;

        sp = utf8_get_char(strHZ, &chr);
        size_t len = sp - strHZ;
        strncpy(singleHZ, strHZ, len);
        singleHZ[len] = '\0';

        if (!PYGetPYMapByHZ(pystate, singleHZ, pystate->findMap.strMap[i], strMap)) {
            free(totalMap);
            return;
        }

        strncat(totalMap, strMap, 2);

        strHZ = sp;
        i ++;
    }

    PYAddUserPhrase(pystate, pivot, totalMap, true);
    free(totalMap);

    return;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
