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

#include <X11/Xlib.h>
#include <limits.h>

#include "ime.h"

#define TABLE_CONFIG_FILENAME "tables.conf"

#define MAX_CODE_LENGTH  12
#define PHRASE_MAX_LENGTH 10
#define FH_MAX_LENGTH  10
#define TABLE_AUTO_SAVE_AFTER 1024

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
    char            strPath[PATH_MAX];
    char            strSymbolFile[PATH_MAX];
    char            strName[MAX_IM_NAME + 1];
    char           *strInputCode;
    unsigned char   iCodeLength;
    char           *strIgnoreChars;
    char            cMatchingKey;
    char            strSymbol[MAX_CODE_LENGTH + 1];
    char            cPinyin;
    unsigned char   bRule;

    RULE           *rule;	//组词规则
    INT8            iIMIndex;	//记录该码表对应于输入法的顺序
    unsigned int    iRecordCount;
    ADJUSTORDER     tableOrder;

    Bool            bUsePY;			//使用拼音
    Bool            bTableAutoSendToClient;	//自动上屏
    Bool            bUseMatchingKey;		//是否模糊匹配
    Bool            bAutoPhrase;		//是否自动造词
    INT8            iSaveAutoPhraseAfter;	//选择N次后保存自动词组，0-不保存，1-立即保存
    Bool            bAutoPhrasePhrase;		//词组是否参与造词
    INT8            iAutoPhrase;		//自动造词长度
    Bool            bTableExactMatch;		//是否只显示精确匹配的候选字/词
    Bool            bPromptTableCode;		//输入完毕后是否提示编码
} TABLE;

typedef struct _RECORD {
    char           *strCode;
    char           *strHZ;
    struct _RECORD *next;
    struct _RECORD *prev;
    unsigned int    iHit;
    unsigned int    iIndex;
    unsigned int    flag:1;
} RECORD;

/* 根据键码生成一个简单的索引，指向该键码起始的第一个记录本 */
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
    INT8            iSelected;
    unsigned int    flag:1;
    struct _AUTOPHRASE *next;	//构造一个队列
} AUTOPHRASE;

typedef union {
    AUTOPHRASE     *autoPhrase;
    RECORD         *record;
} CANDWORD;

typedef struct _TABLECANDWORD {
    unsigned int    flag:1;	//指示该候选字/词是自动组的词还是正常的字/词
    CANDWORD        candWord;
} TABLECANDWORD;

void            LoadTableInfo (void);
Bool            LoadTableDict (void);
void            TableInit (void);
void            FreeTableIM (void);
void            SaveTableDict (void);
Bool            IsInputKey (int iKey);
Bool            IsIgnoreChar (char cChar);
INPUT_RETURN_VALUE DoTableInput (int iKey);
INPUT_RETURN_VALUE TableGetCandWords (SEARCH_MODE mode);
void            TableAddCandWord (RECORD * wbRecord, SEARCH_MODE mode);
void            TableAddAutoCandWord (INT16 which, SEARCH_MODE mode);
INPUT_RETURN_VALUE TableGetLegendCandWords (SEARCH_MODE mode);
void            TableAddLegendCandWord (RECORD * record, SEARCH_MODE mode);
INPUT_RETURN_VALUE TableGetFHCandWords (SEARCH_MODE mode);
INPUT_RETURN_VALUE TableGetPinyinCandWords (SEARCH_MODE mode);
void            TableResetStatus (void);
char           *TableGetLegendCandWord (int iIndex);
char           *TableGetFHCandWord (int iIndex);
Bool            HasMatchingKey (void);
int             TableCompareCode (char *strUser, char *strDict);
int             TableFindFirstMatchCode (void);
RECORD         *TableFindCode (char *strHZ, Bool bMode);
void            TableAdjustOrderByIndex (int iIndex);
void            TableDelPhraseByIndex (int iIndex);
void            TableDelPhraseByHZ (char *strHZ);
void            TableDelPhrase (RECORD * record);
RECORD         *TableHasPhrase (char *strCode, char *strHZ);
Bool            TableCandHasPhrase (char *strHZ);
RECORD         *TableFindPhrase (char *strHZ);
void            TableInsertPhrase (char *strCode, char *strHZ);
char           *TableGetCandWord (int iIndex);
void            TableCreateNewPhrase (void);
void            TableCreatePhraseCode (char *strHZ);
Bool            TablePhraseTips (void);
void            TableSetCandWordsFlag (int iCount, Bool flag);
void            TableResetFlags (void);

void            TableCreateAutoPhrase (INT8 iCount);

void            UpdateHZLastInput (char *);

#endif
