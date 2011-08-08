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

#include "fcitx/fcitx.h"
#include "fcitx-utils/utils.h"

#include "table.h"

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <libintl.h>

#include "im/pinyin/pydef.h"
#include "fcitx/keys.h"
#include "fcitx/ui.h"
#include "fcitx-utils/utarray.h"
#include "fcitx-config/xdg.h"
#include "fcitx/profile.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "module/punc/punc.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "tablepinyinwrapper.h"

static void FreeTableConfig(void *v);
const UT_icd table_icd = {sizeof(TableMetaData), NULL ,NULL, FreeTableConfig};

static void *TableCreate(FcitxInstance* instance);

FCITX_EXPORT_API
FcitxIMClass ime = {
    TableCreate,
    NULL
};

void *TableCreate(FcitxInstance* instance)
{
    FcitxTableState *tbl = fcitx_malloc0(sizeof(FcitxTableState));
    tbl->owner = instance;
    LoadTableInfo(tbl);
    TableMetaData* table;
    for(table = (TableMetaData*) utarray_front(tbl->table);
        table != NULL;
        table = (TableMetaData*) utarray_next(tbl->table, table))
    {
        FcitxLog(INFO, table->strName);
        FcitxRegisterIM(
            instance,
            tbl,
            table->strName,
            table->strIconName,
            TableInit,
            TableResetStatus,
            DoTableInput,
            TableGetCandWords,
            TableGetCandWord,
            TablePhraseTips,
            SaveTableIM,
            NULL,
            table,
            table->iPriority
        );
    }
    
    
    return tbl;
}

void SaveTableIM (void *arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    if (!table->tableDict)
        return;
    if (table->tableDict && table->tableDict->iTableChanged)
        SaveTableDict(table);
}


/*
 * 读取码表输入法的名称和文件路径
 */
