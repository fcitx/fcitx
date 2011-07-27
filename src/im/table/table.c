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
#include "utf8_in_gb18030.h"

#define TEMP_FILE       "FCITX_TABLE_TEMP"

static int cmpi(const void * a, const void *b)
{
    return (*((int*)a)) - (*((int*)b));
}

static void FreeTableConfig(void *v);
static void FreeTable (FcitxTableState* tbl, char iTableIndex);
const UT_icd table_icd = {sizeof(TABLE), NULL ,NULL, FreeTableConfig};
const int iInternalVersion = 3;

static FILE *GetXDGFileTable(const char *fileName, const char *mode, char **retFile, boolean forceUser);
static void *TableCreate(FcitxInstance* instance);
static void Table_LoadPYBaseDict(FcitxTableState *tbl);
static void Table_PYGetPYByHZ(FcitxTableState *tbl, char *a, char* b);
static void Table_PYGetCandWord(FcitxTableState *tbl, int a);
static void Table_DoPYInput(FcitxTableState *tbl, FcitxKeySym sym, unsigned int state);
static void Table_PYGetCandWords(FcitxTableState *tbl, SEARCH_MODE mode);
static void Table_PYGetCandText(FcitxTableState *tbl, int iIndex, char *strText);
static void Table_ResetPY(FcitxTableState *tbl);
static char* Table_PYGetFindString(FcitxTableState *tbl);

static unsigned int    CalHZIndex (char *strHZ);

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
    TABLE* table;
    for(table = (TABLE*) utarray_front(tbl->table);
        table != NULL;
        table = (TABLE*) utarray_next(tbl->table, table))
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

FILE *GetXDGFileTable(const char *fileName, const char *mode, char **retFile, boolean forceUser)
{
    size_t len;
    char ** path;
    if (forceUser)
        path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/table" , NULL, NULL );
    else
        path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/table" , DATADIR, PACKAGE "/table" );

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
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
            TABLE table;
            memset(&table, 0, sizeof(TABLE));
            utarray_push_back(tbl->table, &table);
            TABLE *t = (TABLE*)utarray_back(tbl->table);
            TABLEConfigBind(t, cfile, GetTableConfigDesc());
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

boolean LoadTableDict (FcitxTableState *tbl)
{
    char            strCode[MAX_CODE_LENGTH + 1];
    char            strHZ[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1];
    FILE           *fpDict;
    RECORD         *recTemp;
    char            strPath[PATH_MAX];
    unsigned int    i = 0;
    unsigned int    iTemp, iTempCount;
    char            cChar = 0, cTemp;
    char            iVersion = 1;
    int             iRecordIndex;

    //读入码表
    FcitxLog(DEBUG, _("Loading Table Dict"));

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    {
        size_t len;
        char *pstr = NULL;
        char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/table" , DATADIR, PACKAGE "/table" );
        fpDict = GetXDGFile(table->strPath, path, "r", len, &pstr);
        FcitxLog(INFO, _("Load Table Dict from %s"), pstr);
        free(pstr);
        FreeXDGPath(path);
    }

    if (!fpDict) {
        FcitxLog( DEBUG, _("Cannot load table file: %s"), strPath);
        return false;
    }
    
    //先读取码表的信息
    //判断版本信息
    fread (&iTemp, sizeof (unsigned int), 1, fpDict);
    if (!iTemp) {
        fread (&iVersion, sizeof (char), 1, fpDict);
        iVersion = (iVersion < INTERNAL_VERSION);
        fread (&iTemp, sizeof (unsigned int), 1, fpDict);
    }

    table->strInputCode = (char *) realloc (table->strInputCode, sizeof (char) * (iTemp + 1));
    fread (table->strInputCode, sizeof (char), iTemp + 1, fpDict);
    /*
     * 建立索引，加26是为了为拼音编码预留空间
     */
    tbl->recordIndex = (RECORD_INDEX *) malloc ((strlen (table->strInputCode) + 26) * sizeof (RECORD_INDEX));
    for (iTemp = 0; iTemp < strlen (table->strInputCode) + 26; iTemp++) {
        tbl->recordIndex[iTemp].cCode = 0;
        tbl->recordIndex[iTemp].record = NULL;
    }
    /* ********************************************************************** */

    fread (&(table->iCodeLength), sizeof (unsigned char), 1, fpDict);
    if (table->iTableAutoSendToClient == -1)
        table->iTableAutoSendToClient = table->iCodeLength;
    if (table->iTableAutoSendToClientWhenNone == -1)
        table->iTableAutoSendToClientWhenNone = table->iCodeLength;

    if (!iVersion)
        fread (&(table->iPYCodeLength), sizeof (unsigned char), 1, fpDict);
    else
        table->iPYCodeLength = table->iCodeLength;

    fread (&iTemp, sizeof (unsigned int), 1, fpDict);
    table->strIgnoreChars = (char *) malloc (sizeof (char) * (iTemp + 1));
    fread (table->strIgnoreChars, sizeof (char), iTemp + 1, fpDict);

    fread (&(table->bRule), sizeof (unsigned char), 1, fpDict);

    if (table->bRule) { //表示有组词规则
        table->rule = (RULE *) malloc (sizeof (RULE) * (table->iCodeLength - 1));
        for (i = 0; i < table->iCodeLength - 1; i++) {
            fread (&(table->rule[i].iFlag), sizeof (unsigned char), 1, fpDict);
            fread (&(table->rule[i].iWords), sizeof (unsigned char), 1, fpDict);
            table->rule[i].rule = (RULE_RULE *) malloc (sizeof (RULE_RULE) * table->iCodeLength);
            for (iTemp = 0; iTemp < table->iCodeLength; iTemp++) {
                fread (&(table->rule[i].rule[iTemp].iFlag), sizeof (unsigned char), 1, fpDict);
                fread (&(table->rule[i].rule[iTemp].iWhich), sizeof (unsigned char), 1, fpDict);
                fread (&(table->rule[i].rule[iTemp].iIndex), sizeof (unsigned char), 1, fpDict);
            }
        }
    }

    tbl->recordHead = (RECORD *) malloc (sizeof (RECORD));
    tbl->currentRecord = tbl->recordHead;

    fread (&(table->iRecordCount), sizeof (unsigned int), 1, fpDict);

    for (i = 0; i < SINGLE_HZ_COUNT; i++)
        tbl->tableSingleHZ[i] = (RECORD *) NULL;

    iRecordIndex = 0;
    for (i = 0; i < table->iRecordCount; i++) {
        fread (strCode, sizeof (char), table->iPYCodeLength + 1, fpDict);
        fread (&iTemp, sizeof (unsigned int), 1, fpDict);
        fread (strHZ, sizeof (char), iTemp, fpDict);
        recTemp = (RECORD *) malloc (sizeof (RECORD));
        recTemp->strCode = (char *) malloc (sizeof (char) * (table->iPYCodeLength + 1));
        memset(recTemp->strCode, 0, sizeof (char) * (table->iPYCodeLength + 1));
        strcpy (recTemp->strCode, strCode);
        recTemp->strHZ = (char *) malloc (sizeof (char) * iTemp);
        strcpy (recTemp->strHZ, strHZ);

        if (!iVersion) {
            fread (&cTemp, sizeof (char), 1, fpDict);
            recTemp->bPinyin = cTemp;
        }

        recTemp->flag = 0;

        fread (&(recTemp->iHit), sizeof (unsigned int), 1, fpDict);
        fread (&(recTemp->iIndex), sizeof (unsigned int), 1, fpDict);
        if (recTemp->iIndex > tbl->iTableIndex)
            tbl->iTableIndex = recTemp->iIndex;

        /* 建立索引 */
        if (cChar != recTemp->strCode[0]) {
            cChar = recTemp->strCode[0];
            tbl->recordIndex[iRecordIndex].cCode = cChar;
            tbl->recordIndex[iRecordIndex].record = recTemp;
            iRecordIndex++;
        }
        /* **************************************************************** */
        /** 为单字生成一个表   */
        if (utf8_strlen (recTemp->strHZ) == 1 && !IsIgnoreChar (tbl, strCode[0]) && !recTemp->bPinyin) {
            iTemp = CalHZIndex (recTemp->strHZ);
            if (iTemp < SINGLE_HZ_COUNT) {
                if (tbl->tableSingleHZ[iTemp]) {
                    if (strlen (strCode) > strlen (tbl->tableSingleHZ[iTemp]->strCode))
                        tbl->tableSingleHZ[iTemp] = recTemp;
                }
                else
                    tbl->tableSingleHZ[iTemp] = recTemp;
            }
        }

        if (recTemp->bPinyin)
            table->bHasPinyin = true;

        tbl->currentRecord->next = recTemp;
        recTemp->prev = tbl->currentRecord;
        tbl->currentRecord = recTemp;
    }

    tbl->currentRecord->next = tbl->recordHead;
    tbl->recordHead->prev = tbl->currentRecord;

    fclose (fpDict);
    FcitxLog(DEBUG, _("Load Table Dict OK"));

    //读取相应的特殊符号表
    fpDict = GetXDGFileTable(table->strSymbolFile, "rt", NULL, false);

    if (fpDict) {
        tbl->iFH = CalculateRecordNumber (fpDict);
        tbl->fh = (FH *) malloc (sizeof (FH) * tbl->iFH);

        for (i = 0; i < tbl->iFH; i++) {
            if (EOF == fscanf (fpDict, "%s\n", tbl->fh[i].strFH))
                break;
        }
        tbl->iFH = i;

        fclose (fpDict);
    }

    tbl->strNewPhraseCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
    tbl->strNewPhraseCode[table->iCodeLength] = '\0';
    tbl->iCurrentTableLoaded = tbl->iTableIMIndex;

    tbl->iAutoPhrase = 0;
    if (table->bAutoPhrase) {
        //为自动词组分配空间
        tbl->autoPhrase = (AUTOPHRASE *) malloc (sizeof (AUTOPHRASE) * AUTO_PHRASE_COUNT);

        //读取上次保存的自动词组信息
        FcitxLog(DEBUG, _("Loading Autophrase."));

        strcpy (strPath, table->strName);
        strcat (strPath, "_LastAutoPhrase.tmp");
        fpDict = GetXDGFileTable(strPath, "rb", NULL, true);
        i = 0;
        if (fpDict) {
            fread (&tbl->iAutoPhrase, sizeof (unsigned int), 1, fpDict);

            for (; i < tbl->iAutoPhrase; i++) {
                tbl->autoPhrase[i].strCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
                tbl->autoPhrase[i].strHZ = (char *) malloc (sizeof (char) * (PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1));
                fread (tbl->autoPhrase[i].strCode, table->iCodeLength + 1, 1, fpDict);
                fread (tbl->autoPhrase[i].strHZ, PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1, 1, fpDict);
                fread (&iTempCount, sizeof (unsigned int), 1, fpDict);
                tbl->autoPhrase[i].iSelected = iTempCount;
                if (i == AUTO_PHRASE_COUNT - 1)
                    tbl->autoPhrase[i].next = &tbl->autoPhrase[0];
                else
                    tbl->autoPhrase[i].next = &tbl->autoPhrase[i + 1];
            }
            fclose (fpDict);
        }

        for (; i < AUTO_PHRASE_COUNT; i++) {
            tbl->autoPhrase[i].strCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
            tbl->autoPhrase[i].strHZ = (char *) malloc (sizeof (char) * (PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1));
            tbl->autoPhrase[i].iSelected = 0;
            if (i == AUTO_PHRASE_COUNT - 1)
                tbl->autoPhrase[i].next = &tbl->autoPhrase[0];
            else
                tbl->autoPhrase[i].next = &tbl->autoPhrase[i + 1];
        }

        if (i == AUTO_PHRASE_COUNT)
            tbl->insertPoint = &tbl->autoPhrase[0];
        else
            tbl->insertPoint = &tbl->autoPhrase[i - 1];

#ifdef _DEBUG
        FcitxLog(DEBUG, _("Load Autophrase OK"));
#endif
    }
    else
        tbl->autoPhrase = (AUTOPHRASE *) NULL;

    return true;
}

boolean TableInit (void *arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    FcitxAddon* pyaddon = GetAddonByName(&tbl->owner->addons, "fcitx-pinyin");
    tbl->pyaddon = pyaddon;
    if (pyaddon == NULL)
        return false;
    tbl->PYBaseOrder = AD_FREQ;
    
    Table_ResetPY(tbl);
    return true;
}

void SaveTableIM (void *arg)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    if (!tbl->recordHead)
        return;
    if (tbl->iTableChanged)
        SaveTableDict(tbl);
}

