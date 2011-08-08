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
#ifndef _TABLE_H
#define _TABLE_H

#include "fcitx/configfile.h"
#include "fcitx/ime.h"
#include "fcitx-utils/utarray.h"
#include "tabledict.h"

struct _FcitxInstance;

//用union就会出错，不知道是啥原因
typedef struct _CANDWORD {
    AUTOPHRASE     *autoPhrase;
    RECORD         *record;
    char            strPYPhrase[PHRASE_MAX_LENGTH * 2 + 1];
} CANDWORD;

typedef struct _TABLECANDWORD {
    unsigned int    flag:2;	//指示该候选字/词是自动组的词还是正常的字/词
    CANDWORD        candWord;
} TABLECANDWORD;

typedef enum _CANDTYPE {
    CT_NORMAL = 0,
    CT_AUTOPHRASE,
    CT_PYPHRASE			//临时拼音转换过来的候选字/词
} CANDTYPE;

typedef struct _FcitxTableState {
    UT_array* table; /* 码表 */
    
    char            iTableIMIndex;
    char            iTableCount;
    
    char            iCurrentTableLoaded;
    RECORD	       *pCurCandRecord;	//Records current cand word selected, to update the hit-frequency information
    TABLECANDWORD   tableCandWord[MAX_CAND_WORD*2];
    
    RECORD_INDEX   *recordIndex;
    
    unsigned int            iTableCandDisplayed;
    unsigned int            iTableTotalCandCount;
    char            strTableRemindSource[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1];
        
    boolean            bIsTableDelPhrase;
    HOTKEYS         hkTableDelPhrase[HOT_KEY_COUNT];
    boolean            bIsTableAdjustOrder;
    HOTKEYS         hkTableAdjustOrder[HOT_KEY_COUNT];
    boolean            bIsTableAddPhrase;
    HOTKEYS         hkTableAddPhrase[HOT_KEY_COUNT];
    
    char            iTableNewPhraseHZCount;
    char           *strNewPhraseCode;
    
    boolean            bTablePhraseTips;
    
    ADJUSTORDER     PYBaseOrder;
    boolean		    isSavingTableDic;
    
    struct _FcitxInstance* owner;
    struct _FcitxAddon* pyaddon;
} FcitxTableState;

void            LoadTableInfo (FcitxTableState* tbl);
boolean TableInit (void* arg);
void            FreeTableIM (FcitxTableState* tbl, char i);
void            SaveTableIM (void* arg);

INPUT_RETURN_VALUE DoTableInput (void* arg, FcitxKeySym sym, unsigned int state);
INPUT_RETURN_VALUE TableGetCandWords (void* arg, SEARCH_MODE mode);
void            TableAddCandWord (FcitxTableState* tbl, RECORD* record, SEARCH_MODE mode);
void            TableAddAutoCandWord (FcitxTableState* tbl, short int which, SEARCH_MODE mode);
INPUT_RETURN_VALUE TableGetRemindCandWords (FcitxTableState* tbl, SEARCH_MODE mode);
void            TableAddRemindCandWord (FcitxTableState* tbl, RECORD* record, SEARCH_MODE mode);
INPUT_RETURN_VALUE TableGetFHCandWords (FcitxTableState* tbl, SEARCH_MODE mode);
INPUT_RETURN_VALUE TableGetPinyinCandWords (FcitxTableState* tbl, SEARCH_MODE mode);
void            TableResetStatus (void* arg);
char           *TableGetRemindCandWord (void* arg, int iIndex);
char           *TableGetFHCandWord (FcitxTableState* tbl, int iIndex);
void            TableAdjustOrderByIndex (FcitxTableState* tbl, int iIndex);
void            TableDelPhraseByIndex (FcitxTableState* tbl, int iIndex);
void            TableCreateNewPhrase (FcitxTableState* tbl);
char           *_TableGetCandWord (FcitxTableState* tbl, int iIndex, boolean _bRemind);		//Internal
char           *TableGetCandWord (void* arg, int iIndex);
boolean            TablePhraseTips (void* arg);
void            TableSetCandWordsFlag (FcitxTableState* tbl, int iCount, boolean flag);

void            UpdateHZLastInput (FcitxTableState* tbl, char* str);

ConfigFileDesc *GetTableConfigDesc();

#endif

