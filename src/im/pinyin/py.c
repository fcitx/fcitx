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

#define TEMP_FILE       "FCITX_DICT_TEMP"

FCITX_EXPORT_API
FcitxIMClass ime = {
    PYCreate,
    NULL
};

static void LoadPYPhraseDict(FcitxPinyinState* pystate, FILE* fp, boolean isSystem);

static void * LoadPYBaseDictWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetPYByHZWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetCandWordWrapper(void* arg, FcitxModuleFunctionArg args);
static void * DoPYInputWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetCandWordsWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetCandTextWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYGetFindStringWrapper(void* arg, FcitxModuleFunctionArg args);
static void * PYResetWrapper(void* arg, FcitxModuleFunctionArg args);
static void ReloadConfigPY(void* arg);

void *PYCreate(FcitxInstance* instance)
{
    FcitxPinyinState *pystate = fcitx_malloc0(sizeof(FcitxPinyinState));
    FcitxAddon* pyaddon = GetAddonByName(&instance->addons, FCITX_PINYIN_NAME);
    InitMHPY(&pystate->pyconfig.MHPY_C, MHPY_C_TEMPLATE);
    InitMHPY(&pystate->pyconfig.MHPY_S, MHPY_S_TEMPLATE);
    InitPYTable(&pystate->pyconfig);
    if (!LoadPYConfig(&pystate->pyconfig))
    {
        free(pystate->pyconfig.MHPY_C);
        free(pystate->pyconfig.MHPY_S);
        free(pystate->pyconfig.PYTable);
        free(pystate);
        return NULL;
    }
    
    FcitxRegisterIM(instance,
                    pystate,
                    _("Pinyin"),
                    "pinyin",
                    PYInit,
                    ResetPYStatus,
                    DoPYInput,
                    PYGetCandWords,
                    PYGetCandWord,
                    NULL,
                    SavePY,
                    ReloadConfigPY,
                    NULL,
                    pystate->pyconfig.iPinyinPriority
                   );
    FcitxRegisterIM(instance,
                    pystate,
                    _("Shuangpin"),
                    "shuangpin",
                    SPInit,
                    ResetPYStatus,
                    DoPYInput,
                    PYGetCandWords,
                    PYGetCandWord,
                    NULL,
                    SavePY,
                    ReloadConfigPY,
                    NULL,
                    pystate->pyconfig.iShuangpinPriority
                   );
    pystate->owner = instance;
    
    /* ensure the order! */
    AddFunction(pyaddon, LoadPYBaseDictWrapper); // 0
    AddFunction(pyaddon, PYGetPYByHZWrapper); // 1
    AddFunction(pyaddon, PYGetCandTextWrapper); // 2
    AddFunction(pyaddon, PYGetCandWordWrapper); // 3
    AddFunction(pyaddon, DoPYInputWrapper); // 4
    AddFunction(pyaddon, PYGetCandWordsWrapper); // 5
    AddFunction(pyaddon, PYGetFindStringWrapper); // 6
    AddFunction(pyaddon, PYResetWrapper); // 7
    return pystate;
}