void FreeTable (FcitxTableState *tbl, char iTableIndex)
{
    RECORD         *recTemp, *recNext;
    short           i;

    if (iTableIndex == -1)
        return;

    if (!tbl->recordHead)
        return;

    if (tbl->iTableChanged)
        SaveTableDict (tbl);

    //释放码表
    recTemp = tbl->recordHead->next;
    while (recTemp != tbl->recordHead) {
        recNext = recTemp->next;

        free (recTemp->strCode);
        free (recTemp->strHZ);
        free (recTemp);

        recTemp = recNext;
    }

    free (tbl->recordHead);
    tbl->recordHead = NULL;

    if (tbl->fh) {
        free (tbl->fh);
        tbl->fh = NULL;
        tbl->iFH = 0;
    }

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, iTableIndex);

    table->iRecordCount = 0;
    tbl->iCurrentTableLoaded = -1;

    free (tbl->strNewPhraseCode);

    //释放组词规则的空间
    if (table->rule) {
        for (i = 0; i < table->iCodeLength - 1; i++)
            free (table->rule[i].rule);
        free (table->rule);

        table->rule = NULL;
    }

    //释放索引的空间
    if (tbl->recordIndex) {
        free (tbl->recordIndex);
        tbl->recordIndex = NULL;
    }

    //释放自动词组的空间
    if (tbl->autoPhrase) {
        for (i = 0; i < AUTO_PHRASE_COUNT; i++) {
            free (tbl->autoPhrase[i].strCode);
            free (tbl->autoPhrase[i].strHZ);
        }
        free (tbl->autoPhrase);

        tbl->autoPhrase = (AUTOPHRASE *) NULL;
    }
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

