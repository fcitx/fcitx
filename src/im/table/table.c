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


#include "table.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>
#include <errno.h>

#include "fcitx/fcitx.h"
#include "fcitx/keys.h"
#include "fcitx/ui.h"
#include "fcitx/profile.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx/candidate.h"
#include "fcitx/context.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/utarray.h"
#include "fcitx-utils/utils.h"
#include "im/pinyin/pydef.h"
#include "module/punc/punc.h"
#include "tablepinyinwrapper.h"

#define MAX_TABLE_INPUT 50

static void FreeTableConfig(void *v);
const UT_icd table_icd = {sizeof(TableMetaData), NULL , NULL, FreeTableConfig};
const UT_icd tableCand_icd = {sizeof(TABLECANDWORD*), NULL, NULL, NULL };
typedef struct _TableCandWordSortContext {
    ADJUSTORDER order;
} TableCandWordSortContext;

static void *TableCreate(FcitxInstance* instance);
static int TableCandCmp(const void* a, const void* b, void* arg);
static INPUT_RETURN_VALUE TableKeyBlocker(void* arg, FcitxKeySym sym, unsigned int state);

FCITX_DEFINE_PLUGIN(fcitx_table, ime2, FcitxIMClass2) =  {
    TableCreate,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static boolean LoadTableConfig(TableConfig* config);
static void SaveTableConfig(TableConfig* config);

boolean LoadTableConfig(TableConfig* config)
{
    FcitxConfigFileDesc* configDesc = GetTableGlobalConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp;
    char *file;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-table.config", "r", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveTableConfig(config);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    TableConfigConfigBind(config, cfile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*)config);

    if (fp)
        fclose(fp);

    return true;
}

void SaveTableConfig(TableConfig* config)
{
}

void *TableCreate(FcitxInstance* instance)
{
    FcitxTableState *tbl = fcitx_utils_malloc0(sizeof(FcitxTableState));
    tbl->owner = instance;
    if (!LoadTableConfig(&tbl->config)) {
        free(tbl);
        return NULL;
    }
    LoadTableInfo(tbl);
    TableMetaData* table;
    for (table = (TableMetaData*) utarray_front(tbl->table);
            table != NULL;
            table = (TableMetaData*) utarray_next(tbl->table, table)) {
        FcitxInstanceRegisterIM(
            instance,
            table,
            (strlen(table->uniqueName) == 0) ? table->strIconName : table->uniqueName,
            table->strName,
            table->strIconName,
            TableInit,
            TableResetStatus,
            DoTableInput,
            TableGetCandWords,
            TablePhraseTips,
            SaveTableIM,
            ReloadTableConfig,
            TableKeyBlocker,
            table->iPriority,
            table->langCode
        );
    }


    return tbl;
}

void SaveTableIM(void *arg)
{
    TableMetaData* table = (TableMetaData*) arg;

    if (!table->tableDict)
        return;
    if (table->tableDict && table->tableDict->iTableChanged)
        SaveTableDict(table);
}


/*
 * 读取码表输入法的名称和文件路径
 */
void LoadTableInfo(FcitxTableState *tbl)
{
    char **tablePath;
    size_t len;
    int i = 0;

    FcitxStringHashSet* sset = NULL;
    tbl->bTablePhraseTips = false;
    tbl->iCurrentTableLoaded = -1;

    if (tbl->table) {
        utarray_free(tbl->table);
        tbl->table = NULL;
    }

    tbl->iTableCount = 0;
    utarray_new(tbl->table, &table_icd);

    tablePath = FcitxXDGGetPathWithPrefix(&len, "table");
    sset = FcitxXDGGetFiles("table", NULL, ".conf");

    char **paths = fcitx_utils_malloc0(sizeof(char*) * len);
    for (i = 0; i < len ; i ++)
        paths[i] = NULL;
    FcitxStringHashSet* string;
    for (string = sset;
            string != NULL;
            string = (FcitxStringHashSet*)string->hh.next) {
        int i = 0;
        for (i = len - 1; i >= 0; i--) {
            asprintf(&paths[i], "%s/%s", tablePath[len - i - 1], string->name);
            FcitxLog(DEBUG, "Load Table Config File:%s", paths[i]);
        }
        FcitxLog(INFO, _("Load Table Config File:%s"), string->name);
        FcitxConfigFile* cfile = FcitxConfigParseMultiConfigFile(paths, len, GetTableConfigDesc());
        if (cfile) {
            utarray_extend_back(tbl->table);
            TableMetaData *t = (TableMetaData*)utarray_back(tbl->table);
            TableMetaDataConfigBind(t, cfile, GetTableConfigDesc());
            FcitxConfigBindSync((FcitxGenericConfig*)t);
            FcitxLog(DEBUG, _("Table Config %s is %s"), string->name, (t->bEnabled) ? "Enabled" : "Disabled");
            if (t->bEnabled) {
                t->confName = strdup(string->name);
                t->owner = tbl;
                tbl->iTableCount ++;
            }
            else {
                utarray_pop_back(tbl->table);
            }
        }

        for (i = 0; i < len ; i ++) {
            free(paths[i]);
            paths[i] = NULL;
        }
    }

    free(paths);
    FcitxXDGFreePath(tablePath);
    fcitx_utils_free_string_hash_set(sset);
}

CONFIG_DESC_DEFINE(GetTableConfigDesc, "table.desc")
CONFIG_DESC_DEFINE(GetTableGlobalConfigDesc, "fcitx-table.desc")

boolean TableInit(void *arg)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState *tbl = table->owner;
    boolean flag = true;
    FcitxInstanceSetContext(tbl->owner, CONTEXT_IM_KEYBOARD_LAYOUT, table->kbdlayout);
    FcitxInstanceSetContext(tbl->owner, CONTEXT_SHOW_REMIND_STATUS, &flag);
    if (table->bUseAlternativePageKey) {
        FcitxInstanceSetContext(tbl->owner, CONTEXT_ALTERNATIVE_PREVPAGE_KEY, table->hkAlternativePrevPage);
        FcitxInstanceSetContext(tbl->owner, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY, table->hkAlternativeNextPage);
    }
    FcitxAddon* pyaddon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(tbl->owner), "fcitx-pinyin");
    tbl->pyaddon = pyaddon;
    if (pyaddon == NULL)
        return false;
    tbl->PYBaseOrder = AD_FREQ;

    Table_ResetPY(tbl->pyaddon);
    return true;
}

void TableResetStatus(void* arg)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState *tbl = table->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(tbl->owner);
    tbl->bIsTableAddPhrase = false;
    tbl->bIsTableDelPhrase = false;
    tbl->bIsTableClearFreq = false;
    tbl->bIsTableAdjustOrder = false;
    FcitxInputStateSetIsDoInputOnly(input, false);
    //bSingleHZMode = false;
}