boolean PYInit(void *arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState* )arg;
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
    pystate->PYFAList = (PYFA *) malloc(sizeof(PYFA) * pystate->iPYFACount);
    PYFA* PYFAList = pystate->PYFAList;
    for (i = 0; i < pystate->iPYFACount; i++) {
        fread(PYFAList[i].strMap, sizeof(char) * 2, 1, fp);
        PYFAList[i].strMap[2] = '\0';

        fread(&(PYFAList[i].iBase), sizeof(int), 1, fp);
        PYFAList[i].pyBase = (PyBase *) malloc(sizeof(PyBase) * PYFAList[i].iBase);
        for (j = 0; j < PYFAList[i].iBase; j++) {
            int8_t len;
            fread(&len, sizeof(char), 1, fp);
            fread(PYFAList[i].pyBase[j].strHZ, sizeof(char) * len, 1, fp);
            PYFAList[i].pyBase[j].strHZ[len] = '\0';
            fread(&iLen, sizeof(int), 1, fp);
            PYFAList[i].pyBase[j].iIndex = iLen;
            PYFAList[i].pyBase[j].iHit = 0;
            PYFAList[i].pyBase[j].flag = 0;
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            PYFAList[i].pyBase[j].iPhrase = 0;
            PYFAList[i].pyBase[j].iUserPhrase = 0;
            PYFAList[i].pyBase[j].userPhrase = (PyPhrase *) malloc(sizeof(PyPhrase));
            PYFAList[i].pyBase[j].userPhrase->next = PYFAList[i].pyBase[j].userPhrase;
        }
    }

    fclose(fp);
    pystate->bPYBaseDictLoaded = true;

    pystate->iOrigCounter = pystate->iCounter;

    pystate->pyFreq = (PyFreq *) malloc(sizeof(PyFreq));
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

    pinyinPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/pinyin" , DATADIR, PACKAGE "/pinyin" );

    for(i = 0; i< len; i++)
    {
        snprintf(pathBuf, sizeof(pathBuf), "%s", pinyinPath[i]);
        pathBuf[sizeof(pathBuf) - 1] = '\0';

        dir = opendir(pathBuf);
        if (dir == NULL)
            continue;

        /* collect all *.conf files */
        while((drt = readdir(dir)) != NULL)
        {
            size_t nameLen = strlen(drt->d_name);
            if (nameLen <= strlen(".mb") )
                continue;
            memset(pathBuf,0,sizeof(pathBuf));

            if (strcmp(drt->d_name + nameLen -strlen(".mb"), ".mb") != 0)
                continue;
            if (strcmp(drt->d_name, PY_PHRASE_FILE) == 0)
                continue;
            snprintf(pathBuf, sizeof(pathBuf), "%s/%s", pinyinPath[i], drt->d_name );

            if (stat(pathBuf, &fileStat) == -1)
                continue;

            if (fileStat.st_mode & S_IFREG)
            {
                StringHashSet *string;
                HASH_FIND_STR(sset, drt->d_name, string);
                if (!string)
                {
                    char *bStr = strdup(drt->d_name);
                    string = malloc(sizeof(StringHashSet));
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

void LoadPYPhraseDict(FcitxPinyinState* pystate, FILE *fp, boolean isSystem)
{
    int i, j ,k, count, iLen;
    char strBase[UTF8_MAX_LENGTH + 1];
    PyPhrase *temp, *phrase = NULL;
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

        if (isSystem)
        {
            phrase = (PyPhrase *) malloc(sizeof(PyPhrase) * count);
            temp = phrase;
        }
        else
        {
            PYFAList[i].pyBase[j].iUserPhrase = count;
            temp = PYFAList[i].pyBase[j].userPhrase;
        }

        for (k = 0; k < count; k++) {
            if (!isSystem)
                phrase = (PyPhrase *) malloc(sizeof(PyPhrase));
            fread(&iLen, sizeof(int), 1, fp);
            phrase->strMap = (char *) malloc(sizeof(char) * (iLen + 1));
            fread(phrase->strMap, sizeof(char) * iLen, 1, fp);
            phrase->strMap[iLen] = '\0';
            fread(&iLen, sizeof(int), 1, fp);
            phrase->strPhrase = (char *) malloc(sizeof(char) * (iLen + 1));
            fread(phrase->strPhrase, sizeof(char) * iLen, 1, fp);
            phrase->strPhrase[iLen] = '\0';
            fread(&iLen, sizeof(unsigned int), 1, fp);
            phrase->iIndex = iLen;
            if (iLen > pystate->iCounter)
                pystate->iCounter = iLen;
            if (isSystem)
            {
                phrase->iHit = 0;
                phrase->flag = 0;
                phrase ++;
            }
            else
            {
                fread(&iLen, sizeof(int), 1, fp);
                phrase->iHit = iLen;
                phrase->flag = 0;

                phrase->next = temp->next;
                temp->next = phrase;

                temp = phrase;
            }
        }

        if (isSystem)
        {
            if (PYFAList[i].pyBase[j].iPhrase == 0)
            {
                PYFAList[i].pyBase[j].iPhrase = count;
                PYFAList[i].pyBase[j].phrase = temp;
            }
            else
            {
                int m, n;
                boolean *flag = malloc(sizeof(boolean) * count);
                memset(flag, 0, sizeof(boolean) * count);
                int left = count;
                phrase = temp;
                for (m = 0; m < count; m++)
                {
                    for (n = 0; n < PYFAList[i].pyBase[j].iPhrase; n++)
                    {
                        int result = strcmp(PYFAList[i].pyBase[j].phrase[n].strMap, phrase[m].strMap);
                        if (result == 0)
                        {
                            if (strcmp(PYFAList[i].pyBase[j].phrase[n].strPhrase, phrase[m].strPhrase) == 0)
                                break;
                        }
                    }
                    if (n != PYFAList[i].pyBase[j].iPhrase)
                    {
                        flag[m] = 1;
                        left -- ;
                    }
                }
                int orig = PYFAList[i].pyBase[j].iPhrase;
                if (left >= 0)
                {
                    PYFAList[i].pyBase[j].iPhrase += left;
                    PYFAList[i].pyBase[j].phrase = realloc(PYFAList[i].pyBase[j].phrase, sizeof(PyPhrase) * PYFAList[i].pyBase[j].iPhrase);
                }
                for (m = 0; m < count; m ++)
                {
                    if (flag[m])
                    {
                        free(phrase[m].strMap);
                        free(phrase[m].strPhrase);
                    }
                    else
                    {
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
        LoadPYPhraseDict(pystate, fp, true);
        fclose(fp);
        StringHashSet *sset = GetPYPhraseFiles();
        StringHashSet *curStr;
        while(sset)
        {
            curStr = sset;
            HASH_DEL(sset, curStr);
            fp = GetXDGFileWithPrefix("pinyin", curStr->name, "r", NULL);
            LoadPYPhraseDict(pystate, fp, true);
            fclose(fp);
            free(curStr->name);
            free(curStr);
        }

        pystate->iOrigCounter = pystate->iCounter;
    }

    //下面开始读取用户词库
    fp = GetXDGFileWithPrefix("pinyin", PY_USERPHRASE_FILE, "rb", NULL);
    if (fp) {
        LoadPYPhraseDict(pystate, fp, false);
        fclose(fp);
    }
    //下面读取索引文件
    fp = GetXDGFileWithPrefix("pinyin", PY_INDEX_FILE, "rb", NULL);
    if (fp) {
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

        fclose(fp);
    }
    //下面读取常用词表
    fp = GetXDGFileWithPrefix("pinyin", PY_FREQ_FILE, "rb", NULL);
    if (fp) {
        pPyFreq = pystate->pyFreq;

        fread(&pystate->iPYFreqCount, sizeof(uint), 1, fp);

        for (i = 0; i < pystate->iPYFreqCount; i++) {
            pyFreqTemp = (PyFreq *) malloc(sizeof(PyFreq));
            pyFreqTemp->next = NULL;
            pyFreqTemp->bIsSym = false;

            fread(pyFreqTemp->strPY, sizeof(char) * 11, 1, fp);
            fread(&j, sizeof(int), 1, fp);
            pyFreqTemp->iCount = j;

            pyFreqTemp->HZList = (HZ *) malloc(sizeof(HZ));
            pyFreqTemp->HZList->next = NULL;
            pHZ = pyFreqTemp->HZList;

            for (k = 0; k < pyFreqTemp->iCount; k++) {
                int8_t slen;
                HZTemp = (HZ *) malloc(sizeof(HZ));
                fread(&slen, sizeof(int8_t), 1, fp);
                fread(HZTemp->strHZ, sizeof(char) * slen, 1, fp);
                HZTemp->strHZ[slen] = '\0';
                fread(&j, sizeof(int), 1, fp);
                HZTemp->iPYFA = j;
                fread(&j, sizeof(int), 1, fp);
                HZTemp->iHit = j;
                fread(&j, sizeof(int), 1, fp);
                HZTemp->iIndex = j;
                HZTemp->flag = 0;
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
                pyFreqTemp = (PyFreq *) malloc(sizeof(PyFreq));
                strcpy(pyFreqTemp->strPY, str1);
                pyFreqTemp->next = NULL;
                pyFreqTemp->iCount = 0;
                pyFreqTemp->bIsSym = true;
                pyFreqTemp->HZList = (HZ *) malloc(sizeof(HZ));
                pyFreqTemp->HZList->next = NULL;
                pPyFreq->next = pyFreqTemp;
                pystate->iPYFreqCount++;
            } else {
                if (!pyFreqTemp->bIsSym)        //不能与常用字的编码相同
                    continue;
            }

            HZTemp = (HZ *) malloc(sizeof(HZ));
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
    FcitxInputState *input = &pystate->owner->input;
    FcitxInstance *instance = pystate->owner;
    int i = 0;
    int val;
    char *strGet = NULL;
    char strTemp[MAX_USER_INPUT + 1];

    if (sym == 0 && state == 0)
        sym = -1;

    if (!pystate->bPYBaseDictLoaded)
        LoadPYBaseDict(pystate);
    if (!pystate->bPYOtherDictLoaded)
        LoadPYOtherDict(pystate);

    val = IRV_TO_PROCESS;
    if (!pystate->bIsPYAddFreq && !pystate->bIsPYDelFreq && !pystate->bIsPYDelUserPhr) {
        if ((IsHotKeyLAZ(sym, state)
            || IsHotKey(sym, state, FCITX_SEPARATOR)
            || (pystate->bSP && pystate->bSP_UseSemicolon && IsHotKey(sym, state, FCITX_SEMICOLON)))) {
            input->bIsInRemind = false;
            instance->bShowCursor = true;

            if (IsHotKey(sym, state, FCITX_SEPARATOR)) {
                if (!pystate->iPYInsertPoint)
                    return IRV_TO_PROCESS;
                if (pystate->strFindString[pystate->iPYInsertPoint - 1] == PY_SEPARATOR)
                    return IRV_DO_NOTHING;
            }

            val = i = strlen(pystate->strFindString);

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

            val = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_BACKSPACE) || IsHotKey(sym, state, FCITX_CTRL_H) ) {
            if (pystate->iPYInsertPoint) {
                val = ((pystate->iPYInsertPoint > 1)
                       && (pystate->strFindString[pystate->iPYInsertPoint - 2] == PY_SEPARATOR)) ? 2 : 1;
                int len = strlen(pystate->strFindString + pystate->iPYInsertPoint), i = 0;
                /* 这里使用<=而不是<是因为还有'\0'需要拷贝 */
                for (i = 0; i <= len; i++)
                    pystate->strFindString[i + pystate->iPYInsertPoint - val] = pystate->strFindString[i + pystate->iPYInsertPoint];
                ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
                pystate->iPYInsertPoint -= val;
                val = IRV_DISPLAY_CANDWORDS;

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

                val = IRV_DISPLAY_CANDWORDS;
            }
        } else if (IsHotKey(sym, state, FCITX_DELETE)) {
            if (input->iCodeInputCount) {
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
                val = IRV_DISPLAY_CANDWORDS;
            }
        } else if (IsHotKey(sym, state, FCITX_HOME)) {
            if (input->iCodeInputCount == 0)
                return IRV_DONOT_PROCESS;
            pystate->iPYInsertPoint = 0;
            val = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_END)) {
            if (input->iCodeInputCount == 0)
                return IRV_DONOT_PROCESS;
            pystate->iPYInsertPoint = strlen(pystate->strFindString);
            val = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_RIGHT)) {
            if (!input->iCodeInputCount)
                return IRV_TO_PROCESS;
            if (pystate->iPYInsertPoint == strlen(pystate->strFindString))
                return IRV_DO_NOTHING;
            pystate->iPYInsertPoint++;
            val = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_LEFT)) {
            if (!input->iCodeInputCount)
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

                    val = IRV_DISPLAY_CANDWORDS;
                } else if (pystate->iPYInsertPoint) {
                    pystate->iPYInsertPoint--;
                    val = IRV_DISPLAY_CANDWORDS;
                } else
                    val = IRV_DO_NOTHING;
            } else {
                pystate->iPYInsertPoint--;
                val = IRV_DISPLAY_CANDWORDS;
            }
        } else if (IsHotKey(sym, state, FCITX_SPACE)) {
            if (!input->bIsInRemind) {
                if (pystate->findMap.iMode == PARSE_ERROR)
                    return IRV_DO_NOTHING;

                if (!input->iCandWordCount) {
                    if (input->iCodeInputCount == 1 && input->strCodeInput[0] == ';' && pystate->bSP) {
                        strcpy(GetOutputString(input), "；");
                        return IRV_PUNC;
                    }
                    return IRV_TO_PROCESS;
                }

                strGet = PYGetCandWord(pystate, 0);

                if (strGet) {
                    strcpy(GetOutputString(input), strGet);

                    if (input->bIsInRemind)
                        val = IRV_GET_REMIND;
                    else
                        val = IRV_GET_CANDWORDS;
                } else
                    val = IRV_DISPLAY_CANDWORDS;
            } else {
                strcpy(GetOutputString(input), PYGetRemindCandWord(pystate, 0));
                val = IRV_GET_REMIND;
            }
        } else if (IsHotKey(sym, state, pystate->pyconfig.hkPYDelUserPhr)) {
            if (!pystate->bIsPYDelUserPhr) {
                for (val = 0; val < input->iCandWordCount; val++) {
                    if (pystate->PYCandWords[val].iWhich == PY_CAND_USERPHRASE)
                        goto _NEXT;
                }
                return IRV_TO_PROCESS;

              _NEXT:
                pystate->bIsPYDelUserPhr = true;
                input->bIsDoInputOnly = true;

                SetMessageCount(GetMessageUp(instance), 0);
                AddMessageAtLast(GetMessageUp(instance), MSG_TIPS, _("Press index to delete user phrase (ESC for cancel)"));
                instance->bShowCursor = false;

                return IRV_DISPLAY_MESSAGE;
            }
        } else if (IsHotKey(sym, state, pystate->pyconfig.hkPYAddFreq)) {
            if (!pystate->bIsPYAddFreq && pystate->findMap.iHZCount == 1 && input->iCodeInputCount) {
                pystate->bIsPYAddFreq = true;
                input->bIsDoInputOnly = true;

                SetMessageCount(GetMessageUp(instance), 0);
                AddMessageAtLast(GetMessageUp(instance), MSG_TIPS, _("Press number to make word in frequent list"), pystate->strFindString);
                instance->bShowCursor = false;

                return IRV_DISPLAY_MESSAGE;
            }
        } else if (IsHotKey(sym, state, pystate->pyconfig.hkPYDelFreq)) {
            if (!pystate->bIsPYDelFreq && (pystate->pCurFreq && !pystate->pCurFreq->bIsSym)) {
                for (val = 0; val < input->iCandWordCount; val++) {
                    if (pystate->PYCandWords[val].iWhich != PY_CAND_FREQ)
                        break;
                }

                if (!val)
                    return IRV_DO_NOTHING;

                SetMessageCount(GetMessageUp(instance), 0);
                if (val == 1)
                    AddMessageAtLast(GetMessageUp(instance), MSG_TIPS, _("Press 1 to delete %s in frequent list (ESC for cancel)"), pystate->strFindString);
                else
                    AddMessageAtLast(GetMessageUp(instance), MSG_TIPS, _("Press 1-%d to delete %s in frequent list (ESC for cancel)"), val, pystate->strFindString);

                pystate->bIsPYDelFreq = true;
                input->bIsDoInputOnly = true;

                instance->bShowCursor = false;

                return IRV_DISPLAY_MESSAGE;
            }
        }
    }

    if (val == IRV_TO_PROCESS) {
        if (IsHotKeyDigit(sym, state)) {
            int iKey = sym - Key_0;
            if (iKey == 0)
                iKey = 10;

            if (!input->bIsInRemind) {
                if (!input->iCodeInputCount)
                    return IRV_TO_PROCESS;
                else if (!input->iCandWordCount || (iKey > input->iCandWordCount))
                    return IRV_DO_NOTHING;
                else {
                    if (pystate->bIsPYAddFreq || pystate->bIsPYDelFreq || pystate->bIsPYDelUserPhr) {
                        if (pystate->bIsPYAddFreq) {
                            PYAddFreq(pystate, iKey - 1);
                            pystate->bIsPYAddFreq = false;
                        } else if (pystate->bIsPYDelFreq) {
                            PYDelFreq(pystate, iKey - 1);
                            pystate->bIsPYDelFreq = false;
                        } else {
                            if (pystate->PYCandWords[iKey - 1].iWhich == PY_CAND_USERPHRASE)
                                PYDelUserPhrase(pystate, pystate->PYCandWords[iKey - 1].cand.phrase.iPYFA,
                                                pystate->PYCandWords[iKey - 1].cand.phrase.iBase, pystate->PYCandWords[iKey - 1].cand.phrase.phrase);
                            pystate->bIsPYDelUserPhr = false;
                        }
                        input->bIsDoInputOnly = false;
                        instance->bShowCursor = true;

                        val = IRV_DISPLAY_CANDWORDS;
                    } else {
                        strGet = PYGetCandWord(pystate, iKey - 1);
                        if (strGet) {
                            strcpy(GetOutputString(input), strGet);

                            if (input->bIsInRemind)
                                val = IRV_GET_REMIND;
                            else
                                val = IRV_GET_CANDWORDS;
                        } else
                            val = IRV_DISPLAY_CANDWORDS;
                    }
                }
            } else {
                strcpy(GetOutputString(input), PYGetRemindCandWord(pystate, iKey - 1));
                val = IRV_GET_REMIND;
            }
        } else if (sym == -1) {
            ParsePY(&pystate->pyconfig, pystate->strFindString, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
            pystate->iPYInsertPoint = 0;
            val = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, FCITX_ESCAPE))
            return IRV_TO_PROCESS;
        else {
            if (pystate->bIsPYAddFreq || pystate->bIsPYDelFreq || pystate->bIsPYDelUserPhr)
                return IRV_DO_NOTHING;

            //下面实现以词定字
            if (input->iCandWordCount) {
                if (state == KEY_NONE && (sym == pystate->pyconfig.cPYYCDZ[0] || sym == pystate->pyconfig.cPYYCDZ[1])) {
                    if (pystate->PYCandWords[pystate->iYCDZ].iWhich == PY_CAND_USERPHRASE || pystate->PYCandWords[pystate->iYCDZ].iWhich == PY_CAND_SYMPHRASE) {
                        char *pBase, *pPhrase;

                        pBase = pystate->PYFAList[pystate->PYCandWords[pystate->iYCDZ].cand.phrase.iPYFA].pyBase[pystate->PYCandWords[pystate->iYCDZ].cand.phrase.iBase].strHZ;
                        pPhrase = pystate->PYCandWords[pystate->iYCDZ].cand.phrase.phrase->strPhrase;

                        if (sym == pystate->pyconfig.cPYYCDZ[0])
                            strcpy(GetOutputString(input), pBase);
                        else {
                            int8_t clen;
                            clen = utf8_char_len(pPhrase);
                            strncpy(GetOutputString(input), pPhrase, clen);
                            GetOutputString(input)[clen] = '\0';
                        }
                        SetMessageCount(GetMessageDown(instance), 0);
                        return IRV_GET_CANDWORDS;
                    }
                } else if (!input->bIsInRemind) {
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

                    if (val != -1 && val < input->iCandWordCount) {
                        pystate->iYCDZ = val;
                        PYCreateCandString(pystate);
                        return IRV_DISPLAY_CANDWORDS;
                    }

                    return IRV_TO_PROCESS;
                }
            } else
                return IRV_TO_PROCESS;
        }
    }

    if (!input->bIsInRemind) {
        UpdateCodeInputPY(pystate);
        CalculateCursorPosition(pystate);
    }

    if (val == IRV_DISPLAY_CANDWORDS) {
        SetMessageCount(GetMessageUp(instance), 0);
        if (pystate->iPYSelected) {
            Messages* messageUp = GetMessageUp(instance);
            AddMessageAtLast(GetMessageUp(instance), MSG_OTHER, "");
            for (i = 0; i < pystate->iPYSelected; i++)
                MessageConcat(messageUp, GetMessageCount(messageUp) - 1, pystate->pySelected[i].strHZ);
        }

        for (i = 0; i < pystate->findMap.iHZCount; i++) {
            AddMessageAtLast(GetMessageUp(instance), MSG_CODE, "%s ", pystate->findMap.strPYParsed[i]);
        }

        return PYGetCandWords(pystate, SM_FIRST);
    }

    return (INPUT_RETURN_VALUE) val;
}

/*
 * 本函数根据当前插入点计算光标的实际位置
 */
void CalculateCursorPosition(FcitxPinyinState* pystate)
{
    int i;
    int iTemp;
    FcitxInputState* input = &pystate->owner->input;

    input->iCursorPos = 0;
    for (i = 0; i < pystate->iPYSelected; i++)
        input->iCursorPos += strlen(pystate->pySelected[i].strHZ);

    if (pystate->iPYInsertPoint > strlen(pystate->strFindString))
        pystate->iPYInsertPoint = strlen(pystate->strFindString);
    iTemp = pystate->iPYInsertPoint;

    for (i = 0; i < pystate->findMap.iHZCount; i++) {
        if (strlen(pystate->findMap.strPYParsed[i]) >= iTemp) {
            input->iCursorPos += iTemp;
            break;
        }
        input->iCursorPos += strlen(pystate->findMap.strPYParsed[i]);

        input->iCursorPos++;
        iTemp -= strlen(pystate->findMap.strPYParsed[i]);
    }
}

/*
 * 由于拼音的编辑功能修改了strFindString，必须保证input->strCodeInput与用户的输入一致
 */
void UpdateCodeInputPY(FcitxPinyinState* pystate)
{
    int i;
    FcitxInputState* input = &pystate->owner->input;

    input->strCodeInput[0] = '\0';
    for (i = 0; i < pystate->iPYSelected; i++)
        strcat(input->strCodeInput, pystate->pySelected[i].strPY);
    strcat(input->strCodeInput, pystate->strFindString);
    input->iCodeInputCount = strlen(input->strCodeInput);
}

void PYResetFlags(FcitxPinyinState* pystate)
{
    int i, j, k;
    PyPhrase *phrase;
    PyFreq *freq;
    HZ *hz;
    PYFA *PYFAList = pystate->PYFAList;

    for (i = 0; i < pystate->iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iBase; j++) {
            PYFAList[i].pyBase[j].flag = 0;
            for (k = 0; k < PYFAList[i].pyBase[j].iPhrase; k++)
                PYFAList[i].pyBase[j].phrase[k].flag = 0;
            phrase = PYFAList[i].pyBase[j].userPhrase->next;
            for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
                phrase->flag = 0;
                phrase = phrase->next;
            }
        }
    }

    freq = pystate->pyFreq->next;
    for (i = 0; i < pystate->iPYFreqCount; i++) {
        hz = freq->HZList->next;
        for (j = 0; j < freq->iCount; j++) {
            hz->flag = false;
            hz = hz->next;
        }
        freq = freq->next;
    }
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

INPUT_RETURN_VALUE PYGetCandWords(void* arg, SEARCH_MODE mode)
{
    int iVal;
    FcitxPinyinState *pystate = (FcitxPinyinState*) arg;
    FcitxInputState *input = &pystate->owner->input;
    FcitxInstance* instance = pystate->owner;

    if (pystate->findMap.iMode == PARSE_ERROR) {
        SetMessageCount(GetMessageDown(instance), 0);
        input->iCandPageCount = 0;
        input->iCandWordCount = 0;

        return IRV_DISPLAY_MESSAGE;
    }

    if (input->bIsInRemind)
        return PYGetRemindCandWords(pystate, mode);

    if (mode == SM_FIRST) {
        input->iCurrentCandPage = 0;
        input->iCandPageCount = 0;
        input->iCandWordCount = 0;
        pystate->iYCDZ = 0;

        PYResetFlags(pystate);

        //判断是不是要输入常用字或符号
        pystate->pCurFreq = pystate->pyFreq->next;
        for (iVal = 0; iVal < pystate->iPYFreqCount; iVal++) {
            if (!strcmp(pystate->strFindString, pystate->pCurFreq->strPY))
                break;
            pystate->pCurFreq = pystate->pCurFreq->next;
        }

        if (pystate->pyconfig.bPYCreateAuto)
            PYCreateAuto(pystate);
    } else {
        if (!input->iCandPageCount)
            return IRV_TO_PROCESS;

        if (mode == SM_NEXT) {
            if (input->iCurrentCandPage == input->iCandPageCount)
                return IRV_DO_NOTHING;

            input->iCurrentCandPage++;
        } else {
            if (!input->iCurrentCandPage)
                return IRV_DO_NOTHING;

            input->iCurrentCandPage--;
            //需要将目前的候选词的标志重置
            PYSetCandWordsFlag(pystate, false);
        }

        input->iCandWordCount = 0;
    }

    if (!(pystate->pCurFreq && pystate->pCurFreq->bIsSym)) {
        if (!input->iCurrentCandPage && pystate->strPYAuto[0]) {
            input->iCandWordCount = 1;
            pystate->PYCandWords[0].iWhich = PY_CAND_AUTO;
        }
    }

    if (mode != SM_PREV) {
        PYGetCandWordsForward(pystate);

        if (input->iCurrentCandPage == input->iCandPageCount) {
            if (PYCheckNextCandPage(pystate))
                input->iCandPageCount++;
        }
    } else
        PYGetCandWordsBackward(pystate);

    PYCreateCandString(pystate);

    return IRV_DISPLAY_CANDWORDS;
}

void PYCreateCandString(FcitxPinyinState *pystate)
{
    FcitxInputState* input = &pystate->owner->input;
    char str[3];
    char *pBase = NULL, *pPhrase;
    int iVal;
    MSG_TYPE iType;
    PYCandWord* PYCandWords = pystate->PYCandWords;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxInstance* instance = pystate->owner;
    
    if ( ConfigGetPointAfterNumber(&pystate->owner->config)) {
        str[1] = '.';
        str[2] = '\0';
    } else
        str[1] = '\0';
    SetMessageCount(GetMessageDown(instance), 0);

    for (iVal = 0; iVal < input->iCandWordCount; iVal++) {
        if (iVal == 9)
            str[0] = '0';
        else
            str[0] = iVal + 1 + '0';
        AddMessageAtLast(GetMessageDown(instance), MSG_INDEX, "%s", str);

        iType = MSG_OTHER;
        if (PYCandWords[iVal].iWhich == PY_CAND_AUTO)
            iType = MSG_TIPS;
        if (PYCandWords[iVal].iWhich != PY_CAND_AUTO && iVal == pystate->iYCDZ)
            iType = MSG_FIRSTCAND;

        AddMessageAtLast(GetMessageDown(instance), iType, "");
        if (PYCandWords[iVal].iWhich == PY_CAND_AUTO) {
            MessageConcatLast(GetMessageDown(instance), pystate->strPYAuto);
        } else {
            pPhrase = NULL;
            switch (PYCandWords[iVal].iWhich) {
            case PY_CAND_BASE: //是系统单字
                pBase = PYFAList[PYCandWords[iVal].cand.base.iPYFA].pyBase[PYCandWords[iVal].cand.base.iBase].strHZ;
                break;
            case PY_CAND_USERPHRASE:   //是用户词组
                iType = MSG_USERPHR;
            case PY_CAND_SYMPHRASE:    //是系统词组
                pBase = PYFAList[PYCandWords[iVal].cand.phrase.iPYFA].pyBase[PYCandWords[iVal].cand.phrase.iBase].strHZ;
                pPhrase = PYCandWords[iVal].cand.phrase.phrase->strPhrase;
                break;
            case PY_CAND_FREQ: //是常用字
                pBase = PYCandWords[iVal].cand.freq.hz->strHZ;
                iType = MSG_CODE;
                break;
            case PY_CAND_SYMBOL:       //是特殊符号
                pBase = PYCandWords[iVal].cand.freq.hz->strHZ;
                break;
            }
            MessageConcatLast(GetMessageDown(instance), pBase);
            if (pPhrase)
                MessageConcatLast(GetMessageDown(instance), pPhrase);
        }

        if (iVal != (input->iCandWordCount - 1)) {
            MessageConcatLast(GetMessageDown(instance), " ");
        }
    }
}

void PYGetCandText(FcitxPinyinState* pystate, int iIndex, char *strText)
{
    char *pBase = NULL, *pPhrase;
    PYCandWord* PYCandWords = pystate->PYCandWords;
    PYFA* PYFAList = pystate->PYFAList;

    if (PYCandWords[iIndex].iWhich == PY_CAND_AUTO)
        strcpy(strText, pystate->strPYAuto);
    else {
        pPhrase = NULL;

        switch (PYCandWords[iIndex].iWhich) {
        case PY_CAND_BASE:     //是系统单字
            pBase = PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].strHZ;
            break;
        case PY_CAND_USERPHRASE:       //是用户词组
        case PY_CAND_SYMPHRASE:        //是系统词组
            pBase = PYFAList[PYCandWords[iIndex].cand.phrase.iPYFA].pyBase[PYCandWords[iIndex].cand.phrase.iBase].strHZ;
            pPhrase = PYCandWords[iIndex].cand.phrase.phrase->strPhrase;
            break;
        case PY_CAND_FREQ:     //是常用字
            pBase = PYCandWords[iIndex].cand.freq.hz->strHZ;
            break;
        case PY_CAND_SYMBOL:   //是特殊符号
            pBase = PYCandWords[iIndex].cand.freq.hz->strHZ;
            break;
        }

        strcpy(strText, pBase);
        if (pPhrase)
            strcat(strText, pPhrase);
    }
}

void PYSetCandWordsFlag(FcitxPinyinState* pystate, boolean flag)
{
    FcitxInputState* input = &pystate->owner->input;
    int i;

    for (i = 0; i < input->iCandWordCount; i++)
        PYSetCandWordFlag(pystate, i, flag);
}

void PYSetCandWordFlag(FcitxPinyinState* pystate, int iIndex, boolean flag)
{
    PYCandWord* PYCandWords = pystate->PYCandWords;
    PYFA* PYFAList = pystate->PYFAList;
    switch (PYCandWords[iIndex].iWhich) {
    case PY_CAND_BASE:
        PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].flag = flag;
        break;
    case PY_CAND_SYMPHRASE:
    case PY_CAND_USERPHRASE:
        PYCandWords[iIndex].cand.phrase.phrase->flag = flag;
        break;
    case PY_CAND_FREQ:
        PYCandWords[iIndex].cand.freq.hz->flag = flag;
    case PY_CAND_SYMBOL:
        PYCandWords[iIndex].cand.sym.hz->flag = flag;
        break;
    }
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
                        phrase = PYFAList[candPos.iPYFA].pyBase[candPos.iBase].userPhrase->next;
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
                            phrase = phrase->next;
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

char *PYGetCandWord(void* arg, int iIndex)
{
    FcitxPinyinState *pystate = (FcitxPinyinState* )arg;
    char *pBase = NULL, *pPhrase = NULL;
    char *pBaseMap = NULL, *pPhraseMap = NULL;
    uint *pIndex = NULL;
    boolean bAddNewPhrase = true;
    int i;
    char strHZString[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    int iLen;
    FcitxInputState* input = &pystate->owner->input;
    PYCandWord* PYCandWords = pystate->PYCandWords;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxInstance* instance = pystate->owner;

    if (!input->iCandWordCount)
        return NULL;
    if (iIndex > (input->iCandWordCount - 1))
        iIndex = input->iCandWordCount - 1;
    switch (PYCandWords[iIndex].iWhich) {
    case PY_CAND_AUTO:
        pBase = pystate->strPYAuto;
        pBaseMap = pystate->strPYAutoMap;
        bAddNewPhrase = pystate->pyconfig.bPYSaveAutoAsPhrase;
        break;
    case PY_CAND_BASE:         //是系统单字
        pBase = PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].strHZ;
        pBaseMap = PYFAList[PYCandWords[iIndex].cand.base.iPYFA].strMap;
        pIndex = &(PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].iIndex);
        PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].iHit++;
        pystate->iOrderCount++;
        break;
    case PY_CAND_SYMPHRASE:    //是系统词组
    case PY_CAND_USERPHRASE:   //是用户词组
        pBase = PYFAList[PYCandWords[iIndex].cand.phrase.iPYFA].pyBase[PYCandWords[iIndex].cand.phrase.iBase].strHZ;
        pBaseMap = PYFAList[PYCandWords[iIndex].cand.phrase.iPYFA].strMap;
        pPhrase = PYCandWords[iIndex].cand.phrase.phrase->strPhrase;
        pPhraseMap = PYCandWords[iIndex].cand.phrase.phrase->strMap;
        pIndex = &(PYCandWords[iIndex].cand.phrase.phrase->iIndex);
        PYCandWords[iIndex].cand.phrase.phrase->iHit++;
        pystate->iOrderCount++;
        break;
    case PY_CAND_FREQ:         //是常用字
        pBase = PYCandWords[iIndex].cand.freq.hz->strHZ;
        pBaseMap = PYFAList[PYCandWords[iIndex].cand.freq.hz->iPYFA].strMap;
        PYCandWords[iIndex].cand.freq.hz->iHit++;
        pIndex = &(PYCandWords[iIndex].cand.freq.hz->iIndex);
        pystate->iNewFreqCount++;
        break;
    case PY_CAND_SYMBOL:       //是特殊符号
        pBase = PYCandWords[iIndex].cand.freq.hz->strHZ;
        bAddNewPhrase = false;
        break;
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

    SetMessageText(GetMessageDown(instance), GetMessageCount(GetMessageDown(instance)), pBase);
    if (pPhrase)
        MessageConcat(GetMessageDown(instance), GetMessageCount(GetMessageDown(instance)), pPhrase);
    strcpy(strHZString, pBase);
    if (pPhrase)
        strcat(strHZString, pPhrase);
    iLen = utf8_strlen(strHZString);
    if (iLen == pystate->findMap.iHZCount || PYCandWords[iIndex].iWhich == PY_CAND_SYMBOL) {
        pystate->strPYAuto[0] = '\0';
        for (iLen = 0; iLen < pystate->iPYSelected; iLen++)
            strcat(pystate->strPYAuto, pystate->pySelected[iLen].strHZ);
        strcat(pystate->strPYAuto, strHZString);
        ParsePY(&pystate->pyconfig, input->strCodeInput, &pystate->findMap, PY_PARSE_INPUT_USER, pystate->bSP);
        strHZString[0] = '\0';
        for (i = 0; i < pystate->iPYSelected; i++)
            strcat(strHZString, pystate->pySelected[i].strMap);
        if (pBaseMap)
            strcat(strHZString, pBaseMap);
        if (pPhraseMap)
            strcat(strHZString, pPhraseMap);
        if (bAddNewPhrase && (utf8_strlen(pystate->strPYAuto) <= (MAX_PY_PHRASE_LENGTH)))
            PYAddUserPhrase(pystate, pystate->strPYAuto, strHZString);
        SetMessageCount(GetMessageDown(instance), 0);
        SetMessageCount(GetMessageUp(instance), 0);
        if (UseRemind(&instance->profile)) {
            strcpy(pystate->strPYRemindSource, pystate->strPYAuto);
            strcpy(pystate->strPYRemindMap, strHZString);
            PYGetRemindCandWords(pystate, SM_FIRST);
            pystate->iPYInsertPoint = 0;
            pystate->strFindString[0] = '\0';
        }

        return pystate->strPYAuto;
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
    return NULL;
}

void PYGetCandWordsForward(FcitxPinyinState* pystate)
{
    if (pystate->pCurFreq && pystate->pCurFreq->bIsSym)
        PYGetSymCandWords(pystate, SM_NEXT);
    else {
        PYGetPhraseCandWords(pystate, SM_NEXT);
        if (pystate->pCurFreq)
            PYGetFreqCandWords(pystate, SM_NEXT);
    }

    if (!(pystate->pCurFreq && pystate->pCurFreq->bIsSym))
        PYGetBaseCandWords(pystate, SM_NEXT);
}

void PYGetCandWordsBackward(FcitxPinyinState *pystate)
{
    FcitxInputState* input = &pystate->owner->input;
    if (pystate->pCurFreq && pystate->pCurFreq->bIsSym)
        PYGetSymCandWords(pystate, SM_PREV);
    else {
        PYGetFreqCandWords(pystate, SM_PREV);
        PYGetBaseCandWords(pystate, SM_PREV);
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            return;
        PYGetPhraseCandWords(pystate, SM_PREV);
    }
}

boolean PYCheckNextCandPage(FcitxPinyinState *pystate)
{
    PYCandIndex candPos;
    int val;
    int iMatchedLength;
    char str[3];
    PyPhrase *phrase;
    char strMap[MAX_WORDS_USER_INPUT * 2 + 1];
    HZ *hz;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;

    str[0] = pystate->findMap.strMap[0][0];
    str[1] = pystate->findMap.strMap[0][1];
    str[2] = '\0';
    strMap[0] = '\0';
    if (pystate->pCurFreq && pystate->pCurFreq->bIsSym) {
        hz = pystate->pCurFreq->HZList->next;
        for (val = 0; val < pystate->pCurFreq->iCount; val++) {
            if (!hz->flag)
                return true;
            hz = hz->next;
        }
    } else {
        if (pystate->findMap.iHZCount > 1) {
            for (val = 1; val < pystate->findMap.iHZCount; val++)
                strcat(strMap, pystate->findMap.strMap[val]);
            for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
                if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
                    for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                        phrase = PYFAList[candPos.iPYFA].pyBase[candPos.iBase].userPhrase->next;
                        for (candPos.iPhrase = 0;
                             candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iUserPhrase; candPos.iPhrase++) {
                            val = CmpMap(pyconfig, phrase->strMap, strMap, &iMatchedLength, pystate->bSP);
                            if (!val || (val && (strlen(phrase->strMap) == iMatchedLength))) {
                                    if (!phrase->flag)
                                        return true;
                            }
                            phrase = phrase->next;
                        }
                    }
                }
            }

            for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
                if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
                    for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                        for (candPos.iPhrase = 0;
                             candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iPhrase; candPos.iPhrase++) {
                            if (!PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].flag) {
                                val =
                                    CmpMap(pyconfig, PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap,
                                           strMap, &iMatchedLength, pystate->bSP);
                                if (!val || (val && (strlen(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap)
                                                     == iMatchedLength))) {
                                        return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (pystate->pCurFreq) {
            hz = pystate->pCurFreq->HZList->next;
            for (val = 0; val < pystate->pCurFreq->iCount; val++) {
                if (!hz->flag)
                    return true;
                hz = hz->next;
            }
        }

        for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
            if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
                for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                    if (!PYFAList[candPos.iPYFA].pyBase[candPos.iBase].flag) {
                            return true;
                    }
                }
            }
        }
    }

    return false;
}

