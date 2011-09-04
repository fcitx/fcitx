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
#include <fcitx/candidate.h>

static void FreeTableConfig(void *v);
const UT_icd table_icd = {sizeof(TableMetaData), NULL ,NULL, FreeTableConfig};
const UT_icd tableCand_icd = {sizeof(TABLECANDWORD*), NULL, NULL, NULL };
typedef struct _TableCandWordSortContext {
    ADJUSTORDER order;
} TableCandWordSortContext;

static void *TableCreate(FcitxInstance* instance);
static int TableCandCmp(const void* a, const void* b, void* arg);

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
    for (table = (TableMetaData*) utarray_front(tbl->table);
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

    tbl->hkTableDelPhrase[0].sym = Key_7;
    tbl->hkTableDelPhrase[0].state = KEY_CTRL_COMP;
    tbl->hkTableAdjustOrder[0].sym = Key_6;
    tbl->hkTableAdjustOrder[0].state = KEY_CTRL_COMP;
    tbl->hkTableAddPhrase[0].sym = Key_8;
    tbl->hkTableAddPhrase[0].state = KEY_CTRL_COMP;

    tbl->table = fcitx_malloc0(sizeof(UT_array));
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
                    string = fcitx_malloc0(sizeof(StringHashSet));
                    memset(string, 0, sizeof(StringHashSet));
                    string->name = bStr;
                    HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
                }
            }
        }

        closedir(dir);
    }

    char **paths = fcitx_malloc0(sizeof(char*) *len);
    for (i = 0;i < len ;i ++)
        paths[i] = fcitx_malloc0(sizeof(char) *PATH_MAX);
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
    input->bIsDoInputOnly = false;
    //bSingleHZMode = false;
}