boolean TableCheckNoMatch(TableMetaData* table, const char* code)
{
    FcitxInstance *instance = table->owner->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    if (!table->bTableExactMatch) {
        return FcitxCandidateWordGetListSize(candList) == 0;
    }
    else {
        return (FcitxCandidateWordGetListSize(candList) == 0) && TableFindFirstMatchCode(table, code, false, false) == -1;
    }
}

INPUT_RETURN_VALUE DoTableInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState *tbl = table->owner;
    INPUT_RETURN_VALUE retVal;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(instance);
    char* strCodeInput = FcitxInputStateGetRawInputBuffer(input);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);

    unsigned int cmodtable[] = {FcitxKeyState_None, FcitxKeyState_Alt, FcitxKeyState_Ctrl, FcitxKeyState_Shift};
    if (table->chooseModifier > CM_CTRL)
        table->chooseModifier = CM_CTRL;

    FcitxCandidateWordSetChooseAndModifier(candList, table->strChoose, cmodtable[table->chooseModifier]);
    FcitxCandidateWordSetPageSize(candList, config->iMaxCandWord);

    int iTableIMIndex = utarray_eltidx(tbl->table, table);
    if (iTableIMIndex != tbl->iCurrentTableLoaded) {
        TableMetaData* previousTable = (TableMetaData*) utarray_eltptr(tbl->table, tbl->iCurrentTableLoaded);
        if (previousTable)
            FreeTableDict(previousTable);
        tbl->iCurrentTableLoaded = -1;
    }

    if (tbl->iCurrentTableLoaded == -1) {
        LoadTableDict(table);
        tbl->iCurrentTableLoaded = iTableIMIndex;
    }

    if (FcitxHotkeyIsHotKeyModifierCombine(sym, state))
        return IRV_TO_PROCESS;

    if (tbl->bTablePhraseTips) {
        if (FcitxHotkeyIsHotKey(sym, state, FCITX_CTRL_DELETE)) {
            tbl->bTablePhraseTips = false;
            TableDelPhraseByHZ(table->tableDict, FcitxMessagesGetMessageString(FcitxInputStateGetAuxUp(input), 1));
            return IRV_CLEAN;
        } else if (state == FcitxKeyState_None && (sym != FcitxKey_Control_L && sym != FcitxKey_Control_R && sym != FcitxKey_Shift_L && sym != FcitxKey_Shift_R)) {
            FcitxInstanceCleanInputWindow(instance);
            tbl->bTablePhraseTips = false;
            FcitxUICloseInputWindow(instance);
        }
    }

    retVal = IRV_DO_NOTHING;
    if (state == FcitxKeyState_None &&
        (IsInputKey(table->tableDict, sym)
         || IsEndKey(table, sym)
         || (table->bUseMatchingKey && sym == table->cMatchingKey)
         || (table->bUsePY && sym == table->cPinyin)
         || (strCodeInput[0] == table->cPinyin && table->bUsePY && sym == FcitxKey_apostrophe)
        )
       ) {
        if (FcitxInputStateGetIsInRemind(input))
            FcitxCandidateWordReset(candList);
        FcitxInputStateSetIsInRemind(input, false);

        /* it's not in special state */
        if (!tbl->bIsTableAddPhrase && !tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder && !tbl->bIsTableClearFreq) {
            /* check we use Pinyin or Not */
            if (strCodeInput[0] == table->cPinyin && table->bUsePY) {
                if (FcitxInputStateGetRawInputBufferSize(input) != (MAX_PY_LENGTH * 5 + 1)) {
                    strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = (char) sym;
                    strCodeInput[FcitxInputStateGetRawInputBufferSize(input) + 1] = '\0';
                    FcitxInputStateSetRawInputBufferSize(input, FcitxInputStateGetRawInputBufferSize(input) + 1);
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else
                    retVal = IRV_DO_NOTHING;
            } else {
                /* length is not too large */
                if (((FcitxInputStateGetRawInputBufferSize(input) < table->tableDict->iCodeLength)
                    || (table->tableDict->bHasPinyin && FcitxInputStateGetRawInputBufferSize(input) < table->tableDict->iPYCodeLength)
                    || (((TableCheckNoMatch(table, FcitxInputStateGetRawInputBuffer(input))
                        && table->bNoMatchDontCommit) || !table->bUseAutoSend)
                        && FcitxInputStateGetRawInputBufferSize(input) >= table->tableDict->iCodeLength
                    ))
                    && FcitxInputStateGetRawInputBufferSize(input) <= MAX_TABLE_INPUT
                ) {
                    strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = (char) sym;
                    strCodeInput[FcitxInputStateGetRawInputBufferSize(input) + 1] = '\0';
                    FcitxInputStateSetRawInputBufferSize(input, FcitxInputStateGetRawInputBufferSize(input) + 1);

                    if (FcitxInputStateGetRawInputBufferSize(input) == 1 && strCodeInput[0] == table->cPinyin && table->bUsePY) {
                        retVal = IRV_DISPLAY_LAST;
                    } else {
                        char        *strTemp;
                        char        *strLastFirstCand;
                        CANDTYPE     lastFirstCandType;
                        int          lastPageCount = FcitxCandidateWordPageCount(candList);

                        strLastFirstCand = (char *)NULL;
                        lastFirstCandType = CT_AUTOPHRASE;
                        if (FcitxCandidateWordPageCount(candList) != 0) {
                            // to realize auto-sending HZ to client
                            FcitxCandidateWord *candWord = NULL;
                            candWord = FcitxCandidateWordGetCurrentWindow(candList);
                            if (candWord->owner == table) {
                                TABLECANDWORD* tableCandWord = candWord->priv;
                                lastFirstCandType = tableCandWord->flag;
                                INPUT_RETURN_VALUE ret = _TableGetCandWord(table, tableCandWord, false);
                                if (ret & IRV_FLAG_PENDING_COMMIT_STRING)
                                    strLastFirstCand = FcitxInputStateGetOutputString(input);
                            }
                        }

                        retVal = TableGetCandWords(table);
                        FcitxModuleFunctionArg farg;
                        int key = FcitxInputStateGetRawInputBuffer(input)[0];
                        farg.args[0] = &key;
                        if (!table->bIgnorePunc)
                            strTemp = InvokeFunction(instance, FCITX_PUNC, GETPUNC, farg);
                        else
                            strTemp = NULL;
                        if (IsEndKey(table, sym)) {
                            if (FcitxInputStateGetRawInputBufferSize(input) == 1)
                                return IRV_TO_PROCESS;

                            if (FcitxCandidateWordPageCount(candList) == 0) {
                                FcitxInputStateSetRawInputBufferSize(input, 0);
                                return IRV_CLEAN;
                            }

                            if (FcitxCandidateWordGetCurrentWindowSize(candList) == 1) {
                                retVal = FcitxCandidateWordChooseByIndex(candList, 0);
                                return retVal;
                            }

                            return IRV_DISPLAY_CANDWORDS;
                        } else if (table->bUseAutoSend
                                   && table->iTableAutoSendToClientWhenNone
                                   && (!(retVal & IRV_FLAG_PENDING_COMMIT_STRING))
                                   && (FcitxInputStateGetRawInputBufferSize(input) >= (table->iTableAutoSendToClientWhenNone + 1))
                                   && TableCheckNoMatch(table, FcitxInputStateGetRawInputBuffer(input))) {
                            if (strLastFirstCand && (lastFirstCandType != CT_AUTOPHRASE)) {
                                FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), strLastFirstCand);
                            } else if (table->bSendRawPreedit) {
                                strCodeInput[FcitxInputStateGetRawInputBufferSize(input) - 1] = '\0';
                                FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), strCodeInput);
                            }
                            retVal = IRV_DISPLAY_CANDWORDS;
                            FcitxInputStateSetRawInputBufferSize(input, 1);
                            strCodeInput[0] = sym;
                            strCodeInput[1] = '\0';
                        } else if ((FcitxInputStateGetRawInputBufferSize(input) == 1) && strTemp && lastPageCount == 0) {  //如果第一个字母是标点，并且没有候选字/词，则当做标点处理──适用于二笔这样的输入法
                            strcpy(FcitxInputStateGetOutputString(input), strTemp);
                            retVal = IRV_PUNC;
                        }
                    }
                } else {
                    if (table->bUseAutoSend && table->iTableAutoSendToClient) {
                        retVal = IRV_DISPLAY_CANDWORDS;
                        if (FcitxCandidateWordPageCount(candList)) {
                            FcitxCandidateWord* candWord = FcitxCandidateWordGetCurrentWindow(candList);
                            if (candWord->owner == table) {
                                TABLECANDWORD* tableCandWord = candWord->priv;
                                if (tableCandWord->flag != CT_AUTOPHRASE) {
                                    INPUT_RETURN_VALUE ret = TableGetCandWord(table, candWord);
                                    if (ret & IRV_FLAG_PENDING_COMMIT_STRING) {
                                        FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), FcitxInputStateGetOutputString(input));
                                    }
                                }
                            }
                        } else if (table->bSendRawPreedit) {
                            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), strCodeInput);
                        }

                        FcitxInputStateSetRawInputBufferSize(input, 1);
                        strCodeInput[0] = sym;
                        strCodeInput[1] = '\0';
                        FcitxInputStateSetIsInRemind(input, false);

                    } else
                        retVal = IRV_DO_NOTHING;
                }
            }
        }
    } else {
        if (tbl->bIsTableAddPhrase) {
            if (FcitxHotkeyIsHotKey(sym, state, FCITX_LEFT)) {
                if (tbl->iTableNewPhraseHZCount < table->tableDict->iHZLastInputCount && tbl->iTableNewPhraseHZCount < PHRASE_MAX_LENGTH) {
                    tbl->iTableNewPhraseHZCount++;
                    TableCreateNewPhrase(table);
                }
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_RIGHT)) {
                if (tbl->iTableNewPhraseHZCount > 2) {
                    tbl->iTableNewPhraseHZCount--;
                    TableCreateNewPhrase(table);
                }
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ENTER)) {
                if (strcmp("????", FcitxMessagesGetMessageString(FcitxInputStateGetAuxDown(input), 0)) != 0)
                    TableInsertPhrase(table->tableDict, FcitxMessagesGetMessageString(FcitxInputStateGetAuxDown(input), 1), FcitxMessagesGetMessageString(FcitxInputStateGetAuxDown(input), 0));
                tbl->bIsTableAddPhrase = false;
                FcitxInputStateSetIsDoInputOnly(input, false);
                return IRV_CLEAN;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
                tbl->bIsTableAddPhrase = false;
                FcitxInputStateSetIsDoInputOnly(input, false);
                return IRV_CLEAN;
            } else {
                return IRV_DO_NOTHING;
            }
            return IRV_DISPLAY_MESSAGE;
        }
        if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkTableAddPhrase)) {
            if (!tbl->bIsTableAddPhrase) {
                if (table->tableDict->iHZLastInputCount < 2 || !table->tableDict->bRule) //词组最少为两个汉字
                    return IRV_DO_NOTHING;

                tbl->bTablePhraseTips = false;
                tbl->iTableNewPhraseHZCount = 2;
                tbl->bIsTableAddPhrase = true;
                FcitxInputStateSetIsDoInputOnly(input, true);
                FcitxInputStateSetShowCursor(input, false);

                FcitxInstanceCleanInputWindow(instance);
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Left/Right to choose selected character, Press Enter to confirm, Press Escape to Cancel"));

                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_FIRSTCAND, "");
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_CODE, "");

                TableCreateNewPhrase(table);
                retVal = IRV_DISPLAY_MESSAGE;
            } else
                retVal = IRV_TO_PROCESS;

            return retVal;
        } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_CTRL_ALT_E)) {
            char            strPY[100];

            //如果拼音单字字库没有读入，则读入它
            Table_LoadPYBaseDict(tbl->pyaddon);

            //如果刚刚输入的是个词组，刚不查拼音
            if (fcitx_utf8_strlen(FcitxInputStateGetOutputString(input)) != 1)
                return IRV_DO_NOTHING;

            FcitxInputStateSetRawInputBufferSize(input, 0);

            FcitxInstanceCleanInputWindow(instance);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_INPUT, "%s", FcitxInputStateGetOutputString(input));

            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_CODE, _("Pinyin: "));
            Table_PYGetPYByHZ(tbl->pyaddon, FcitxInputStateGetOutputString(input), strPY);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, (strPY[0]) ? strPY : _("Cannot found Pinyin"));
            FcitxInputStateSetShowCursor(input, false);

            return IRV_DISPLAY_MESSAGE;
        }

        if (!FcitxInputStateGetRawInputBufferSize(input) && !FcitxInputStateGetIsInRemind(input))
            return IRV_TO_PROCESS;

        if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
            if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder || tbl->bIsTableClearFreq) {
                TableResetStatus(table);
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT | MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", FcitxInputStateGetRawInputBuffer(input));
                retVal = IRV_DISPLAY_CANDWORDS;
            } else
                return IRV_CLEAN;
        } else if (FcitxHotkeyCheckChooseKey(sym, state, table->strChoose) >= 0) {
            int iKey;
            iKey = FcitxHotkeyCheckChooseKey(sym, state, table->strChoose);

            if (FcitxCandidateWordPageCount(candList) == 0)
                return IRV_TO_PROCESS;

            if (FcitxCandidateWordGetByIndex(candList, iKey) == NULL)
                return IRV_DO_NOTHING;
            else {
                FcitxCandidateWord* candWord = FcitxCandidateWordGetByIndex(candList, iKey);
                if (candWord->owner == table && tbl->bIsTableDelPhrase) {
                    TableDelPhraseByIndex(table, candWord->priv);
                    tbl->bIsTableDelPhrase = false;
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else if (candWord->owner == table && tbl->bIsTableAdjustOrder) {
                    TableAdjustOrderByIndex(table, candWord->priv);
                    tbl->bIsTableAdjustOrder = false;
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else if (candWord->owner == table && tbl->bIsTableClearFreq) {
                    TableClearFreqByIndex(table, candWord->priv);
                    tbl->bIsTableClearFreq = false;
                    retVal = IRV_DISPLAY_CANDWORDS;
                }
                else
                    return IRV_TO_PROCESS;
            }
        } else if (!tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder && !tbl->bIsTableClearFreq) {
            if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkTableAdjustOrder)) {
                if (FcitxCandidateWordGetListSize(candList) < 2 || FcitxInputStateGetIsInRemind(input))
                    return IRV_DO_NOTHING;

                tbl->bIsTableAdjustOrder = true;
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Choose the phrase to be put in the front, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            } else if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkTableDelPhrase)) {
                if (!FcitxCandidateWordPageCount(candList) || FcitxInputStateGetIsInRemind(input))
                    return IRV_DO_NOTHING;

                tbl->bIsTableDelPhrase = true;
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Choose the phrase to be deleted, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            } else if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkTableClearFreq)) {
                if (!FcitxCandidateWordPageCount(candList) || FcitxInputStateGetIsInRemind(input))
                    return IRV_DO_NOTHING;

                tbl->bIsTableClearFreq = true;
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Choose the phrase to clear typing history, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
                if (!FcitxInputStateGetRawInputBufferSize(input)) {
                    FcitxInputStateSetIsInRemind(input, false);
                    return IRV_DONOT_PROCESS_CLEAN;
                }

                FcitxInputStateSetRawInputBufferSize(input, FcitxInputStateGetRawInputBufferSize(input) - 1);
                strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = '\0';

                if (FcitxInputStateGetRawInputBufferSize(input) == 1 && strCodeInput[0] == table->cPinyin && table->bUsePY) {
                    FcitxCandidateWordReset(candList);
                    retVal = IRV_DISPLAY_LAST;
                } else if (FcitxInputStateGetRawInputBufferSize(input))
                    retVal = IRV_DISPLAY_CANDWORDS;
                else
                    retVal = IRV_CLEAN;
            } else if (FcitxHotkeyIsHotKey(sym, state, table->hkCommitKey)) {
                if (FcitxInputStateGetRawInputBufferSize(input) == 1 && strCodeInput[0] == table->cPinyin && table->bUsePY)
                    retVal = IRV_COMMIT_STRING;
                else {
                    if (FcitxCandidateWordPageCount(candList) == 0 && table->bCommitKeyCommitWhenNoMatch) {
                        FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), FcitxInputStateGetRawInputBuffer(input));
                        FcitxInputStateSetRawInputBufferSize(input, 0);
                        FcitxInputStateGetRawInputBuffer(input)[0] = '\0';
                        FcitxInputStateSetIsInRemind(input, false);
                        FcitxInstanceCleanInputWindow(instance);
                        FcitxUIUpdateInputWindow(instance);
                        retVal = IRV_DO_NOTHING;
                    }
                    else {
                        retVal = FcitxCandidateWordChooseByIndex(candList, 0);
                        if (retVal == IRV_TO_PROCESS)
                            retVal = IRV_DO_NOTHING;
                    }
                }
            } else {
                return IRV_TO_PROCESS;
            }
        }
    }

    if (!FcitxInputStateGetIsInRemind(input)) {
        if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder || tbl->bIsTableClearFreq)
            FcitxInputStateSetIsDoInputOnly(input, true);
    }

    if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder || tbl->bIsTableClearFreq || FcitxInputStateGetIsInRemind(input))
        FcitxInputStateSetShowCursor(input, false);
    else {
        FcitxInputStateSetShowCursor(input, true);
        FcitxInputStateSetCursorPos(input, strlen(FcitxInputStateGetRawInputBuffer(input)));
        FcitxInputStateSetClientCursorPos(input, 0);
    }

    return retVal;
}