void LoadTableInfo (FcitxTableState *tbl)
{
    char **tablePath;
    size_t len;
    char pathBuf[PATH_MAX];
    int i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;

    StringHashSet* sset = NULL;
    tbl->bTablePhraseTips = false;
    tbl->iCurrentTableLoaded = -1;

    if (tbl->table)
    {
        utarray_free(tbl->table);
        tbl->table = NULL;
    }

    tbl->hkTableDelPhrase[0].sym = Key_7; tbl->hkTableDelPhrase[0].state = KEY_CTRL_COMP;
    tbl->hkTableAdjustOrder[0].sym = Key_6; tbl->hkTableAdjustOrder[0].state = KEY_CTRL_COMP;
    tbl->hkTableAddPhrase[0].sym = Key_8; tbl->hkTableAddPhrase[0].state = KEY_CTRL_COMP;

    tbl->table = malloc(sizeof(UT_array));
    tbl->iTableCount = 0;
    utarray_init(tbl->table, &table_icd);

    tablePath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/table" , DATADIR, PACKAGE "/table" );

    for (i = 0; i< len; i++)
    {
        snprintf(pathBuf, sizeof(pathBuf), "%s", tablePath[i]);
        pathBuf[sizeof(pathBuf) - 1] = '\0';

        dir = opendir(pathBuf);
        if (dir == NULL)
            continue;

        /* collect all *.conf files */
        while ((drt = readdir(dir)) != NULL)
        {
            size_t nameLen = strlen(drt->d_name);
            if (nameLen <= strlen(".conf") )
                continue;
            memset(pathBuf,0,sizeof(pathBuf));

            if (strcmp(drt->d_name + nameLen -strlen(".conf"), ".conf") != 0)
                continue;
            snprintf(pathBuf, sizeof(pathBuf), "%s/%s", tablePath[i], drt->d_name );

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

    char **paths = malloc(sizeof(char*) *len);
    for (i = 0;i < len ;i ++)
        paths[i] = malloc(sizeof(char) *PATH_MAX);
    StringHashSet* string;
    for (string = sset;
            string != NULL;
            string = (StringHashSet*)string->hh.next)
    {
        int i = 0;
        for (i = len -1; i >= 0; i--)
        {
            snprintf(paths[i], PATH_MAX, "%s/%s", tablePath[len - i - 1], string->name);
            FcitxLog(DEBUG, "Load Table Config File:%s", paths[i]);
        }
        FcitxLog(INFO, _("Load Table Config File:%s"), string->name);
        ConfigFile* cfile = ParseMultiConfigFile(paths, len, GetTableConfigDesc());
        if (cfile)
        {
            TableMetaData table;
            memset(&table, 0, sizeof(TableMetaData));
            utarray_push_back(tbl->table, &table);
            TableMetaData *t = (TableMetaData*)utarray_back(tbl->table);
            TableMetaDataConfigBind(t, cfile, GetTableConfigDesc());
            ConfigBindSync((GenericConfig*)t);
            FcitxLog(DEBUG, _("Table Config %s is %s"),string->name, (t->bEnabled)?"Enabled":"Disabled");
            if (t->bEnabled)
                tbl->iTableCount ++;
            else
            {
                utarray_pop_back(tbl->table);
            }
        }
    }

    for (i = 0;i < len ;i ++)
        free(paths[i]);
    free(paths);


    FreeXDGPath(tablePath);

    StringHashSet *curStr;
    while (sset)
    {
        curStr = sset;
        HASH_DEL(sset, curStr);
        free(curStr->name);
        free(curStr);
    }
}

CONFIG_DESC_DEFINE(GetTableConfigDesc, "table.desc")

boolean TableInit (void *arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    FcitxAddon* pyaddon = GetAddonByName(&tbl->owner->addons, "fcitx-pinyin");
    tbl->pyaddon = pyaddon;
    if (pyaddon == NULL)
        return false;
    tbl->PYBaseOrder = AD_FREQ;
    
    Table_ResetPY(tbl->pyaddon);
    return true;
}

void TableResetStatus (void* arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    FcitxInputState *input = &tbl->owner->input;
    tbl->bIsTableAddPhrase = false;
    tbl->bIsTableDelPhrase = false;
    tbl->bIsTableAdjustOrder = false;
    tbl->iTableTotalCandCount = 0;
    input->bIsDoInputOnly = false;
    //bSingleHZMode = false;
}

INPUT_RETURN_VALUE DoTableInput (void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    INPUT_RETURN_VALUE retVal;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    FcitxIM* currentIM = GetCurrentIM(instance);
    TableMetaData* table = (TableMetaData*) currentIM->priv;
    tbl->iTableIMIndex = utarray_eltidx(tbl->table, currentIM->priv);
    if (tbl->iTableIMIndex != tbl->iCurrentTableLoaded)
    {
        TableMetaData* previousTable = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iCurrentTableLoaded);
        if (previousTable)
            FreeTableDict(previousTable);
        tbl->iCurrentTableLoaded = -1;
    }

    if (tbl->iCurrentTableLoaded == -1)
    {
        LoadTableDict (table);
        tbl->iCurrentTableLoaded = tbl->iTableIMIndex;
    }
    
    if (IsHotKeyModifierCombine(sym, state))
        return IRV_TO_PROCESS;

    if (tbl->bTablePhraseTips) {
        if (IsHotKey(sym, state, FCITX_CTRL_DELETE)) {
            tbl->bTablePhraseTips = false;
            TableDelPhraseByHZ (table->tableDict, GetMessageString(input->msgAuxUp, 1));
            return IRV_CLEAN;
        }
        else if (state == KEY_NONE && (sym != Key_Control_L && sym != Key_Control_R && sym != Key_Shift_L && sym != Key_Shift_R )) {
            CleanInputWindow(instance);
            tbl->bTablePhraseTips = false;
            CloseInputWindow(instance);
        }
    }

    retVal = IRV_DO_NOTHING;
    if (state == KEY_NONE && (IsInputKey (table->tableDict, sym) || IsEndKey (table, sym) || sym == table->cMatchingKey || sym == table->cPinyin)) {
        input->bIsInRemind = false;

        /* it's not in special state */
        if (!tbl->bIsTableAddPhrase && !tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder) {
            /* check we use Pinyin or Not */
            if (input->strCodeInput[0] == table->cPinyin && table->bUsePY) {
                if (input->iCodeInputCount != (MAX_PY_LENGTH * 5 + 1)) {
                    input->strCodeInput[input->iCodeInputCount++] = (char) sym;
                    input->strCodeInput[input->iCodeInputCount] = '\0';
                    retVal = IRV_DISPLAY_FIRST_PAGE;
                }
                else
                    retVal = IRV_DO_NOTHING;
            }
            else {
                /* length is not too large */
                if ((input->iCodeInputCount < table->tableDict->iCodeLength) || (table->tableDict->bHasPinyin && input->iCodeInputCount < table->tableDict->iPYCodeLength)) {
                    input->strCodeInput[input->iCodeInputCount++] = (char) sym;
                    input->strCodeInput[input->iCodeInputCount] = '\0';

                    if (input->iCodeInputCount == 1 && input->strCodeInput[0] == table->cPinyin && table->bUsePY) {
                        input->iCandWordCount = 0;
                        retVal = IRV_DISPLAY_LAST;
                    }
                    else {
                        char        *strTemp;
                        char        *strLastFirstCand;
                        CANDTYPE     lastFirstCandType;
                        RECORD      *pLastCandRecord = (RECORD *)NULL;

                        strLastFirstCand = (char *)NULL;
                        lastFirstCandType = CT_AUTOPHRASE;
                        if ( input->iCandWordCount ) {         // to realize auto-sending HZ to client
                            strLastFirstCand = _TableGetCandWord (tbl, 0,false);
                            lastFirstCandType = tableCandWord[0].flag;
                            pLastCandRecord = tbl->pCurCandRecord;
                        }

                        retVal = IRV_DISPLAY_FIRST_PAGE;
                        FcitxModuleFunctionArg farg;
                        int key = input->strCodeInput[0];
                        farg.args[0] = &key;
                        strTemp = InvokeFunction(instance, FCITX_PUNC, GETPUNC, farg);
                        if (IsEndKey (table, sym)) {
                            if (input->iCodeInputCount == 1)
                                return IRV_TO_PROCESS;

                            if (!input->iCandWordCount) {
                                input->iCodeInputCount = 0;
                                return IRV_CLEAN;
                            }

                            if (input->iCandWordCount == 1) {
                                strcpy (GetOutputString(input), TableGetCandWord (tbl, 0));
                                return IRV_GET_CANDWORDS;
                            }

                            return IRV_DISPLAY_CANDWORDS;
                        }
                        else if (table->iTableAutoSendToClientWhenNone && (input->iCodeInputCount == (table->iTableAutoSendToClientWhenNone + 1)) && !input->iCandWordCount) {
                            if ( strLastFirstCand && (lastFirstCandType!=CT_AUTOPHRASE)) {
                                CommitString(instance, GetCurrentIC(instance), strLastFirstCand);
                            }
                            retVal = IRV_DISPLAY_FIRST_PAGE;
                            input->iCodeInputCount = 1;
                            input->strCodeInput[0] = sym;
                            input->strCodeInput[1] = '\0';
                        }
                        else if (table->iTableAutoSendToClient && (input->iCodeInputCount >= table->iTableAutoSendToClient)) {
                            if (input->iCandWordCount == 1 && (tableCandWord[0].flag != CT_AUTOPHRASE || (tableCandWord[0].flag == CT_AUTOPHRASE && !table->iSaveAutoPhraseAfter))) {  //如果只有一个候选词，则送到客户程序中
                                retVal = IRV_DO_NOTHING;
                                if (tableCandWord[0].flag == CT_NORMAL) {
                                    if (tableCandWord[0].candWord.record->bPinyin)
                                        retVal = IRV_DISPLAY_CANDWORDS;
                                }

                                if (retVal != IRV_DISPLAY_CANDWORDS) {
                                    strcpy (GetOutputString(input), TableGetCandWord (tbl, 0));
                                    input->iCandWordCount = 0;
                                    if (input->bIsInRemind)
                                        retVal = IRV_GET_REMIND;
                                    else
                                        retVal = IRV_GET_CANDWORDS;
                                }
                            }
                        }
                        else if ((input->iCodeInputCount == 1) && strTemp && !input->iCandWordCount) {    //如果第一个字母是标点，并且没有候选字/词，则当做标点处理──适用于二笔这样的输入法
                            strcpy (GetOutputString(input), strTemp);
                            retVal = IRV_PUNC;
                        }
                    }
                }
                else {
                    if (table->iTableAutoSendToClient) {
                        if (input->iCandWordCount) {
                            if (tableCandWord[0].flag != CT_AUTOPHRASE) {
                                CommitString(instance, GetCurrentIC(instance), TableGetCandWord (tbl, 0));
                            }
                        }

                        input->iCodeInputCount = 1;
                        input->strCodeInput[0] = sym;
                        input->strCodeInput[1] = '\0';
                        input->bIsInRemind = false;
                        
                        retVal = IRV_DISPLAY_FIRST_PAGE;
                    }
                    else
                        retVal = IRV_DO_NOTHING;
                }
            }
        }
    }
    else {
        if (tbl->bIsTableAddPhrase) {
            if (IsHotKey(sym, state, FCITX_LEFT))
            {
                if (tbl->iTableNewPhraseHZCount < table->tableDict->iHZLastInputCount && tbl->iTableNewPhraseHZCount < PHRASE_MAX_LENGTH) {
                    tbl->iTableNewPhraseHZCount++;
                    TableCreateNewPhrase (tbl);
                }
            }
            else if (IsHotKey(sym, state, FCITX_RIGHT))
            {
                if (tbl->iTableNewPhraseHZCount > 2) {
                    tbl->iTableNewPhraseHZCount--;
                    TableCreateNewPhrase (tbl);
                }
            }
            else if (IsHotKey(sym, state, FCITX_ENTER))
            {
                if (strcmp("????", GetMessageString(input->msgAuxDown, 0)) != 0)
                    TableInsertPhrase (table->tableDict, GetMessageString(input->msgAuxDown, 1), GetMessageString(input->msgAuxDown, 0));
                tbl->bIsTableAddPhrase = false;
                input->bIsDoInputOnly = false;
                return IRV_CLEAN;
            }
            else if (IsHotKey(sym, state, FCITX_ESCAPE))
            {
                tbl->bIsTableAddPhrase = false;
                input->bIsDoInputOnly = false;
                return IRV_CLEAN;
            }
            else {
                return IRV_DO_NOTHING;
            }
            return IRV_DISPLAY_MESSAGE;
        }
        if (IsHotKey (sym, state, tbl->hkTableAddPhrase)) {
            if (!tbl->bIsTableAddPhrase) {
                if (table->tableDict->iHZLastInputCount < 2 || !table->tableDict->bRule) //词组最少为两个汉字
                    return IRV_DO_NOTHING;

                tbl->bTablePhraseTips = false;
                tbl->iTableNewPhraseHZCount = 2;
                tbl->bIsTableAddPhrase = true;
                input->bIsDoInputOnly = true;
                instance->bShowCursor = false;

                CleanInputWindow(instance);
                AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Left/Right to increase selected character, Press Enter to confirm, Press Escape to Cancel"));

                AddMessageAtLast(input->msgAuxDown, MSG_FIRSTCAND, "");
                AddMessageAtLast(input->msgAuxDown, MSG_CODE, "");

                TableCreateNewPhrase (tbl);
                retVal = IRV_DISPLAY_MESSAGE;
            }
            else
                retVal = IRV_TO_PROCESS;

            return retVal;
        }
        else if (IsHotKey (sym, state, FCITX_CTRL_ALT_E)) {
            char            strPY[100];

            //如果拼音单字字库没有读入，则读入它
            Table_LoadPYBaseDict (tbl->pyaddon);

            //如果刚刚输入的是个词组，刚不查拼音
            if (utf8_strlen (GetOutputString(input)) != 1)
                return IRV_DO_NOTHING;

            input->iCodeInputCount = 0;
            
            CleanInputWindow(instance);
            AddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%s", GetOutputString(input));

            AddMessageAtLast(input->msgAuxDown, MSG_CODE, _("Pinyin: "));
            Table_PYGetPYByHZ (tbl->pyaddon, GetOutputString(input), strPY);
            AddMessageAtLast(input->msgAuxDown, MSG_TIPS, (strPY[0]) ? strPY : _("Cannot found Pinyin"));
            instance->bShowCursor = false;

            return IRV_DISPLAY_MESSAGE;
        }

        if (!input->iCodeInputCount && !input->bIsInRemind)
            return IRV_TO_PROCESS;

        if (IsHotKey(sym, state, FCITX_ESCAPE)) {
            if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder) {
                TableResetStatus (tbl);
                CleanInputWindowUp(instance);
                AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
                retVal = IRV_DISPLAY_CANDWORDS;
            }
            else
                return IRV_CLEAN;
        }
        else if (state == KEY_NONE && IsChooseKey(table, sym)) {
            int iKey;
            iKey = IsChooseKey(table, sym);

            if (!input->bIsInRemind) {
                if (!input->iCandWordCount)
                    return IRV_TO_PROCESS;

                if (iKey > input->iCandWordCount)
                    return IRV_DO_NOTHING;
                else {
                    if (tbl->bIsTableDelPhrase) {
                        TableDelPhraseByIndex (tbl, iKey);
                        tbl->bIsTableDelPhrase = false;
                        retVal = IRV_DISPLAY_FIRST_PAGE;
                    }
                    else if (tbl->bIsTableAdjustOrder) {
                        TableAdjustOrderByIndex (tbl, iKey);
                        tbl->bIsTableDelPhrase = false;
                        retVal = IRV_DISPLAY_FIRST_PAGE;
                    }
                    else {
                        //如果是拼音，进入快速调整字频方式
                        if (strcmp (input->strCodeInput, table->strSymbol) && input->strCodeInput[0] == table->cPinyin && table->bUsePY)
                            Table_PYGetCandWord (tbl->pyaddon, iKey - 1);
                        strcpy (GetOutputString(input), TableGetCandWord (tbl, iKey - 1));

                        if (input->bIsInRemind)
                            retVal = IRV_GET_REMIND;
                        else
                            retVal = IRV_GET_CANDWORDS;
                    }
                }
            }
            else {
                strcpy (GetOutputString(input), TableGetRemindCandWord (tbl, iKey - 1));
                retVal = IRV_GET_REMIND;
            }
        }
        else if (!tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder) {
            if (IsHotKey (sym, state, tbl->hkTableAdjustOrder)) {
                if ((tbl->iTableCandDisplayed == input->iCandWordCount && input->iCandWordCount < 2) || input->bIsInRemind)
                    return IRV_DO_NOTHING;

                tbl->bIsTableAdjustOrder = true;
                CleanInputWindowUp(instance);
                AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Choose the phrase to be put in the front, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            }
            else if (IsHotKey (sym, state, tbl->hkTableDelPhrase)) {
                if (!input->iCandWordCount || input->bIsInRemind)
                    return IRV_DO_NOTHING;

                tbl->bIsTableDelPhrase = true;
                CleanInputWindowUp(instance);
                AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Choose the phrase to be deleted, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            }
            else if (IsHotKey(sym, state, FCITX_BACKSPACE)) {
                if (!input->iCodeInputCount) {
                    input->bIsInRemind = false;
                    return IRV_DONOT_PROCESS_CLEAN;
                }

                input->iCodeInputCount--;
                input->strCodeInput[input->iCodeInputCount] = '\0';

                if (input->iCodeInputCount == 1 && input->strCodeInput[0] == table->cPinyin && table->bUsePY) {
                    input->iCandWordCount = 0;
                    retVal = IRV_DISPLAY_LAST;
                }
                else if (input->iCodeInputCount)
                    retVal = IRV_DISPLAY_FIRST_PAGE;
                else
                    retVal = IRV_CLEAN;
            }
            else if (IsHotKey(sym, state, FCITX_SPACE)) {
                if (!input->bIsInRemind) {
                    if (!(table->bUsePY && input->iCodeInputCount == 1 && input->strCodeInput[0] == table->cPinyin)) {
                        if (strcmp (input->strCodeInput, table->strSymbol) && input->strCodeInput[0] == table->cPinyin && table->bUsePY)
                            Table_PYGetCandWord (tbl->pyaddon, SM_FIRST);

                        if (!input->iCandWordCount) {
                            input->iCodeInputCount = 0;
                            return IRV_CLEAN;
                        }

                        strcpy (GetOutputString(input), TableGetCandWord (tbl, 0));
                    }
                    else
                    {
                        CleanInputWindowDown(instance);
                    }

                    if (input->bIsInRemind)
                        retVal = IRV_GET_REMIND;
                    else
                        retVal = IRV_GET_CANDWORDS;

                }
                else {
                    strcpy (GetOutputString(input), TableGetRemindCandWord (tbl, 0));
                    retVal = IRV_GET_REMIND;
                }
            }
            else
            {
                /* friendly to cursor move, don't left cursor move while input text */
                if (input->iCodeInputCount != 0 &&
                    (IsHotKey(sym, state, FCITX_LEFT) ||
                    IsHotKey(sym, state, FCITX_RIGHT) ||
                    IsHotKey(sym, state, FCITX_HOME) ||
                    IsHotKey(sym, state, FCITX_END)))
                {
                    return IRV_DO_NOTHING;
                }
                else
                    return IRV_TO_PROCESS;
            }
        }
    }

    if (!input->bIsInRemind) {
        if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder)
            input->bIsDoInputOnly = true;
    }

    if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder || input->bIsInRemind)
        instance->bShowCursor = false;
    else
    {
        instance->bShowCursor = true;
        input->iCursorPos = strlen(input->strCodeInput);
    }

    return retVal;
}