void PYGetPhraseCandWords(FcitxPinyinState* pystate, SEARCH_MODE mode)
{
    PYCandIndex candPos;
    char str[3];
    int val, iMatchedLength;
    char strMap[MAX_WORDS_USER_INPUT * 2 + 1];
    PyPhrase *phrase;
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;

    if (pystate->findMap.iHZCount == 1)
        return;
    str[0] = pystate->findMap.strMap[0][0];
    str[1] = pystate->findMap.strMap[0][1];
    str[2] = '\0';
    strMap[0] = '\0';
    for (val = 1; val < pystate->findMap.iHZCount; val++)
        strcat(strMap, pystate->findMap.strMap[val]);
    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                    phrase = PYFAList[candPos.iPYFA].pyBase[candPos.iBase].userPhrase->next;
                    for (candPos.iPhrase = 0;
                         candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iUserPhrase; candPos.iPhrase++) {
                            val = CmpMap(pyconfig, phrase->strMap, strMap, &iMatchedLength, pystate->bSP);
                            if (!val || (val && (strlen(phrase->strMap) == iMatchedLength))) {
                                if ((mode != SM_PREV && !phrase->flag)
                                    || (mode == SM_PREV && phrase->flag)) {
                                    if (!PYAddPhraseCandWord(pystate, candPos, phrase, mode, false))
                                        goto _end;
                                }
                            }

                        phrase = phrase->next;
                    }
            }
        }
    }

    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                    for (candPos.iPhrase = 0; candPos.iPhrase < PYFAList[candPos.iPYFA].pyBase[candPos.iBase].iPhrase; candPos.iPhrase++) {
                            val =
                                CmpMap(pyconfig, PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap,
                                       strMap, &iMatchedLength, pystate->bSP);
                            if (!val
                                || (val
                                    &&
                                    (strlen
                                     (PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].strMap) == iMatchedLength))) {
                                if ((mode != SM_PREV && !PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].flag)
                                    || (mode == SM_PREV && PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase].flag)) {
                                    if (!PYAddPhraseCandWord
                                        (pystate, candPos, &(PYFAList[candPos.iPYFA].pyBase[candPos.iBase].phrase[candPos.iPhrase]), mode, true))
                                        goto _end;
                                }
                            }
                    }
            }
        }
    }

  _end:
    PYSetCandWordsFlag(pystate, true);
}