INPUT_RETURN_VALUE TableGetCandWord(void* arg, FcitxCandidateWord* candWord)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState *tbl = table->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(tbl->owner);
    TABLECANDWORD* tableCandWord = candWord->priv;

    INPUT_RETURN_VALUE retVal = _TableGetCandWord(table, tableCandWord, true);
    if (retVal & IRV_FLAG_PENDING_COMMIT_STRING) {
        if (table->bAutoPhrase && (fcitx_utf8_strlen(FcitxInputStateGetOutputString(input)) == 1 || (fcitx_utf8_strlen(FcitxInputStateGetOutputString(input)) > 1 && table->bAutoPhrasePhrase)))
            UpdateHZLastInput(table, FcitxInputStateGetOutputString(input));

        if (tbl->pCurCandRecord)
            TableUpdateHitFrequency(table, tbl->pCurCandRecord);
    }

    return retVal;
}

/*
 * 第二个参数表示是否进入联想模式，实现自动上屏功能时，不能使用联想模式
 */
INPUT_RETURN_VALUE _TableGetCandWord(TableMetaData* table, TABLECANDWORD* tableCandWord, boolean _bRemind)
{
    char           *pCandWord = NULL;
    FcitxTableState* tbl = table->owner;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxProfile *profile = FcitxInstanceGetProfile(instance);

    if (tableCandWord->flag == CT_FH)
        return TableGetFHCandWord(table, tableCandWord);

    FcitxInputStateSetIsInRemind(input, false);

    if (tableCandWord->flag == CT_NORMAL)
        tbl->pCurCandRecord = tableCandWord->candWord.record;
    else
        tbl->pCurCandRecord = (RECORD *)NULL;

    if (table->tableDict->iTableChanged >= TABLE_AUTO_SAVE_AFTER)
        SaveTableDict(table);

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
                TableInsertPhrase(table->tableDict, tableCandWord->candWord.autoPhrase->strCode, tableCandWord->candWord.autoPhrase->strHZ);
        }
        pCandWord = tableCandWord->candWord.autoPhrase->strHZ;
        break;
    case CT_FH:
        pCandWord = table->tableDict->fh[tableCandWord->candWord.iFHIndex].strFH;
        break;
    case CT_REMIND: {
        strcpy(tbl->strTableRemindSource, tableCandWord->candWord.record->strHZ + strlen(tbl->strTableRemindSource));
        strcpy(FcitxInputStateGetOutputString(input), tbl->strTableRemindSource);
        INPUT_RETURN_VALUE retVal = TableGetRemindCandWords(table);
        if (retVal == IRV_DISPLAY_CANDWORDS)
            return IRV_COMMIT_STRING_REMIND;
        else
            return IRV_COMMIT_STRING;
    }
    }

    if (profile->bUseRemind && _bRemind) {
        strcpy(tbl->strTableRemindSource, pCandWord);
        strcpy(FcitxInputStateGetOutputString(input), pCandWord);
        INPUT_RETURN_VALUE retVal = TableGetRemindCandWords(table);
        if (retVal == IRV_DISPLAY_CANDWORDS)
            return IRV_COMMIT_STRING_REMIND;
    } else {
        if (table->bPromptTableCode) {
            RECORD         *temp;

            FcitxInstanceCleanInputWindow(instance);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));

            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, "%s", pCandWord);
            temp = table->tableDict->tableSingleHZ[CalHZIndex(pCandWord)];
            if (temp) {
                FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_CODE, "%s", temp->strCode);
            }
        } else {
            FcitxInstanceCleanInputWindow(instance);
        }
    }

    if (fcitx_utf8_strlen(pCandWord) == 1)
        FcitxInputStateSetLastIsSingleChar(input, 1);
    else
        FcitxInputStateSetLastIsSingleChar(input, 0);

    strcpy(FcitxInputStateGetOutputString(input), pCandWord);
    return IRV_COMMIT_STRING;
}