char *TableGetCandWord (void* arg, int iIndex)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    char *str;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    str=_TableGetCandWord(tbl, iIndex, true);
    if (str) {
        if (table->bAutoPhrase && (utf8_strlen (str) == 1 || (utf8_strlen (str) > 1 && table->bAutoPhrasePhrase)))
            UpdateHZLastInput (tbl, str);

        if ( tbl->pCurCandRecord )
            TableUpdateHitFrequency (table->tableDict, tbl->pCurCandRecord);
    }

    return str;
}

/*
 * 第二个参数表示是否进入联想模式，实现自动上屏功能时，不能使用联想模式
 */
char           *_TableGetCandWord (FcitxTableState* tbl, int iIndex, boolean _bRemind)
{
    char           *pCandWord = NULL;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    FcitxProfile *profile = &instance->profile;

    if (!strcmp (input->strCodeInput, table->strSymbol))
        return TableGetFHCandWord (tbl, iIndex);

    input->bIsInRemind = false;

    if (!input->iCandWordCount)
        return NULL;

    if (iIndex > (input->iCandWordCount - 1))
        iIndex = input->iCandWordCount - 1;

    if (tableCandWord[iIndex].flag == CT_NORMAL)
        tbl->pCurCandRecord = tableCandWord[iIndex].candWord.record;
    else
        tbl->pCurCandRecord = (RECORD *)NULL;

    table->tableDict->iTableChanged++;
    if (table->tableDict->iTableChanged >= TABLE_AUTO_SAVE_AFTER)
        SaveTableDict (table);

    switch (tableCandWord[iIndex].flag) {
    case CT_NORMAL:
        pCandWord = tableCandWord[iIndex].candWord.record->strHZ;
        break;
    case CT_AUTOPHRASE:
        if (table->iSaveAutoPhraseAfter) {
            /* 当_bRemind为false时，不应该计算自动组词的频度，因此此时实际并没有选择这个词 */
            if (table->iSaveAutoPhraseAfter >= tableCandWord[iIndex].candWord.autoPhrase->iSelected && _bRemind)
                tableCandWord[iIndex].candWord.autoPhrase->iSelected++;
            if (table->iSaveAutoPhraseAfter == tableCandWord[iIndex].candWord.autoPhrase->iSelected)    //保存自动词组
                TableInsertPhrase (table->tableDict, tableCandWord[iIndex].candWord.autoPhrase->strCode, tableCandWord[iIndex].candWord.autoPhrase->strHZ);
        }
        pCandWord = tableCandWord[iIndex].candWord.autoPhrase->strHZ;
        break;
    case CT_PYPHRASE:
        pCandWord = tableCandWord[iIndex].candWord.strPYPhrase;
        break;
    default:
        ;
    }

    if (UseRemind(profile) && _bRemind) {
        strcpy (tbl->strTableRemindSource, pCandWord);
        TableGetRemindCandWords (tbl, SM_FIRST);
    }
    else {
        if (table->bPromptTableCode) {
            RECORD         *temp;

            CleanInputWindow(instance);
            AddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%s", input->strCodeInput);

            AddMessageAtLast(input->msgAuxDown, MSG_TIPS, "%s", pCandWord);
            temp = table->tableDict->tableSingleHZ[CalHZIndex (pCandWord)];
            if (temp) {
                AddMessageAtLast(input->msgAuxDown, MSG_CODE, "%s", temp->strCode);
            }
        }
        else {
            CleanInputWindow(instance);
        }
    }

    if (utf8_strlen (pCandWord) == 1)
        input->lastIsSingleHZ = 1;
    else
        input->lastIsSingleHZ = 0;

    return pCandWord;
}