/*
 * 将一个词加入到候选列表的合适位置中
 * b = true 表示是系统词组，false表示是用户词组
 */
boolean PYAddPhraseCandWord(FcitxPinyinState* pystate, PYCandIndex pos, PyPhrase * phrase, SEARCH_MODE mode, boolean b)
{
    FcitxInputState* input = &pystate->owner->input;
    PYCandWord* PYCandWords = pystate->PYCandWords;
    PYFA* PYFAList = pystate->PYFAList;
    char str[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    int i = 0, j, iStart = 0;

    strcpy(str, PYFAList[pos.iPYFA].pyBase[pos.iBase].strHZ);
    strcat(str, phrase->strPhrase);
    if (pystate->strPYAuto[0]) {
        if (!strcmp(pystate->strPYAuto, str)) {
            phrase->flag = 1;
            return true;
        }
    }

    switch (pystate->pyconfig.phraseOrder) {
    case AD_NO:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO) {
                    iStart = i + 1;
                    i++;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_USERPHRASE || PYCandWords[i].iWhich == PY_CAND_SYMPHRASE) {
                    if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) >= strlen(phrase->strPhrase)) {
                        i++;
                        break;
                    }
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return false;
                else
                    i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_USERPHRASE || PYCandWords[i].iWhich == PY_CAND_SYMPHRASE)
                    if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) < strlen(phrase->strPhrase))
                        break;
            }
            if (i > ConfigGetMaxCandWord(&pystate->owner->config))
                return false;
        }
        break;
        //下面两部分可以放在一起××××××××××××××××××××××××××××××××××××××××××
    case AD_FAST:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO) {
                    iStart = ++i;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_USERPHRASE || PYCandWords[i].iWhich == PY_CAND_SYMPHRASE) {
                    if (strlen(phrase->strPhrase) < strlen(PYCandWords[i].cand.phrase.phrase->strPhrase)) {
                        i++;
                        break;
                    } else if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) == strlen(phrase->strPhrase)) {
                        if (phrase->iIndex < PYCandWords[i].cand.phrase.phrase->iIndex) {
                            i++;
                            break;
                        }
                        if (phrase->iIndex == PYCandWords[i].cand.phrase.phrase->iIndex) {
                            if (phrase->iHit <= PYCandWords[i].cand.phrase.phrase->iHit) {
                                i++;
                                break;
                            }
                        }
                    }
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return true;
                i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_USERPHRASE || PYCandWords[i].iWhich == PY_CAND_SYMPHRASE) {
                    if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) < strlen(phrase->strPhrase))
                        break;
                    else if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) == strlen(phrase->strPhrase)) {
                        if (phrase->iIndex > PYCandWords[i].cand.phrase.phrase->iIndex)
                            break;
                        if (phrase->iIndex == PYCandWords[i].cand.phrase.phrase->iIndex) {
                            if (phrase->iHit > PYCandWords[i].cand.phrase.phrase->iHit)
                                break;
                        }
                    }
                }
            }

            if (i == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
        }
        break;
    case AD_FREQ:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO) {
                    iStart = ++i;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_USERPHRASE || PYCandWords[i].iWhich == PY_CAND_SYMPHRASE) {
                    if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) < strlen(phrase->strPhrase)) {
                        i++;
                        break;
                    } else if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) == strlen(phrase->strPhrase)) {
                        if (phrase->iHit < PYCandWords[i].cand.phrase.phrase->iHit) {
                            i++;
                            break;
                        }
                        if (phrase->iHit == PYCandWords[i].cand.phrase.phrase->iHit) {
                            if (phrase->iIndex <= PYCandWords[i].cand.phrase.phrase->iIndex) {
                                i++;
                                break;
                            }
                        }
                    }
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return true;
                i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_USERPHRASE || PYCandWords[i].iWhich == PY_CAND_SYMPHRASE) {
                    if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) < strlen(phrase->strPhrase))
                        break;
                    else if (strlen(PYCandWords[i].cand.phrase.phrase->strPhrase) == strlen(phrase->strPhrase)) {
                        if (phrase->iHit > PYCandWords[i].cand.phrase.phrase->iHit)
                            break;
                        if (phrase->iHit == PYCandWords[i].cand.phrase.phrase->iHit) {
                            if (phrase->iIndex > PYCandWords[i].cand.phrase.phrase->iIndex)
                                break;
                        }
                    }
                }
            }
            if (i == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
        }
        break;
    }
    //×××××××××××××××××××××××××××××××××××××××××××××××××××××

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config)) {
            for (j = iStart; j < i; j++) {
                PYCandWords[j].iWhich = PYCandWords[j + 1].iWhich;
                switch (PYCandWords[j].iWhich) {
                case PY_CAND_BASE:
                    PYCandWords[j].cand.base.iPYFA = PYCandWords[j + 1].cand.base.iPYFA;
                    PYCandWords[j].cand.base.iBase = PYCandWords[j + 1].cand.base.iBase;
                    break;
                case PY_CAND_SYMPHRASE:
                case PY_CAND_USERPHRASE:
                    PYCandWords[j].cand.phrase.phrase = PYCandWords[j + 1].cand.phrase.phrase;
                    PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j + 1].cand.phrase.iPYFA;
                    PYCandWords[j].cand.phrase.iBase = PYCandWords[j + 1].cand.phrase.iBase;
                    break;
                case PY_CAND_FREQ:
                    PYCandWords[j].cand.freq.hz = PYCandWords[j + 1].cand.freq.hz;
                    PYCandWords[j].cand.freq.strPY = PYCandWords[j + 1].cand.freq.strPY;
                    break;
                }
            }
        } else {
            //插在i的前面
            for (j = input->iCandWordCount; j > i; j--) {
                PYCandWords[j].iWhich = PYCandWords[j - 1].iWhich;
                switch (PYCandWords[j].iWhich) {
                case PY_CAND_BASE:
                    PYCandWords[j].cand.base.iPYFA = PYCandWords[j - 1].cand.base.iPYFA;
                    PYCandWords[j].cand.base.iBase = PYCandWords[j - 1].cand.base.iBase;
                    break;
                case PY_CAND_SYMPHRASE:
                case PY_CAND_USERPHRASE:
                    PYCandWords[j].cand.phrase.phrase = PYCandWords[j - 1].cand.phrase.phrase;
                    PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j - 1].cand.phrase.iPYFA;
                    PYCandWords[j].cand.phrase.iBase = PYCandWords[j - 1].cand.phrase.iBase;
                    break;
                case PY_CAND_FREQ:
                    PYCandWords[j].cand.freq.hz = PYCandWords[j - 1].cand.freq.hz;
                    PYCandWords[j].cand.freq.strPY = PYCandWords[j - 1].cand.freq.strPY;
                    break;
                }
            }
        }
    } else {
        j = input->iCandWordCount;
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            j--;
        for (; j > i; j--) {
            PYCandWords[j].iWhich = PYCandWords[j - 1].iWhich;
            switch (PYCandWords[j].iWhich) {
            case PY_CAND_BASE:
                PYCandWords[j].cand.base.iPYFA = PYCandWords[j - 1].cand.base.iPYFA;
                PYCandWords[j].cand.base.iBase = PYCandWords[j - 1].cand.base.iBase;
                break;
            case PY_CAND_SYMPHRASE:
            case PY_CAND_USERPHRASE:
                PYCandWords[j].cand.phrase.phrase = PYCandWords[j - 1].cand.phrase.phrase;
                PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j - 1].cand.phrase.iPYFA;
                PYCandWords[j].cand.phrase.iBase = PYCandWords[j - 1].cand.phrase.iBase;
                break;
            case PY_CAND_FREQ:
                PYCandWords[j].cand.freq.hz = PYCandWords[j - 1].cand.freq.hz;
                PYCandWords[j].cand.freq.strPY = PYCandWords[j - 1].cand.freq.strPY;
                break;
            }
        }
    }

    PYCandWords[i].iWhich = (b) ? PY_CAND_SYMPHRASE : PY_CAND_USERPHRASE;
    PYCandWords[i].cand.phrase.phrase = phrase;
    PYCandWords[i].cand.phrase.iPYFA = pos.iPYFA;
    PYCandWords[i].cand.phrase.iBase = pos.iBase;
    if (input->iCandWordCount != ConfigGetMaxCandWord(&pystate->owner->config))
        input->iCandWordCount++;
    return true;
}