INPUT_RETURN_VALUE DoTableInput (void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    INPUT_RETURN_VALUE retVal;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    FcitxIM* currentIM = GetCurrentIM(instance);
    TableMetaData* table = (TableMetaData*) currentIM->priv;

    CandidateWordSetChoose(input->candList, table->strChoose);
    CandidateWordSetPageSize(input->candList, ConfigGetMaxCandWord(instance->config));

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
        if (input->bIsInRemind)
            CandidateWordReset(input->candList);
        input->bIsInRemind = false;

        /* it's not in special state */
        if (!tbl->bIsTableAddPhrase && !tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder) {
            /* check we use Pinyin or Not */
            if (input->strCodeInput[0] == table->cPinyin && table->bUsePY) {
                if (input->iCodeInputCount != (MAX_PY_LENGTH * 5 + 1)) {
                    input->strCodeInput[input->iCodeInputCount++] = (char) sym;
                    input->strCodeInput[input->iCodeInputCount] = '\0';
                    retVal = IRV_DISPLAY_CANDWORDS;
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
                        retVal = IRV_DISPLAY_LAST;
                    }
                    else {
                        char        *strTemp;
                        char        *strLastFirstCand;
                        CANDTYPE     lastFirstCandType;
                        int          lastPageCount = CandidateWordPageCount(input->candList);

                        strLastFirstCand = (char *)NULL;
                        lastFirstCandType = CT_AUTOPHRASE;
                        if ( CandidateWordPageCount(input->candList) != 0 ) {
                            // to realize auto-sending HZ to client
                            CandidateWord *candWord = NULL;
                            candWord = CandidateWordGetCurrentWindow(input->candList);
                            if (candWord->owner == tbl)
                            {
                                TABLECANDWORD* tableCandWord = candWord->priv;
                                lastFirstCandType = tableCandWord->flag;
                                INPUT_RETURN_VALUE ret = _TableGetCandWord (tbl, tableCandWord, false);
                                if (ret & IRV_FLAG_PENDING_COMMIT_STRING)
                                    strLastFirstCand = GetOutputString(input);
                            }
                        }

                        retVal = IRV_DISPLAY_CANDWORDS;
                        FcitxModuleFunctionArg farg;
                        int key = input->strCodeInput[0];
                        farg.args[0] = &key;
                        strTemp = InvokeFunction(instance, FCITX_PUNC, GETPUNC, farg);
                        if (IsEndKey (table, sym)) {
                            if (input->iCodeInputCount == 1)
                                return IRV_TO_PROCESS;

                            if (CandidateWordPageCount(input->candList) == 0) {
                                input->iCodeInputCount = 0;
                                return IRV_CLEAN;
                            }

                            if (CandidateWordGetCurrentWindowSize(input->candList) == 1) {
                                retVal = CandidateWordChooseByIndex(input->candList, 0);
                                return retVal;
                            }

                            return IRV_DISPLAY_CANDWORDS;
                        }
                        else if (table->iTableAutoSendToClientWhenNone && (input->iCodeInputCount == (table->iTableAutoSendToClientWhenNone + 1)) && lastPageCount == 0) {
                            if ( strLastFirstCand && (lastFirstCandType!=CT_AUTOPHRASE)) {
                                CommitString(instance, GetCurrentIC(instance), strLastFirstCand);
                            }
                            retVal = IRV_DISPLAY_CANDWORDS;
                            input->iCodeInputCount = 1;
                            input->strCodeInput[0] = sym;
                            input->strCodeInput[1] = '\0';
                        }
                        else if ((input->iCodeInputCount == 1) && strTemp && lastPageCount == 0) {    //如果第一个字母是标点，并且没有候选字/词，则当做标点处理──适用于二笔这样的输入法
                            strcpy (GetOutputString(input), strTemp);
                            retVal = IRV_PUNC;
                        }
                    }
                }
                else {
                    if (table->iTableAutoSendToClient) {
                        retVal = IRV_DISPLAY_CANDWORDS;
                        if (CandidateWordPageCount(input->candList)) {
                            CandidateWord* candWord = CandidateWordGetCurrentWindow(input->candList);
                            if (candWord->owner == tbl)
                            {
                                TABLECANDWORD* tableCandWord = candWord->priv;
                                if (tableCandWord->flag != CT_AUTOPHRASE) {
                                    INPUT_RETURN_VALUE ret = TableGetCandWord (tbl, candWord);
                                    if (ret & IRV_FLAG_PENDING_COMMIT_STRING)
                                    {
                                        CommitString(instance, GetCurrentIC(instance), GetOutputString(input));
                                        input->iHZInputed += (int) (utf8_strlen(GetOutputString(input)));
                                    }
                                }
                            }
                        }

                        input->iCodeInputCount = 1;
                        input->strCodeInput[0] = sym;
                        input->strCodeInput[1] = '\0';
                        input->bIsInRemind = false;

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
                input->bShowCursor = false;

                CleanInputWindow(instance);
                AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Left/Right to choose selected character, Press Enter to confirm, Press Escape to Cancel"));

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
            input->bShowCursor = false;

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
        else if (CheckChooseKey(sym, state, table->strChoose) >= 0) {
            int iKey;
            iKey = CheckChooseKey(sym, state, table->strChoose);

            if (CandidateWordPageCount(input->candList) == 0)
                return IRV_TO_PROCESS;

            if (CandidateWordGetByIndex(input->candList, iKey) == NULL)
                return IRV_DO_NOTHING;
            else {
                CandidateWord* candWord = CandidateWordGetByIndex(input->candList, iKey);
                if (candWord->owner == tbl && tbl->bIsTableDelPhrase) {
                    TableDelPhraseByIndex (tbl, candWord->priv);
                    tbl->bIsTableDelPhrase = false;
                    retVal = IRV_DISPLAY_CANDWORDS;
                }
                else if (candWord->owner == tbl && tbl->bIsTableAdjustOrder) {
                    TableAdjustOrderByIndex (tbl, candWord->priv);
                    tbl->bIsTableDelPhrase = false;
                    retVal = IRV_DISPLAY_CANDWORDS;
                }
                else
                    return IRV_TO_PROCESS;
            }
        }
        else if (!tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder) {
            if (IsHotKey (sym, state, tbl->hkTableAdjustOrder)) {
                if (CandidateWordGetListSize(input->candList) < 2 || input->bIsInRemind)
                    return IRV_DO_NOTHING;

                tbl->bIsTableAdjustOrder = true;
                CleanInputWindowUp(instance);
                AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Choose the phrase to be put in the front, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            }
            else if (IsHotKey (sym, state, tbl->hkTableDelPhrase)) {
                if (!CandidateWordPageCount(input->candList) || input->bIsInRemind)
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
                    CandidateWordReset(input->candList);
                    retVal = IRV_DISPLAY_LAST;
                }
                else if (input->iCodeInputCount)
                    retVal = IRV_DISPLAY_CANDWORDS;
                else
                    retVal = IRV_CLEAN;
            }
            else if (IsHotKey(sym, state, FCITX_SPACE)) {
                if (input->iCodeInputCount == 1 && input->strCodeInput[0] == table->cPinyin && table->bUsePY)
                    retVal = IRV_COMMIT_STRING;
                else {
                    retVal = CandidateWordChooseByIndex(input->candList, 0);
                    if (retVal == IRV_TO_PROCESS)
                        retVal = IRV_DO_NOTHING;
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
        input->bShowCursor = false;
    else
    {
        input->bShowCursor = true;
        input->iCursorPos = strlen(input->strCodeInput);
    }

    return retVal;
}

INPUT_RETURN_VALUE TableGetCandWord (void* arg, CandidateWord* candWord)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInputState* input = &tbl->owner->input;
    TABLECANDWORD* tableCandWord = candWord->priv;

    INPUT_RETURN_VALUE retVal =_TableGetCandWord(tbl, tableCandWord, true);
    if (retVal & IRV_FLAG_PENDING_COMMIT_STRING) {
        if (table->bAutoPhrase && (utf8_strlen (GetOutputString(input)) == 1 || (utf8_strlen (GetOutputString(input)) > 1 && table->bAutoPhrasePhrase)))
            UpdateHZLastInput (tbl, GetOutputString(input));

        if ( tbl->pCurCandRecord )
            TableUpdateHitFrequency (table->tableDict, tbl->pCurCandRecord);
    }

    return retVal;
}

/*
 * 第二个参数表示是否进入联想模式，实现自动上屏功能时，不能使用联想模式
 */
INPUT_RETURN_VALUE _TableGetCandWord (FcitxTableState* tbl, TABLECANDWORD* tableCandWord, boolean _bRemind)
{
    char           *pCandWord = NULL;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    FcitxProfile *profile = instance->profile;

    if (!strcmp (input->strCodeInput, table->strSymbol))
        return TableGetFHCandWord (tbl, tableCandWord);

    input->bIsInRemind = false;

    if (tableCandWord->flag == CT_NORMAL)
        tbl->pCurCandRecord = tableCandWord->candWord.record;
    else
        tbl->pCurCandRecord = (RECORD *)NULL;

    table->tableDict->iTableChanged++;
    if (table->tableDict->iTableChanged >= TABLE_AUTO_SAVE_AFTER)
        SaveTableDict (table);

    switch (tableCandWord->flag) {
    case CT_NORMAL:
        pCandWord = tableCandWord->candWord.record->strHZ;
        break;
    case CT_AUTOPHRASE:
        if (table->iSaveAutoPhraseAfter) {
            /* 当_bRemind为false时，不应该计算自动组词的频度，因此此时实际并没有选择这个词 */
            if (table->iSaveAutoPhraseAfter >= tableCandWord->candWord.autoPhrase->iSelected && _bRemind)
                tableCandWord->candWord.autoPhrase->iSelected++;
            if (table->iSaveAutoPhraseAfter == tableCandWord->candWord.autoPhrase->iSelected)    //保存自动词组
                TableInsertPhrase (table->tableDict, tableCandWord->candWord.autoPhrase->strCode, tableCandWord->candWord.autoPhrase->strHZ);
        }
        pCandWord = tableCandWord->candWord.autoPhrase->strHZ;
        break;
    case CT_FH:
        pCandWord = table->tableDict->fh[tableCandWord->candWord.iFHIndex].strFH;
        break;
    case CT_REMIND:
    {
        strcpy (tbl->strTableRemindSource, tableCandWord->candWord.record->strHZ + strlen (tbl->strTableRemindSource));
        strcpy(GetOutputString(input), tbl->strTableRemindSource);
        INPUT_RETURN_VALUE retVal = TableGetRemindCandWords (tbl);
        if (retVal == IRV_DISPLAY_CANDWORDS)
            return IRV_COMMIT_STRING_REMIND;
        else
            return IRV_COMMIT_STRING;
    }
    }

    if (UseRemind(profile) && _bRemind) {
        strcpy (tbl->strTableRemindSource, pCandWord);
        strcpy(GetOutputString(input), pCandWord);
        INPUT_RETURN_VALUE retVal = TableGetRemindCandWords (tbl);
        if (retVal == IRV_DISPLAY_CANDWORDS)
            return IRV_COMMIT_STRING_REMIND;
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

    strcpy(GetOutputString(input), pCandWord);
    return IRV_COMMIT_STRING;
}

INPUT_RETURN_VALUE TableGetPinyinCandWords (FcitxTableState* tbl)
{
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    strcpy (Table_PYGetFindString(tbl->pyaddon), input->strCodeInput + 1);

    Table_DoPYInput (tbl->pyaddon, 0,0);
    Table_PYGetCandWords (tbl->pyaddon);

    input->strCodeInput[0] = table->cPinyin;
    input->strCodeInput[1] = '\0';

    strcat (input->strCodeInput, Table_PYGetFindString(tbl->pyaddon));
    input->iCodeInputCount = strlen (input->strCodeInput);

    CleanInputWindowUp(instance);
    AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
    input->iCursorPos = input->iCodeInputCount;


    //下面将拼音的候选字表转换为码表输入法的样式
    CandidateWord* candWord;
    for (candWord = CandidateWordGetFirst(input->candList);
            candWord != NULL;
            candWord = CandidateWordGetNext(input->candList, candWord))
    {
        char* pstr;
        if (utf8_strlen (candWord->strWord) == 1) {
            RECORD* recTemp = table->tableDict->tableSingleHZ[CalHZIndex (candWord->strWord)];
            if (!recTemp)
                pstr = (char *) NULL;
            else
                pstr = recTemp->strCode;

        }
        else
            pstr = (char *) NULL;

        if (pstr)
            candWord->strExtra = strdup(pstr);
        tbl->pygetcandword = candWord->callback;
        candWord->callback = Table_PYGetCandWord;
        candWord->owner = tbl;
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetCandWords (void* arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    int             i;
    RECORD         *recTemp;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (input->strCodeInput[0] == '\0')
        return IRV_TO_PROCESS;

    if (input->bIsInRemind)
        return TableGetRemindCandWords (tbl);

    if (!strcmp (input->strCodeInput, table->strSymbol))
        return TableGetFHCandWords (tbl);

    if (input->strCodeInput[0] == table->cPinyin && table->bUsePY)
        return TableGetPinyinCandWords (tbl);

    if (TableFindFirstMatchCode (table, input->strCodeInput) == -1 && !table->tableDict->iAutoPhrase) {
        if (input->iCodeInputCount) {
            SetMessageCount(input->msgPreedit, 0);
            AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
            input->iCursorPos = strlen(input->strCodeInput);
        }
        //Not Found
        return IRV_DISPLAY_CANDWORDS;
    }

    TableCandWordSortContext context;
    context.order = table->tableOrder;
    UT_array candTemp;
    utarray_init(&candTemp, &tableCand_icd);

    while (table->tableDict->currentRecord && table->tableDict->currentRecord != table->tableDict->recordHead) {
        if (!TableCompareCode (table, input->strCodeInput, table->tableDict->currentRecord->strCode)) {
            TABLECANDWORD* tableCandWord = fcitx_malloc0(sizeof(TABLECANDWORD));
            TableAddCandWord (table->tableDict->currentRecord, tableCandWord);
            utarray_push_back(&candTemp, &tableCandWord);
        }
        table->tableDict->currentRecord = table->tableDict->currentRecord->next;
    }

    /* seems AD_NO will go back to n^2, really effect performance */
    if (table->tableOrder != AD_NO)
        utarray_sort_r(&candTemp, TableCandCmp, &context);

    TABLECANDWORD** pcand = NULL;
    for (pcand = (TABLECANDWORD**) utarray_front(&candTemp);
            pcand != NULL;
            pcand = (TABLECANDWORD**) utarray_next(&candTemp, pcand))
    {
        TABLECANDWORD* tableCandWord = *pcand;
        CandidateWord candWord;
        candWord.callback = TableGetCandWord;
        candWord.owner = tbl;
        candWord.priv = tableCandWord;
        candWord.strWord = strdup (tableCandWord->candWord.record->strHZ);
        candWord.strExtra = NULL;

        const char* pstr = NULL;
        if ((tableCandWord->flag == CT_NORMAL) && (tableCandWord->candWord.record->bPinyin)) {
            if (utf8_strlen (tableCandWord->candWord.record->strHZ) == 1) {
                recTemp = table->tableDict->tableSingleHZ[CalHZIndex (tableCandWord->candWord.record->strHZ)];
                if (!recTemp)
                    pstr = (char *) NULL;
                else
                    pstr = recTemp->strCode;
            }
            else
                pstr = (char *) NULL;
        }
        else if (HasMatchingKey (table, input->strCodeInput))
            pstr = (tableCandWord->flag == CT_NORMAL) ? tableCandWord->candWord.record->strCode : tableCandWord->candWord.autoPhrase->strCode;
        else
            pstr = ((tableCandWord->flag == CT_NORMAL) ? tableCandWord->candWord.record->strCode : tableCandWord->candWord.autoPhrase->strCode) + input->iCodeInputCount;

        if (pstr)
            candWord.strExtra = strdup(pstr);

        CandidateWordAppend(input->candList, &candWord);
    }
    utarray_clear(&candTemp);

    if (table->tableDict->bRule && table->bAutoPhrase && input->iCodeInputCount == table->tableDict->iCodeLength) {
        for (i = table->tableDict->iAutoPhrase - 1; i >= 0; i--) {
            if (!TableCompareCode (table, input->strCodeInput, table->tableDict->autoPhrase[i].strCode)) {
                if (TableHasPhrase (table->tableDict, table->tableDict->autoPhrase[i].strCode, table->tableDict->autoPhrase[i].strHZ)) {
                    TABLECANDWORD* tableCandWord = fcitx_malloc0(sizeof(TABLECANDWORD));
                    TableAddAutoCandWord (tbl, i, tableCandWord);
                    utarray_push_back(&candTemp, &tableCandWord);
                }
            }
        }
    }

    for (pcand = (TABLECANDWORD**) utarray_front(&candTemp);
            pcand != NULL;
            pcand = (TABLECANDWORD**) utarray_next(&candTemp, pcand))
    {
        TABLECANDWORD* tableCandWord = *pcand;
        CandidateWord candWord;
        candWord.callback = TableGetCandWord;
        candWord.owner = tbl;
        candWord.priv = tableCandWord;
        candWord.strWord = strdup (tableCandWord->candWord.autoPhrase->strHZ);
        candWord.strExtra = NULL;

        CandidateWordAppend(input->candList, &candWord);
    }

    utarray_done(&candTemp);

    if (input->iCodeInputCount) {
        SetMessageCount(input->msgPreedit, 0);
        AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
        input->iCursorPos = strlen(input->strCodeInput);
    }

    INPUT_RETURN_VALUE retVal = IRV_DISPLAY_CANDWORDS;

    if (table->iTableAutoSendToClient && (input->iCodeInputCount >= table->iTableAutoSendToClient)) {
        if (CandidateWordGetCurrentWindowSize(input->candList) == 1) {  //如果只有一个候选词，则送到客户程序中
            CandidateWord* candWord = CandidateWordGetCurrentWindow(input->candList);
            if (candWord->owner == tbl)
            {
                TABLECANDWORD* tableCandWord = candWord->priv;
                if (tableCandWord->flag != CT_AUTOPHRASE || (tableCandWord->flag == CT_AUTOPHRASE && !table->iSaveAutoPhraseAfter))
                    if (!(tableCandWord->flag == CT_NORMAL && tableCandWord->candWord.record->bPinyin))
                        retVal = CandidateWordChooseByIndex(input->candList, 0);
            }
        }
    }

    return retVal;
}

void TableAddAutoCandWord (FcitxTableState* tbl, short which, TABLECANDWORD* tableCandWord)
{
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    tableCandWord->flag = CT_AUTOPHRASE;
    tableCandWord->candWord.autoPhrase = &table->tableDict->autoPhrase[which];
}

void TableAddCandWord (RECORD * record, TABLECANDWORD* tableCandWord)
{
    tableCandWord->flag = CT_NORMAL;
    tableCandWord->candWord.record = record;
}

/*
 * 根据序号调整词组顺序，序号从1开始
 * 将指定的字/词调整到同样编码的最前面
 */
void TableAdjustOrderByIndex (FcitxTableState* tbl, TABLECANDWORD* tableCandWord)
{
    RECORD         *recTemp;
    int             iTemp;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    recTemp = tableCandWord->candWord.record;
    while (!strcmp (recTemp->strCode, recTemp->prev->strCode))
        recTemp = recTemp->prev;
    if (recTemp == tableCandWord->candWord.record)   //说明已经是第一个
        return;

    //将指定的字/词放到recTemp前
    tableCandWord->candWord.record->prev->next = tableCandWord->candWord.record->next;
    tableCandWord->candWord.record->next->prev = tableCandWord->candWord.record->prev;
    recTemp->prev->next = tableCandWord->candWord.record;
    tableCandWord->candWord.record->prev = recTemp->prev;
    recTemp->prev = tableCandWord->candWord.record;
    tableCandWord->candWord.record->next = recTemp;

    table->tableDict->iTableChanged++;

    //需要的话，更新索引
    if (tableCandWord->candWord.record->strCode[1] == '\0') {
        for (iTemp = 0; iTemp < strlen (table->tableDict->strInputCode); iTemp++) {
            if (tbl->recordIndex[iTemp].cCode == tableCandWord->candWord.record->strCode[0]) {
                tbl->recordIndex[iTemp].record = tableCandWord->candWord.record;
                break;
            }
        }
    }
}

/*
 * 根据序号删除词组，序号从1开始
 */
void TableDelPhraseByIndex (FcitxTableState* tbl, TABLECANDWORD* tableCandWord)
{
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (tableCandWord->flag != CT_NORMAL)
        return;

    if (utf8_strlen (tableCandWord->candWord.record->strHZ) <= 1)
        return;

    TableDelPhrase (table->tableDict, tableCandWord->candWord.record);
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

    if (!bCanntFindCode)
    {
        SetMessageCount(input->msgAuxDown, 2);
        SetMessageText(input->msgAuxDown, 1, table->tableDict->strNewPhraseCode);
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
INPUT_RETURN_VALUE TableGetRemindCandWords (FcitxTableState* tbl)
{
    int             iLength;
    RECORD         *tableRemind = NULL;
    GenericConfig *fc = &tbl->owner->config->gconfig;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    boolean bDisablePagingInRemind = *ConfigGetBindValue(fc, "Output", "RemindModeDisablePaging").boolvalue;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (!tbl->strTableRemindSource[0])
        return IRV_TO_PROCESS;

    input->strCodeInput[0] = '\0';
    input->iCodeInputCount = 0;
    CandidateWordReset(input->candList);

    iLength = utf8_strlen (tbl->strTableRemindSource);
    tableRemind = table->tableDict->recordHead->next;

    while (tableRemind != table->tableDict->recordHead) {
        if (bDisablePagingInRemind && CandidateWordGetListSize(input->candList) >= CandidateWordGetPageSize(input->candList))
            break;

        if (((iLength + 1) == utf8_strlen (tableRemind->strHZ))) {
            if (!utf8_strncmp (tableRemind->strHZ, tbl->strTableRemindSource, iLength) && utf8_get_nth_char(tableRemind->strHZ, iLength)) {
                TABLECANDWORD* tableCandWord = fcitx_malloc0(sizeof(TABLECANDWORD));
                TableAddRemindCandWord (tableRemind, tableCandWord);
                CandidateWord candWord;
                candWord.callback = TableGetCandWord;
                candWord.owner = tbl;
                candWord.priv = tableCandWord;
                candWord.strExtra = NULL;
                candWord.strWord = strdup(tableCandWord->candWord.record->strHZ + strlen (tbl->strTableRemindSource));
                CandidateWordAppend(input->candList, &candWord);
            }
        }

        tableRemind = tableRemind->next;
    }

    CleanInputWindowUp(instance);
    AddMessageAtLast(input->msgAuxUp, MSG_TIPS, _("Remind:"));
    AddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%s", tbl->strTableRemindSource);
    input->bIsInRemind = (CandidateWordPageCount(input->candList));

    if (input->bIsInRemind)
        return IRV_DISPLAY_CANDWORDS;
    else
        return IRV_CLEAN;
}

void TableAddRemindCandWord (RECORD * record, TABLECANDWORD* tableCandWord)
{
    tableCandWord->flag = CT_REMIND;
    tableCandWord->candWord.record = record;
}

INPUT_RETURN_VALUE TableGetRemindCandWord (void* arg, TABLECANDWORD* tableCandWord)
{
    FcitxTableState* tbl = (FcitxTableState*) arg;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    tableCandWord->candWord.record->iHit++;
    strcpy (tbl->strTableRemindSource, tableCandWord->candWord.record->strHZ + strlen (tbl->strTableRemindSource));
    TableGetRemindCandWords (tbl);

    strcpy(GetOutputString(input), tbl->strTableRemindSource);
    return IRV_COMMIT_STRING_REMIND;
}

INPUT_RETURN_VALUE TableGetFHCandWords (FcitxTableState* tbl)
{
    int             i;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    CleanInputWindowUp(instance);
    AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);
    input->iCursorPos = input->iCodeInputCount;

    if (!table->tableDict->iFH)
        return IRV_DISPLAY_MESSAGE;

    for (i = 0; i < table->tableDict->iFH; i++) {
        TABLECANDWORD* tableCandWord = fcitx_malloc0(sizeof(TABLECANDWORD));
        tableCandWord->flag = CT_FH;
        tableCandWord->candWord.iFHIndex = i;
        CandidateWord candWord;
        candWord.callback = TableGetCandWord;
        candWord.owner = tbl;
        candWord.priv = tableCandWord;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(table->tableDict->fh[i].strFH);
        CandidateWordAppend(input->candList, &candWord);
    }
    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetFHCandWord (FcitxTableState* tbl, TABLECANDWORD* tableCandWord)
{
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TableMetaData* table = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    CleanInputWindowDown(instance);

    strcpy(GetOutputString(input), table->tableDict->fh[tableCandWord->candWord.iFHIndex].strFH);
    return IRV_COMMIT_STRING;
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
            input->bShowCursor = false;

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

int TableCandCmp(const void* a, const void* b, void *arg)
{
    TABLECANDWORD* canda = *(TABLECANDWORD**)a;
    TABLECANDWORD* candb = *(TABLECANDWORD**)b;
    TableCandWordSortContext* context = arg;

    if (candb->candWord.record->bPinyin && !canda->candWord.record->bPinyin)
        return -1;
    else if (canda->candWord.record->bPinyin && !candb->candWord.record->bPinyin)
        return 1;
    switch (context->order)
    {
    case AD_NO:
        return 0;
    case AD_FAST:
    {
        int result = strcmp(canda->candWord.record->strCode, candb->candWord.record->strCode);
        if (result != 0)
            return result;
        return candb->candWord.record->iIndex - canda->candWord.record->iIndex;
    }
    break;
    case AD_FREQ:
    {
        int result = strcmp(canda->candWord.record->strCode, candb->candWord.record->strCode);
        if (result != 0)
            return result;
        return candb->candWord.record->iHit - canda->candWord.record->iHit;
    }
    break;
    }
    return 0;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