INPUT_RETURN_VALUE TableGetPinyinCandWords (FcitxTableState* tbl, SEARCH_MODE mode)
{
    int             i;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (mode == SM_FIRST) {
        strcpy (Table_PYGetFindString(tbl->pyaddon), input->strCodeInput + 1);

        Table_DoPYInput (tbl->pyaddon, 0,0);
        Table_PYGetCandWords (tbl->pyaddon, mode);

        input->strCodeInput[0] = table->cPinyin;
        input->strCodeInput[1] = '\0';

        strcat (input->strCodeInput, Table_PYGetFindString(tbl->pyaddon));
        input->iCodeInputCount = strlen (input->strCodeInput);
    }
    else
        Table_PYGetCandWords (tbl->pyaddon, mode);

    //下面将拼音的候选字表转换为码表输入法的样式
    for (i = 0; i < input->iCandWordCount; i++) {
        tableCandWord[i].flag = CT_PYPHRASE;
        Table_PYGetCandText (tbl->pyaddon, i, tableCandWord[i].candWord.strPYPhrase);
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetCandWords (void* arg, SEARCH_MODE mode)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    int             i;
    RECORD         *recTemp;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (input->strCodeInput[0] == '\0')
        return IRV_TO_PROCESS;

    if (input->bIsInRemind)
        return TableGetRemindCandWords (tbl, mode);
    
    if (!strcmp (input->strCodeInput, table->strSymbol))
        return TableGetFHCandWords (tbl, mode);

    if (input->strCodeInput[0] == table->cPinyin && table->bUsePY)
        TableGetPinyinCandWords (tbl, mode);
    else {
        if (mode == SM_FIRST) {
            input->iCandWordCount = 0;
            tbl->iTableCandDisplayed = 0;
            tbl->iTableTotalCandCount = 0;
            input->iCurrentCandPage = 0;
            input->iCandPageCount = 0;
            TableResetFlags (table->tableDict);
            if (TableFindFirstMatchCode (table, input->strCodeInput) == -1 && !table->tableDict->iAutoPhrase) {
                if (input->iCodeInputCount) {
                    SetMessageCount(input->msgPreedit, 0);
                    AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
                    input->iCursorPos = strlen(input->strCodeInput);
                }
                return IRV_DISPLAY_CANDWORDS;   //Not Found
            }
        }
        else {
            if (!tbl->iTableTotalCandCount)
                return IRV_TO_PROCESS;

            if (mode == SM_NEXT) {
                if (tbl->iTableCandDisplayed >= tbl->iTableTotalCandCount)
                    return IRV_DO_NOTHING;
            }
            else {
                if (tbl->iTableCandDisplayed == input->iCandWordCount)
                    return IRV_DO_NOTHING;

                tbl->iTableCandDisplayed -= input->iCandWordCount;
                TableSetCandWordsFlag (tbl, input->iCandWordCount, false);
            }

            TableFindFirstMatchCode (table, input->strCodeInput);
        }

        input->iCandWordCount = 0;
        if (mode == SM_PREV && table->tableDict->bRule && table->bAutoPhrase && input->iCodeInputCount == table->tableDict->iCodeLength) {
            for (i = table->tableDict->iAutoPhrase - 1; i >= 0; i--) {
                if (!TableCompareCode (table, input->strCodeInput, table->tableDict->autoPhrase[i].strCode) && table->tableDict->autoPhrase[i].flag) {
                    if (TableHasPhrase (table->tableDict, table->tableDict->autoPhrase[i].strCode, table->tableDict->autoPhrase[i].strHZ))
                        TableAddAutoCandWord (tbl, i, mode);
                }
            }
        }

        if (input->iCandWordCount < ConfigGetMaxCandWord(&tbl->owner->config)) {
            while (table->tableDict->currentRecord && table->tableDict->currentRecord != table->tableDict->recordHead) {
                if ((mode == SM_PREV) ^ (!table->tableDict->currentRecord->flag)) {
                    if (!TableCompareCode (table, input->strCodeInput, table->tableDict->currentRecord->strCode)) {
                        if (mode == SM_FIRST)
                            tbl->iTableTotalCandCount++;
                        TableAddCandWord (tbl, table->tableDict->currentRecord, mode);
                    }
                }

                table->tableDict->currentRecord = table->tableDict->currentRecord->next;
            }
        }

        if (table->tableDict->bRule && table->bAutoPhrase && mode != SM_PREV && input->iCodeInputCount == table->tableDict->iCodeLength) {
            for (i = table->tableDict->iAutoPhrase - 1; i >= 0; i--) {
                if (!TableCompareCode (table, input->strCodeInput, table->tableDict->autoPhrase[i].strCode) && !table->tableDict->autoPhrase[i].flag) {
                    if (TableHasPhrase (table->tableDict, table->tableDict->autoPhrase[i].strCode, table->tableDict->autoPhrase[i].strHZ)) {
                        if (mode == SM_FIRST)
                            tbl->iTableTotalCandCount++;
                        TableAddAutoCandWord (tbl, i, mode);
                    }
                }
            }
        }

        TableSetCandWordsFlag (tbl, input->iCandWordCount, true);

        if (mode != SM_PREV)
            tbl->iTableCandDisplayed += input->iCandWordCount;

        //由于input->iCurrentCandPage和input->iCandPageCount用来指示是否显示上/下翻页图标，因此，此处需要设置一下
        input->iCurrentCandPage = (tbl->iTableCandDisplayed == input->iCandWordCount) ? 0 : 1;
        input->iCandPageCount = (tbl->iTableCandDisplayed >= tbl->iTableTotalCandCount) ? 1 : 2;
        if (input->iCandWordCount == tbl->iTableTotalCandCount)
            input->iCandPageCount = 0;
        /* **************************************************** */
    }

    if (input->iCodeInputCount) {
        SetMessageCount(input->msgPreedit, 0);
        AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
        input->iCursorPos = strlen(input->strCodeInput);
    }
    
    for (i = 0; i < input->iCandWordCount; i++) {
        char selectKey;
        selectKey = table->strChoose[i];

        MSG_TYPE mType;
        char* pMsg = NULL;
        if (tableCandWord[i].flag == CT_AUTOPHRASE)
            mType = MSG_TIPS;
        else
            mType = ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER);

        switch (tableCandWord[i].flag) {
        case CT_NORMAL:
            pMsg = tableCandWord[i].candWord.record->strHZ;
            break;
        case CT_AUTOPHRASE:
            pMsg = tableCandWord[i].candWord.autoPhrase->strHZ;
            break;
        case CT_PYPHRASE:
            pMsg = tableCandWord[i].candWord.strPYPhrase;
            break;
        default:
            ;
        }
        const char* pstr = NULL;

        if (tableCandWord[i].flag == CT_PYPHRASE) {
            if (utf8_strlen (tableCandWord[i].candWord.strPYPhrase) == 1) {
                recTemp = table->tableDict->tableSingleHZ[CalHZIndex (tableCandWord[i].candWord.strPYPhrase)];

                if (!recTemp)
                    pstr = (char *) NULL;
                else
                    pstr = recTemp->strCode;
            }
            else
                pstr = (char *) NULL;
        }
        else if ((tableCandWord[i].flag == CT_NORMAL) && (tableCandWord[i].candWord.record->bPinyin)) {
            if (utf8_strlen (tableCandWord[i].candWord.record->strHZ) == 1) {
                recTemp = table->tableDict->tableSingleHZ[CalHZIndex (tableCandWord[i].candWord.record->strHZ)];
                if (!recTemp)
                    pstr = (char *) NULL;
                else
                    pstr = recTemp->strCode;
            }
            else
                pstr = (char *) NULL;
        }
        else if (HasMatchingKey (table, input->strCodeInput))
            pstr = (tableCandWord[i].flag == CT_NORMAL) ? tableCandWord[i].candWord.record->strCode : tableCandWord[i].candWord.autoPhrase->strCode;
        else
            pstr = ((tableCandWord[i].flag == CT_NORMAL) ? tableCandWord[i].candWord.record->strCode : tableCandWord[i].candWord.autoPhrase->strCode) + input->iCodeInputCount;

        if (!pstr)
            pstr = "";

        SetCandidateWord(instance, i ,selectKey, pMsg, pstr);
    }

    return IRV_DISPLAY_CANDWORDS;
}

