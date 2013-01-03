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
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx/candidate.h"
#include "fcitx/context.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/utarray.h"
#include "fcitx-utils/utils.h"
#include "im/pinyin/fcitx-pinyin.h"
#include "module/punc/fcitx-punc.h"
#include "module/pinyin-enhance/fcitx-pinyin-enhance.h"

#define MAX_TABLE_INPUT 50

static void TableMetaDataFree(TableMetaData *table);
typedef struct {
    ADJUSTORDER order;
    int simpleLevel;
} TableCandWordSortContext;

static void *TableCreate(FcitxInstance* instance);
static int TableCandCmp(const void* a, const void* b, void* arg);
static INPUT_RETURN_VALUE TableKeyBlocker(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE Table_PYGetCandWord(void *arg,
                                              FcitxCandidateWord *candidateWord);

FCITX_DEFINE_PLUGIN(fcitx_table, ime2, FcitxIMClass2) =  {
    TableCreate,
    NULL,
    ReloadTableConfig,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static boolean LoadTableConfig(TableConfig* config);
static void SaveTableConfig(TableConfig* config);

static inline unsigned int GetTableMod(TableMetaData* table)
{
    static unsigned int cmodtable[] = {
        FcitxKeyState_None, FcitxKeyState_Alt,
        FcitxKeyState_Ctrl, FcitxKeyState_Shift
    };
    if (fcitx_unlikely(table->chooseModifier >= _CM_COUNT))
        table->chooseModifier = _CM_COUNT - 1;

    return cmodtable[table->chooseModifier];
}

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

void SaveTableConfig(TableConfig *config)
{
    FCITX_UNUSED(config);
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

static inline char* TableMetaDataGetName(TableMetaData* table)
{
    return (strlen(table->uniqueName) == 0) ?
        table->strIconName : table->uniqueName;
}

static inline void TableMetaDataInsert(TableMetaData** tableSet, TableMetaData* table)
{
    char* name = TableMetaDataGetName(table);
    HASH_ADD_KEYPTR(hh, *tableSet, name, strlen(name), table);
}

static inline TableMetaData* TableMetaDataFind(TableMetaData* table, const char* name)
{
    TableMetaData* result = NULL;
    HASH_FIND_STR(table, name, result);
    return result;
}

static inline void TableMetaDataUnlink(TableMetaData** tableSet, TableMetaData* table)
{
    HASH_DEL(*tableSet, table);
}

static inline void TableMetaDataRemove(TableMetaData** tableSet, TableMetaData* table)
{
    HASH_DEL(*tableSet, table);
    TableMetaDataFree(table);
}

static inline void TableMetaDataRegister(FcitxTableState* tbl, TableMetaData* table)
{
    table->status = TABLE_REGISTERED;
    FcitxInstanceRegisterIM(
        tbl->owner,
        table,
        TableMetaDataGetName(table),
        table->strName,
        table->strIconName,
        TableInit,
        TableResetStatus,
        DoTableInput,
        TableGetCandWords,
        TablePhraseTips,
        SaveTableIM,
        NULL,
        TableKeyBlocker,
        table->iPriority,
        table->langCode
    );
}

static inline char* TableConfigStealTableName(FcitxConfigFile* cfile)
{
    FcitxConfigOption* option = FcitxConfigFileGetOption(cfile, "CodeTable",
                                                         "UniqueName");
    if (option && strlen(option->rawValue))
        return option->rawValue;

    option = FcitxConfigFileGetOption(cfile, "CodeTable", "IconName");
    if (option)
        return option->rawValue;
    return NULL;
}

/*
 * Read table configuration
 * and returns whether table im is really changed.
 */
boolean LoadTableInfo(FcitxTableState *tbl)
{
    char **tablePath;
    size_t len;

    FcitxStringHashSet* sset = NULL;
    tbl->bTablePhraseTips = false;
    if (tbl->curLoadedTable) {
        FreeTableDict(tbl->curLoadedTable);
        tbl->curLoadedTable = NULL;
    }

    tablePath = FcitxXDGGetPathWithPrefix(&len, "table");
    sset = FcitxXDGGetFiles("table", NULL, ".conf");

    {
        TableMetaData* titer = tbl->tables;
        while(titer)
        {
            titer->status = TABLE_PENDING;
            titer = titer->hh.next;
        }
    }

    boolean imchanged = false;
    char *paths[len];
    HASH_FOREACH(string, sset, FcitxStringHashSet) {
        int i;
        for (i = len - 1; i >= 0; i--) {
            fcitx_utils_alloc_cat_str(paths[i], tablePath[len - i - 1],
                                      "/", string->name);
            FcitxLog(DEBUG, "Load Table Config File:%s", paths[i]);
        }
        // FcitxLog(INFO, _("Load Table Config File:%s"), string->name);
        FcitxConfigFile *cfile;
        cfile = FcitxConfigParseMultiConfigFile(paths, len,
                                                GetTableConfigDesc());
        if (cfile) {
            do {
                char *tableName = TableConfigStealTableName(cfile);
                boolean needunregister = true;
                if (!tableName)
                    break;
                TableMetaData* t = TableMetaDataFind(tbl->tables, tableName);
                if (!t) {
                    t = fcitx_utils_new(TableMetaData);
                    needunregister = false;
                } else {
                    TableMetaDataUnlink(&tbl->tables, t);
                }
                TableMetaDataConfigBind(t, cfile, GetTableConfigDesc());
                FcitxConfigBindSync((FcitxGenericConfig*)t);
                if (t->bEnabled) {
                    t->confName = strdup(string->name);
                    t->owner = tbl;
                    TableMetaDataInsert(&tbl->tables, t);
                    if (needunregister)
                        t->status = TABLE_REGISTERED;
                    else
                        t->status = TABLE_NEW;
                } else {
                    if (needunregister) {
                        FcitxInstanceUnregisterIM(tbl->owner,
                                                  TableMetaDataGetName(t));
                        imchanged = true;
                    }
                    TableMetaDataFree(t);
                }
            } while(0);
        }
        for (i = len - 1;i >= 0;i--) {
            free(paths[i]);
        }
    }
    FcitxXDGFreePath(tablePath);
    fcitx_utils_free_string_hash_set(sset);

    TableMetaData* titer = tbl->tables;
    while (titer) {
        /*
         * if it's this case, the configuration file may already gone
         * thus let's remove it
         */
        if (titer->status == TABLE_PENDING) {
            TableMetaData* cur = titer;
            FcitxInstanceUnregisterIM(tbl->owner, TableMetaDataGetName(cur));
            TableMetaDataRemove(&tbl->tables, cur);
            imchanged = true;
        } else {
            if (titer->status != TABLE_REGISTERED) {
                // FcitxLog(INFO, "register %s", TableMetaDataGetName(titer));
                TableMetaDataRegister(tbl, titer);
                imchanged = true;
            }
            titer = titer->hh.next;
        }
    }

    tbl->iTableCount = HASH_COUNT(tbl->tables);
    return imchanged;
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
        FcitxInstanceSetContext(tbl->owner, CONTEXT_ALTERNATIVE_PREVPAGE_KEY,
                                table->hkAlternativePrevPage);
        FcitxInstanceSetContext(tbl->owner, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY,
                                table->hkAlternativeNextPage);
    }
    FcitxAddon* pyaddon = FcitxPinyinGetAddon(tbl->owner);
    tbl->pyaddon = pyaddon;
    tbl->PYBaseOrder = AD_FREQ;

    FcitxPinyinReset(tbl->owner);
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
    } else {
        return (FcitxCandidateWordGetListSize(candList) == 0) &&
            TableFindFirstMatchCode(table, code, false, false) == -1;
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
    char *output_str = FcitxInputStateGetOutputString(input);

    FcitxCandidateWordSetChooseAndModifier(candList, table->strChoose,
                                           GetTableMod(table));
    FcitxCandidateWordSetPageSize(candList, config->iMaxCandWord);

    if (table != tbl->curLoadedTable && tbl->curLoadedTable) {
        FreeTableDict(tbl->curLoadedTable);
        tbl->curLoadedTable = NULL;
    }

    if (tbl->curLoadedTable == NULL) {
        if (!LoadTableDict(table)) {
            FcitxInstanceUnregisterIM(instance, TableMetaDataGetName(table));
            FcitxInstanceUpdateIMList(instance);
            return IRV_DONOT_PROCESS;
        }
        tbl->curLoadedTable = table;
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
        if (!tbl->bIsTableAddPhrase && !tbl->bIsTableDelPhrase &&
            !tbl->bIsTableAdjustOrder && !tbl->bIsTableClearFreq) {
            size_t raw_size = FcitxInputStateGetRawInputBufferSize(input);
            if (FcitxHotkeyCheckChooseKeyAndModifier(sym, state,
                                                     table->strChoose,
                                                     GetTableMod(table)) >= 0) {
                size_t len1 = strlen(strCodeInput);
                size_t len = len1 > raw_size ? raw_size : len1;
                char buf[len + 2];
                memcpy(buf, strCodeInput, len);
                buf[len] = (char)sym;
                buf[len + 1] = '\0';
                if (TableFindFirstMatchCode(table, buf, false, false) == -1)
                    return IRV_TO_PROCESS;
            }

            /* check we use Pinyin or Not */
            if (strCodeInput[0] == table->cPinyin && table->bUsePY) {
                if (raw_size != (MAX_PY_LENGTH * 5 + 1)) {
                    strCodeInput[raw_size] = (char) sym;
                    raw_size++;
                    strCodeInput[raw_size] = '\0';
                    FcitxInputStateSetRawInputBufferSize(input, raw_size);
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else {
                    retVal = IRV_DO_NOTHING;
                }
            } else {
                /* length is not too large */
                if (((raw_size < table->tableDict->iCodeLength) ||
                     (table->tableDict->bHasPinyin &&
                      raw_size < table->tableDict->iPYCodeLength) ||
                     (((TableCheckNoMatch(table, FcitxInputStateGetRawInputBuffer(input)) &&
                        table->bNoMatchDontCommit) || !table->bUseAutoSend) &&
                      raw_size >= table->tableDict->iCodeLength)) &&
                    raw_size <= MAX_TABLE_INPUT) {
                    strCodeInput[raw_size] = (char)sym;
                    raw_size++;
                    strCodeInput[raw_size] = '\0';
                    FcitxInputStateSetRawInputBufferSize(input, raw_size);

                    if (raw_size == 1 && strCodeInput[0] == table->cPinyin &&
                        table->bUsePY) {
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
                                    strLastFirstCand = output_str;
                            }
                        }

                        retVal = TableGetCandWords(table);
                        int key = FcitxInputStateGetRawInputBuffer(input)[0];
                        if (!table->bIgnorePunc) {
                            strTemp = FcitxPuncGetPunc(instance, &key);
                        } else {
                            strTemp = NULL;
                        }
                        if (IsEndKey(table, sym)) {
                            if (raw_size == 1)
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
                                   && (raw_size >= (table->iTableAutoSendToClientWhenNone + 1))
                                   && TableCheckNoMatch(table, FcitxInputStateGetRawInputBuffer(input))) {
                            if (strLastFirstCand && (lastFirstCandType != CT_AUTOPHRASE)) {
                                FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), strLastFirstCand);
                            } else if (table->bSendRawPreedit) {
                                strCodeInput[raw_size - 1] = '\0';
                                FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), strCodeInput);
                            }
                            retVal = IRV_DISPLAY_CANDWORDS;
                            FcitxInputStateSetRawInputBufferSize(input, 1);
                            strCodeInput[0] = sym;
                            strCodeInput[1] = '\0';
                        } else if ((raw_size == 1) && strTemp &&
                                   lastPageCount == 0) {
                            /**
                             * 如果第一个字母是标点，并且没有候选字/词
                             * 则当做标点处理──适用于二笔这样的输入
                             **/
                            strcpy(output_str, strTemp);
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
                                        FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), output_str);
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
        FcitxMessages *msg_up = FcitxInputStateGetAuxUp(input);
        FcitxMessages *msg_down = FcitxInputStateGetAuxDown(input);
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
                if (strcmp("????", FcitxMessagesGetMessageString(msg_down, 0)))
                    TableInsertPhrase(table->tableDict, FcitxMessagesGetMessageString(msg_down, 1), FcitxMessagesGetMessageString(msg_down, 0));
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
                FcitxMessagesAddMessageStringsAtLast(msg_up, MSG_TIPS, _("Left/Right to choose selected character, Press Enter to confirm, Press Escape to Cancel"));

                FcitxMessagesAddMessageStringsAtLast(msg_down,
                                                     MSG_FIRSTCAND, "");
                FcitxMessagesAddMessageStringsAtLast(msg_down, MSG_CODE, "");
                TableCreateNewPhrase(table);
                retVal = IRV_DISPLAY_MESSAGE;
            } else
                retVal = IRV_TO_PROCESS;

            return retVal;
        } else if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkLookupPinyin) && tbl->pyaddon) {
            char strPY[128];

            //如果刚刚输入的是个词组，刚不查拼音
            if (fcitx_utf8_strlen(output_str) != 1)
                return IRV_DO_NOTHING;

            FcitxInputStateSetRawInputBufferSize(input, 0);

            FcitxInstanceCleanInputWindow(instance);
            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input),
                                                 MSG_INPUT, output_str);

            FcitxMessagesAddMessageStringsAtLast(
                FcitxInputStateGetAuxDown(input), MSG_CODE, _("Pinyin: "));
            const int8_t *py_list;
            if ((py_list = FcitxPinyinEnhanceFindPy(tbl->owner, output_str))
                && *py_list) {
                int8_t i;
                int offset = 0;
                for (i = 0;i < *py_list && offset + 16 < sizeof(strPY);i++) {
                    int len;
                    const int8_t *py;
                    if (i > 0) {
                        memcpy(strPY + offset, ", ", 2);
                        offset += 2;
                    }
                    py = pinyin_enhance_pylist_get(py_list, i);
                    FcitxPinyinEnhancePyToString(tbl->owner, strPY + offset,
                                                 py, &len);
                    offset += len;
                }
            } else {
                FcitxPinyinLoadBaseDict(tbl->owner);
                FcitxPinyinGetPyByHZ(tbl->owner, output_str, strPY);
            }
            FcitxMessagesAddMessageStringsAtLast(
                FcitxInputStateGetAuxDown(input), MSG_TIPS,
                (strPY[0]) ? strPY : _("Cannot found Pinyin"));
            FcitxInputStateSetShowCursor(input, false);

            return IRV_DISPLAY_MESSAGE;
        }

        if (!FcitxInputStateGetRawInputBufferSize(input) && !FcitxInputStateGetIsInRemind(input))
            return IRV_TO_PROCESS;

        if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
            if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder || tbl->bIsTableClearFreq) {
                TableResetStatus(table);
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageStringsAtLast(
                    FcitxInputStateGetPreedit(input), MSG_INPUT,
                    FcitxInputStateGetRawInputBuffer(input));
                FcitxMessagesAddMessageStringsAtLast(
                    FcitxInputStateGetClientPreedit(input),
                    MSG_INPUT | MSG_DONOT_COMMIT_WHEN_UNFOCUS,
                    FcitxInputStateGetRawInputBuffer(input));
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
                    FcitxInputStateSetIsDoInputOnly(input, false);
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else if (candWord->owner == table && tbl->bIsTableAdjustOrder) {
                    TableAdjustOrderByIndex(table, candWord->priv);
                    tbl->bIsTableAdjustOrder = false;
                    FcitxInputStateSetIsDoInputOnly(input, false);
                    retVal = IRV_DISPLAY_CANDWORDS;
                } else if (candWord->owner == table && tbl->bIsTableClearFreq) {
                    TableClearFreqByIndex(table, candWord->priv);
                    tbl->bIsTableClearFreq = false;
                    FcitxInputStateSetIsDoInputOnly(input, false);
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
                FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Choose the phrase to be put in the front, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            } else if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkTableDelPhrase)) {
                if (!FcitxCandidateWordPageCount(candList) || FcitxInputStateGetIsInRemind(input))
                    return IRV_DO_NOTHING;

                tbl->bIsTableDelPhrase = true;
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Choose the phrase to be deleted, Press Escape to Cancel"));
                retVal = IRV_DISPLAY_MESSAGE;
            } else if (FcitxHotkeyIsHotKey(sym, state, tbl->config.hkTableClearFreq)) {
                if (!FcitxCandidateWordPageCount(candList) || FcitxInputStateGetIsInRemind(input))
                    return IRV_DO_NOTHING;

                tbl->bIsTableClearFreq = true;
                FcitxInstanceCleanInputWindowUp(instance);
                FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, _("Choose the phrase to clear typing history, Press Escape to Cancel"));
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
            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input), MSG_INPUT, FcitxInputStateGetRawInputBuffer(input));

            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxDown(input), MSG_TIPS, pCandWord);
            temp = table->tableDict->tableSingleHZ[CalHZIndex(pCandWord)];
            if (temp) {
                FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxDown(input), MSG_CODE, temp->strCode);
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

    if (!tbl->pyaddon)
        return IRV_DISPLAY_CANDWORDS;

    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    strcpy(FcitxPinyinGetFindString(tbl->owner),
           FcitxInputStateGetRawInputBuffer(input) + 1);

    FcitxKeySym dummy1 = FcitxKey_None;
    unsigned int dummy2 = 0;
    FcitxPinyinDoInput(tbl->owner, &dummy1, &dummy2);
    FcitxPinyinGetCandwords(tbl->owner);

    FcitxInputStateGetRawInputBuffer(input)[0] = table->cPinyin;
    FcitxInputStateGetRawInputBuffer(input)[1] = '\0';

    strcat(FcitxInputStateGetRawInputBuffer(input),
           FcitxPinyinGetFindString(tbl->owner));
    FcitxInputStateSetRawInputBufferSize(input, strlen(FcitxInputStateGetRawInputBuffer(input)));

    FcitxInstanceCleanInputWindowUp(instance);
    FcitxMessagesAddMessageStringsAtLast(
        FcitxInputStateGetPreedit(input), MSG_INPUT,
        FcitxInputStateGetRawInputBuffer(input));
    FcitxMessagesAddMessageStringsAtLast(
        FcitxInputStateGetClientPreedit(input),
        MSG_INPUT | MSG_DONOT_COMMIT_WHEN_UNFOCUS,
        FcitxInputStateGetRawInputBuffer(input));
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

    if (FcitxInputStateGetRawInputBuffer(input)[0] == table->cPinyin && table->bUsePY && tbl->pyaddon)
        return TableGetPinyinCandWords(table);

    if (TableFindFirstMatchCode(table, FcitxInputStateGetRawInputBuffer(input), table->bTableExactMatch, true) == -1 && !table->tableDict->iAutoPhrase) {
        if (FcitxInputStateGetRawInputBufferSize(input)) {
            FcitxMessagesSetMessageCount(FcitxInputStateGetPreedit(input), 0);
            FcitxMessagesSetMessageCount(FcitxInputStateGetClientPreedit(input), 0);
            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, FcitxInputStateGetRawInputBuffer(input));
            FcitxMessageType type = MSG_INPUT;
            if (!table->bSendRawPreedit)
                type |= MSG_DONOT_COMMIT_WHEN_UNFOCUS;
            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetClientPreedit(input), type, FcitxInputStateGetRawInputBuffer(input));
            FcitxInputStateSetCursorPos(input, strlen(FcitxInputStateGetRawInputBuffer(input)));
            FcitxInputStateSetClientCursorPos(input, 0);
        }
        //Not Found
        return IRV_DISPLAY_CANDWORDS;
    }

    TableCandWordSortContext context;
    context.order = table->tableOrder;
    context.simpleLevel = table->iSimpleLevel;
    UT_array candTemp;
    utarray_init(&candTemp, fcitx_ptr_icd);

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
        FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, FcitxInputStateGetRawInputBuffer(input));
        FcitxInputStateSetCursorPos(input, strlen(FcitxInputStateGetRawInputBuffer(input)));
        FcitxCandidateWord* candWord = NULL;
        if (table->bFirstCandidateAsPreedit && (candWord = FcitxCandidateWordGetFirst(candList))) {
            FcitxMessagesSetMessageCount(FcitxInputStateGetClientPreedit(input), 0);
            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT, candWord->strWord);
            FcitxInputStateSetClientCursorPos(input, 0);
        }
        else {
            FcitxMessagesSetMessageCount(FcitxInputStateGetClientPreedit(input), 0);
            FcitxMessageType type = MSG_INPUT;
            if (!table->bSendRawPreedit)
                type |= MSG_DONOT_COMMIT_WHEN_UNFOCUS;
            FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetClientPreedit(input), type, FcitxInputStateGetRawInputBuffer(input));
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
        size_t tmp_len = strlen(table->tableDict->strInputCode);
        for (iTemp = 0; iTemp < tmp_len; iTemp++) {
            if (table->tableDict->recordIndex[iTemp].cCode ==
                tableCandWord->candWord.record->strCode[0]) {
                table->tableDict->recordIndex[iTemp].record = tableCandWord->candWord.record;
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
    FcitxMessages *msg_down = FcitxInputStateGetAuxDown(input);

    FcitxMessagesSetMessageTextStrings(msg_down, 0, "");
    for (i = tbl->iTableNewPhraseHZCount; i > 0; i--)
        FcitxMessagesMessageConcat(msg_down, 0, table->tableDict->hzLastInput[table->tableDict->iHZLastInputCount - i].strHZ);

    boolean bCanntFindCode = TableCreatePhraseCode(table->tableDict, FcitxMessagesGetMessageString(msg_down, 0));

    if (!bCanntFindCode) {
        FcitxMessagesSetMessageCount(msg_down, 2);
        FcitxMessagesSetMessageTextStrings(msg_down, 1,
                                           table->tableDict->strNewPhraseCode);
    } else {
        FcitxMessagesSetMessageCount(msg_down, 1);
        FcitxMessagesSetMessageTextStrings(msg_down, 0, "????");
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
    FcitxCandidateWordList *cand_list = FcitxInputStateGetCandidateList(input);

    if (!tbl->strTableRemindSource[0])
        return IRV_TO_PROCESS;

    FcitxInputStateGetRawInputBuffer(input)[0] = '\0';
    FcitxInputStateSetRawInputBufferSize(input, 0);
    FcitxCandidateWordReset(cand_list);

    iLength = fcitx_utf8_strlen(tbl->strTableRemindSource);
    tableRemind = table->tableDict->recordHead->next;

    while (tableRemind != table->tableDict->recordHead) {
        if (bDisablePagingInRemind &&
            FcitxCandidateWordGetListSize(cand_list) >=
            FcitxCandidateWordGetPageSize(cand_list))
            break;

        if (((iLength + 1) == fcitx_utf8_strlen(tableRemind->strHZ))) {
            if (!fcitx_utf8_strncmp(tableRemind->strHZ,
                                    tbl->strTableRemindSource, iLength) &&
                fcitx_utf8_get_nth_char(tableRemind->strHZ, iLength)) {
                TABLECANDWORD *tableCandWord = fcitx_utils_new(TABLECANDWORD);
                TableAddRemindCandWord(tableRemind, tableCandWord);
                FcitxCandidateWord candWord;
                candWord.callback = TableGetCandWord;
                candWord.owner = table;
                candWord.priv = tableCandWord;
                candWord.strExtra = NULL;
                candWord.strWord = strdup(tableCandWord->candWord.record->strHZ + strlen(tbl->strTableRemindSource));
                candWord.wordType = MSG_OTHER;
                FcitxCandidateWordAppend(cand_list, &candWord);
            }
        }
        tableRemind = tableRemind->next;
    }

    FcitxInstanceCleanInputWindowUp(instance);
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input),
                                         MSG_TIPS, _("Remind:"));
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetAuxUp(input),
                                         MSG_INPUT, tbl->strTableRemindSource);
    int count = FcitxCandidateWordPageCount(cand_list);
    FcitxInputStateSetIsInRemind(input, count);
    if (count) {
        return IRV_DISPLAY_CANDWORDS;
    } else {
        return IRV_CLEAN;
    }
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
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, FcitxInputStateGetRawInputBuffer(input));
    FcitxMessagesAddMessageStringsAtLast(FcitxInputStateGetClientPreedit(input), MSG_INPUT | MSG_DONOT_COMMIT_WHEN_UNFOCUS, FcitxInputStateGetRawInputBuffer(input));
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
    RECORD *recTemp = NULL;
    char strTemp[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1] = "", *ps;
    short i, j;
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
    FcitxMessages *msg_up = FcitxInputStateGetAuxUp(input);
    FcitxMessages *msg_down = FcitxInputStateGetAuxDown(input);
    for (i = 0; i < (table->tableDict->iHZLastInputCount - j - 1); i++) {
        recTemp = TableFindPhrase(table->tableDict, ps);
        if (recTemp) {
            FcitxInstanceCleanInputWindow(instance);
            FcitxMessagesAddMessageStringsAtLast(
                msg_up, MSG_TIPS, _("Phrase is already in Dict "));
            FcitxMessagesAddMessageStringsAtLast(msg_up, MSG_INPUT, ps);

            FcitxMessagesAddMessageStringsAtLast(msg_down, MSG_FIRSTCAND,
                                                 _("Code is "));
            FcitxMessagesAddMessageStringsAtLast(msg_down, MSG_CODE,
                                                 recTemp->strCode);
            FcitxMessagesAddMessageStringsAtLast(msg_down, MSG_TIPS,
                                                 _(" Ctrl+Delete To Delete"));
            tbl->bTablePhraseTips = true;
            FcitxInputStateSetShowCursor(input, false);

            return true;
        }
        ps = ps + fcitx_utf8_char_len(ps);
    }

    return false;
}

