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
#ifndef _TABLE_H
#define _TABLE_H

#include "fcitx/configfile.h"
#include "fcitx/ime.h"
#include "fcitx-utils/utarray.h"
#include "tabledict.h"
#include "fcitx/candidate.h"
#include "fcitx/instance.h"

typedef union {
    AUTOPHRASE     *autoPhrase;
    RECORD         *record;
    int             iFHIndex;
} CANDWORD;

typedef enum {
    CT_NORMAL = 0,
    CT_AUTOPHRASE,
    CT_REMIND,
    CT_FH
} CANDTYPE;

typedef struct {
    CANDTYPE        flag;   //指示该候选字/词是自动组的词还是正常的字/词
    CANDWORD        candWord;
} TABLECANDWORD;

typedef struct {
    FcitxGenericConfig   config;
    FcitxHotkey     hkTableDelPhrase[HOT_KEY_COUNT];
    FcitxHotkey     hkTableAdjustOrder[HOT_KEY_COUNT];
    FcitxHotkey     hkTableAddPhrase[HOT_KEY_COUNT];
    FcitxHotkey     hkTableClearFreq[HOT_KEY_COUNT];
    FcitxHotkey     hkLookupPinyin[HOT_KEY_COUNT];
} TableConfig;

typedef struct _FcitxTableState {
    TableMetaData* tables; /* 码表 */

    TableConfig config;

    char            iTableCount;

    TableMetaData* curLoadedTable;
    RECORD         *pCurCandRecord; //Records current cand word selected, to update the hit-frequency information

    char            strTableRemindSource[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1];

    boolean         bIsTableDelPhrase;
    boolean         bIsTableAdjustOrder;
    boolean         bIsTableAddPhrase;
    boolean         bIsTableClearFreq;

    char            iTableNewPhraseHZCount;

    boolean            bTablePhraseTips;

    ADJUSTORDER     PYBaseOrder;
    boolean         isSavingTableDic;

    FcitxInstance *owner;
    FcitxAddon *pyaddon;
    FcitxCandidateWordCommitCallback pygetcandword;
} FcitxTableState;

boolean         LoadTableInfo(FcitxTableState* tbl);
boolean         TableInit(void* arg);
void            SaveTableIM(void* arg);
void            ReloadTableConfig(void* arg);

INPUT_RETURN_VALUE DoTableInput(void* arg, FcitxKeySym sym, unsigned int state);
INPUT_RETURN_VALUE TableGetCandWords(void* arg);
void               TableAddCandWord(RECORD * record, TABLECANDWORD* tableCandWord);
void               TableAddAutoCandWord(TableMetaData* table, short int which, TABLECANDWORD* tableCandWord);
INPUT_RETURN_VALUE TableGetRemindCandWords(TableMetaData* table);
void               TableAddRemindCandWord(RECORD * record, TABLECANDWORD* tableCandWord);
INPUT_RETURN_VALUE TableGetFHCandWords(TableMetaData* table);
INPUT_RETURN_VALUE TableGetPinyinCandWords(TableMetaData* table);
void               TableResetStatus(void* arg);
INPUT_RETURN_VALUE TableGetRemindCandWord(void* arg, TABLECANDWORD* tableCandWord);
INPUT_RETURN_VALUE TableGetFHCandWord(TableMetaData* table, TABLECANDWORD* tableCandWord);
void               TableAdjustOrderByIndex(TableMetaData* table, TABLECANDWORD* tableCandWord);
void               TableClearFreqByIndex(TableMetaData* table, TABLECANDWORD* tableCandWord);
void               TableDelPhraseByIndex(TableMetaData* table, TABLECANDWORD* tableCandWord);
void               TableCreateNewPhrase(TableMetaData* table);
INPUT_RETURN_VALUE _TableGetCandWord(TableMetaData* table, TABLECANDWORD* tableCandWord, boolean _bRemind);
INPUT_RETURN_VALUE TableGetCandWord(void* arg, FcitxCandidateWord* candWord);
boolean            TablePhraseTips(void* arg);

void UpdateHZLastInput(TableMetaData* table, const char* str);

FcitxConfigFileDesc *GetTableConfigDesc();
FcitxConfigFileDesc *GetTableGlobalConfigDesc();
CONFIG_BINDING_DECLARE(TableConfig);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