INPUT_RETURN_VALUE TableGetPinyinCandWords(TableMetaData* table)
{
    FcitxTableState* tbl = table->owner;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    strcpy(Table_PYGetFindString(tbl->pyaddon), FcitxInputStateGetRawInputBuffer(input) + 1);

    Table_DoPYInput(tbl->pyaddon, 0, 0);
    Table_PYGetCandWords(tbl->pyaddon);

    FcitxInputStateGetRawInputBuffer(input)[0] = table->cPinyin;
    FcitxInputStateGetRawInputBuffer(input)[1] = '\0';

    strcat(FcitxInputStateGetRawInputBuffer(input), Table_PYGetFindString(tbl->pyaddon));
    FcitxInputStateSetRawInputBufferSize(input, strlen(FcitxInputStateGetRawInputBuffer(input)));

    FcitxInstanceCleanInputWindowUp(instance);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT | MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", FcitxInputStateGetRawInputBuffer(input));
    FcitxInputStateSetCursorPos(input, FcitxInputStateGetRawInputBufferSize(input));
    FcitxInputStateSetClientCursorPos(input, 0);

    //下面将拼音的候选字表转换为码表输入法的样式
    FcitxCandidateWord* candWord;
    for (candWord = FcitxCandidateWordGetFirst(FcitxInputStateGetCandidateList(input));
            candWord != NULL;
            candWord = FcitxCandidateWordGetNext(FcitxInputStateGetCandidateList(input), candWord)) {
        char* pstr;
        if (fcitx_utf8_strlen(candWord->strWord) == 1) {
            RECORD* recTemp = table->tableDict->tableSingleHZ[CalHZIndex(candWord->strWord)];
            if (!recTemp)
                pstr = (char *) NULL;
            else
                pstr = recTemp->strCode;

        } else
            pstr = (char *) NULL;

        if (pstr) {
            candWord->strExtra = strdup(pstr);
            candWord->extraType = MSG_CODE;
        }
        tbl->pygetcandword = candWord->callback;
        candWord->callback = Table_PYGetCandWord;
        candWord->owner = table;
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetCandWords(void* arg)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState *tbl = table->owner;
    int             i;
    RECORD         *recTemp;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);

    if (FcitxInputStateGetRawInputBuffer(input)[0] == '\0')
        return IRV_TO_PROCESS;

    if (FcitxInputStateGetIsInRemind(input))
        return TableGetRemindCandWords(table);

    if (!strcmp(FcitxInputStateGetRawInputBuffer(input), table->strSymbol))
        return TableGetFHCandWords(table);

    if (FcitxInputStateGetRawInputBuffer(input)[0] == table->cPinyin && table->bUsePY)
        return TableGetPinyinCandWords(table);

    if (TableFindFirstMatchCode(table, FcitxInputStateGetRawInputBuffer(input), table->bTableExactMatch, true) == -1 && !table->tableDict->iAutoPhrase) {
        if (FcitxInputStateGetRawInputBufferSize(input)) {
            FcitxMessagesSetMessageCount(FcitxInputStateGetPreedit(input), 0);
            FcitxMessagesSetMessageCount(FcitxInputStateGetClientPreedit(input), 0);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));
            FcitxMessageType type = MSG_INPUT;
            if (!table->bSendRawPreedit)
                type |= MSG_DONOT_COMMIT_WHEN_UNFOCUS;
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), type, "%s", FcitxInputStateGetRawInputBuffer(input));
            FcitxInputStateSetCursorPos(input, strlen(FcitxInputStateGetRawInputBuffer(input)));
            FcitxInputStateSetClientCursorPos(input, 0);
        }
        //Not Found
        return IRV_DISPLAY_CANDWORDS;
    }

    TableCandWordSortContext context;
    context.order = table->tableOrder;
    UT_array candTemp;
    utarray_init(&candTemp, &tableCand_icd);

    while (table->tableDict->currentRecord && table->tableDict->currentRecord != table->tableDict->recordHead) {
        if (table->tableDict->currentRecord->type != RECORDTYPE_CONSTRUCT &&
            table->tableDict->currentRecord->type != RECORDTYPE_PROMPT &&
            !TableCompareCode(table, FcitxInputStateGetRawInputBuffer(input), table->tableDict->currentRecord->strCode, table->bTableExactMatch)) {
            TABLECANDWORD* tableCandWord = fcitx_utils_malloc0(sizeof(TABLECANDWORD));
            TableAddCandWord(table->tableDict->currentRecord, tableCandWord);
            utarray_push_back(&candTemp, &tableCandWord);
        }
        table->tableDict->currentRecord = table->tableDict->currentRecord->next;
    }

    /* seems AD_NO will go back to n^2, really effect performance */
    if (table->tableOrder != AD_NO)
        utarray_msort_r(&candTemp, TableCandCmp, &context);

    TABLECANDWORD** pcand = NULL;
    for (pcand = (TABLECANDWORD**) utarray_front(&candTemp);
            pcand != NULL;
            pcand = (TABLECANDWORD**) utarray_next(&candTemp, pcand)) {
        TABLECANDWORD* tableCandWord = *pcand;
        FcitxCandidateWord candWord;
        candWord.callback = TableGetCandWord;
        candWord.owner = table;
        candWord.priv = tableCandWord;
        candWord.strWord = strdup(tableCandWord->candWord.record->strHZ);
        candWord.strExtra = NULL;
        candWord.wordType = MSG_OTHER;

        const char* pstr = NULL;
        if ((tableCandWord->flag == CT_NORMAL) && (tableCandWord->candWord.record->type == RECORDTYPE_PINYIN)) {
            if (fcitx_utf8_strlen(tableCandWord->candWord.record->strHZ) == 1) {
                recTemp = table->tableDict->tableSingleHZ[CalHZIndex(tableCandWord->candWord.record->strHZ)];
                if (!recTemp)
                    pstr = (char *) NULL;
                else
                    pstr = recTemp->strCode;
            } else
                pstr = (char *) NULL;
        } else if (HasMatchingKey(table, FcitxInputStateGetRawInputBuffer(input)))
            pstr = (tableCandWord->flag == CT_NORMAL) ? tableCandWord->candWord.record->strCode : tableCandWord->candWord.autoPhrase->strCode;
        else
            pstr = ((tableCandWord->flag == CT_NORMAL) ? tableCandWord->candWord.record->strCode : tableCandWord->candWord.autoPhrase->strCode) + FcitxInputStateGetRawInputBufferSize(input);

        if (pstr) {
            if (table->customPrompt) {
                size_t codelen = strlen(pstr);
                int i = 0;
                int totallen = 0;
                for (i = 0; i < codelen; i ++) {
                    RECORD* rec = table->tableDict->promptCode[(uint8_t) pstr[i]];
                    if (rec) {
                        totallen += strlen(rec->strHZ);
                    } else
                        totallen += 1;
                }

                candWord.strExtra = fcitx_utils_malloc0(sizeof(char) * (totallen + 1 + 3));
                if (codelen)
                    strcpy(candWord.strExtra, "\xef\xbd\x9e");
                for (i = 0; i < codelen; i ++) {
                    RECORD* rec = table->tableDict->promptCode[(uint8_t) pstr[i]];
                    if (rec) {
                        strcat(candWord.strExtra, rec->strHZ);
                    } else {
                        char temp[2] = {pstr[i], '\0'};
                        strcat(candWord.strExtra, temp);
                    }
                }
            }
            else {
                candWord.strExtra = strdup(pstr);
            }
            candWord.extraType = MSG_CODE;
        }

        FcitxCandidateWordAppend(candList, &candWord);
    }
    utarray_clear(&candTemp);

    if (table->tableDict->bRule && table->bAutoPhrase && FcitxInputStateGetRawInputBufferSize(input) == table->tableDict->iCodeLength) {
        for (i = table->tableDict->iAutoPhrase - 1; i >= 0; i--) {
            if (!TableCompareCode(table, FcitxInputStateGetRawInputBuffer(input), table->tableDict->autoPhrase[i].strCode, table->bTableExactMatch)) {
                if (TableHasPhrase(table->tableDict, table->tableDict->autoPhrase[i].strCode, table->tableDict->autoPhrase[i].strHZ)) {
                    TABLECANDWORD* tableCandWord = fcitx_utils_malloc0(sizeof(TABLECANDWORD));
                    TableAddAutoCandWord(table, i, tableCandWord);
                    utarray_push_back(&candTemp, &tableCandWord);
                }
            }
        }
    }

    for (pcand = (TABLECANDWORD**) utarray_front(&candTemp);
            pcand != NULL;
            pcand = (TABLECANDWORD**) utarray_next(&candTemp, pcand)) {
        TABLECANDWORD* tableCandWord = *pcand;
        FcitxCandidateWord candWord;
        candWord.callback = TableGetCandWord;
        candWord.owner = table;
        candWord.priv = tableCandWord;
        candWord.strWord = strdup(tableCandWord->candWord.autoPhrase->strHZ);
        candWord.strExtra = NULL;
        candWord.wordType = MSG_USERPHR;

        FcitxCandidateWordAppend(candList, &candWord);
    }

    utarray_done(&candTemp);

    INPUT_RETURN_VALUE retVal = IRV_DISPLAY_CANDWORDS;

    if (table->bUseAutoSend && table->iTableAutoSendToClient && (FcitxInputStateGetRawInputBufferSize(input) >= table->iTableAutoSendToClient)) {
        if (FcitxCandidateWordGetCurrentWindowSize(candList) == 1) {  //如果只有一个候选词，则送到客户程序中
            FcitxCandidateWord* candWord = FcitxCandidateWordGetCurrentWindow(candList);
            if (candWord->owner == table) {
                TABLECANDWORD* tableCandWord = candWord->priv;
                if (tableCandWord->flag != CT_AUTOPHRASE || (tableCandWord->flag == CT_AUTOPHRASE && !table->iSaveAutoPhraseAfter))
                    if (!(tableCandWord->flag == CT_NORMAL && tableCandWord->candWord.record->type == RECORDTYPE_PINYIN))
                        retVal = FcitxCandidateWordChooseByIndex(candList, 0);
            }
        }
    }

    if (FcitxInputStateGetRawInputBufferSize(input)) {
        FcitxMessagesSetMessageCount(FcitxInputStateGetPreedit(input), 0);
        FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));
        FcitxInputStateSetCursorPos(input, strlen(FcitxInputStateGetRawInputBuffer(input)));
        FcitxCandidateWord* candWord = NULL;
        if (table->bFirstCandidateAsPreedit && (candWord = FcitxCandidateWordGetFirst(candList))) {
            FcitxMessagesSetMessageCount(FcitxInputStateGetClientPreedit(input), 0);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, "%s", candWord->strWord);
            FcitxInputStateSetClientCursorPos(input, 0);
        }
        else {
            FcitxMessagesSetMessageCount(FcitxInputStateGetClientPreedit(input), 0);
            FcitxMessageType type = MSG_INPUT;
            if (!table->bSendRawPreedit)
                type |= MSG_DONOT_COMMIT_WHEN_UNFOCUS;
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), type, "%s", FcitxInputStateGetRawInputBuffer(input));
            FcitxInputStateSetClientCursorPos(input, 0);
        }
    }

    return retVal;
}