void UpdateHZLastInput(TableMetaData* table, const char *str)
{
    unsigned int i, j;
    unsigned int str_len = fcitx_utf8_strlen(str);
    TableDict *const tableDict = table->tableDict;
    SINGLE_HZ *const hzLastInput = tableDict->hzLastInput;

    for (i = 0;i < str_len;i++) {
        if (tableDict->iHZLastInputCount < PHRASE_MAX_LENGTH) {
            tableDict->iHZLastInputCount++;
        } else {
            for (j = 0;j < (tableDict->iHZLastInputCount - 1);j++) {
                strncpy(hzLastInput[j].strHZ, hzLastInput[j + 1].strHZ,
                        fcitx_utf8_char_len(hzLastInput[j + 1].strHZ));
            }
        }
        unsigned int char_len = fcitx_utf8_char_len(str);
        strncpy(hzLastInput[tableDict->iHZLastInputCount - 1].strHZ,
                str, char_len);
        hzLastInput[tableDict->iHZLastInputCount - 1].strHZ[char_len] = '\0';
        str += char_len;
    }

    if (tableDict->bRule && table->bAutoPhrase) {
        TableCreateAutoPhrase(table, (char)(str_len));
    }
}

void TableMetaDataFree(TableMetaData *table)
{
    if (!table)
        return;
    FcitxConfigFree(&table->config);
    fcitx_utils_free(table->confName);
    free(table);
}