void TableAddAutoCandWord (FcitxTableState* tbl, short which, SEARCH_MODE mode)
{
    int             i, j;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            i = input->iCandWordCount - 1;
            for (j = 0; j < input->iCandWordCount - 1; j++)
                tableCandWord[j].candWord.autoPhrase = tableCandWord[j + 1].candWord.autoPhrase;
        }
        else
            i = input->iCandWordCount++;
        tableCandWord[i].flag = CT_AUTOPHRASE;
        tableCandWord[i].candWord.autoPhrase = &table->tableDict->autoPhrase[which];
    }
    else {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
            return;

        tableCandWord[input->iCandWordCount].flag = CT_AUTOPHRASE;
        tableCandWord[input->iCandWordCount++].candWord.autoPhrase = &table->tableDict->autoPhrase[which];
    }
}

void TableAddCandWord (FcitxTableState* tbl, RECORD * record, SEARCH_MODE mode)
{
    int             i = 0, j;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    switch (table->tableOrder) {
    case AD_NO:
        if (mode == SM_PREV) {
            if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
                i = input->iCandWordCount - 1;
            else {
                for (i = 0; i < input->iCandWordCount; i++) {
                    if (tableCandWord[i].flag != CT_NORMAL)
                        break;
                }
            }
        }
        else {
            if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
                return;
            tableCandWord[input->iCandWordCount].flag = CT_NORMAL;
            tableCandWord[input->iCandWordCount++].candWord.record = record;
            return;
        }
        break;
    case AD_FREQ:
        if (mode == SM_PREV) {
            for (i = input->iCandWordCount - 1; i >= 0; i--) {
                if (tableCandWord[i].flag == CT_NORMAL) {
                    if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) < 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iHit >= record->iHit))
                        break;
                }
            }

            if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
                if (i < 0)
                    return;
            }
            else
                i++;
        }
        else {
            for (; i < input->iCandWordCount; i++) {
                if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) > 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iHit < record->iHit))
                    break;
            }
            if (i == ConfigGetMaxCandWord(&tbl->owner->config))
                return;
        }
        break;

    case AD_FAST:
        if (mode == SM_PREV) {
            for (i = input->iCandWordCount - 1; i >= 0; i--) {
                if (tableCandWord[i].flag == CT_NORMAL) {
                    if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) < 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iIndex >= record->iIndex))
                        break;
                }
            }

            if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
                if (i < 0)
                    return;
            }
            else
                i++;
        }
        else {
            for (i = 0; i < input->iCandWordCount; i++) {
                if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) > 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iIndex < record->iIndex))
                    break;
            }

            if (i == ConfigGetMaxCandWord(&tbl->owner->config))
                return;
        }
        break;
    }

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            for (j = 0; j < i; j++) {
                tableCandWord[j].flag = tableCandWord[j + 1].flag;
                if (tableCandWord[j].flag == CT_NORMAL)
                    tableCandWord[j].candWord.record = tableCandWord[j + 1].candWord.record;
                else
                    tableCandWord[j].candWord.autoPhrase = tableCandWord[j + 1].candWord.autoPhrase;
            }
        }
        else {
            for (j = input->iCandWordCount; j > i; j--) {
                tableCandWord[j].flag = tableCandWord[j - 1].flag;
                if (tableCandWord[j].flag == CT_NORMAL)
                    tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
                else
                    tableCandWord[j].candWord.autoPhrase = tableCandWord[j - 1].candWord.autoPhrase;
            }
        }
    }
    else {
        j = input->iCandWordCount;
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
            j--;
        for (; j > i; j--) {
            tableCandWord[j].flag = tableCandWord[j - 1].flag;
            if (tableCandWord[j].flag == CT_NORMAL)
                tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
            else
                tableCandWord[j].candWord.autoPhrase = tableCandWord[j - 1].candWord.autoPhrase;
        }
    }

    tableCandWord[i].flag = CT_NORMAL;
    tableCandWord[i].candWord.record = record;

    if (input->iCandWordCount != ConfigGetMaxCandWord(&tbl->owner->config))
        input->iCandWordCount++;
}