void TableAddAutoCandWord(TableMetaData* table, short which, TABLECANDWORD* tableCandWord)
{
    tableCandWord->flag = CT_AUTOPHRASE;
    tableCandWord->candWord.autoPhrase = &table->tableDict->autoPhrase[which];
}

void TableAddCandWord(RECORD * record, TABLECANDWORD* tableCandWord)
{
    tableCandWord->flag = CT_NORMAL;
    tableCandWord->candWord.record = record;
}

void TableClearFreqByIndex(TableMetaData* table, TABLECANDWORD* tableCandWord)
{
    RECORD         *recTemp;

    recTemp = tableCandWord->candWord.record;
    recTemp->iHit = 0;

    table->tableDict->iTableChanged++;
}

/*
 * 根据序号调整词组顺序，序号从1开始
 * 将指定的字/词调整到同样编码的最前面
 */
void TableAdjustOrderByIndex(TableMetaData* table, TABLECANDWORD* tableCandWord)
{
    RECORD         *recTemp;
    int             iTemp;

    FcitxTableState* tbl = table->owner;

    recTemp = tableCandWord->candWord.record;
    while (!strcmp(recTemp->strCode, recTemp->prev->strCode))
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
        for (iTemp = 0; iTemp < strlen(table->tableDict->strInputCode); iTemp++) {
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
void TableDelPhraseByIndex(TableMetaData* table, TABLECANDWORD* tableCandWord)
{
    if (tableCandWord->flag != CT_NORMAL)
        return;

    if (fcitx_utf8_strlen(tableCandWord->candWord.record->strHZ) <= 1)
        return;

    TableDelPhrase(table->tableDict, tableCandWord->candWord.record);
}

void TableCreateNewPhrase(TableMetaData* table)
{
    int             i;
    FcitxTableState* tbl = table->owner;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);

    FcitxMessagesSetMessageText(FcitxInputStateGetAuxDown(input), 0, "");
    for (i = tbl->iTableNewPhraseHZCount; i > 0; i--)
        FcitxMessagesMessageConcat(FcitxInputStateGetAuxDown(input) , 0 , table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - i].strHZ);

    boolean bCanntFindCode = TableCreatePhraseCode(table->tableDict, FcitxMessagesGetMessageString(FcitxInputStateGetAuxDown(input), 0));

    if (!bCanntFindCode) {
        FcitxMessagesSetMessageCount(FcitxInputStateGetAuxDown(input), 2);
        FcitxMessagesSetMessageText(FcitxInputStateGetAuxDown(input), 1, table->tableDict->strNewPhraseCode);
    } else {
        FcitxMessagesSetMessageCount(FcitxInputStateGetAuxDown(input), 1);
        FcitxMessagesSetMessageText(FcitxInputStateGetAuxDown(input), 0, "????");
    }

}