//********************************************
void PYGetSymCandWords(FcitxPinyinState* pystate, SEARCH_MODE mode)
{
    int i;
    HZ *hz;

    if (pystate->pCurFreq && pystate->pCurFreq->bIsSym) {
        hz = pystate->pCurFreq->HZList->next;
        for (i = 0; i < pystate->pCurFreq->iCount; i++) {
            if ((mode != SM_PREV && !hz->flag)
                || (mode == SM_PREV && hz->flag)) {
                if (!PYAddSymCandWord(pystate, hz, mode))
                    break;
            }
            hz = hz->next;
        }
    }

    PYSetCandWordsFlag(pystate, true);
}

/*
 * 将一个符号加入到候选列表的合适位置中
 * 符号不需进行频率调整
 */
boolean PYAddSymCandWord(FcitxPinyinState* pystate, HZ * hz, SEARCH_MODE mode)
{
    int i, j;
    FcitxInputState* input = &pystate->owner->input;
    PYCandWord* PYCandWords = pystate->PYCandWords;

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config)) {
            i = input->iCandWordCount - 1;
            for (j = 0; j < i; j++)
                PYCandWords[j].cand.sym.hz = PYCandWords[j + 1].cand.sym.hz;
        } else
            i = input->iCandWordCount;
    } else {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            return false;
        i = input->iCandWordCount;
        for (j = input->iCandWordCount - 1; j > i; j--)
            PYCandWords[j].cand.sym.hz = PYCandWords[j - 1].cand.sym.hz;
    }

    PYCandWords[i].iWhich = PY_CAND_SYMBOL;
    PYCandWords[i].cand.sym.hz = hz;
    if (input->iCandWordCount != ConfigGetMaxCandWord(&pystate->owner->config))
        input->iCandWordCount++;
    return true;
}

//*****************************************************