void TableSetCandWordsFlag (FcitxTableState* tbl, int iCount, boolean flag)
{
    int             i;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;

    for (i = 0; i < iCount; i++) {
        if (tableCandWord[i].flag == CT_NORMAL)
            tableCandWord[i].candWord.record->flag = flag;
        else
            tableCandWord[i].candWord.autoPhrase->flag = flag;
    }
}

/*
 * 根据序号调整词组顺序，序号从1开始
 * 将指定的字/词调整到同样编码的最前面
 */
void TableAdjustOrderByIndex (FcitxTableState* tbl, int iIndex)
{
    RECORD         *recTemp;
    int             iTemp;
    TABLECANDWORD*  tableCandWord = tbl->tableCandWord;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (tableCandWord[iIndex - 1].flag != CT_NORMAL)
        return;

    recTemp = tableCandWord[iIndex - 1].candWord.record;
    while (!strcmp (recTemp->strCode, recTemp->prev->strCode))
        recTemp = recTemp->prev;
    if (recTemp == tableCandWord[iIndex - 1].candWord.record)   //说明已经是第一个
        return;

    //将指定的字/词放到recTemp前
    tableCandWord[iIndex - 1].candWord.record->prev->next = tableCandWord[iIndex - 1].candWord.record->next;
    tableCandWord[iIndex - 1].candWord.record->next->prev = tableCandWord[iIndex - 1].candWord.record->prev;
    recTemp->prev->next = tableCandWord[iIndex - 1].candWord.record;
    tableCandWord[iIndex - 1].candWord.record->prev = recTemp->prev;
    recTemp->prev = tableCandWord[iIndex - 1].candWord.record;
    tableCandWord[iIndex - 1].candWord.record->next = recTemp;

    table->tableDict->iTableChanged++;

    //需要的话，更新索引
    if (tableCandWord[iIndex - 1].candWord.record->strCode[1] == '\0') {
        for (iTemp = 0; iTemp < strlen (table->tableDict->strInputCode); iTemp++) {
            if (tbl->recordIndex[iTemp].cCode == tableCandWord[iIndex - 1].candWord.record->strCode[0]) {
                tbl->recordIndex[iTemp].record = tableCandWord[iIndex - 1].candWord.record;
                break;
            }
        }
    }
}