/*
 * 获取联想候选字列表
 */
INPUT_RETURN_VALUE TableGetRemindCandWords(TableMetaData* table)
{
    FcitxTableState* tbl = table->owner;
    int             iLength;
    RECORD         *tableRemind = NULL;
    FcitxGlobalConfig *config = FcitxInstanceGetGlobalConfig(tbl->owner);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    boolean bDisablePagingInRemind = config->bDisablePagingInRemind;

    if (!tbl->strTableRemindSource[0])
        return IRV_TO_PROCESS;

    FcitxInputStateGetRawInputBuffer(input)[0] = '\0';
    FcitxInputStateSetRawInputBufferSize(input, 0);
    FcitxCandidateWordReset(FcitxInputStateGetCandidateList(input));

    iLength = fcitx_utf8_strlen(tbl->strTableRemindSource);
    tableRemind = table->tableDict->recordHead->next;

    while (tableRemind != table->tableDict->recordHead) {
        if (bDisablePagingInRemind && FcitxCandidateWordGetListSize(FcitxInputStateGetCandidateList(input)) >= FcitxCandidateWordGetPageSize(FcitxInputStateGetCandidateList(input)))
            break;

        if (((iLength + 1) == fcitx_utf8_strlen(tableRemind->strHZ))) {
            if (!fcitx_utf8_strncmp(tableRemind->strHZ, tbl->strTableRemindSource, iLength) && fcitx_utf8_get_nth_char(tableRemind->strHZ, iLength)) {
                TABLECANDWORD* tableCandWord = fcitx_utils_malloc0(sizeof(TABLECANDWORD));
                TableAddRemindCandWord(tableRemind, tableCandWord);
                FcitxCandidateWord candWord;
                candWord.callback = TableGetCandWord;
                candWord.owner = table;
                candWord.priv = tableCandWord;
                candWord.strExtra = NULL;
                candWord.strWord = strdup(tableCandWord->candWord.record->strHZ + strlen(tbl->strTableRemindSource));
                candWord.wordType = MSG_OTHER;
                FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
            }
        }

        tableRemind = tableRemind->next;
    }

    FcitxInstanceCleanInputWindowUp(instance);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Remind:"));
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_INPUT, "%s", tbl->strTableRemindSource);
    FcitxInputStateSetIsInRemind(input, (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input))));

    if (FcitxInputStateGetIsInRemind(input))
        return IRV_DISPLAY_CANDWORDS;
    else
        return IRV_CLEAN;
}

