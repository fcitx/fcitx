#ifndef TABLEDICT_H
#define TABLEDICT_H

#include <fcitx-utils/utf8.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/hotkey.h>
#include <fcitx-utils/memory.h>

#define MAX_CODE_LENGTH  30
#define PHRASE_MAX_LENGTH 10
#define FH_MAX_LENGTH  10
#define TABLE_AUTO_SAVE_AFTER 1024
#define AUTO_PHRASE_COUNT 10000
#define SINGLE_HZ_COUNT 66000

#define RECORDTYPE_NORMAL 0x0
#define RECORDTYPE_PINYIN 0x1
#define RECORDTYPE_CONSTRUCT 0x2
#define RECORDTYPE_PROMPT 0x3

struct _FcitxTableState;

typedef enum _ADJUSTORDER {
    AD_NO = 0,
    AD_FAST = 1,
    AD_FREQ = 2
} ADJUSTORDER;

typedef struct _FH {
    char            strFH[FH_MAX_LENGTH * 2 + 1];
} FH;

typedef struct _RULE_RULE {
    unsigned char   iFlag;  // 1 --> 正序   0 --> 逆序
    unsigned char   iWhich; //第几个字
    unsigned char   iIndex; //第几个编码
} RULE_RULE;

typedef struct _RULE {
    unsigned char   iWords; //多少个字
    unsigned char   iFlag;  //1 --> 大于等于iWords  0 --> 等于iWords
    RULE_RULE      *rule;
} RULE;

typedef struct _RECORD {
    char           *strCode;
    char           *strHZ;
    struct _RECORD *next;
    struct _RECORD *prev;
    unsigned int    iHit;
    unsigned int    iIndex;
    int8_t          type;
} RECORD;

typedef struct _AUTOPHRASE {
    char           *strHZ;
    char           *strCode;
    char            iSelected;
    struct _AUTOPHRASE *next;   //构造一个队列
} AUTOPHRASE;

/* 根据键码生成一个简单的索引，指向该键码起始的第一个记录 */
typedef struct _RECORD_INDEX {
    RECORD         *record;
    char            cCode;
} RECORD_INDEX;

typedef struct _SINGLE_HZ {
    char            strHZ[UTF8_MAX_LENGTH + 1];
} SINGLE_HZ;

typedef struct _TableMetaData {
    FcitxGenericConfig   config;
    char           *uniqueName;
    char           *strName;
    char           *strIconName;
    char           *strPath;
    ADJUSTORDER     tableOrder;
    int             iPriority;
    boolean         bUsePY;
    char            cPinyin;    //输入该键后，表示进入临时拼音状态
    int             iTableAutoSendToClient; //自动上屏
    int             iTableAutoSendToClientWhenNone; //空码自动上屏
    boolean         bSendRawPreedit;
    char           *strEndCode; //中止键，按下该键相当于输入该键后再按一个空格
    boolean         bUseMatchingKey; //是否模糊匹配
    char            cMatchingKey;
    boolean         bTableExactMatch;    //是否只显示精确匹配的候选字/词
    boolean         bAutoPhrase; //是否自动造词
    boolean         bAutoPhrasePhrase;   //词组是否参与造词
    int             iAutoPhraseLength;    //自动造词长度
    int             iSaveAutoPhraseAfter;   //选择N次后保存自动词组，0-不保存，1-立即保存
    boolean         bPromptTableCode;    //输入完毕后是否提示编码
    char           *strSymbol;
    char           *strSymbolFile;
    char           *strChoose;      //设置选择键
    char           *langCode;
    char           *kbdlayout;
    boolean         customPrompt;
    boolean         bUseAlternativePageKey;
    boolean         bFirstCandidateAsPreedit;
    boolean         bCommitAndPassByInvalidKey;
    boolean         bIgnorePunc;
    FcitxHotkey     hkAlternativePrevPage[2];
    FcitxHotkey     hkAlternativeNextPage[2];
    boolean         bEnabled;

    struct _FcitxTableState* owner;
    struct _TableDict* tableDict;
} TableMetaData;

typedef struct _TableDict {

    char* strInputCode;
    RECORD_INDEX* recordIndex;
    unsigned char iCodeLength;
    unsigned char iPYCodeLength;
    char* strIgnoreChars;
    unsigned char   bRule;
    RULE* rule;
    unsigned int iRecordCount;
    RECORD* tableSingleHZ[SINGLE_HZ_COUNT];
    RECORD* tableSingleHZCons[SINGLE_HZ_COUNT];
    unsigned int iTableIndex;
    boolean bHasPinyin;
    RECORD* currentRecord;
    RECORD* recordHead;
    int iFH;
    FH* fh;
    char* strNewPhraseCode;
    AUTOPHRASE* autoPhrase;
    AUTOPHRASE* insertPoint;
    int iAutoPhrase;
    int iTableChanged;
    int iHZLastInputCount;
    SINGLE_HZ       hzLastInput[PHRASE_MAX_LENGTH]; //Records last HZ input
    RECORD* promptCode[256];
    FcitxMemoryPool* pool;
} TableDict;

boolean LoadTableDict(TableMetaData* tableMetaData);
void SaveTableDict(TableMetaData* tableMetaData);
void FreeTableDict(TableMetaData* tableMetaData);

void TableInsertPhrase(TableDict* tableDict, const char *strCode, const char *strHZ);
RECORD *TableFindPhrase(const TableDict* tableDict, const char *strHZ);
boolean TableCreatePhraseCode(TableDict* tableDict, char* strHZ);
void TableCreateAutoPhrase(TableMetaData* tableMetaData, char iCount);
RECORD *TableHasPhrase(const TableDict* tableDict, const char *strCode, const char *strHZ);
void TableDelPhraseByHZ(TableDict* tableDict, const char *strHZ);
void TableDelPhrase(TableDict* tableDict, RECORD * record);
void TableUpdateHitFrequency(TableMetaData* tableMetaData, RECORD * record);
int TableCompareCode(const TableMetaData* tableMetaData, const char *strUser, const char *strDict);
int TableFindFirstMatchCode(TableMetaData* tableMetaData, const char* strCodeInput);
void TableResetFlags(TableDict* tableDict);

boolean IsInputKey(const TableDict* tableDict, int iKey);
boolean IsEndKey(const TableMetaData* tableMetaData, char cChar);
boolean IsIgnoreChar(const TableDict* tableDict, char cChar);
unsigned int CalHZIndex(char *strHZ);
boolean HasMatchingKey(const TableMetaData* tableMetaData, const char* strCodeInput);
CONFIG_BINDING_DECLARE(TableMetaData);

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