/*
 * 根据序号删除词组，序号从1开始
 */
void TableDelPhraseByIndex (FcitxTableState* tbl, int iIndex)
{
    TABLECANDWORD *tableCandWord = tbl->tableCandWord;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (tableCandWord[iIndex - 1].flag != CT_NORMAL)
        return;

    if (utf8_strlen (tableCandWord[iIndex - 1].candWord.record->strHZ) <= 1)
        return;

    TableDelPhrase (table->tableDict, tableCandWord[iIndex - 1].candWord.record);
}

void TableCreateNewPhrase (FcitxTableState* tbl)
{
    int             i;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState* input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    SetMessageText(input->msgAuxDown, 0, "");
    for (i = tbl->iTableNewPhraseHZCount; i > 0; i--)
        MessageConcat(input->msgAuxDown , 0 ,table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - i].strHZ);

    boolean bCanntFindCode = TableCreatePhraseCode (table->tableDict, GetMessageString(input->msgAuxDown, 0));

    if (bCanntFindCode)
    {    
        SetMessageCount(input->msgAuxDown, 2);
        SetMessageText(input->msgAuxDown, 1, tbl->strNewPhraseCode);
    }
    else
    {
        SetMessageCount(input->msgAuxDown, 1);
        SetMessageText(input->msgAuxDown, 0, "????");
    }

}

/*
 * 获取联想候选字列表
 */
INPUT_RETURN_VALUE TableGetRemindCandWords (FcitxTableState* tbl, SEARCH_MODE mode)
{
    int             iLength;
    int             i;
    RECORD         *tableRemind = NULL;
    unsigned int    iTableTotalLengendCandCount = 0;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    GenericConfig *fc = &tbl->owner->config.gconfig;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    boolean bDisablePagingInRemind = *ConfigGetBindValue(fc, "Output", "RemindModeDisablePaging").boolvalue;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (!tbl->strTableRemindSource[0])
        return IRV_TO_PROCESS;

    iLength = utf8_strlen (tbl->strTableRemindSource);
    if (mode == SM_FIRST) {
        input->iCurrentCandPage = 0;
        input->iCandPageCount = 0;
        TableResetFlags(table->tableDict);
    }
    else {
        if (!input->iCandPageCount)
            return IRV_TO_PROCESS;

        if (mode == SM_NEXT) {
            if (input->iCurrentCandPage == input->iCandPageCount)
                return IRV_DO_NOTHING;

            input->iCurrentCandPage++;
        }
        else {
            if (!input->iCurrentCandPage)
                return IRV_DO_NOTHING;

            TableSetCandWordsFlag (tbl, input->iCandWordCount, false);
            input->iCurrentCandPage--;
        }
    }

    input->iCandWordCount = 0;
    tableRemind = table->tableDict->recordHead->next;

    while (tableRemind != table->tableDict->recordHead) {
        if (((mode == SM_PREV) ^ (!tableRemind->flag)) && ((iLength + 1) == utf8_strlen (tableRemind->strHZ))) {
            if (!utf8_strncmp (tableRemind->strHZ, tbl->strTableRemindSource, iLength) && utf8_get_nth_char(tableRemind->strHZ, iLength)) {
                if (mode == SM_FIRST)
                    iTableTotalLengendCandCount++;

                TableAddRemindCandWord (tbl, tableRemind, mode);
            }
        }

        tableRemind = tableRemind->next;
    }

    TableSetCandWordsFlag (tbl, input->iCandWordCount, true);

    if (mode == SM_FIRST && bDisablePagingInRemind)
        input->iCandPageCount = iTableTotalLengendCandCount / ConfigGetMaxCandWord(&tbl->owner->config) - ((iTableTotalLengendCandCount % ConfigGetMaxCandWord(&tbl->owner->config)) ? 0 : 1);

    CleanInputWindow(instance);
    AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Remind:"));
    AddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%s", tbl->strTableRemindSource);

    for (i = 0; i < input->iCandWordCount; i++) {
        char selectKey;
        if (i == 9)
            selectKey = '0';
        else
            selectKey = i + 1 + '0';
        SetCandidateWord(instance, i, selectKey, tableCandWord[i].candWord.record->strHZ + strlen (tbl->strTableRemindSource), "");
    }

    input->bIsInRemind = (input->iCandWordCount != 0);

    return IRV_DISPLAY_CANDWORDS;
}

void TableAddRemindCandWord (FcitxTableState* tbl, RECORD * record, SEARCH_MODE mode)
{
    int             i, j;
    TABLECANDWORD *tableCandWord = tbl->tableCandWord;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (mode == SM_PREV) {
        for (i = input->iCandWordCount - 1; i >= 0; i--) {
            if (tableCandWord[i].candWord.record->iHit >= record->iHit)
                break;
        }

        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            if (i < 0)
                return;
        }
        else
            i++;
    }
    else {
        for (i = 0; i < input->iCandWordCount; i++) {
            if (tableCandWord[i].candWord.record->iHit < record->iHit)
                break;
        }

        if (i == ConfigGetMaxCandWord(&tbl->owner->config))
            return;
    }

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            for (j = 0; j < i; j++)
                tableCandWord[j].candWord.record = tableCandWord[j + 1].candWord.record;
        }
        else {
            for (j = input->iCandWordCount; j > i; j--)
                tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
        }
    }
    else {
        j = input->iCandWordCount;
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
            j--;
        for (; j > i; j--)
            tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
    }

    tableCandWord[i].flag = CT_NORMAL;
    tableCandWord[i].candWord.record = record;

    if (input->iCandWordCount != ConfigGetMaxCandWord(&tbl->owner->config))
        input->iCandWordCount++;
}