void SaveTableDict (FcitxTableState *tbl)
{
    RECORD         *recTemp;
    char            strPathTemp[PATH_MAX];
    char            strPath[PATH_MAX];
    char       *pstr;
    FILE           *fpDict;
    unsigned int    iTemp;
    unsigned int    i;
    char            cTemp;

    //先将码表保存在一个临时文件中，如果保存成功，再将该临时文件改名是码表名──这样可以防止码表文件被破坏
    if ( tbl->isSavingTableDic )
        return;

    if (!tbl->iTableChanged)
        return;
    
    if (tbl->iCurrentTableLoaded == -1)
        return;

    tbl->isSavingTableDic = true;

    fpDict = GetXDGFileTable(TEMP_FILE, "wb", &pstr, true);
    if (!fpDict) {
        tbl->isSavingTableDic = false;
        FcitxLog(ERROR, _("Cannot create table file: %s"), pstr);
        return;
    }
    strcpy(strPathTemp, pstr);
    free(pstr);

    //写入版本号--如果第一个字为0,表示后面那个字节为版本号，为了与老版本兼容
    iTemp = 0;
    fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
    fwrite (&iInternalVersion, sizeof (char), 1, fpDict);

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    iTemp = strlen (table->strInputCode);
    fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
    fwrite (table->strInputCode, sizeof (char), iTemp + 1, fpDict);
    fwrite (&(table->iCodeLength), sizeof (char), 1, fpDict);
    fwrite (&(table->iPYCodeLength), sizeof (char), 1, fpDict);
    iTemp = strlen (table->strIgnoreChars);
    fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
    fwrite (table->strIgnoreChars, sizeof (char), iTemp + 1, fpDict);

    fwrite (&(table->bRule), sizeof (unsigned char), 1, fpDict);
    if (table->bRule) { //表示有组词规则
        for (i = 0; i < table->iCodeLength - 1; i++) {
            fwrite (&(table->rule[i].iFlag), sizeof (unsigned char), 1, fpDict);
            fwrite (&(table->rule[i].iWords), sizeof (unsigned char), 1, fpDict);
            for (iTemp = 0; iTemp < table->iCodeLength; iTemp++) {
                fwrite (&(table->rule[i].rule[iTemp].iFlag), sizeof (unsigned char), 1, fpDict);
                fwrite (&(table->rule[i].rule[iTemp].iWhich), sizeof (unsigned char), 1, fpDict);
                fwrite (&(table->rule[i].rule[iTemp].iIndex), sizeof (unsigned char), 1, fpDict);
            }
        }
    }

    fwrite (&(table->iRecordCount), sizeof (unsigned int), 1, fpDict);
    recTemp = tbl->recordHead->next;
    while (recTemp != tbl->recordHead) {
        fwrite (recTemp->strCode, sizeof (char), table->iPYCodeLength + 1, fpDict);

        iTemp = strlen (recTemp->strHZ) + 1;
        fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
        fwrite (recTemp->strHZ, sizeof (char), iTemp, fpDict);

        cTemp = recTemp->bPinyin;
        fwrite (&cTemp, sizeof (char), 1, fpDict);
        fwrite (&(recTemp->iHit), sizeof (unsigned int), 1, fpDict);
        fwrite (&(recTemp->iIndex), sizeof (unsigned int), 1, fpDict);
        recTemp = recTemp->next;
    }

    fclose (fpDict);
    fpDict = GetXDGFileTable(table->strPath, NULL, &pstr, true);
    if (access (pstr, 0))
        unlink (pstr);
    rename (strPathTemp, pstr);
    free(pstr);

    FcitxLog(DEBUG, _("Rename OK"));

    tbl->iTableChanged = 0;

    if (tbl->autoPhrase) {
        //保存上次的自动词组信息
        fpDict = GetXDGFileTable(TEMP_FILE, "wb", &pstr, true);
        strncpy (strPathTemp, pstr, PATH_MAX);
        if (fpDict) {
            fwrite (&tbl->iAutoPhrase, sizeof (int), 1, fpDict);
            for (i = 0; i < tbl->iAutoPhrase; i++) {
                fwrite (tbl->autoPhrase[i].strCode, table->iCodeLength + 1, 1, fpDict);
                fwrite (tbl->autoPhrase[i].strHZ, PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1, 1, fpDict);
                iTemp = tbl->autoPhrase[i].iSelected;
                fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
            }
            fclose (fpDict);
        }
        free(pstr);

        strncpy (strPath, table->strName, PATH_MAX);
        strncat (strPath, "_LastAutoPhrase.tmp", PATH_MAX);
        fpDict = GetXDGFileTable(strPath, NULL, &pstr, true);
        if (access (pstr, F_OK))
            unlink (pstr);
        rename (strPathTemp, pstr);
        free(pstr);
    }

    tbl->isSavingTableDic = false;
}

boolean IsInputKey (FcitxTableState* tbl, int iKey)
{
    char           *p;

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    p = table->strInputCode;
    if (!p)
        return false;

    while (*p) {
        if (iKey == *p)
            return true;
        p++;
    }

    if (table->bHasPinyin) {
        if (iKey >= 'a' && iKey <= 'z')
            return true;
    }

    return false;
}

boolean IsEndKey (FcitxTableState* tbl, char cChar)
{
    char           *p;

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    p = table->strEndCode;
    if (!p)
        return false;

    while (*p) {
        if (cChar == *p)
            return true;
        p++;
    }

    return false;
}

boolean IsIgnoreChar (FcitxTableState* tbl, char cChar)
{
    char           *p;

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    p = table->strIgnoreChars;
    while (*p) {
        if (cChar == *p)
            return true;
        p++;
    }

    return false;
}

char IsChooseKey (FcitxTableState* tbl, int iKey)
{
    int i = 0;

    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    while (table->strChoose[i]) {
        if (iKey == table->strChoose[i])
            return i + 1;
        i++;
    }

    return 0;
}