int TableCandCmp(const void* a, const void* b, void *arg)
{
    TABLECANDWORD* canda = *(TABLECANDWORD**)a;
    TABLECANDWORD* candb = *(TABLECANDWORD**)b;
    TableCandWordSortContext* context = arg;

    if (context->simpleLevel > 0) {
        size_t lengthA = strlen(canda->candWord.record->strCode);
        size_t lengthB = strlen(candb->candWord.record->strCode);

        if (lengthA <= context->simpleLevel && lengthB <= context->simpleLevel) {
            /* we use msort which is stable, so it doesn't matter */
            return 0;
        }
        if (lengthA > context->simpleLevel && lengthB <= context->simpleLevel) {
            return 1;
        }
        if (lengthA <= context->simpleLevel && lengthB > context->simpleLevel) {
            return -1;
        }
    }

    switch (context->order) {
    case AD_NO:
        /* actually this is dead code, since AD_NO doesn't sort at all */
        return 0;
    case AD_FAST: {
        int result = strcmp(canda->candWord.record->strCode,
                            candb->candWord.record->strCode);
        if (result != 0)
            return result;
        return candb->candWord.record->iIndex - canda->candWord.record->iIndex;
    }
    case AD_FREQ: {
        int result = strcmp(canda->candWord.record->strCode,
                            candb->candWord.record->strCode);
        if (result != 0)
            return result;
        return candb->candWord.record->iHit - canda->candWord.record->iHit;
    }
    }
    return 0;
}