void TableAddRemindCandWord(RECORD * record, TABLECANDWORD* tableCandWord)
{
    tableCandWord->flag = CT_REMIND;
    tableCandWord->candWord.record = record;
}

INPUT_RETURN_VALUE TableGetRemindCandWord(void* arg, TABLECANDWORD* tableCandWord)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState* tbl = table->owner;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    tableCandWord->candWord.record->iHit++;
    strcpy(tbl->strTableRemindSource, tableCandWord->candWord.record->strHZ + strlen(tbl->strTableRemindSource));
    TableGetRemindCandWords(table);

    strcpy(FcitxInputStateGetOutputString(input), tbl->strTableRemindSource);
    return IRV_COMMIT_STRING_REMIND;
}

INPUT_RETURN_VALUE TableGetFHCandWords(TableMetaData* table)
{
    int             i;
    FcitxTableState* tbl = table->owner;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    FcitxInstanceCleanInputWindowUp(instance);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT | MSG_DONOT_COMMIT_WHEN_UNFOCUS, "%s", FcitxInputStateGetRawInputBuffer(input));
    FcitxInputStateSetCursorPos(input, FcitxInputStateGetRawInputBufferSize(input));
    FcitxInputStateSetClientCursorPos(input, 0);

    if (!table->tableDict->iFH)
        return IRV_DISPLAY_MESSAGE;

    for (i = 0; i < table->tableDict->iFH; i++) {
        TABLECANDWORD* tableCandWord = fcitx_utils_malloc0(sizeof(TABLECANDWORD));
        tableCandWord->flag = CT_FH;
        tableCandWord->candWord.iFHIndex = i;
        FcitxCandidateWord candWord;
        candWord.callback = TableGetCandWord;
        candWord.owner = table;
        candWord.priv = tableCandWord;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(table->tableDict->fh[i].strFH);
        candWord.wordType = MSG_OTHER;
        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }
    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetFHCandWord(TableMetaData* table, TABLECANDWORD* tableCandWord)
{
    FcitxTableState* tbl = table->owner;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    strcpy(FcitxInputStateGetOutputString(input), table->tableDict->fh[tableCandWord->candWord.iFHIndex].strFH);
    return IRV_COMMIT_STRING;
}

