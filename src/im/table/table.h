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

#define MAX_CODE_LENGTH  30
#define PHRASE_MAX_LENGTH 10
#define FH_MAX_LENGTH  10
#define TABLE_AUTO_SAVE_AFTER 1024
#define AUTO_PHRASE_COUNT 10000
#define SINGLE_HZ_COUNT 66000

struct _FcitxInstance;

typedef struct _RULE_RULE {
    unsigned char   iFlag;	// 1 --> 正序   0 --> 逆序
    unsigned char   iWhich;	//第几个字
    unsigned char   iIndex;	//第几个编码
} RULE_RULE;

typedef struct _RULE {
    unsigned char   iWords;	//多少个字
    unsigned char   iFlag;	//1 --> 大于等于iWords  0 --> 等于iWords
    RULE_RULE      *rule;
} RULE;

typedef struct _TABLE {
    GenericConfig   config;
    char           *strPath;
    char           *strSymbolFile;
    char           *strName;
    char           *strIconName;
    char           *strInputCode;
    unsigned char   iCodeLength;
    unsigned char   iPYCodeLength;
    char           *strEndCode;	//中止键，按下该键相当于输入该键后再按一个空格
    char           *strIgnoreChars;
    char            cMatchingKey;
    char           *strSymbol;
    char            cPinyin;	//输入该键后，表示进入临时拼音状态
    unsigned char   bRule;

    RULE           *rule;	//组词规则
    unsigned int    iRecordCount;
    ADJUSTORDER     tableOrder;

    int             iPriority;
    boolean            bUsePY;	//使用拼音
    int             iTableAutoSendToClient;	//自动上屏
    int             iTableAutoSendToClientWhenNone;	//空码自动上屏
    boolean            bUseMatchingKey;	//是否模糊匹配
    boolean            bAutoPhrase;	//是否自动造词
    int             iSaveAutoPhraseAfter;	//选择N次后保存自动词组，0-不保存，1-立即保存
    boolean            bAutoPhrasePhrase;	//词组是否参与造词
    int             iAutoPhrase;	//自动造词长度
    boolean            bTableExactMatch;	//是否只显示精确匹配的候选字/词
    boolean            bPromptTableCode;	//输入完毕后是否提示编码

    boolean            bHasPinyin;		//标记该码表中是否有拼音
    char           *strChoose;		//设置选择键
    boolean            bEnabled;
} TABLE;

typedef struct _RECORD {
    char           *strCode;
    char           *strHZ;
    struct _RECORD *next;
    struct _RECORD *prev;
    unsigned int    iHit;
    unsigned int    iIndex;
    unsigned int    flag:1;
    unsigned int    bPinyin:1;
} RECORD;

/* 根据键码生成一个简单的索引，指向该键码起始的第一个记录 */
typedef struct _RECORD_INDEX {
    RECORD         *record;
    char            cCode;
} RECORD_INDEX;

typedef struct _FH {
    char            strFH[FH_MAX_LENGTH * 2 + 1];
} FH;

typedef struct _AUTOPHRASE {
    char           *strHZ;
    char           *strCode;
    char            iSelected;
    unsigned int    flag:1;
    struct _AUTOPHRASE *next;	//构造一个队列
} AUTOPHRASE;

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
    
    RECORD         *currentRecord;
    RECORD	       *recordHead;
    
    RECORD         *tableSingleHZ[SINGLE_HZ_COUNT];		//Records the single characters in table to speed auto phrase
    RECORD	       *pCurCandRecord;	//Records current cand word selected, to update the hit-frequency information
    TABLECANDWORD   tableCandWord[MAX_CAND_WORD*2];
    
    RECORD_INDEX   *recordIndex;
    
    AUTOPHRASE     *autoPhrase;
    AUTOPHRASE     *insertPoint;
    
    uint            iAutoPhrase;
    uint            iTableCandDisplayed;
    uint            iTableTotalCandCount;
    char            strTableRemindSource[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1];
    
    FH             *fh;
    int             iFH ;
    unsigned int    iTableIndex;
    
    boolean            bIsTableDelPhrase;
    HOTKEYS         hkTableDelPhrase[HOT_KEY_COUNT];
    boolean            bIsTableAdjustOrder;
    HOTKEYS         hkTableAdjustOrder[HOT_KEY_COUNT];
    boolean            bIsTableAddPhrase;
    HOTKEYS         hkTableAddPhrase[HOT_KEY_COUNT];
    
    int             iTableChanged;
    char            iTableNewPhraseHZCount;
    boolean            bCanntFindCode;	//Records if new phrase has corresponding code - should be always false
    char           *strNewPhraseCode;
    
    SINGLE_HZ       hzLastInput[PHRASE_MAX_LENGTH];	//Records last HZ input
    short           iHZLastInputCount ;
    boolean            bTablePhraseTips;
    
    ADJUSTORDER     PYBaseOrder;
    boolean		    isSavingTableDic;
    
    struct _FcitxInstance* owner;
    struct _FcitxAddon* pyaddon;
} FcitxTableState;

void            LoadTableInfo (FcitxTableState* tbl);
boolean            LoadTableDict (FcitxTableState* tbl);
boolean TableInit (void* arg);
void            FreeTableIM (FcitxTableState* tbl, char i);
void            SaveTableIM (void* arg);
void            SaveTableDict (FcitxTableState* tbl);
boolean            IsInputKey (FcitxTableState* tbl, int iKey);
boolean            IsIgnoreChar (FcitxTableState* tbl, char cChar);
boolean            IsEndKey (FcitxTableState* tbl, char cChar);
char            IsChooseKey (FcitxTableState* tbl, int iKey);

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
boolean            HasMatchingKey (FcitxTableState* tbl);
int             TableCompareCode (FcitxTableState* tbl, char* strUser, char* strDict);
int             TableFindFirstMatchCode (FcitxTableState* tbl);
void            TableAdjustOrderByIndex (FcitxTableState* tbl, int iIndex);
void            TableDelPhraseByIndex (FcitxTableState* tbl, int iIndex);
void            TableDelPhraseByHZ (FcitxTableState* tbl, const char* strHZ);
void            TableDelPhrase (FcitxTableState* tbl, RECORD* record);
RECORD         *TableHasPhrase (FcitxTableState* tbl, const char* strCode, const char* strHZ);
RECORD         *TableFindPhrase (FcitxTableState* tbl, const char* strHZ);
void            TableInsertPhrase (FcitxTableState* tbl, const char* strCode, const char* strHZ);
char	       *_TableGetCandWord (FcitxTableState* tbl, int iIndex, boolean _bRemind);		//Internal
char           *TableGetCandWord (void* arg, int iIndex);
void		TableUpdateHitFrequency (FcitxTableState* tbl, RECORD* record);
void            TableCreateNewPhrase (FcitxTableState* tbl);
void            TableCreatePhraseCode (FcitxTableState* tbl, char* strHZ);
boolean            TablePhraseTips (void* arg);
void            TableSetCandWordsFlag (FcitxTableState* tbl, int iCount, boolean flag);
void            TableResetFlags (FcitxTableState* tbl);

void            TableCreateAutoPhrase (FcitxTableState* tbl, char iCount);

void            UpdateHZLastInput (FcitxTableState* tbl, char* str);

ConfigFileDesc *GetTableConfigDesc();
CONFIG_BINDING_DECLARE(TABLE);

#endif