INPUT_RETURN_VALUE DoTableInput (void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    INPUT_RETURN_VALUE retVal;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (IsHotKeyModifierCombine(sym, state))
        return IRV_TO_PROCESS;

    FcitxIM* currentIM = GetCurrentIM(instance);
    tbl->iTableIMIndex = utarray_eltidx(tbl->table, currentIM->priv);
    if (tbl->iTableIMIndex != tbl->iCurrentTableLoaded)
        FreeTable(tbl, tbl->iCurrentTableLoaded);

    if (tbl->iCurrentTableLoaded == -1)
        LoadTableDict (tbl);

    if (tbl->bTablePhraseTips) {
        if (IsHotKey(sym, state, FCITX_CTRL_DELETE)) {
            tbl->bTablePhraseTips = false;
            TableDelPhraseByHZ (tbl, GetMessageString(instance->messageUp, 1));
            return IRV_CLEAN;
        }
        else if (state == KEY_NONE && (sym != Key_Control_L && sym != Key_Control_R && sym != Key_Shift_L && sym != Key_Shift_R )) {
            SetMessageCount(instance->messageUp, 0);
            SetMessageCount(instance->messageDown, 0);
            tbl->bTablePhraseTips = false;
            CloseInputWindow(instance);
        }
    }

    retVal = IRV_DO_NOTHING;
    if (state == KEY_NONE && (IsInputKey (tbl, sym) || IsEndKey (tbl, sym) || sym == table->cMatchingKey || sym == table->cPinyin)) {
        input->bIsInRemind = false;

        if (!tbl->bIsTableAddPhrase && !tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder) {
            if (input->strCodeInput[0] == table->cPinyin && table->bUsePY) {
                if (input->iCodeInputCount != (MAX_PY_LENGTH * 5 + 1)) {
                    input->strCodeInput[input->iCodeInputCount++] = (char) sym;
                    input->strCodeInput[input->iCodeInputCount] = '\0';
                    retVal = TableGetCandWords (tbl, SM_FIRST);
                }
                else
                    retVal = IRV_DO_NOTHING;
            }
            else {
                if ((input->iCodeInputCount < table->iCodeLength) || (table->bHasPinyin && input->iCodeInputCount < table->iPYCodeLength)) {
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

                        retVal = TableGetCandWords (tbl, SM_FIRST);
                        FcitxModuleFunctionArg farg;
                        int key = input->strCodeInput[0];
                        farg.args[0] = &key;
                        strTemp = InvokeFunction(instance, FCITX_PUNC, GETPUNC, farg);
                        if (IsEndKey (tbl, sym)) {
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
                                strcpy (GetOutputString(input), strLastFirstCand);
                                TableUpdateHitFrequency(tbl, pLastCandRecord);
                                retVal=IRV_GET_CANDWORDS_NEXT;
                            }
                            else
                                retVal = IRV_DISPLAY_CANDWORDS;
                            input->iCodeInputCount = 1;
                            input->strCodeInput[0] = sym;
                            input->strCodeInput[1] = '\0';
                            TableGetCandWords (tbl, SM_FIRST);
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
                                strcpy (GetOutputString(input), TableGetCandWord (tbl, 0));
                                retVal = IRV_GET_CANDWORDS_NEXT;
                            }
                            else
                                retVal = IRV_DISPLAY_CANDWORDS;
                        }
                        else
                            retVal = IRV_DISPLAY_CANDWORDS;

                        input->iCodeInputCount = 1;
                        input->strCodeInput[0] = sym;
                        input->strCodeInput[1] = '\0';
                        input->bIsInRemind = false;

                        if (retVal != IRV_DISPLAY_CANDWORDS)
                            TableGetCandWords (tbl, SM_FIRST);
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
                if (tbl->iTableNewPhraseHZCount < tbl->iHZLastInputCount && tbl->iTableNewPhraseHZCount < PHRASE_MAX_LENGTH) {
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
                if (!tbl->bCanntFindCode)
                    TableInsertPhrase (tbl, GetMessageString(instance->messageDown, 1), GetMessageString(instance->messageDown, 0));
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
                if (tbl->iHZLastInputCount < 2 || !table->bRule) //词组最少为两个汉字
                    return IRV_DO_NOTHING;

                tbl->bTablePhraseTips = false;
                tbl->iTableNewPhraseHZCount = 2;
                tbl->bIsTableAddPhrase = true;
                input->bIsDoInputOnly = true;
                instance->bShowCursor = false;

                SetMessageCount(instance->messageUp , 0);
                AddMessageAtLast(instance->messageUp, MSG_TIPS, "左/右键增加/减少，ENTER确定，ESC取消");

                SetMessageCount(instance->messageDown , 0);
                AddMessageAtLast(instance->messageDown, MSG_FIRSTCAND, "");
                AddMessageAtLast(instance->messageDown, MSG_CODE, "");

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
            Table_LoadPYBaseDict (tbl);

            //如果刚刚输入的是个词组，刚不查拼音
            if (utf8_strlen (GetOutputString(input)) != 1)
                return IRV_DO_NOTHING;

            input->iCodeInputCount = 0;
            SetMessageCount(instance->messageUp, 0);
            AddMessageAtLast(instance->messageUp, MSG_INPUT, "%s", GetOutputString(input));

            SetMessageCount(instance->messageDown, 0);
            AddMessageAtLast(instance->messageDown, MSG_CODE, "读音：");
            Table_PYGetPYByHZ (tbl, GetOutputString(input), strPY);
            AddMessageAtLast(instance->messageDown, MSG_TIPS, (strPY[0]) ? strPY : "无法查到该字读音");
            instance->bShowCursor = false;

            return IRV_DISPLAY_MESSAGE;
        }

        if (!input->iCodeInputCount && !input->bIsInRemind)
            return IRV_TO_PROCESS;

        if (IsHotKey(sym, state, FCITX_ESCAPE)) {
            if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder) {
                TableResetStatus (tbl);
                retVal = IRV_DISPLAY_CANDWORDS;
            }
            else
                return IRV_CLEAN;
        }
        else if (state == KEY_NONE && IsChooseKey(tbl, sym)) {
            int iKey;
            iKey = IsChooseKey(tbl, sym);

            if (!input->bIsInRemind) {
                if (!input->iCandWordCount)
                    return IRV_TO_PROCESS;

                if (iKey > input->iCandWordCount)
                    return IRV_DO_NOTHING;
                else {
                    if (tbl->bIsTableDelPhrase) {
                        TableDelPhraseByIndex (tbl, iKey);
                        retVal = TableGetCandWords (tbl, SM_FIRST);
                    }
                    else if (tbl->bIsTableAdjustOrder) {
                        TableAdjustOrderByIndex (tbl, iKey);
                        retVal = TableGetCandWords (tbl, SM_FIRST);
                    }
                    else {
                        //如果是拼音，进入快速调整字频方式
                        if (strcmp (input->strCodeInput, table->strSymbol) && input->strCodeInput[0] == table->cPinyin && table->bUsePY)
                            Table_PYGetCandWord (tbl, iKey - 1);
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
                SetMessageCount(instance->messageUp, 0);
                AddMessageAtLast(instance->messageUp, MSG_TIPS, "选择需要提前的词组序号，ESC结束");
                retVal = IRV_DISPLAY_MESSAGE;
            }
            else if (IsHotKey (sym, state, tbl->hkTableDelPhrase)) {
                if (!input->iCandWordCount || input->bIsInRemind)
                    return IRV_DO_NOTHING;

                tbl->bIsTableDelPhrase = true;
                SetMessageCount(instance->messageUp, 0);
                AddMessageAtLast(instance->messageUp, MSG_TIPS, "选择需要删除的词组序号，ESC取消");
                retVal = IRV_DISPLAY_MESSAGE;
            }
            else if (IsHotKey(sym, state, FCITX_BACKSPACE) || IsHotKey(sym, state, FCITX_CTRL_H)) {
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
                    retVal = TableGetCandWords (tbl, SM_FIRST);
                else
                    retVal = IRV_CLEAN;
            }
            else if (IsHotKey(sym, state, FCITX_SPACE)) {
                if (!input->bIsInRemind) {
                    if (!(table->bUsePY && input->iCodeInputCount == 1 && input->strCodeInput[0] == table->cPinyin)) {
                        if (strcmp (input->strCodeInput, table->strSymbol) && input->strCodeInput[0] == table->cPinyin && table->bUsePY)
                            Table_PYGetCandWord (tbl, SM_FIRST);

                        if (!input->iCandWordCount) {
                            input->iCodeInputCount = 0;
                            return IRV_CLEAN;
                        }

                        strcpy (GetOutputString(input), TableGetCandWord (tbl, 0));
                    }
                    else
                        SetMessageCount(instance->messageDown, 0);

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
        if (!tbl->bIsTableDelPhrase && !tbl->bIsTableAdjustOrder) {
            SetMessageCount(instance->messageUp,0);
            if (input->iCodeInputCount) {
                AddMessageAtLast(instance->messageUp, MSG_INPUT, "%s", input->strCodeInput);
            }
        }
        else
            input->bIsDoInputOnly = true;
    }

    if (tbl->bIsTableDelPhrase || tbl->bIsTableAdjustOrder || input->bIsInRemind)
        instance->bShowCursor = false;
    else
    {
        instance->bShowCursor = true;
        input->iCursorPos = input->iCodeInputCount;
    }

    return retVal;
}

char           *TableGetCandWord (void* arg, int iIndex)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    char *str;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    str=_TableGetCandWord(tbl, iIndex, true);
    if (str) {
        if (table->bAutoPhrase && (utf8_strlen (str) == 1 || (utf8_strlen (str) > 1 && table->bAutoPhrasePhrase)))
            UpdateHZLastInput (tbl, str);

        if ( tbl->pCurCandRecord )
            TableUpdateHitFrequency (tbl, tbl->pCurCandRecord);
    }

    return str;
}

void        TableUpdateHitFrequency (FcitxTableState* tbl, RECORD * record)
{
    record->iHit++;
    record->iIndex = ++tbl->iTableIndex;
}

/*
 * 第二个参数表示是否进入联想模式，实现自动上屏功能时，不能使用联想模式
 */
char           *_TableGetCandWord (FcitxTableState* tbl, int iIndex, boolean _bRemind)
{
    char           *pCandWord = NULL;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
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

    tbl->iTableChanged++;
    if (tbl->iTableChanged >= TABLE_AUTO_SAVE_AFTER)
        SaveTableDict (tbl);

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
                TableInsertPhrase (tbl, tableCandWord[iIndex].candWord.autoPhrase->strCode, tableCandWord[iIndex].candWord.autoPhrase->strHZ);
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

            SetMessageCount(instance->messageUp, 0);
            AddMessageAtLast(instance->messageUp, MSG_INPUT, "%s", input->strCodeInput);

            SetMessageCount(instance->messageDown, 0);
            AddMessageAtLast(instance->messageDown, MSG_TIPS, "%s", pCandWord);
            temp = tbl->tableSingleHZ[CalHZIndex (pCandWord)];
            if (temp) {
                AddMessageAtLast(instance->messageDown, MSG_CODE, "%s", temp->strCode);
            }
        }
        else {
            SetMessageCount(instance->messageDown, 0);
            SetMessageCount(instance->messageUp, 0);
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
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (mode == SM_FIRST) {
        strcpy (Table_PYGetFindString(tbl), input->strCodeInput + 1);

        Table_DoPYInput (tbl, 0,0);

        input->strCodeInput[0] = table->cPinyin;
        input->strCodeInput[1] = '\0';

        strcat (input->strCodeInput, Table_PYGetFindString(tbl));
        input->iCodeInputCount = strlen (input->strCodeInput);
    }
    else
        Table_PYGetCandWords (tbl, mode);

    //下面将拼音的候选字表转换为码表输入法的样式
    for (i = 0; i < input->iCandWordCount; i++) {
        tableCandWord[i].flag = CT_PYPHRASE;
        Table_PYGetCandText (tbl, i, tableCandWord[i].candWord.strPYPhrase);
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetCandWords (void* arg, SEARCH_MODE mode)
{
    FcitxTableState *tbl = (FcitxTableState*) arg;
    int             i;
    char            strTemp[3], *pstr;
    RECORD         *recTemp;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
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
            TableResetFlags (tbl);
            if (TableFindFirstMatchCode (tbl) == -1 && !tbl->iAutoPhrase) {
                SetMessageCount(instance->messageDown, 0);
                return IRV_DISPLAY_CANDWORDS;   //Not Found
            }
        }
        else {
            if (!input->iCandWordCount)
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

            TableFindFirstMatchCode (tbl);
        }

        input->iCandWordCount = 0;
        if (mode == SM_PREV && table->bRule && table->bAutoPhrase && input->iCodeInputCount == table->iCodeLength) {
            for (i = tbl->iAutoPhrase - 1; i >= 0; i--) {
                if (!TableCompareCode (tbl, input->strCodeInput, tbl->autoPhrase[i].strCode) && tbl->autoPhrase[i].flag) {
                    if (TableHasPhrase (tbl, tbl->autoPhrase[i].strCode, tbl->autoPhrase[i].strHZ))
                        TableAddAutoCandWord (tbl, i, mode);
                }
            }
        }

        if (input->iCandWordCount < ConfigGetMaxCandWord(&tbl->owner->config)) {
            while (tbl->currentRecord && tbl->currentRecord != tbl->recordHead) {
                if ((mode == SM_PREV) ^ (!tbl->currentRecord->flag)) {
                    if (!TableCompareCode (tbl, input->strCodeInput, tbl->currentRecord->strCode)) {
                        if (mode == SM_FIRST)
                            tbl->iTableTotalCandCount++;
                        TableAddCandWord (tbl, tbl->currentRecord, mode);
                    }
                }

                tbl->currentRecord = tbl->currentRecord->next;
            }
        }

        if (table->bRule && table->bAutoPhrase && mode != SM_PREV && input->iCodeInputCount == table->iCodeLength) {
            for (i = tbl->iAutoPhrase - 1; i >= 0; i--) {
                if (!TableCompareCode (tbl, input->strCodeInput, tbl->autoPhrase[i].strCode) && !tbl->autoPhrase[i].flag) {
                    if (TableHasPhrase (tbl, tbl->autoPhrase[i].strCode, tbl->autoPhrase[i].strHZ)) {
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

    if (ConfigGetPointAfterNumber(&tbl->owner->config)) {
        strTemp[1] = '.';
        strTemp[2] = '\0';
    }
    else
        strTemp[1] = '\0';

    SetMessageCount(instance->messageDown, 0);
    for (i = 0; i < input->iCandWordCount; i++) {
        strTemp[0] = i + 1 + '0';
        if (i == 9)
            strTemp[0] = '0';
        AddMessageAtLast(instance->messageDown, MSG_INDEX, "%s", strTemp);

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
        AddMessageAtLast(instance->messageDown, mType, "%s", pMsg);

        if (tableCandWord[i].flag == CT_PYPHRASE) {
            if (utf8_strlen (tableCandWord[i].candWord.strPYPhrase) == 1) {
                recTemp = tbl->tableSingleHZ[CalHZIndex (tableCandWord[i].candWord.strPYPhrase)];

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
                recTemp = tbl->tableSingleHZ[CalHZIndex (tableCandWord[i].candWord.record->strHZ)];
                if (!recTemp)
                    pstr = (char *) NULL;
                else
                    pstr = recTemp->strCode;
            }
            else
                pstr = (char *) NULL;
        }
        else if (HasMatchingKey (tbl))
            pstr = (tableCandWord[i].flag == CT_NORMAL) ? tableCandWord[i].candWord.record->strCode : tableCandWord[i].candWord.autoPhrase->strCode;
        else
            pstr = ((tableCandWord[i].flag == CT_NORMAL) ? tableCandWord[i].candWord.record->strCode : tableCandWord[i].candWord.autoPhrase->strCode) + input->iCodeInputCount;

        if (pstr)
            AddMessageAtLast(instance->messageDown, MSG_CODE, "%s", pstr);
        else
            AddMessageAtLast(instance->messageDown, MSG_CODE, "");

        if (i != (input->iCandWordCount - 1)) {
            MessageConcatLast(instance->messageDown, " ");
        }
    }

    return IRV_DISPLAY_CANDWORDS;
}

void TableAddAutoCandWord (FcitxTableState* tbl, short which, SEARCH_MODE mode)
{
    int             i, j;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (mode == SM_PREV) {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            i = input->iCandWordCount - 1;
            for (j = 0; j < input->iCandWordCount - 1; j++)
                tableCandWord[j].candWord.autoPhrase = tableCandWord[j + 1].candWord.autoPhrase;
        }
        else
            i = input->iCandWordCount++;
        tableCandWord[i].flag = CT_AUTOPHRASE;
        tableCandWord[i].candWord.autoPhrase = &tbl->autoPhrase[which];
    }
    else {
        if (input->iCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
            return;

        tableCandWord[input->iCandWordCount].flag = CT_AUTOPHRASE;
        tableCandWord[input->iCandWordCount++].candWord.autoPhrase = &tbl->autoPhrase[which];
    }
}

void TableAddCandWord (FcitxTableState* tbl, RECORD * record, SEARCH_MODE mode)
{
    int             i = 0, j;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
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

void TableResetFlags (FcitxTableState* tbl)
{
    RECORD         *record = tbl->recordHead->next;
    short           i;

    while (record != tbl->recordHead) {
        record->flag = false;
        record = record->next;
    }
    for (i = 0; i < tbl->iAutoPhrase; i++)
        tbl->autoPhrase[i].flag = false;
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
boolean HasMatchingKey (FcitxTableState* tbl)
{
    char           *str;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    str = input->strCodeInput;
    while (*str) {
        if (*str++ == table->cMatchingKey)
            return true;
    }
    return false;
}

int TableCompareCode (FcitxTableState* tbl, char *strUser, char *strDict)
{
    int             i;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    for (i = 0; i < strlen (strUser); i++) {
        if (!strDict[i])
            return strUser[i];
        if (strUser[i] != table->cMatchingKey || !table->bUseMatchingKey) {
            if (strUser[i] != strDict[i])
                return (strUser[i] - strDict[i]);
        }
    }
    if (table->bTableExactMatch) {
        if (strlen (strUser) != strlen (strDict))
            return -999;    //随意的一个值
    }

    return 0;
}

int TableFindFirstMatchCode (FcitxTableState* tbl)
{
    int             i = 0;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (!tbl->recordHead)
        return -1;

    if (table->bUseMatchingKey && (input->strCodeInput[0] == table->cMatchingKey))
        i = 0;
    else {
        while (input->strCodeInput[0] != tbl->recordIndex[i].cCode) {
            if (!tbl->recordIndex[i].cCode)
                break;
            i++;
        }
    }
    tbl->currentRecord = tbl->recordIndex[i].record;
    if (!tbl->currentRecord)
        return -1;

    while (tbl->currentRecord != tbl->recordHead) {
        if (!TableCompareCode (tbl, input->strCodeInput, tbl->currentRecord->strCode)) {
            return i;
        }
        tbl->currentRecord = tbl->currentRecord->next;
        i++;
    }

    return -1;          //Not found
}

/*
 * 反查编码
 * bMode=true表示用于组词，此时不查一、二级简码。但如果只有二级简码时返回二级简码，不查一级简码
 */
/*RECORD         *TableFindCode (char *strHZ, boolean bMode)
{
    RECORD         *recShort = NULL;    //记录二级简码的位置
    int             i;

    for (i = 0; i < iSingleHZCount; i++) {
    if (!strcmp (tableSingleHZ[i]->strHZ, strHZ) && !IsIgnoreChar (tableSingleHZ[i]->strCode[0])) {
        if (!bMode)
        return tableSingleHZ[i];

        if (strlen (tableSingleHZ[i]->strCode) == 2)
        recShort = tableSingleHZ[i];
        else if (strlen (tableSingleHZ[i]->strCode) > 2)
        return tableSingleHZ[i];
    }
    }

    return recShort;
}*/

/*
 * 根据序号调整词组顺序，序号从1开始
 * 将指定的字/词调整到同样编码的最前面
 */
void TableAdjustOrderByIndex (FcitxTableState* tbl, int iIndex)
{
    RECORD         *recTemp;
    int             iTemp;
    TABLECANDWORD*  tableCandWord = tbl->tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

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

    tbl->iTableChanged++;

    //需要的话，更新索引
    if (tableCandWord[iIndex - 1].candWord.record->strCode[1] == '\0') {
        for (iTemp = 0; iTemp < strlen (table->strInputCode); iTemp++) {
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
    if (tableCandWord[iIndex - 1].flag != CT_NORMAL)
        return;

    if (utf8_strlen (tableCandWord[iIndex - 1].candWord.record->strHZ) <= 1)
        return;

    TableDelPhrase (tbl, tableCandWord[iIndex - 1].candWord.record);
}

/*
 * 根据字串删除词组
 */
void TableDelPhraseByHZ (FcitxTableState* tbl, const char *strHZ)
{
    RECORD         *recTemp;

    recTemp = TableFindPhrase (tbl, strHZ);
    if (recTemp)
        TableDelPhrase (tbl, recTemp);
}

void TableDelPhrase (FcitxTableState* tbl, RECORD * record)
{
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);
    record->prev->next = record->next;
    record->next->prev = record->prev;

    free (record->strCode);
    free (record->strHZ);
    free (record);

    table->iRecordCount--;
}

/*
 *判断某个词组是不是已经在词库中,有返回NULL，无返回插入点
 */
RECORD         *TableHasPhrase (FcitxTableState* tbl, const char *strCode, const char *strHZ)
{
    RECORD         *recTemp;
    int             i;

    i = 0;
    while (strCode[0] != tbl->recordIndex[i].cCode)
        i++;

    recTemp = tbl->recordIndex[i].record;
    while (recTemp != tbl->recordHead) {
        if (!recTemp->bPinyin) {
            if (strcmp (recTemp->strCode, strCode) > 0)
                break;
            else if (!strcmp (recTemp->strCode, strCode)) {
                if (!strcmp (recTemp->strHZ, strHZ))    //该词组已经在词库中
                    return NULL;
            }
        }
        recTemp = recTemp->next;
    }

    return recTemp;
}

/*
 *根据字串判断词库中是否有某个字/词，注意该函数会忽略拼音词组
 */
RECORD         *TableFindPhrase (FcitxTableState* tbl, const char *strHZ)
{
    RECORD         *recTemp;
    char            strTemp[UTF8_MAX_LENGTH + 1];
    int             i;

    //首先，先查找第一个汉字的编码
    strncpy(strTemp, strHZ, utf8_char_len(strHZ));
    strTemp[utf8_char_len(strHZ)] = '\0';

    recTemp = tbl->tableSingleHZ[CalHZIndex (strTemp)];
    if (!recTemp)
        return (RECORD *) NULL;

    //然后根据该编码找到检索的起始点
    i = 0;
    while (recTemp->strCode[0] != tbl->recordIndex[i].cCode)
        i++;

    recTemp = tbl->recordIndex[i].record;
    while (recTemp != tbl->recordHead) {
        if (recTemp->strCode[0] != tbl->recordIndex[i].cCode)
            break;
        if (!strcmp (recTemp->strHZ, strHZ)) {
            if (!recTemp->bPinyin)
                return recTemp;
        }

        recTemp = recTemp->next;
    }

    return (RECORD *) NULL;
}

void TableInsertPhrase (FcitxTableState* tbl, const char *strCode, const char *strHZ)
{
    RECORD         *insertPoint, *dictNew;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    insertPoint = TableHasPhrase (tbl, strCode, strHZ);

    if (!insertPoint)
        return;

    dictNew = (RECORD *) malloc (sizeof (RECORD));
    dictNew->strCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
    strcpy (dictNew->strCode, strCode);
    dictNew->strHZ = (char *) malloc (sizeof (char) * (strlen (strHZ) + 1));
    strcpy (dictNew->strHZ, strHZ);
    dictNew->iHit = 0;
    dictNew->iIndex = tbl->iTableIndex;

    dictNew->prev = insertPoint->prev;
    insertPoint->prev->next = dictNew;
    insertPoint->prev = dictNew;
    dictNew->next = insertPoint;

    table->iRecordCount++;
}

void TableCreateNewPhrase (FcitxTableState* tbl)
{
    int             i;
    FcitxInstance *instance = tbl->owner;

    SetMessageText(instance->messageDown, 0, "");
    for (i = tbl->iTableNewPhraseHZCount; i > 0; i--)
        MessageConcat(instance->messageDown , 0 ,tbl->hzLastInput[tbl->iHZLastInputCount - i].strHZ);

    TableCreatePhraseCode (tbl, GetMessageString(instance->messageDown, 0));

    if (!tbl->bCanntFindCode)
    {    
        SetMessageCount(instance->messageDown, 2);
        SetMessageText(instance->messageDown, 1, tbl->strNewPhraseCode);
    }
    else
    {
        SetMessageCount(instance->messageDown, 1);
        SetMessageText(instance->messageDown, 0, "????");
    }

}

void TableCreatePhraseCode (FcitxTableState* tbl, char *strHZ)
{
    unsigned char   i;
    unsigned char   i1, i2;
    size_t          iLen;
    char            strTemp[UTF8_MAX_LENGTH + 1] = {'\0', };
    RECORD         *recTemp;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    tbl->bCanntFindCode = false;
    iLen = utf8_strlen (strHZ);
    if (iLen >= table->iCodeLength) {
        i2 = table->iCodeLength;
        i1 = 1;
    }
    else {
        i2 = iLen;
        i1 = 0;
    }

    for (i = 0; i < table->iCodeLength - 1; i++) {
        if (table->rule[i].iWords == i2 && table->rule[i].iFlag == i1)
            break;
    }

    for (i1 = 0; i1 < table->iCodeLength; i1++) {
        int clen;
        char* ps;
        if (table->rule[i].rule[i1].iFlag) {
            ps = utf8_get_nth_char(strHZ, table->rule[i].rule[i1].iWhich - 1);
            clen = utf8_char_len(ps);
            strncpy(strTemp, ps, clen);
        }
        else {
            ps = utf8_get_nth_char(strHZ, iLen - table->rule[i].rule[i1].iWhich);
            clen = utf8_char_len(ps);
            strncpy(strTemp, ps, clen);
        }

        recTemp = tbl->tableSingleHZ[CalHZIndex (strTemp)];

        if (!recTemp) {
            tbl->bCanntFindCode = true;
            break;
        }

        tbl->strNewPhraseCode[i1] = recTemp->strCode[table->rule[i].rule[i1].iIndex - 1];
    }
}

/*
 * 获取联想候选字列表
 */
INPUT_RETURN_VALUE TableGetRemindCandWords (FcitxTableState* tbl, SEARCH_MODE mode)
{
    char            strTemp[3];
    int             iLength;
    int             i;
    RECORD         *tableRemind = NULL;
    unsigned int    iTableTotalLengendCandCount = 0;
    TABLECANDWORD* tableCandWord = tbl->tableCandWord;
    GenericConfig *fc = &tbl->owner->config.gconfig;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    boolean bDisablePagingInRemind = *ConfigGetBindValue(fc, "Output", "RemindModeDisablePaging").boolvalue;

    if (!tbl->strTableRemindSource[0])
        return IRV_TO_PROCESS;

    iLength = utf8_strlen (tbl->strTableRemindSource);
    if (mode == SM_FIRST) {
        input->iCurrentRemindCandPage = 0;
        input->iRemindCandPageCount = 0;
        TableResetFlags(tbl);
    }
    else {
        if (!input->iRemindCandPageCount)
            return IRV_TO_PROCESS;

        if (mode == SM_NEXT) {
            if (input->iCurrentRemindCandPage == input->iRemindCandPageCount)
                return IRV_DO_NOTHING;

            input->iCurrentRemindCandPage++;
        }
        else {
            if (!input->iCurrentRemindCandPage)
                return IRV_DO_NOTHING;

            TableSetCandWordsFlag (tbl, input->iRemindCandWordCount, false);
            input->iCurrentRemindCandPage--;
        }
    }

    input->iRemindCandWordCount = 0;
    tableRemind = tbl->recordHead->next;

    while (tableRemind != tbl->recordHead) {
        if (((mode == SM_PREV) ^ (!tableRemind->flag)) && ((iLength + 1) == utf8_strlen (tableRemind->strHZ))) {
            if (!utf8_strncmp (tableRemind->strHZ, tbl->strTableRemindSource, iLength) && utf8_get_nth_char(tableRemind->strHZ, iLength)) {
                if (mode == SM_FIRST)
                    iTableTotalLengendCandCount++;

                TableAddRemindCandWord (tbl, tableRemind, mode);
            }
        }

        tableRemind = tableRemind->next;
    }

    TableSetCandWordsFlag (tbl, input->iRemindCandWordCount, true);

    if (mode == SM_FIRST && bDisablePagingInRemind)
        input->iRemindCandPageCount = iTableTotalLengendCandCount / ConfigGetMaxCandWord(&tbl->owner->config) - ((iTableTotalLengendCandCount % ConfigGetMaxCandWord(&tbl->owner->config)) ? 0 : 1);

    SetMessageCount(instance->messageUp, 0);
    AddMessageAtLast(instance->messageUp, MSG_TIPS, "联想：");
    AddMessageAtLast(instance->messageUp, MSG_INPUT, "%s", tbl->strTableRemindSource);

    if (ConfigGetPointAfterNumber(&tbl->owner->config)) {
        strTemp[1] = '.';
        strTemp[2] = '\0';
    }
    else
        strTemp[1] = '\0';

    SetMessageCount(instance->messageDown, 0);
    for (i = 0; i < input->iRemindCandWordCount; i++) {
        if (i == 9)
            strTemp[0] = '0';
        else
            strTemp[0] = i + 1 + '0';
        AddMessageAtLast(instance->messageDown, MSG_INDEX, "%s", strTemp);

        AddMessageAtLast(instance->messageDown, ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER), "%s", tableCandWord[i].candWord.record->strHZ + strlen (tbl->strTableRemindSource));
        if (i != (input->iRemindCandWordCount - 1)) {
            MessageConcatLast(instance->messageDown, " ");
        }
    }

    input->bIsInRemind = (input->iRemindCandWordCount != 0);

    return IRV_DISPLAY_CANDWORDS;
}

void TableAddRemindCandWord (FcitxTableState* tbl, RECORD * record, SEARCH_MODE mode)
{
    int             i, j;
    TABLECANDWORD *tableCandWord = tbl->tableCandWord;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (mode == SM_PREV) {
        for (i = input->iRemindCandWordCount - 1; i >= 0; i--) {
            if (tableCandWord[i].candWord.record->iHit >= record->iHit)
                break;
        }

        if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            if (i < 0)
                return;
        }
        else
            i++;
    }
    else {
        for (i = 0; i < input->iRemindCandWordCount; i++) {
            if (tableCandWord[i].candWord.record->iHit < record->iHit)
                break;
        }

        if (i == ConfigGetMaxCandWord(&tbl->owner->config))
            return;
    }

    if (mode == SM_PREV) {
        if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config)) {
            for (j = 0; j < i; j++)
                tableCandWord[j].candWord.record = tableCandWord[j + 1].candWord.record;
        }
        else {
            for (j = input->iRemindCandWordCount; j > i; j--)
                tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
        }
    }
    else {
        j = input->iRemindCandWordCount;
        if (input->iRemindCandWordCount == ConfigGetMaxCandWord(&tbl->owner->config))
            j--;
        for (; j > i; j--)
            tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
    }

    tableCandWord[i].flag = CT_NORMAL;
    tableCandWord[i].candWord.record = record;

    if (input->iRemindCandWordCount != ConfigGetMaxCandWord(&tbl->owner->config))
        input->iRemindCandWordCount++;
}

char           *TableGetRemindCandWord (void* arg, int iIndex)
{
    FcitxTableState* tbl = (FcitxTableState*) arg;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;
    TABLECANDWORD *tableCandWord = tbl->tableCandWord;
    if (input->iRemindCandWordCount) {
        if (iIndex > (input->iRemindCandWordCount - 1))
            iIndex = input->iRemindCandWordCount - 1;

        tableCandWord[iIndex].candWord.record->iHit++;
        strcpy (tbl->strTableRemindSource, tableCandWord[iIndex].candWord.record->strHZ + strlen (tbl->strTableRemindSource));
        TableGetRemindCandWords (tbl, SM_FIRST);

        return tbl->strTableRemindSource;
    }
    return NULL;
}

INPUT_RETURN_VALUE TableGetFHCandWords (FcitxTableState* tbl, SEARCH_MODE mode)
{
    char            strTemp[3];
    int             i;
    FcitxInstance *instance = tbl->owner;
    FcitxInputState *input = &instance->input;

    if (!tbl->iFH)
        return IRV_DISPLAY_MESSAGE;

    if (ConfigGetPointAfterNumber(&tbl->owner->config)) {
        strTemp[1] = '.';
        strTemp[2] = '\0';
    }
    else
        strTemp[1] = '\0';

    SetMessageCount(instance->messageDown, 0);

    if (mode == SM_FIRST) {
        input->iCandPageCount = tbl->iFH / ConfigGetMaxCandWord(&tbl->owner->config) - ((tbl->iFH % ConfigGetMaxCandWord(&tbl->owner->config)) ? 0 : 1);
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
        strTemp[0] = i + 1 + '0';
        if (i == 9)
            strTemp[0] = '0';
        AddMessageAtLast(instance->messageDown, MSG_INDEX, strTemp);
        AddMessageAtLast(instance->messageDown, ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER), "%s", tbl->fh[input->iCurrentCandPage * ConfigGetMaxCandWord(&tbl->owner->config) + i].strFH);
        if (i != (ConfigGetMaxCandWord(&tbl->owner->config) - 1)) {
            MessageConcatLast(instance->messageDown, " ");
        }
        if ((input->iCurrentCandPage * ConfigGetMaxCandWord(&tbl->owner->config) + i) >= (tbl->iFH - 1)) {
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
    SetMessageCount(instance->messageDown, 0);

    if (input->iCandWordCount) {
        if (iIndex > (input->iCandWordCount - 1))
            iIndex = input->iCandWordCount - 1;

        return tbl->fh[input->iCurrentCandPage * ConfigGetMaxCandWord(&tbl->owner->config) + iIndex].strFH;
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

    if (!tbl->recordHead)
        return false;

    //如果最近输入了一个词组，这个工作就不需要了
    if (input->lastIsSingleHZ != 1)
        return false;

    j = (tbl->iHZLastInputCount > PHRASE_MAX_LENGTH) ? tbl->iHZLastInputCount - PHRASE_MAX_LENGTH : 0;
    for (i = j; i < tbl->iHZLastInputCount; i++)
        strcat (strTemp, tbl->hzLastInput[i].strHZ);
    //如果只有一个汉字，这个工作也不需要了
    if (utf8_strlen (strTemp) < 2)
        return false;

    //首先要判断是不是已经在词库中
    ps = strTemp;
    for (i = 0; i < (tbl->iHZLastInputCount - j - 1); i++) {
        recTemp = TableFindPhrase (tbl, ps);
        if (recTemp) {
            SetMessageCount(instance->messageUp, 0);
            AddMessageAtLast(instance->messageUp, MSG_TIPS, "词库中有词组 ");
            AddMessageAtLast(instance->messageUp, MSG_INPUT, "%s", ps);

            SetMessageCount(instance->messageDown, 0);
            AddMessageAtLast(instance->messageDown, MSG_FIRSTCAND, "编码为 ");
            AddMessageAtLast(instance->messageDown, MSG_CODE, "%s", recTemp->strCode);
            AddMessageAtLast(instance->messageDown, MSG_TIPS, " ^DEL删除");
            tbl->bTablePhraseTips = true;
            instance->bShowCursor = false;

            return true;
        }
        ps = ps + utf8_char_len(ps);
    }

    return false;
}

void TableCreateAutoPhrase (FcitxTableState* tbl, char iCount)
{
    char            *strHZ;
    short           i, j, k;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    if (!tbl->autoPhrase)
        return;

    strHZ=(char *)malloc((table->iAutoPhrase * UTF8_MAX_LENGTH + 1)*sizeof(char));
    /*
     * 为了提高效率，此处只重新生成新录入字构成的词组
     */
    j = tbl->iHZLastInputCount - table->iAutoPhrase - iCount;
    if (j < 0)
        j = 0;

    for (; j < tbl->iHZLastInputCount - 1; j++) {
        for (i = table->iAutoPhrase; i >= 2; i--) {
            if ((j + i - 1) > tbl->iHZLastInputCount)
                continue;

            strcpy (strHZ, tbl->hzLastInput[j].strHZ);
            for (k = 1; k < i; k++)
                strcat (strHZ, tbl->hzLastInput[j + k].strHZ);

            //再去掉重复的词组
            for (k = 0; k < tbl->iAutoPhrase; k++) {
                if (!strcmp (tbl->autoPhrase[k].strHZ, strHZ))
                    goto _next;
            }
            //然后去掉系统中已经有的词组
            if (TableFindPhrase (tbl, strHZ))
                goto _next;

            TableCreatePhraseCode (tbl, strHZ);
            if (tbl->iAutoPhrase != AUTO_PHRASE_COUNT) {
                tbl->autoPhrase[tbl->iAutoPhrase].flag = false;
                strcpy (tbl->autoPhrase[tbl->iAutoPhrase].strCode, tbl->strNewPhraseCode);
                strcpy (tbl->autoPhrase[tbl->iAutoPhrase].strHZ, strHZ);
                tbl->autoPhrase[tbl->iAutoPhrase].iSelected = 0;
                tbl->iAutoPhrase++;
            }
            else {
                tbl->insertPoint->flag = false;
                strcpy (tbl->insertPoint->strCode, tbl->strNewPhraseCode);
                strcpy (tbl->insertPoint->strHZ, strHZ);
                tbl->insertPoint->iSelected = 0;
                tbl->insertPoint = tbl->insertPoint->next;
            }

_next:
            continue;
        }

    }

    free(strHZ);
}

void UpdateHZLastInput (FcitxTableState* tbl, char *str)
{
    int             i, j;
    char           *pstr;
    TABLE* table = (TABLE*) utarray_eltptr(tbl->table, tbl->iTableIMIndex);

    pstr = str;

    for (i = 0; i < utf8_strlen (str) ; i++) {
        if (tbl->iHZLastInputCount < PHRASE_MAX_LENGTH)
            tbl->iHZLastInputCount++;
        else {
            for (j = 0; j < (tbl->iHZLastInputCount - 1); j++) {
                strncpy(tbl->hzLastInput[j].strHZ, tbl->hzLastInput[j + 1].strHZ, utf8_char_len(tbl->hzLastInput[j + 1].strHZ));
            }
        }
        strncpy(tbl->hzLastInput[tbl->iHZLastInputCount - 1].strHZ, pstr, utf8_char_len(pstr));
        tbl->hzLastInput[tbl->iHZLastInputCount - 1].strHZ[utf8_char_len(pstr)] = '\0';
        pstr = pstr + utf8_char_len(pstr);
    }

    if (table->bRule && table->bAutoPhrase)
        TableCreateAutoPhrase (tbl, (char) (utf8_strlen (str)));
}

void FreeTableConfig(void *v)
{
    TABLE *table = (TABLE*) v;
    if (!table)
        return;
    FreeConfigFile(table->config.configFile);
    free(table->strPath);
    free(table->strSymbolFile);
    free(table->strName);
    free(table->strIconName);
    free(table->strInputCode);
    free(table->strEndCode);
    free(table->strIgnoreChars);
    free(table->strSymbol);
    free(table->strChoose);
}

void Table_LoadPYBaseDict(FcitxTableState *tbl)
{
    FcitxModuleFunctionArg args;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_LOADBASEDICT, args);
}

void Table_PYGetPYByHZ(FcitxTableState *tbl, char *a, char* b)
{
    FcitxModuleFunctionArg args;
    args.args[0] = a;
    args.args[1] = b;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_PYGETPYBYHZ, args);
}

void Table_PYGetCandWord(FcitxTableState *tbl, int a)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &a;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_PYGETCANDWORD, args);
}

void Table_DoPYInput(FcitxTableState *tbl, FcitxKeySym sym, unsigned int state)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &sym;
    args.args[1] = &state;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_DOPYINPUT, args);
}

void Table_PYGetCandWords(FcitxTableState *tbl, SEARCH_MODE mode)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &mode;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_PYGETCANDWORDS, args);
}

void Table_PYGetCandText(FcitxTableState *tbl, int iIndex, char *strText)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &iIndex;
    args.args[1] = strText;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_PYGETCANDTEXT, args);
}