void PYGetBaseCandWords(FcitxPinyinState* pystate, SEARCH_MODE mode)
{
    PYCandIndex candPos = {
        0, 0, 0
    };
    char str[3];
    PYFA* PYFAList = pystate->PYFAList;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;

    str[0] = pystate->findMap.strMap[0][0];
    str[1] = pystate->findMap.strMap[0][1];
    str[2] = '\0';
    for (candPos.iPYFA = 0; candPos.iPYFA < pystate->iPYFACount; candPos.iPYFA++) {
        if (!Cmp2Map(pyconfig, PYFAList[candPos.iPYFA].strMap, str, pystate->bSP)) {
            for (candPos.iBase = 0; candPos.iBase < PYFAList[candPos.iPYFA].iBase; candPos.iBase++) {
                    if ((mode != SM_PREV && !PYFAList[candPos.iPYFA].pyBase[candPos.iBase].flag)
                        || (mode == SM_PREV && PYFAList[candPos.iPYFA].pyBase[candPos.iBase].flag)) {
                        if (!PYIsInFreq(pystate, PYFAList[candPos.iPYFA].pyBase[candPos.iBase].strHZ)) {
                            if (!PYAddBaseCandWord(pystate, candPos, mode))
                                goto _end;
                        }
                    }
            }
        }
    }

  _end:
    PYSetCandWordsFlag(pystate, true);
}

/*
 * 将一个字加入到候选列表的合适位置中
 */
boolean PYAddBaseCandWord(FcitxPinyinState* pystate, PYCandIndex pos, SEARCH_MODE mode)
{
    int i = 0, j;
    int iStart = 0;
    FcitxInputState* input = &pystate->owner->input;
    PYCandWord* PYCandWords = pystate->PYCandWords;
    PYFA* PYFAList = pystate->PYFAList;

    switch (pystate->pyconfig.baseOrder) {
    case AD_NO:
        if (mode == SM_PREV) {
            if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i = input->iCandWordCount - 1;
            else
                i = input->iCandWordCount;
        } else {
            if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                return false;
            i = input->iCandWordCount;
        }
        break;
    case AD_FAST:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO || PYCandWords[i].iWhich == PY_CAND_FREQ) {
                    iStart = i + 1;
                    i++;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_BASE) {
                    if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                        [PYCandWords[i].cand.base.iBase].iIndex > PYFAList[pos.iPYFA].pyBase[pos.iBase].iIndex) {
                        i++;
                        break;
                    } else
                        if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                            [PYCandWords[i].cand.base.iBase].iIndex == PYFAList[pos.iPYFA].pyBase[pos.iBase].iIndex) {
                        if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                            [PYCandWords[i].cand.base.iBase].iHit >= PYFAList[pos.iPYFA].pyBase[pos.iBase].iHit) {
                            i++;
                            break;
                        }
                    }
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return true;
                i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_BASE) {
                    if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                        [PYCandWords[i].cand.base.iBase].iIndex < PYFAList[pos.iPYFA].pyBase[pos.iBase].iIndex)
                        break;
                    else if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase[PYCandWords[i].cand.base.iBase].iIndex ==
                             PYFAList[pos.iPYFA].pyBase[pos.iBase].iIndex) {
                        if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase[PYCandWords[i].cand.base.iBase].iHit <
                            PYFAList[pos.iPYFA].pyBase[pos.iBase].iHit)
                            break;
                    }
                }
            }

            if (i == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
        }

        break;
    case AD_FREQ:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO || PYCandWords[i].iWhich == PY_CAND_FREQ) {
                    iStart = i + 1;
                    i++;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_BASE) {
                    if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                        [PYCandWords[i].cand.base.iBase].iHit > PYFAList[pos.iPYFA].pyBase[pos.iBase].iHit) {
                        i++;
                        break;
                    } else
                        if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                            [PYCandWords[i].cand.base.iBase].iHit == PYFAList[pos.iPYFA].pyBase[pos.iBase].iHit) {
                        if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                            [PYCandWords[i].cand.base.iBase].iIndex >= PYFAList[pos.iPYFA].pyBase[pos.iBase].iIndex) {
                            i++;
                            break;
                        }
                    }
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return true;
                i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_BASE) {
                    if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase
                        [PYCandWords[i].cand.base.iBase].iHit < PYFAList[pos.iPYFA].pyBase[pos.iBase].iHit)
                        break;
                    else if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase[PYCandWords[i].cand.base.iBase].iHit ==
                             PYFAList[pos.iPYFA].pyBase[pos.iBase].iHit) {
                        if (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase[PYCandWords[i].cand.base.iBase].iIndex <
                            PYFAList[pos.iPYFA].pyBase[pos.iBase].iIndex)
                            break;
                    }
                }
            }
            if (i == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
        }
        break;
    }

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config)) {
            for (j = iStart; j < i; j++) {
                PYCandWords[j].iWhich = PYCandWords[j + 1].iWhich;
                switch (PYCandWords[j].iWhich) {
                case PY_CAND_BASE:
                    PYCandWords[j].cand.base.iPYFA = PYCandWords[j + 1].cand.base.iPYFA;
                    PYCandWords[j].cand.base.iBase = PYCandWords[j + 1].cand.base.iBase;
                    break;
                case PY_CAND_SYMPHRASE:
                case PY_CAND_USERPHRASE:
                    PYCandWords[j].cand.phrase.phrase = PYCandWords[j + 1].cand.phrase.phrase;
                    PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j + 1].cand.phrase.iPYFA;
                    PYCandWords[j].cand.phrase.iBase = PYCandWords[j + 1].cand.phrase.iBase;
                    break;
                case PY_CAND_FREQ:
                    PYCandWords[j].cand.freq.hz = PYCandWords[j + 1].cand.freq.hz;
                    PYCandWords[j].cand.freq.strPY = PYCandWords[j + 1].cand.freq.strPY;
                    break;
                }
            }
        } else {
            for (j = input->iCandWordCount; j > i; j--) {
                PYCandWords[j].iWhich = PYCandWords[j - 1].iWhich;
                switch (PYCandWords[j].iWhich) {
                case PY_CAND_BASE:
                    PYCandWords[j].cand.base.iPYFA = PYCandWords[j - 1].cand.base.iPYFA;
                    PYCandWords[j].cand.base.iBase = PYCandWords[j - 1].cand.base.iBase;
                    break;
                case PY_CAND_SYMPHRASE:
                case PY_CAND_USERPHRASE:
                    PYCandWords[j].cand.phrase.phrase = PYCandWords[j - 1].cand.phrase.phrase;
                    PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j - 1].cand.phrase.iPYFA;
                    PYCandWords[j].cand.phrase.iBase = PYCandWords[j - 1].cand.phrase.iBase;
                    break;
                case PY_CAND_FREQ:
                    PYCandWords[j].cand.freq.hz = PYCandWords[j - 1].cand.freq.hz;
                    PYCandWords[j].cand.freq.strPY = PYCandWords[j - 1].cand.freq.strPY;
                    break;
                }
            }
        }
    } else {
        j = input->iCandWordCount;
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            j--;
        for (; j > i; j--) {
            PYCandWords[j].iWhich = PYCandWords[j - 1].iWhich;
            switch (PYCandWords[j].iWhich) {
            case PY_CAND_BASE:
                PYCandWords[j].cand.base.iPYFA = PYCandWords[j - 1].cand.base.iPYFA;
                PYCandWords[j].cand.base.iBase = PYCandWords[j - 1].cand.base.iBase;
                break;
            case PY_CAND_SYMPHRASE:
            case PY_CAND_USERPHRASE:
                PYCandWords[j].cand.phrase.phrase = PYCandWords[j - 1].cand.phrase.phrase;
                PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j - 1].cand.phrase.iPYFA;
                PYCandWords[j].cand.phrase.iBase = PYCandWords[j - 1].cand.phrase.iBase;
                break;
            case PY_CAND_FREQ:
                PYCandWords[j].cand.freq.hz = PYCandWords[j - 1].cand.freq.hz;
                PYCandWords[j].cand.freq.strPY = PYCandWords[j - 1].cand.freq.strPY;
                break;
            }
        }
    }

    PYCandWords[i].iWhich = PY_CAND_BASE;
    PYCandWords[i].cand.base.iPYFA = pos.iPYFA;
    PYCandWords[i].cand.base.iBase = pos.iBase;
    if (input->iCandWordCount != ConfigGetMaxCandWord(&pystate->owner->config))
        input->iCandWordCount++;
    return true;
}

void PYGetFreqCandWords(FcitxPinyinState* pystate, SEARCH_MODE mode)
{
    int i;
    HZ *hz;

    if (pystate->pCurFreq && !pystate->pCurFreq->bIsSym) {
        hz = pystate->pCurFreq->HZList->next;
        for (i = 0; i < pystate->pCurFreq->iCount; i++) {
            if ((mode != SM_PREV && !hz->flag)
                || (mode == SM_PREV && hz->flag)) {
                if (!PYAddFreqCandWord(pystate, hz, pystate->pCurFreq->strPY, mode))
                    break;
            }
            hz = hz->next;
        }
    }

    PYSetCandWordsFlag(pystate, true);
}

/*
 * 将一个常用字加入到候选列表的合适位置中
 */