INPUT_RETURN_VALUE TableKeyBlocker(void* arg, FcitxKeySym sym, unsigned int state)
{
    TableMetaData *table = arg;
    FcitxInstance *instance = table->owner->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    do {
        if (!table->bCommitAndPassByInvalidKey)
            break;
        if (!FcitxHotkeyIsHotKeySimple(sym, state))
            break;
        FcitxCandidateWordList *cand_list;
        cand_list = FcitxInputStateGetCandidateList(input);
        if (FcitxCandidateWordPageCount(cand_list)) {
            FcitxCandidateWord *candWord;
            candWord = FcitxCandidateWordGetCurrentWindow(cand_list);
            if (candWord->owner != table)
                break;
            TABLECANDWORD* tableCandWord = candWord->priv;
            if (tableCandWord->flag == CT_AUTOPHRASE)
                break;
            INPUT_RETURN_VALUE ret = TableGetCandWord(table, candWord);
            if (!(ret & IRV_FLAG_PENDING_COMMIT_STRING))
                break;
            FcitxInstanceCommitString(
                instance, FcitxInstanceGetCurrentIC(instance),
                FcitxInputStateGetOutputString(input));
        } else if (table->bSendRawPreedit) {
            FcitxInstanceCommitString(
                instance, FcitxInstanceGetCurrentIC(instance),
                FcitxInputStateGetRawInputBuffer(input));
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

void ReloadTableConfig(void* arg)
{
    FcitxTableState* tbl = arg;
    LoadTableConfig(&tbl->config);
    if (LoadTableInfo(tbl))
        FcitxInstanceUpdateIMList(tbl->owner);
}

INPUT_RETURN_VALUE
Table_PYGetCandWord(void* arg, FcitxCandidateWord* candidateWord)
{
    TableMetaData* table = arg;
    FcitxTableState* tbl = table->owner;
    INPUT_RETURN_VALUE retVal = tbl->pygetcandword(tbl->pyaddon->addonInstance,
                                                   candidateWord);
    FcitxPinyinReset(tbl->owner);
    FcitxInputState *state = FcitxInstanceGetInputState(tbl->owner);
    if (!(retVal & IRV_FLAG_PENDING_COMMIT_STRING)) {
        strcpy(FcitxInputStateGetOutputString(state), candidateWord->strWord);
    }

    return IRV_COMMIT_STRING | IRV_FLAG_RESET_INPUT;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