void Table_ResetPY(FcitxTableState *tbl)
{
    FcitxModuleFunctionArg args;
    InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_PYRESET, args);
}

char* Table_PYGetFindString(FcitxTableState *tbl)
{
    FcitxModuleFunctionArg args;
    char * str = InvokeModuleFunction(tbl->pyaddon, FCITX_PINYIN_PYGETFINDSTRING, args);
    return str;
}

unsigned int CalHZIndex (char *strHZ)
{
    unsigned int iutf = 0;
    int l = utf8_char_len(strHZ);
    unsigned char* utf = (unsigned char*) strHZ;
    unsigned int *res;
    int idx;

    if (l == 2)
    {
        iutf = *utf++ << 8;
        iutf |= *utf++;
    }
    else if (l == 3)
    {
        iutf = *utf++ << 16;
        iutf |= *utf++ << 8;
        iutf |= *utf++;
    }
    else if (l == 4)
    {
        iutf = *utf++ << 24;

        iutf |= *utf++ << 16;
        iutf |= *utf++ << 8;
        iutf |= *utf++;
    }

    res = bsearch(&iutf, utf8_in_gb18030, 63360, sizeof(int), cmpi);
    if (res)
        idx = res - utf8_in_gb18030;
    else
        idx = 63361;
    return idx;
}