char           *TableGetRemindCandWord (void* arg, int iIndex)
{
    FcitxTableState* tbl = (FcitxTableState*) arg;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TABLECANDWORD *tableCandWord = tbl->tableCandWord;
    if (input->iCandWordCount) {
        if (iIndex > (input->iCandWordCount - 1))
            iIndex = input->iCandWordCount - 1;

        tableCandWord[iIndex].candWord.record->iHit++;
        strcpy (tbl->strTableRemindSource, tableCandWord[iIndex].candWord.record->strHZ + strlen (tbl->strTableRemindSource));
        TableGetRemindCandWords (tbl, SM_FIRST);

        return tbl->strTableRemindSource;
    }
    return NULL;
}

INPUT_RETURN_VALUE TableGetFHCandWords (FcitxTableState* tbl, SEARCH_MODE mode)
{
    int             i;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (!table->tableDict->iFH)
        return IRV_DISPLAY_MESSAGE;

    if (mode == SM_FIRST) {
        input->iCandPageCount = table->tableDict->iFH / ConfigGetMaxCandWord(&tbl->owner->config) - ((table->tableDict->iFH % ConfigGetMaxCandWord(&tbl->owner->config)) ? 0 : 1);
        input->iCurrentCandPage = 0;
    }
    else {
        if (!input->iCandPageCount)
            return IRV_TO_PROCESS;

        if (mode == SM_NEXT) {
            if (input->iCurrentCandPage == input->iCandPageCount)
                return IRV_DO_NOTHING;

            input->iCurrentCandPage++;
        }
        else {
            if (!input->iCurrentCandPage)
                return IRV_DO_NOTHING;

            input->iCurrentCandPage--;
        }
    }

    for (i = 0; i < ConfigGetMaxCandWord(&tbl->owner->config); i++) {
        char selectKey;
        
        selectKey = i + 1 + '0';
        if (i == 9)
            selectKey = '0';
        
        SetCandidateWord(instance, i, selectKey, table->tableDict->fh[input->iCurrentCandPage * ConfigGetMaxCandWord(&tbl->owner->config) + i].strFH, "");
        if ((input->iCurrentCandPage * ConfigGetMaxCandWord(&tbl->owner->config) + i) >= (table->tableDict->iFH - 1)) {
            i++;
            break;
        }
    }

    input->iCandWordCount = i;
    return IRV_DISPLAY_CANDWORDS;
}

char           *TableGetFHCandWord (FcitxTableState* tbl, int iIndex)
{
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    CleanInputWindowDown(instance);

    if (input->iCandWordCount) {
        if (iIndex > (input->iCandWordCount - 1))
            iIndex = input->iCandWordCount - 1;

        return table->tableDict->fh[input->iCurrentCandPage * ConfigGetMaxCandWord(&tbl->owner->config) + iIndex].strFH;
    }
    return NULL;
}

boolean TablePhraseTips (void *arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    RECORD         *recTemp = NULL;
    char            strTemp[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1] = "\0", *ps;
    short           i, j;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (!table->tableDict->recordHead)
        return false;

    //如果最近输入了一个词组，这个工作就不需要了
    if (input->lastIsSingleHZ != 1)
        return false;

    j = (table->tableDict->iHZLastInputCount > PHRASE_MAX_LENGTH) ? table->tableDict->iHZLastInputCount - PHRASE_MAX_LENGTH : 0;
    for (i = j; i < table->tableDict->iHZLastInputCount; i++)
        strcat (strTemp, table->tableDict->hzLastInput[i].strHZ);
    //如果只有一个汉字，这个工作也不需要了
    if (utf8_strlen (strTemp) < 2)
        return false;

    //首先要判断是不是已经在词库中
    ps = strTemp;
    for (i = 0; i < (table->tableDict->iHZLastInputCount - j - 1); i++) {
        recTemp = TableFindPhrase (table->tableDict, ps);
        if (recTemp) {
            CleanInputWindow(instance);
            AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Phrase is already in Dict "));
            AddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%s", ps);

            AddMessageAtLast(input->msgAuxDown, MSG_FIRSTCAND, _("Code is "));
            AddMessageAtLast(input->msgAuxDown, MSG_CODE, "%s", recTemp->strCode);
            AddMessageAtLast(input->msgAuxDown, MSG_TIPS, _(" Ctrl+Delete To Delete"));
            tbl->bTablePhraseTips = true;
            instance->bShowCursor = false;

            return true;
        }
        ps = ps + utf8_char_len(ps);
    }

    return false;
}

void UpdateHZLastInput (FcitxTableState* tbl, char *str)
{
    int             i, j;
    char           *pstr;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    pstr = str;

    for (i = 0; i < utf8_strlen (str) ; i++) {
        if (table->tableDict->iHZLastInputCount < PHRASE_MAX_LENGTH)
            table->tableDict->iHZLastInputCount++;
        else {
            for (j = 0; j < (table->tableDict->iHZLastInputCount - 1); j++) {
                strncpy(table->tableDict->hzLastInput[j].strHZ, table->tableDict->hzLastInput[j + 1].strHZ, utf8_char_len(table->tableDict->hzLastInput[j + 1].strHZ));
            }
        }
        strncpy(table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - 1].strHZ, pstr, utf8_char_len(pstr));
        table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - 1].strHZ[utf8_char_len(pstr)] = '\0';
        pstr = pstr + utf8_char_len(pstr);
    }

    if (table->tableDict->bRule && table->bAutoPhrase)
        TableCreateAutoPhrase (table, (char) (utf8_strlen (str)));
}

void FreeTableConfig(void *v)
{
    TableMetaData *table = (TableMetaData*) v;
    if (!table)
        return;
    FreeConfigFile(table->config.configFile);
    free(table->strPath);
    free(table->strSymbolFile);
    free(table->strName);
    free(table->strIconName);
    free(table->strEndCode);
    free(table->strSymbol);
    free(table->strChoose);
}