boolean TablePhraseTips(void *arg)
{
    TableMetaData* table = (TableMetaData*) arg;
    FcitxTableState *tbl = table->owner;
    RECORD         *recTemp = NULL;
    char            strTemp[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1] = "\0", *ps;
    short           i, j;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    if (!table->tableDict->recordHead)
        return false;

    //如果最近输入了一个词组，这个工作就不需要了
    if (FcitxInputStateGetLastIsSingleChar(input) != 1)
        return false;

    j = (table->tableDict->iHZLastInputCount > PHRASE_MAX_LENGTH) ? table->tableDict->iHZLastInputCount - PHRASE_MAX_LENGTH : 0;
    for (i = j; i < table->tableDict->iHZLastInputCount; i++)
        strcat(strTemp, table->tableDict->hzLastInput[i].strHZ);
    //如果只有一个汉字，这个工作也不需要了
    if (fcitx_utf8_strlen(strTemp) < 2)
        return false;

    //首先要判断是不是已经在词库中
    ps = strTemp;
    for (i = 0; i < (table->tableDict->iHZLastInputCount - j - 1); i++) {
        recTemp = TableFindPhrase(table->tableDict, ps);
        if (recTemp) {
            FcitxInstanceCleanInputWindow(instance);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Phrase is already in Dict "));
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_INPUT, "%s", ps);

            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_FIRSTCAND, _("Code is "));
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_CODE, "%s", recTemp->strCode);
            FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, _(" Ctrl+Delete To Delete"));
            tbl->bTablePhraseTips = true;
            FcitxInputStateSetShowCursor(input, false);

            return true;
        }
        ps = ps + fcitx_utf8_char_len(ps);
    }

    return false;
}

void UpdateHZLastInput(TableMetaData* table, char *str)
{
    int             i, j;
    char           *pstr;

    pstr = str;

    for (i = 0; i < fcitx_utf8_strlen(str) ; i++) {
        if (table->tableDict->iHZLastInputCount < PHRASE_MAX_LENGTH)
            table->tableDict->iHZLastInputCount++;
        else {
            for (j = 0; j < (table->tableDict->iHZLastInputCount - 1); j++) {
                strncpy(table->tableDict->hzLastInput[j].strHZ, table->tableDict->hzLastInput[j + 1].strHZ, fcitx_utf8_char_len(table->tableDict->hzLastInput[j + 1].strHZ));
            }
        }
        strncpy(table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - 1].strHZ, pstr, fcitx_utf8_char_len(pstr));
        table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - 1].strHZ[fcitx_utf8_char_len(pstr)] = '\0';
        pstr = pstr + fcitx_utf8_char_len(pstr);
    }

    if (table->tableDict->bRule && table->bAutoPhrase)
        TableCreateAutoPhrase(table, (char)(fcitx_utf8_strlen(str)));
}

void FreeTableConfig(void *v)
{
    TableMetaData *table = (TableMetaData*) v;
    if (!table)
        return;
    FcitxConfigFreeConfigFile(table->config.configFile);
    FcitxHotkeyFree(table->hkAlternativePrevPage);
    FcitxHotkeyFree(table->hkAlternativeNextPage);
    free(table->strPath);
    free(table->strSymbolFile);
    free(table->uniqueName);
    free(table->strName);
    free(table->strIconName);
    free(table->strEndCode);
    free(table->strSymbol);
    free(table->strChoose);
    fcitx_utils_free(table->confName);
}

int TableCandCmp(const void* a, const void* b, void *arg)
{
    TABLECANDWORD* canda = *(TABLECANDWORD**)a;
    TABLECANDWORD* candb = *(TABLECANDWORD**)b;
    TableCandWordSortContext* context = arg;

    switch (context->order) {
    case AD_NO:
        return 0;
    case AD_FAST: {
        int result = strcmp(canda->candWord.record->strCode, candb->candWord.record->strCode);
        if (result != 0)
            return result;
        return candb->candWord.record->iIndex - canda->candWord.record->iIndex;
    }
    break;
    case AD_FREQ: {
        int result = strcmp(canda->candWord.record->strCode, candb->candWord.record->strCode);
        if (result != 0)
            return result;
        return candb->candWord.record->iHit - canda->candWord.record->iHit;
    }
    break;
    }
    return 0;
}

INPUT_RETURN_VALUE TableKeyBlocker(void* arg, FcitxKeySym sym, unsigned int state)
{
    TableMetaData* table = arg;
    FcitxInstance* instance = table->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);

    do {
        if (!table->bCommitAndPassByInvalidKey)
            break;
        if (!FcitxHotkeyIsHotKeySimple(sym, state))
            break;

        if (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input))) {
            FcitxCandidateWord* candWord = FcitxCandidateWordGetCurrentWindow(FcitxInputStateGetCandidateList(input));
            if (candWord->owner != table)
                break;
            TABLECANDWORD* tableCandWord = candWord->priv;
            if (tableCandWord->flag == CT_AUTOPHRASE)
                break;
            INPUT_RETURN_VALUE ret = TableGetCandWord(table, candWord);
            if (!(ret & IRV_FLAG_PENDING_COMMIT_STRING))
                break;
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), FcitxInputStateGetOutputString(input));
        }
        else if (table->bSendRawPreedit) {
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), FcitxInputStateGetRawInputBuffer(input));
        }
        FcitxInputStateSetRawInputBufferSize(input, 0);
        FcitxInputStateGetRawInputBuffer(input)[0] = '\0';
        FcitxInputStateSetIsInRemind(input, false);
        FcitxInstanceCleanInputWindow(instance);
        FcitxUIUpdateInputWindow(instance);
        return IRV_FLAG_FORWARD_KEY;
    } while (0);
    return FcitxStandardKeyBlocker(input, sym, state);
}

/* we should try not to break the existing on even if the config file is not exist anymore */
void ReloadTableConfig(void* arg)
{
    TableMetaData* table = arg;
    size_t len = 0;
    int i = 0;
    char** tablePath = FcitxXDGGetPathWithPrefix(&len, "table");
    LoadTableConfig(&table->owner->config);

    char **paths = fcitx_utils_malloc0(sizeof(char*) * len);
    for (i = 0; i < len ; i ++)
        paths[i] = NULL;

    for (i = len - 1; i >= 0; i--) {
        asprintf(&paths[i], "%s/%s", tablePath[len - i - 1], table->confName);
        FcitxLog(DEBUG, "Load Table Config File:%s", paths[i]);
    }
    FcitxConfigFile* cfile = FcitxConfigParseMultiConfigFile(paths, len, GetTableConfigDesc());
    if (cfile) {
        TableMetaDataConfigBind(table, cfile, GetTableConfigDesc());
        FcitxConfigBindSync((FcitxGenericConfig*)table);

        int iTableIMIndex = utarray_eltidx(table->owner->table, table);
        if (iTableIMIndex == table->owner->iCurrentTableLoaded) {
            UpdateTableMetaData(table);
        }
    }
    for (i = 0; i < len ; i ++) {
        free(paths[i]);
        paths[i] = NULL;
    }

    free(paths);
    FcitxXDGFreePath(tablePath);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