boolean PYAddFreqCandWord(FcitxPinyinState* pystate, HZ * hz, char *strPY, SEARCH_MODE mode)
{
    int i = 0, j;
    int iStart = 0;
    FcitxInputState* input = &pystate->owner->input;
    PYCandWord* PYCandWords = pystate->PYCandWords;

    switch (pystate->pyconfig.freqOrder) {
    case AD_NO:
        if (mode == SM_PREV) {
            if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i = input->iCandWordCount - 1;
        } else {
            if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                return false;
            i = input->iCandWordCount;
        }
        break;
    case AD_FAST:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO) {
                    iStart = i + 1;
                    i++;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_FREQ && (hz->iIndex <= PYCandWords[i].cand.freq.hz->iIndex)) {
                    i++;
                    break;
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return true;
                i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_FREQ && (hz->iIndex > PYCandWords[i].cand.freq.hz->iIndex))
                    break;
            }
            if (i == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
        }
        break;
    case AD_FREQ:
        if (mode == SM_PREV) {
            for (i = (input->iCandWordCount - 1); i >= 0; i--) {
                if (PYCandWords[i].iWhich == PY_CAND_AUTO) {
                    iStart = i + 1;
                    i++;
                    break;
                } else if (PYCandWords[i].iWhich == PY_CAND_FREQ && (hz->iHit <= PYCandWords[i].cand.freq.hz->iHit)) {
                    i++;
                    break;
                }
            }

            if (i < 0) {
                if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                    return true;
                i = 0;
            } else if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                i--;
        } else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (PYCandWords[i].iWhich == PY_CAND_FREQ && (hz->iHit > PYCandWords[i].cand.freq.hz->iHit))
                    break;
            }
            if (i == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
        }
        break;
    }

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config)) {
            for (j = iStart; j < i; j++) {
                PYCandWords[j].iWhich = PYCandWords[j + 1].iWhich;
                switch (PYCandWords[j].iWhich) {
                case PY_CAND_BASE:
                    PYCandWords[j].cand.base.iPYFA = PYCandWords[j + 1].cand.base.iPYFA;
                    PYCandWords[j].cand.base.iBase = PYCandWords[j + 1].cand.base.iBase;
                    break;
                case PY_CAND_SYMPHRASE:
                case PY_CAND_USERPHRASE:
                    PYCandWords[j].cand.phrase.phrase = PYCandWords[j + 1].cand.phrase.phrase;
                    PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j + 1].cand.phrase.iPYFA;
                    PYCandWords[j].cand.phrase.iBase = PYCandWords[j + 1].cand.phrase.iBase;
                    break;
                case PY_CAND_FREQ:
                    PYCandWords[j].cand.freq.hz = PYCandWords[j + 1].cand.freq.hz;
                    PYCandWords[j].cand.freq.strPY = PYCandWords[j + 1].cand.freq.strPY;
                    break;
                }
            }
        } else {
            for (j = input->iCandWordCount; j > i; j--) {
                PYCandWords[j].iWhich = PYCandWords[j - 1].iWhich;
                switch (PYCandWords[j].iWhich) {
                case PY_CAND_BASE:
                    PYCandWords[j].cand.base.iPYFA = PYCandWords[j - 1].cand.base.iPYFA;
                    PYCandWords[j].cand.base.iBase = PYCandWords[j - 1].cand.base.iBase;
                    break;
                case PY_CAND_SYMPHRASE:
                case PY_CAND_USERPHRASE:
                    PYCandWords[j].cand.phrase.phrase = PYCandWords[j - 1].cand.phrase.phrase;
                    PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j - 1].cand.phrase.iPYFA;
                    PYCandWords[j].cand.phrase.iBase = PYCandWords[j - 1].cand.phrase.iBase;
                    break;
                case PY_CAND_FREQ:
                    PYCandWords[j].cand.freq.hz = PYCandWords[j - 1].cand.freq.hz;
                    PYCandWords[j].cand.freq.strPY = PYCandWords[j - 1].cand.freq.strPY;
                    break;
                }
            }
        }
    } else {
        j = input->iCandWordCount;
        if (input->iCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            j--;
        for (; j > i; j--) {
            PYCandWords[j].iWhich = PYCandWords[j - 1].iWhich;
            switch (PYCandWords[j].iWhich) {
            case PY_CAND_BASE:
                PYCandWords[j].cand.base.iPYFA = PYCandWords[j - 1].cand.base.iPYFA;
                PYCandWords[j].cand.base.iBase = PYCandWords[j - 1].cand.base.iBase;
                break;
            case PY_CAND_SYMPHRASE:
            case PY_CAND_USERPHRASE:
                PYCandWords[j].cand.phrase.phrase = PYCandWords[j - 1].cand.phrase.phrase;
                PYCandWords[j].cand.phrase.iPYFA = PYCandWords[j - 1].cand.phrase.iPYFA;
                PYCandWords[j].cand.phrase.iBase = PYCandWords[j - 1].cand.phrase.iBase;
                break;
            case PY_CAND_FREQ:
                PYCandWords[j].cand.freq.hz = PYCandWords[j - 1].cand.freq.hz;
                PYCandWords[j].cand.freq.strPY = PYCandWords[j - 1].cand.freq.strPY;
                break;
            }
        }
    }

    PYCandWords[i].iWhich = PY_CAND_FREQ;
    PYCandWords[i].cand.freq.hz = hz;
    PYCandWords[i].cand.freq.strPY = strPY;
    if (input->iCandWordCount != ConfigGetMaxCandWord(&pystate->owner->config))
        input->iCandWordCount++;
    return true;
}

/*
 * 将一个词组保存到用户词组库中
 * 返回true表示是新词组
 */
boolean PYAddUserPhrase(FcitxPinyinState* pystate, char *phrase, char *map)
{
    PyPhrase *userPhrase, *newPhrase, *temp;
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
        if (!strcmp(map + 2, userPhrase->strMap)
            && !strcmp(phrase + clen, userPhrase->strPhrase))
            return false;
        userPhrase = userPhrase->next;
    }

    //然后，看它是不是在系统词组库中
    for (k = 0; k < PYFAList[i].pyBase[j].iPhrase; k++)
        if (!strcmp(map + 2, PYFAList[i].pyBase[j].phrase[k].strMap)
            && !strcmp(phrase + clen, PYFAList[i].pyBase[j].phrase[k].strPhrase))
            return false;
    //下面将词组添加到列表中
    newPhrase = (PyPhrase *) malloc(sizeof(PyPhrase));
    newPhrase->strMap = (char *) malloc(sizeof(char) * strlen(map + 2) + 1);
    newPhrase->strPhrase = (char *) malloc(sizeof(char) * strlen(phrase + clen) + 1);
    strcpy(newPhrase->strMap, map + 2);
    strcpy(newPhrase->strPhrase, phrase + clen);
    newPhrase->iIndex = ++pystate->iCounter;
    newPhrase->iHit = 1;
    newPhrase->flag = 0;
    temp = PYFAList[i].pyBase[j].userPhrase;
    userPhrase = PYFAList[i].pyBase[j].userPhrase->next;
    for (k = 0; k < PYFAList[i].pyBase[j].iUserPhrase; k++) {
        if (CmpMap(pyconfig, map + 2, userPhrase->strMap, &iTemp, pystate->bSP) > 0)
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

void PYDelUserPhrase(FcitxPinyinState* pystate, int iPYFA, int iBase, PyPhrase * phrase)
{
    PyPhrase *temp;
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
    free(phrase->strPhrase);
    free(phrase->strMap);
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

    if (pystate->isSavingPYUserPhrase)
        return;

    pystate->isSavingPYUserPhrase = true;

    fp = GetXDGFileWithPrefix("pinyin", TEMP_FILE, "wb", &pstr);
    if (!fp) {
        pystate->isSavingPYUserPhrase = false;
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
                phrase = PYFAList[i].pyBase[j].userPhrase->next;
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
                    phrase = phrase->next;
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

    pystate->isSavingPYUserPhrase = false;
}

void SavePYFreq(FcitxPinyinState *pystate)
{
    int i, j, k;
    char *pstr;
    char strPathTemp[PATH_MAX];
    FILE *fp;
    PyFreq *pPyFreq;
    HZ *hz;

    if (pystate->isSavingPYFreq)
        return;

    pystate->isSavingPYFreq = true;
    fp = GetXDGFileWithPrefix("pinyin", TEMP_FILE, "wb", &pstr);
    if (!fp) {
        pystate->isSavingPYFreq = false;
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

    pystate->isSavingPYFreq = false;
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

    if (pystate->isSavingPYIndex)
        return;

    pystate->isSavingPYIndex = true;
    fp = GetXDGFileWithPrefix("pinyin", TEMP_FILE, "wb", &pstr);
    if (!fp) {
        pystate->isSavingPYIndex = false;
        FcitxLog(ERROR, _("Cannot Save Pinyin Index: %s"), pstr);
        return;
    }
    strcpy(strPathTemp, pstr);
    free(pstr);

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

    pystate->isSavingPYIndex = false;
}

/*
 * 设置拼音的常用字表
 * 只有以下情形才能设置
 *	当用户输入单字时
 * 至于常用词的问题暂时不考虑
 */
void PYAddFreq(FcitxPinyinState* pystate, int iIndex)
{
    int i;
    HZ *HZTemp;
    PyFreq *freq;
    HZ *hz;
    PYFA* PYFAList = pystate->PYFAList;
    PYCandWord* PYCandWords = pystate->PYCandWords;

    //能到这儿来，就说明候选列表中都是单字
    //首先，看这个字是不是已经在常用字表中
    i = 1;
    if (pystate->pCurFreq) {
        i = -1;
        if (pystate->PYCandWords[iIndex].iWhich != PY_CAND_FREQ) {
            //说明该字是系统单字
            HZTemp = pystate->pCurFreq->HZList->next;
            for (i = 0; i < pystate->pCurFreq->iCount; i++) {
                if (!strcmp(PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].strHZ, HZTemp->strHZ)) {
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
    PYSetCandWordsFlag(pystate, false);
    //需要添加该字，此时该字必然是系统单字
    if (!pystate->pCurFreq) {
        freq = (PyFreq *) malloc(sizeof(PyFreq));
        freq->HZList = (HZ *) malloc(sizeof(HZ));
        freq->HZList->next = NULL;
        strcpy(freq->strPY, pystate->strFindString);
        freq->next = NULL;
        freq->iCount = 0;
        freq->bIsSym = false;
        pystate->pCurFreq = pystate->pyFreq;
        for (i = 0; i < pystate->iPYFreqCount; i++)
            pystate->pCurFreq = pystate->pCurFreq->next;
        pystate->pCurFreq->next = freq;
        pystate->iPYFreqCount++;
        pystate->pCurFreq = freq;
    }

    HZTemp = (HZ *) malloc(sizeof(HZ));
    memset(HZTemp, 0 , sizeof(HZ));
    strcpy(HZTemp->strHZ, PYFAList[PYCandWords[iIndex].cand.base.iPYFA].pyBase[PYCandWords[iIndex].cand.base.iBase].strHZ);
    HZTemp->iPYFA = PYCandWords[iIndex].cand.base.iPYFA;
    HZTemp->iHit = 0;
    HZTemp->iIndex = 0;
    HZTemp->flag = 0;
    HZTemp->next = NULL;
    //将HZTemp加到链表尾部
    hz = pystate->pCurFreq->HZList;
    for (i = 0; i < pystate->pCurFreq->iCount; i++)
        hz = hz->next;
    hz->next = HZTemp;
    pystate->pCurFreq->iCount++;
    pystate->iNewFreqCount++;
    if (pystate->iNewFreqCount == AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
        pystate->iNewFreqCount = 0;
    }
}

/*
 * 删除拼音常用字表中的某个字
 */
void PYDelFreq(FcitxPinyinState *pystate, int iIndex)
{
    HZ *hz;
    PYCandWord* PYCandWords = pystate->PYCandWords;

    //能到这儿来，就说明候选列表中都是单字
    //首先，看这个字是不是已经在常用字表中
    if (PYCandWords[iIndex].iWhich != PY_CAND_FREQ)
        return;
    PYSetCandWordsFlag(pystate, false);
    //先找到需要删除单字的位置
    hz = pystate->pCurFreq->HZList;
    while (hz->next != PYCandWords[iIndex].cand.freq.hz)
        hz = hz->next;
    hz->next = PYCandWords[iIndex].cand.freq.hz->next;
    free(PYCandWords[iIndex].cand.freq.hz);
    pystate->pCurFreq->iCount--;
    pystate->iNewFreqCount++;
    if (pystate->iNewFreqCount == AUTOSAVE_FREQ_COUNT) {
        SavePYFreq(pystate);
        pystate->iNewFreqCount = 0;
    }
}

/*
 * 判断一个字是否已经是常用字
 */
boolean PYIsInFreq(FcitxPinyinState* pystate, char *strHZ)
{
    HZ *hz;
    int i;

    if (!pystate->pCurFreq || pystate->pCurFreq->bIsSym)
        return false;
    hz = pystate->pCurFreq->HZList->next;
    for (i = 0; i < pystate->pCurFreq->iCount; i++) {
        if (!strcmp(strHZ, hz->strHZ))
            return true;
        hz = hz->next;
    }

    return false;
}

/*
 * 取得拼音的联想字串
 * 	按照频率来定排列顺序
 */
INPUT_RETURN_VALUE PYGetRemindCandWords(void *arg, SEARCH_MODE mode)
{
    int i, j;
    char strTemp[2];
    PyPhrase *phrase;
    FcitxPinyinState* pystate = (FcitxPinyinState*) arg;
    GenericConfig *fc = &pystate->owner->config.gconfig;
    boolean bDisablePagingInRemind = *(ConfigGetBindValue(fc, "Output", "RemindModeDisablePaging").boolvalue);
    FcitxInstance *instance = pystate->owner;
    FcitxInputState *input = &pystate->owner->input;
    PYFA* PYFAList = pystate->PYFAList;

    if (!pystate->strPYRemindSource[0])
        return IRV_TO_PROCESS;
    if (mode == SM_FIRST) {
        input->iRemindCandPageCount = 0;
        input->iRemindCandWordCount = 0;
        input->iCurrentRemindCandPage = 0;
        PYResetFlags(pystate);
        pystate->pyBaseForLengend = NULL;
        for (i = 0; i < pystate->iPYFACount; i++) {
            if (!strncmp(pystate->strPYRemindMap, PYFAList[i].strMap, 2)) {
                for (j = 0; j < PYFAList[i].iBase; j++) {
                    if (!utf8_strncmp(pystate->strPYRemindSource, PYFAList[i].pyBase[j].strHZ, 1)) {
                        pystate->pyBaseForLengend = &(PYFAList[i].pyBase[j]);
                        goto _HIT;
                    }
                }
            }
        }

      _HIT:
        if (!pystate->pyBaseForLengend)
            return IRV_TO_PROCESS;
        instance->bShowCursor = false;
    } else {
        if (!input->iRemindCandPageCount)
            return IRV_TO_PROCESS;
        if (mode == SM_NEXT) {
            if (input->iCurrentRemindCandPage == input->iRemindCandPageCount)
                return IRV_DO_NOTHING;
            input->iRemindCandWordCount = 0;
            input->iCurrentRemindCandPage++;
        } else {
            if (!input->iCurrentRemindCandPage)
                return IRV_DO_NOTHING;
            input->iCurrentRemindCandPage--;
            PYSetRemindCandWordsFlag(pystate, false);
        }
    }

    for (i = 0; i < pystate->pyBaseForLengend->iPhrase; i++) {
        if (utf8_strlen(pystate->strPYRemindSource) == 1) {
            if (utf8_strlen(pystate->pyBaseForLengend->phrase[i].strPhrase) == 1 && ((mode != SM_PREV && !pystate->pyBaseForLengend->phrase[i].flag)
                                                                            || (mode == SM_PREV && pystate->pyBaseForLengend->phrase[i].flag))) {
                if (!PYAddLengendCandWord(pystate, &pystate->pyBaseForLengend->phrase[i], mode))
                    break;
            }
        } else if (strlen(pystate->pyBaseForLengend->phrase[i].strPhrase) == strlen(pystate->strPYRemindSource)) {
            if (!strncmp
                (pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource),
                 pystate->pyBaseForLengend->phrase[i].strPhrase, strlen(pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource)))
                && ((mode != SM_PREV && !pystate->pyBaseForLengend->phrase[i].flag)
                    || (mode == SM_PREV && pystate->pyBaseForLengend->phrase[i].flag))) {
                if (!PYAddLengendCandWord(pystate, &pystate->pyBaseForLengend->phrase[i], mode))
                    break;
            }
        }
    }

    phrase = pystate->pyBaseForLengend->userPhrase->next;
    for (i = 0; i < pystate->pyBaseForLengend->iUserPhrase; i++) {
        if (utf8_strlen(pystate->strPYRemindSource) == 1) {
            if (utf8_strlen(phrase->strPhrase) == 1 && ((mode != SM_PREV && !phrase->flag)
                                                        || (mode == SM_PREV && phrase->flag))) {
                if (!PYAddLengendCandWord(pystate, phrase, mode))
                    break;
            }
        } else if (strlen(phrase->strPhrase) == strlen(pystate->strPYRemindSource)) {
            if (!strncmp
                (pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource),
                 phrase->strPhrase, strlen(pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource)))
                && ((mode != SM_PREV && !phrase->flag)
                    || (mode == SM_PREV && phrase->flag))) {
                if (!PYAddLengendCandWord(pystate, phrase, mode))
                    break;
            }
        }

        phrase = phrase->next;
    }

    PYSetRemindCandWordsFlag(pystate, true);
    if (!bDisablePagingInRemind && mode != SM_PREV && input->iCurrentRemindCandPage == input->iRemindCandPageCount) {
        for (i = 0; i < pystate->pyBaseForLengend->iPhrase; i++) {
            if (utf8_strlen(pystate->strPYRemindSource) == 1) {
                if (utf8_strlen(pystate->pyBaseForLengend->phrase[i].strPhrase) == 1 && !pystate->pyBaseForLengend->phrase[i].flag) {
                    input->iRemindCandPageCount++;
                    goto _NEWPAGE;
                }
            } else if (strlen(pystate->pyBaseForLengend->phrase[i].strPhrase) == strlen(pystate->strPYRemindSource)) {
                if (!strncmp
                    (pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource),
                     pystate->pyBaseForLengend->phrase[i].strPhrase, strlen(pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource)))
                    && (!pystate->pyBaseForLengend->phrase[i].flag)) {
                    input->iRemindCandPageCount++;
                    goto _NEWPAGE;
                }
            }
        }

        phrase = pystate->pyBaseForLengend->userPhrase->next;
        for (i = 0; i < pystate->pyBaseForLengend->iUserPhrase; i++) {
            if (utf8_strlen(pystate->strPYRemindSource) == 1) {
                if (utf8_strlen(phrase->strPhrase) == 1 && (!phrase->flag)) {
                    input->iRemindCandPageCount++;
                    goto _NEWPAGE;
                }
            } else if (strlen(pystate->pyBaseForLengend->phrase[i].strPhrase) == strlen(pystate->strPYRemindSource)) {
                if (!strncmp
                    (pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource),
                     phrase->strPhrase, strlen(pystate->strPYRemindSource + utf8_char_len(pystate->strPYRemindSource)))
                    && (!phrase->flag)) {
                    input->iRemindCandPageCount++;
                    goto _NEWPAGE;
                }
            }
            phrase = phrase->next;
        }
      _NEWPAGE:
        ;
    }

    SetMessageCount(GetMessageUp(instance), 0);
    AddMessageAtLast(GetMessageUp(instance), MSG_TIPS, _("Remind: "));
    AddMessageAtLast(GetMessageUp(instance), MSG_INPUT, "%s", pystate->strPYRemindSource);
    strTemp[1] = '\0';
    SetMessageCount(GetMessageDown(instance), 0);
    for (i = 0; i < input->iRemindCandWordCount; i++) {
        strTemp[0] = i + 1 + '0';
        if (i == 9)
            strTemp[0] = '0';

        AddMessageAtLast(GetMessageDown(instance), MSG_INDEX, "%s", strTemp);
        AddMessageAtLast(GetMessageDown(instance),
                         ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER),
                         "%s", pystate->PYRemindCandWords[i].phrase->strPhrase + pystate->PYRemindCandWords[i].iLength);
        if (i != (input->iRemindCandWordCount - 1)) {
            MessageConcatLast(GetMessageDown(instance), " ");
        }
    }

    input->bIsInRemind = (input->iRemindCandWordCount != 0);
    return IRV_DISPLAY_CANDWORDS;
}

boolean PYAddLengendCandWord(FcitxPinyinState* pystate, PyPhrase * phrase, SEARCH_MODE mode)
{
    int i = 0, j;
    FcitxInputState* input = &pystate->owner->input;
    PYRemindCandWord* PYRemindCandWords = pystate->PYRemindCandWords;

    if (mode == SM_PREV) {
        for (i = (input->iRemindCandWordCount - 1); i >= 0; i--) {
            if (PYRemindCandWords[i].phrase->iHit >= phrase->iHit) {
                i++;
                break;
            }
        }

        if (i < 0) {
            if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
                return true;
            i = 0;
        } else if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            i--;
    } else {
        for (i = 0; i < input->iRemindCandWordCount; i++) {
            if (PYRemindCandWords[i].phrase->iHit < phrase->iHit)
                break;
        }
        if (i == ConfigGetMaxCandWord(&pystate->owner->config))
            return true;
    }

    if (mode == SM_PREV) {
        if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config)) {
            for (j = 0; j < i; j++) {
                PYRemindCandWords[j].phrase = PYRemindCandWords[j + 1].phrase;
                PYRemindCandWords[j].iLength = PYRemindCandWords[j + 1].iLength;
            }
        } else {
            for (j = input->iRemindCandWordCount; j > i; j--) {
                PYRemindCandWords[j].phrase = PYRemindCandWords[j - 1].phrase;
                PYRemindCandWords[j].iLength = PYRemindCandWords[j - 1].iLength;
            }
        }
    } else {
        j = input->iRemindCandWordCount;
        if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&pystate->owner->config))
            j--;
        for (; j > i; j--) {
            PYRemindCandWords[j].phrase = PYRemindCandWords[j - 1].phrase;
            PYRemindCandWords[j].iLength = PYRemindCandWords[j - 1].iLength;
        }
    }

    PYRemindCandWords[i].phrase = phrase;
    PYRemindCandWords[i].iLength = strlen(pystate->strPYRemindSource) - utf8_char_len(pystate->strPYRemindSource);
    if (input->iRemindCandWordCount != ConfigGetMaxCandWord(&pystate->owner->config))
        input->iRemindCandWordCount++;
    return true;
}

char *PYGetRemindCandWord(void *arg, int iIndex)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    FcitxInputState* input = &pystate->owner->input;
    PYRemindCandWord* PYRemindCandWords = pystate->PYRemindCandWords;
    if (input->iRemindCandWordCount) {
        if (iIndex > (input->iRemindCandWordCount - 1))
            iIndex = input->iRemindCandWordCount - 1;
        strcpy(pystate->strPYRemindSource, PYRemindCandWords[iIndex].phrase->strPhrase + PYRemindCandWords[iIndex].iLength);
        strcpy(pystate->strPYRemindMap, PYRemindCandWords[iIndex].phrase->strMap + PYRemindCandWords[iIndex].iLength);
        PYGetRemindCandWords(pystate, SM_FIRST);
        return pystate->strPYRemindSource;
    }

    return NULL;
}

void PYSetRemindCandWordsFlag(FcitxPinyinState*pystate, boolean flag)
{
    int i;
    FcitxInputState* input = &pystate->owner->input;

    for (i = 0; i < input->iRemindCandWordCount; i++)
        pystate->PYRemindCandWords[i].phrase->flag = flag;
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

void* PYGetCandTextWrapper(void * arg, FcitxModuleFunctionArg args)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    int *a = args.args[0];
    char *b = args.args[1];
    PYGetCandText(pystate, *a, b);
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
    SEARCH_MODE *a = args.args[0];
    PYGetCandWords(arg, *a);
    return NULL;
}

void* PYGetCandWordWrapper(void * arg, FcitxModuleFunctionArg args)
{
    int *a = args.args[0];
    PYGetCandWord(arg, *a);
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

    return NULL;
}

void ReloadConfigPY(void* arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;

    LoadPYConfig(&pystate->pyconfig);
}
