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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "core/fcitx.h"
#include "tools/tools.h"

#include "im/table/table.h"
#include "im/special/punc.h"

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "ui/InputWindow.h"
#include "im/pinyin/py.h"
#include "im/pinyin/pyParser.h"
#include "ui/ui.h"
#include "tools/utarray.h"
#include <sys/types.h>
#include <dirent.h>
#include "fcitx-config/xdg.h"
#include "fcitx-config/profile.h"
#include "fcitx-config/cutils.h"

static void FreeTableConfig(void *v);
UT_icd table_icd = {sizeof(TABLE), NULL ,NULL, FreeTableConfig};
TableState tbl;

extern char     strPYAuto[];

extern INT8     iInternalVersion;

extern Display *dpy;

extern char     strCodeInput[];
extern Bool     bIsDoInputOnly;
extern int      iCandPageCount;
extern int      iCurrentCandPage;
extern int      iCandWordCount;
extern int      iLegendCandWordCount;
extern int      iLegendCandPageCount;
extern int      iCurrentLegendCandPage;
extern int      iCodeInputCount;
extern char     strStringGet[];
extern Bool     bIsInLegend;
extern INT8     lastIsSingleHZ;

extern ADJUSTORDER baseOrder;
extern Bool     bSP;
extern Bool     bPYBaseDictLoaded;

extern PYFA    *PYFAList;
extern PYCandWord PYCandWords[];

extern char     strFindString[];
extern ParsePYStruct findMap;

ConfigFileDesc* tableConfigDesc = NULL;

int TablePriorityCmp(const void *a, const void *b)
{
    TABLE *ta, *tb;
    ta = (TABLE*)a;
    tb = (TABLE*)b;
    return ta->iPriority - tb->iPriority;
}

/*
 * 读取码表输入法的名称和文件路径
 */
void LoadTableInfo (void)
{
    char **tablePath;
    size_t len;
    char pathBuf[PATH_MAX];
    int i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;

	StringHashSet* sset = NULL;
    tbl.bTablePhraseTips = False;

    if (tbl.table)
    {
        utarray_free(tbl.table);
        tbl.table = NULL;
    }

    tbl.hkTableDelPhrase[0].iKeyCode = CTRL_7;
    tbl.hkTableAdjustOrder[0].iKeyCode = CTRL_6;
    tbl.hkTableAddPhrase[0].iKeyCode = CTRL_8;

    tbl.table = malloc(sizeof(UT_array));
    tbl.iTableCount = 0;
    utarray_init(tbl.table, &table_icd);

    tablePath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/table" , DATADIR, "fcitx/data/table" );

    for(i = 0; i< len; i++)
    {
        snprintf(pathBuf, sizeof(pathBuf), "%s", tablePath[i]);
        pathBuf[sizeof(pathBuf) - 1] = '\0';

        dir = opendir(pathBuf);
        if (dir == NULL)
            continue;

		/* collect all *.conf files */
        while((drt = readdir(dir)) != NULL)
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
            utarray_push_back(tbl.table, &table);
            TABLE *t = (TABLE*)utarray_back(tbl.table);
            TABLEConfigBind(t, cfile, GetTableConfigDesc());
            ConfigBindSync((GenericConfig*)t);
            FcitxLog(DEBUG, _("Table Config %s is %s"),string->name, (t->bEnabled)?"Enabled":"Disabled");
            if (t->bEnabled)
                tbl.iTableCount ++;
            else
            {
                utarray_pop_back(tbl.table);
            }
        }
    }

    utarray_sort(tbl.table, TablePriorityCmp);

    for (i = 0;i < len ;i ++)
        free(paths[i]);
    free(paths);
		

    FreeXDGPath(tablePath);

	StringHashSet *curStr;
	while(sset)
	{
		curStr = sset;
		HASH_DEL(sset, curStr);
		free(curStr->name);
        free(curStr);
	}
}

ConfigFileDesc *GetTableConfigDesc()
{
	if (!tableConfigDesc)
	{
		FILE *tmpfp;
		tmpfp = GetXDGFileData("table.desc", "r", NULL);
		tableConfigDesc = ParseConfigFileDescFp(tmpfp);
		fclose(tmpfp);
	}

	return tableConfigDesc;
}

Bool LoadTableDict (void)
{
    char            strCode[MAX_CODE_LENGTH + 1];
    char            strHZ[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1];
    FILE           *fpDict;
    RECORD         *recTemp;
    char            strPath[PATH_MAX];
    unsigned int    i = 0;
    unsigned int    iTemp, iTempCount;
    char            cChar = 0, cTemp;
    INT8            iVersion = 1;
    int             iRecordIndex;

    //首先，来看看我们该调入哪个码表
    for (i = 0; i < tbl.iTableCount; i++) {
        TABLE* table =(TABLE*) utarray_eltptr(tbl.table, i);
        if (table->iIMIndex == gs.iIMIndex)
            tbl.iTableIMIndex = i;
    }

    //读入码表
#ifdef _DEBUG
    FcitxLog(DEBUG, _("Loading Table Dict"));
#endif

    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    {
        size_t len;
        char *pstr = NULL;
        char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/table" , DATADIR, "fcitx/data/table" );
        fpDict = GetXDGFile(table->strPath, path, "r", len, &pstr);
        FcitxLog(INFO, _("Load Table Dict from %s"), pstr); 
        free(pstr);
        FreeXDGPath(path);
    }
	
    if (!fpDict) {
	    FcitxLog( DEBUG, _("Cannot load table file: %s"), strPath);
	    return False;
	}

    //先读取码表的信息
    //判断版本信息
    fread (&iTemp, sizeof (unsigned int), 1, fpDict);
    if (!iTemp) {
	fread (&iVersion, sizeof (INT8), 1, fpDict);
	iVersion = (iVersion < iInternalVersion);
	fread (&iTemp, sizeof (unsigned int), 1, fpDict);
    }

    table->strInputCode = (char *) malloc (sizeof (char) * (iTemp + 1));
    fread (table->strInputCode, sizeof (char), iTemp + 1, fpDict);
    /*
     * 建立索引，加26是为了为拼音编码预留空间
     */
    tbl.recordIndex = (RECORD_INDEX *) malloc ((strlen (table->strInputCode) + 26) * sizeof (RECORD_INDEX));
    for (iTemp = 0; iTemp < strlen (table->strInputCode) + 26; iTemp++) {
	tbl.recordIndex[iTemp].cCode = 0;
	tbl.recordIndex[iTemp].record = NULL;
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

    if (table->bRule) {	//表示有组词规则
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

    tbl.recordHead = (RECORD *) malloc (sizeof (RECORD));
    tbl.currentRecord = tbl.recordHead;

    fread (&(table->iRecordCount), sizeof (unsigned int), 1, fpDict);

    for (i = 0; i < SINGLE_HZ_COUNT; i++)
	tbl.tableSingleHZ[i] = (RECORD *) NULL;

    iRecordIndex = 0;
    for (i = 0; i < table->iRecordCount; i++) {
	fread (strCode, sizeof (char), table->iPYCodeLength + 1, fpDict);
	fread (&iTemp, sizeof (unsigned int), 1, fpDict);
	fread (strHZ, sizeof (char), iTemp, fpDict);
	recTemp = (RECORD *) malloc (sizeof (RECORD));
	recTemp->strCode = (char *) malloc0 (sizeof (char) * (table->iPYCodeLength + 1));
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
	if (recTemp->iIndex > tbl.iTableIndex)
	    tbl.iTableIndex = recTemp->iIndex;

	/* 建立索引 */
	if (cChar != recTemp->strCode[0]) {
	    cChar = recTemp->strCode[0];
	    tbl.recordIndex[iRecordIndex].cCode = cChar;
	    tbl.recordIndex[iRecordIndex].record = recTemp;
	    iRecordIndex++;
	}
	/* **************************************************************** */
	/** 为单字生成一个表   */
	if (utf8_strlen (recTemp->strHZ) == 1 && !IsIgnoreChar (strCode[0]) && !recTemp->bPinyin) {
	    iTemp = CalHZIndex (recTemp->strHZ);
	    if (iTemp >= 0 && iTemp < SINGLE_HZ_COUNT) {
		if (tbl.tableSingleHZ[iTemp]) {
		    if (strlen (strCode) > strlen (tbl.tableSingleHZ[iTemp]->strCode))
			tbl.tableSingleHZ[iTemp] = recTemp;
		}
		else
		    tbl.tableSingleHZ[iTemp] = recTemp;
	    }
	}

	if (recTemp->bPinyin)
	    table->bHasPinyin = True;

	tbl.currentRecord->next = recTemp;
	recTemp->prev = tbl.currentRecord;
	tbl.currentRecord = recTemp;
    }

    tbl.currentRecord->next = tbl.recordHead;
    tbl.recordHead->prev = tbl.currentRecord;

    fclose (fpDict);
#ifdef _DEBUG
    FcitxLog(DEBUG, _("Load Table Dict OK"));
#endif

    //读取相应的特殊符号表
    fpDict = GetXDGFileTable(table->strSymbolFile, "rt", NULL, False);

    if (fpDict) {
	tbl.iFH = CalculateRecordNumber (fpDict);
	tbl.fh = (FH *) malloc (sizeof (FH) * tbl.iFH);

	for (i = 0; i < tbl.iFH; i++) {
	    if (EOF == fscanf (fpDict, "%s\n", tbl.fh[i].strFH))
		break;
	}
	tbl.iFH = i;

	fclose (fpDict);
    }

    tbl.strNewPhraseCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
    tbl.strNewPhraseCode[table->iCodeLength] = '\0';
    tbl.bTableDictLoaded = True;

    tbl.iAutoPhrase = 0;
    if (table->bAutoPhrase) {
	//为自动词组分配空间
	tbl.autoPhrase = (AUTOPHRASE *) malloc (sizeof (AUTOPHRASE) * AUTO_PHRASE_COUNT);

	//读取上次保存的自动词组信息
#ifdef _DEBUG
	FcitxLog(DEBUG, _("Loading Autophrase."));
#endif

	strcpy (strPath, table->strName);
	strcat (strPath, "_LastAutoPhrase.tmp");
	fpDict = GetXDGFileTable(strPath, "rb", NULL, True);
	i = 0;
	if (fpDict) {
	    fread (&tbl.iAutoPhrase, sizeof (unsigned int), 1, fpDict);

	    for (; i < tbl.iAutoPhrase; i++) {
		tbl.autoPhrase[i].strCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
		tbl.autoPhrase[i].strHZ = (char *) malloc (sizeof (char) * (PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1));
		fread (tbl.autoPhrase[i].strCode, table->iCodeLength + 1, 1, fpDict);
		fread (tbl.autoPhrase[i].strHZ, PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1, 1, fpDict);
		fread (&iTempCount, sizeof (unsigned int), 1, fpDict);
		tbl.autoPhrase[i].iSelected = iTempCount;
		if (i == AUTO_PHRASE_COUNT - 1)
		    tbl.autoPhrase[i].next = &tbl.autoPhrase[0];
		else
		    tbl.autoPhrase[i].next = &tbl.autoPhrase[i + 1];
	    }
	    fclose (fpDict);
	}

	for (; i < AUTO_PHRASE_COUNT; i++) {
	    tbl.autoPhrase[i].strCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
	    tbl.autoPhrase[i].strHZ = (char *) malloc (sizeof (char) * (PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1));
	    tbl.autoPhrase[i].iSelected = 0;
	    if (i == AUTO_PHRASE_COUNT - 1)
		tbl.autoPhrase[i].next = &tbl.autoPhrase[0];
	    else
		tbl.autoPhrase[i].next = &tbl.autoPhrase[i + 1];
	}

	if (i == AUTO_PHRASE_COUNT)
	    tbl.insertPoint = &tbl.autoPhrase[0];
	else
	    tbl.insertPoint = &tbl.autoPhrase[i - 1];

#ifdef _DEBUG
	FcitxLog(DEBUG, _("Load Autophrase OK"));
#endif
    }
    else
	tbl.autoPhrase = (AUTOPHRASE *) NULL;

    return True;
}

void TableInit (void)
{
    bSP = False;
    tbl.PYBaseOrder = fc.baseOrder;
    strPYAuto[0] = '\0';
}

/*
 * 释放当前码表所占用的内存
 * 目的是切换码表时使占用的内存减少
 */
void FreeTableIM (void)
{
    RECORD         *recTemp, *recNext;
    INT16           i;

    if (!tbl.recordHead)
	return;

    if (tbl.iTableChanged)
	SaveTableDict ();

    //释放码表
    recTemp = tbl.recordHead->next;
    while (recTemp != tbl.recordHead) {
	recNext = recTemp->next;

	free (recTemp->strCode);
	free (recTemp->strHZ);
	free (recTemp);

	recTemp = recNext;
    }

    free (tbl.recordHead);
    tbl.recordHead = NULL;

    if (tbl.iFH) {
	free (tbl.fh);
	tbl.iFH = 0;
    }
    
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    table->iRecordCount = 0;
    tbl.bTableDictLoaded = False;

    free (tbl.strNewPhraseCode);

    //释放组词规则的空间
    if (table->rule) {
	for (i = 0; i < table->iCodeLength - 1; i++)
	    free (table->rule[i].rule);
	free (table->rule);

	table->rule = NULL;
    }

    //释放索引的空间
    if (tbl.recordIndex) {
	free (tbl.recordIndex);
	tbl.recordIndex = NULL;
    }

    //释放自动词组的空间
    if (tbl.autoPhrase) {
	for (i = 0; i < AUTO_PHRASE_COUNT; i++) {
	    free (tbl.autoPhrase[i].strCode);
	    free (tbl.autoPhrase[i].strHZ);
	}
	free (tbl.autoPhrase);

	tbl.autoPhrase = (AUTOPHRASE *) NULL;
    }

    fc.baseOrder = tbl.PYBaseOrder;
}

void TableResetStatus (void)
{
    tbl.bIsTableAddPhrase = False;
    tbl.bIsTableDelPhrase = False;
    tbl.bIsTableAdjustOrder = False;
    bIsDoInputOnly = False;
    //bSingleHZMode = False;
}

void SaveTableDict (void)
{
    RECORD         *recTemp;
    char            strPathTemp[PATH_MAX];
    char            strPath[PATH_MAX];
    char	   *pstr;
    FILE           *fpDict;
    unsigned int    iTemp;
    unsigned int    i;
    char            cTemp;

    //先将码表保存在一个临时文件中，如果保存成功，再将该临时文件改名是码表名──这样可以防止码表文件被破坏
#ifdef _ENABLE_LOG
    int		iDebug = 0;
    char	buf[PATH_MAX];
    struct tm	*ts;
    time_t	now;
    FILE	*logfile = (FILE *)NULL;;
#endif

    if ( tbl.isSavingTableDic )
	return;

    if (!tbl.iTableChanged)
        return;

    tbl.isSavingTableDic = True;
    
#ifdef _ENABLE_LOG       
    logfile = GetXDGFileUser("fcitx-dict.log", "wt", NULL);
    if ( logfile ) {
	now = time(NULL);
	ts = localtime(&now);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	fprintf (logfile, "(%s)Saving table dict...\n",buf);
    }
#endif

    
    fpDict = GetXDGFileTable(TEMP_FILE, "wb", &pstr, True);
    if (!fpDict) {
	tbl.isSavingTableDic = False;
	FcitxLog(ERROR, _("Cannot create table file: %s"), pstr);
	return;
    }
    strcpy(strPathTemp, pstr);
    free(pstr);

#ifdef _DEBUG
    FcitxLog(DEBUG, _("Saving TABLE Dict."));
#endif

    //写入版本号--如果第一个字为0,表示后面那个字节为版本号，为了与老版本兼容
    iTemp = 0;
    fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
    fwrite (&iInternalVersion, sizeof (INT8), 1, fpDict);
    
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    iTemp = strlen (table->strInputCode);
    fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
    fwrite (table->strInputCode, sizeof (char), iTemp + 1, fpDict);
    fwrite (&(table->iCodeLength), sizeof (INT8), 1, fpDict);
    fwrite (&(table->iPYCodeLength), sizeof (INT8), 1, fpDict);
    iTemp = strlen (table->strIgnoreChars);
    fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
    fwrite (table->strIgnoreChars, sizeof (char), iTemp + 1, fpDict);

    fwrite (&(table->bRule), sizeof (unsigned char), 1, fpDict);
    if (table->bRule) {	//表示有组词规则
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
    recTemp = tbl.recordHead->next;
    while (recTemp != tbl.recordHead) {
	fwrite (recTemp->strCode, sizeof (char), table->iPYCodeLength + 1, fpDict);

#ifdef _ENABLE_LOG
	if ( logfile )
	    fprintf (logfile, "\t%d:%s", iDebug, recTemp->strCode);
#endif

	iTemp = strlen (recTemp->strHZ) + 1;
	fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
	fwrite (recTemp->strHZ, sizeof (char), iTemp, fpDict);

#ifdef _ENABLE_LOG
	if ( logfile )
	    fprintf (logfile, " %s", recTemp->strHZ);
#endif

	cTemp = recTemp->bPinyin;
	fwrite (&cTemp, sizeof (char), 1, fpDict);
	fwrite (&(recTemp->iHit), sizeof (unsigned int), 1, fpDict);
	fwrite (&(recTemp->iIndex), sizeof (unsigned int), 1, fpDict);

#ifdef _ENABLE_LOG
        if (logfile) {
	    fprintf (logfile, " %d %d\n", recTemp->iHit, recTemp->iIndex);
	    iDebug++;
	}
#endif

	recTemp = recTemp->next;
    }

    fclose (fpDict);

#ifdef _ENABLE_LOG
    if (logfile) {
        fprintf (logfile, "Table dict saved\n");
        fclose (logfile);
    }
#endif

    fpDict = GetXDGFileTable(table->strPath, NULL, &pstr, True);
    if (access (pstr, 0))
        unlink (pstr);
    rename (strPathTemp, pstr);
    free(pstr);

#ifdef _DEBUG
    FcitxLog(DEBUG, _("Rename OK"));
#endif

    tbl.iTableChanged = 0;

    if (tbl.autoPhrase) {
	//保存上次的自动词组信息
	fpDict = GetXDGFileTable(TEMP_FILE, "wb", &pstr, True);
	strcpy (strPathTemp,pstr);
	if (fpDict) {
	    fwrite (&tbl.iAutoPhrase, sizeof (int), 1, fpDict);
	    for (i = 0; i < tbl.iAutoPhrase; i++) {
		fwrite (tbl.autoPhrase[i].strCode, table->iCodeLength + 1, 1, fpDict);
		fwrite (tbl.autoPhrase[i].strHZ, PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1, 1, fpDict);
		iTemp = tbl.autoPhrase[i].iSelected;
		fwrite (&iTemp, sizeof (unsigned int), 1, fpDict);
	    }
	    fclose (fpDict);
	}
    free(pstr);

	strcpy (strPath, table->strName);
	strcat (strPath, "_LastAutoPhrase.tmp");
	fpDict = GetXDGFileTable(strPath, NULL, &pstr, True);
	if (access (pstr, 0))
	    unlink (pstr);
	rename (strPathTemp, pstr);
    free(pstr);
    }

    tbl.isSavingTableDic = False;
}

Bool IsInputKey (int iKey)
{
    char           *p;

    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    p = table->strInputCode;
    if (!p)
	return False;

    while (*p) {
	if (iKey == *p)
	    return True;
	p++;
    }

    if (table->bHasPinyin) {
	if (iKey >= 'a' && iKey <= 'z')
	    return True;
    }

    return False;
}

Bool IsEndKey (char cChar)
{
    char           *p;

    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    p = table->strEndCode;
    if (!p)
	return False;

    while (*p) {
	if (cChar == *p)
	    return True;
	p++;
    }

    return False;
}

Bool IsIgnoreChar (char cChar)
{
    char           *p;

    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    p = table->strIgnoreChars;
    while (*p) {
	if (cChar == *p)
	    return True;
	p++;
    }

    return False;
}

INT8 IsChooseKey (int iKey)
{
    int i = 0;

    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    while (table->strChoose[i]) {
	if (iKey == table->strChoose[i])
	    return i + 1;
	i++;
    }

    return 0;
}

INPUT_RETURN_VALUE DoTableInput (unsigned int sym, unsigned int state, int keyCount)
{
    INPUT_RETURN_VALUE retVal;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
        
    unsigned int iKeyState;
    unsigned int iKey;
    iKeyState = state - (state & KEY_NUMLOCK) - (state & KEY_CAPSLOCK) - (state & KEY_SCROLLLOCK);
    iKey = GetKey (sym, iKeyState, keyCount);

    if (!tbl.bTableDictLoaded)
	LoadTableDict ();

    if (tbl.bTablePhraseTips) {
        if (iKey == CTRL_DELETE) {
            tbl.bTablePhraseTips = False;
            TableDelPhraseByHZ (messageUp.msg[1].strMsg);
            return IRV_DONOT_PROCESS_CLEAN;
        }
        else if (iKey != LCTRL && iKey != RCTRL && iKey != LSHIFT && iKey != RSHIFT) {
            SetMessageCount(&messageUp, 0);
            SetMessageCount(&messageDown, 0);
            tbl.bTablePhraseTips = False;
            CloseInputWindow();
        }
    }

    retVal = IRV_DO_NOTHING;
    if (IsInputKey (iKey) || IsEndKey (iKey) || iKey == table->cMatchingKey || iKey == table->cPinyin) {
	bIsInLegend = False;

	if (!tbl.bIsTableAddPhrase && !tbl.bIsTableDelPhrase && !tbl.bIsTableAdjustOrder) {
	    if (strCodeInput[0] == table->cPinyin && table->bUsePY) {
		if (iCodeInputCount != (MAX_PY_LENGTH * 5 + 1)) {
		    strCodeInput[iCodeInputCount++] = (char) iKey;
		    strCodeInput[iCodeInputCount] = '\0';
		    retVal = TableGetCandWords (SM_FIRST);
		}
		else
		    retVal = IRV_DO_NOTHING;
	    }
	    else {
		if ((iCodeInputCount < table->iCodeLength) || (table->bHasPinyin && iCodeInputCount < table->iPYCodeLength)) {
		    strCodeInput[iCodeInputCount++] = (char) iKey;
		    strCodeInput[iCodeInputCount] = '\0';

		    if (iCodeInputCount == 1 && strCodeInput[0] == table->cPinyin && table->bUsePY) {
			iCandWordCount = 0;
			retVal = IRV_DISPLAY_LAST;
		    }
		    else {
			char		*strTemp;
			char		*strLastFirstCand;
			CANDTYPE	 lastFirstCandType;
			RECORD		*pLastCandRecord = (RECORD *)NULL;
			

			strLastFirstCand = (char *)NULL;
			lastFirstCandType = CT_AUTOPHRASE;
			if ( iCandWordCount ) {			// to realize auto-sending HZ to client
			    strLastFirstCand = _TableGetCandWord (0,False);
			    lastFirstCandType = tableCandWord[0].flag;
			    pLastCandRecord = tbl.pCurCandRecord;
		        }
			
			retVal = TableGetCandWords (SM_FIRST);
			strTemp = GetPunc (strCodeInput[0]);
			if (IsEndKey (iKey)) {
			    if (iCodeInputCount == 1)
				return IRV_TO_PROCESS;

			    if (!iCandWordCount) {
				iCodeInputCount = 0;
				return IRV_CLEAN;
			    }

			    if (iCandWordCount == 1) {
				strcpy (strStringGet, TableGetCandWord (0));
				return IRV_GET_CANDWORDS;
			    }

			    return IRV_DISPLAY_CANDWORDS;
			}
			else if (table->iTableAutoSendToClientWhenNone && (iCodeInputCount == (table->iTableAutoSendToClientWhenNone + 1)) && !iCandWordCount) {
			    if ( strLastFirstCand && (lastFirstCandType!=CT_AUTOPHRASE)) {
			    	strcpy (strStringGet, strLastFirstCand);
				TableUpdateHitFrequency(pLastCandRecord);
				retVal=IRV_GET_CANDWORDS_NEXT;
			    }
			    else 
			    	retVal = IRV_DISPLAY_CANDWORDS;
			    iCodeInputCount = 1;
			    strCodeInput[0] = iKey;
			    strCodeInput[1] = '\0';
			    TableGetCandWords (SM_FIRST);
			}
			else if (table->iTableAutoSendToClient && (iCodeInputCount >= table->iTableAutoSendToClient)) {
			    if (iCandWordCount == 1 && (tableCandWord[0].flag != CT_AUTOPHRASE || (tableCandWord[0].flag == CT_AUTOPHRASE && !table->iSaveAutoPhraseAfter))) {	//如果只有一个候选词，则送到客户程序中
			    	retVal = IRV_DO_NOTHING;
				if (tableCandWord[0].flag == CT_NORMAL) {
				    if (tableCandWord[0].candWord.record->bPinyin)
					retVal = IRV_DISPLAY_CANDWORDS;
				}

				if (retVal != IRV_DISPLAY_CANDWORDS) {
				    strcpy (strStringGet, TableGetCandWord (0));
				    iCandWordCount = 0;
				    if (bIsInLegend)
					retVal = IRV_GET_LEGEND;
				    else
					retVal = IRV_GET_CANDWORDS;
				}
			    }
			}
			else if ((iCodeInputCount == 1) && strTemp && !iCandWordCount) {	//如果第一个字母是标点，并且没有候选字/词，则当做标点处理──适用于二笔这样的输入法
			    strcpy (strStringGet, strTemp);
			    retVal = IRV_PUNC;
			}
		    }
		}
		else {
		    if (table->iTableAutoSendToClient) {
			if (iCandWordCount) {
			    if (tableCandWord[0].flag != CT_AUTOPHRASE) {
				strcpy (strStringGet, TableGetCandWord (0));
				retVal = IRV_GET_CANDWORDS_NEXT;
			    }
			    else
				retVal = IRV_DISPLAY_CANDWORDS;
			}
			else
			    retVal = IRV_DISPLAY_CANDWORDS;

			iCodeInputCount = 1;
			strCodeInput[0] = iKey;
			strCodeInput[1] = '\0';
			bIsInLegend = False;

			if (retVal != IRV_DISPLAY_CANDWORDS)
			    TableGetCandWords (SM_FIRST);
		    }
		    else
			retVal = IRV_DO_NOTHING;
		}
	    }
	}
    }
    else {
	if (tbl.bIsTableAddPhrase) {
	    switch (iKey) {
	    case LEFT:
		if (tbl.iTableNewPhraseHZCount < tbl.iHZLastInputCount && tbl.iTableNewPhraseHZCount < PHRASE_MAX_LENGTH) {
		    tbl.iTableNewPhraseHZCount++;
		    TableCreateNewPhrase ();
		}
		break;
	    case RIGHT:
		if (tbl.iTableNewPhraseHZCount > 2) {
		    tbl.iTableNewPhraseHZCount--;
		    TableCreateNewPhrase ();
		}
		break;
	    case ENTER_K:
		if (!tbl.bCanntFindCode)
		    TableInsertPhrase (messageDown.msg[1].strMsg, messageDown.msg[0].strMsg);
	    case ESC:
		tbl.bIsTableAddPhrase = False;
		bIsDoInputOnly = False;
		return IRV_CLEAN;
	    default:
		return IRV_DO_NOTHING;
	    }

	    return IRV_DISPLAY_MESSAGE;
	}
	if (IsHotKey (iKey, tbl.hkTableAddPhrase)) {
	    if (!tbl.bIsTableAddPhrase) {
		if (tbl.iHZLastInputCount < 2 || !table->bRule)	//词组最少为两个汉字
		    return IRV_DO_NOTHING;

		tbl.iTableNewPhraseHZCount = 2;
		tbl.bIsTableAddPhrase = True;
		bIsDoInputOnly = True;
		inputWindow.bShowCursor = False;

		SetMessageCount(&messageUp , 0);
        AddMessageAtLast(&messageUp, MSG_TIPS, "左/右键增加/减少，ENTER确定，ESC取消");

		SetMessageCount(&messageDown , 0);
        AddMessageAtLast(&messageDown, MSG_FIRSTCAND, "");
        AddMessageAtLast(&messageDown, MSG_CODE, "");

		TableCreateNewPhrase ();
		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else
		retVal = IRV_TO_PROCESS;

	    return retVal;
	}
	else if (IsHotKey (iKey, fc.hkGetPY)) {
	    char            strPY[100];

	    //如果拼音单字字库没有读入，则读入它
	    if (!bPYBaseDictLoaded)
		LoadPYBaseDict ();

	    //如果刚刚输入的是个词组，刚不查拼音
	    if (utf8_strlen (strStringGet) != 1)
		return IRV_DO_NOTHING;

	    iCodeInputCount = 0;
        SetMessageCount(&messageUp, 0);
        AddMessageAtLast(&messageUp, MSG_INPUT, strStringGet);
        
        SetMessageCount(&messageDown, 0);
        AddMessageAtLast(&messageDown, MSG_CODE, "读音：");
	    PYGetPYByHZ (strStringGet, strPY);
        AddMessageAtLast(&messageDown, MSG_TIPS, (strPY[0]) ? strPY : "无法查到该字读音");
	    inputWindow.bShowCursor = False;

	    return IRV_DISPLAY_MESSAGE;
	}

	if (!iCodeInputCount && !bIsInLegend)
	    return IRV_TO_PROCESS;

	if (iKey == ESC) {
	    if (tbl.bIsTableDelPhrase || tbl.bIsTableAdjustOrder) {
		TableResetStatus ();
		retVal = IRV_DISPLAY_CANDWORDS;
	    }
	    else
		return IRV_CLEAN;
	}
	else if (IsChooseKey(iKey)) {
	    iKey = IsChooseKey(iKey);

	    if (!bIsInLegend) {
		if (!iCandWordCount)
		    return IRV_TO_PROCESS;

		if (iKey > iCandWordCount)
		    return IRV_DO_NOTHING;
		else {
		    if (tbl.bIsTableDelPhrase) {
			TableDelPhraseByIndex (iKey);
			retVal = TableGetCandWords (SM_FIRST);
		    }
		    else if (tbl.bIsTableAdjustOrder) {
			TableAdjustOrderByIndex (iKey);
			retVal = TableGetCandWords (SM_FIRST);
		    }
		    else {
			//如果是拼音，进入快速调整字频方式
			if (strcmp (strCodeInput, table->strSymbol) && strCodeInput[0] == table->cPinyin && table->bUsePY)
			    PYGetCandWord (iKey - 1);
			strcpy (strStringGet, TableGetCandWord (iKey - 1));

			if (bIsInLegend)
			    retVal = IRV_GET_LEGEND;
			else
			    retVal = IRV_GET_CANDWORDS;
		    }
		}
	    }
	    else {
		strcpy (strStringGet, TableGetLegendCandWord (iKey - 1));
		retVal = IRV_GET_LEGEND;
	    }
	}
	else if (!tbl.bIsTableDelPhrase && !tbl.bIsTableAdjustOrder) {
	    if (IsHotKey (iKey, tbl.hkTableAdjustOrder)) {
		if ((tbl.iTableCandDisplayed == iCandWordCount && iCandWordCount < 2) || bIsInLegend)
		    return IRV_DO_NOTHING;

		tbl.bIsTableAdjustOrder = True;
        SetMessageCount(&messageUp, 0);
        AddMessageAtLast(&messageUp, MSG_TIPS, "选择需要提前的词组序号，ESC结束");
		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else if (IsHotKey (iKey, tbl.hkTableDelPhrase)) {
		if (!iCandWordCount || bIsInLegend)
		    return IRV_DO_NOTHING;

		tbl.bIsTableDelPhrase = True;
        SetMessageCount(&messageUp, 0);
        AddMessageAtLast(&messageUp, MSG_TIPS, "选择需要删除的词组序号，ESC取消");
		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else if (iKey == (XK_BackSpace & 0x00FF) || iKey == CTRL_H) {
		if (!iCodeInputCount) {
		    bIsInLegend = False;
		    return IRV_DONOT_PROCESS_CLEAN;
		}

		iCodeInputCount--;
		strCodeInput[iCodeInputCount] = '\0';
		
		if (iCodeInputCount == 1 && strCodeInput[0] == table->cPinyin && table->bUsePY) {
		    iCandWordCount = 0;
		    retVal = IRV_DISPLAY_LAST;
		}
		else if (iCodeInputCount)
		    retVal = TableGetCandWords (SM_FIRST);
		else
		    retVal = IRV_CLEAN;
	    }
	    else if (iKey == ' ') {
		if (!bIsInLegend) {
		    if (!(table->bUsePY && iCodeInputCount == 1 && strCodeInput[0] == table->cPinyin)) {
			if (strcmp (strCodeInput, table->strSymbol) && strCodeInput[0] == table->cPinyin && table->bUsePY)
			    PYGetCandWord (0);

			if (!iCandWordCount) {
			    iCodeInputCount = 0;
			    return IRV_CLEAN;
			}

			strcpy (strStringGet, TableGetCandWord (0));
		    }
		    else
			SetMessageCount(&messageDown, 0);

		    if (bIsInLegend)
			retVal = IRV_GET_LEGEND;
		    else
			retVal = IRV_GET_CANDWORDS;

		}
		else {
		    strcpy (strStringGet, TableGetLegendCandWord (0));
		    retVal = IRV_GET_LEGEND;
		}
	    }
	    else
		return IRV_TO_PROCESS;
	}
    }

    if (!bIsInLegend) {
	if (!tbl.bIsTableDelPhrase && !tbl.bIsTableAdjustOrder) {
		SetMessageCount(&messageUp,0);
	    if (iCodeInputCount) {
        AddMessageAtLast(&messageUp, MSG_INPUT, strCodeInput);
	    }
	}
	else
	    bIsDoInputOnly = True;
    }

    if (tbl.bIsTableDelPhrase || tbl.bIsTableAdjustOrder || bIsInLegend)
	inputWindow.bShowCursor = False;
    else
	inputWindow.bShowCursor = True;

    return retVal;
}

char           *TableGetCandWord (int iIndex)
{
    char *str;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    
    str=_TableGetCandWord(iIndex, True);
    if (str) {
    	if (table->bAutoPhrase && (utf8_strlen (str) == 1 || (utf8_strlen (str) > 1 && table->bAutoPhrasePhrase)))
    	    UpdateHZLastInput (str);
	    
	if ( tbl.pCurCandRecord )
	    TableUpdateHitFrequency (tbl.pCurCandRecord);
    }
    
    return str;
}

void		TableUpdateHitFrequency (RECORD * record)
{
    record->iHit++;
    record->iIndex = ++tbl.iTableIndex;
}

/*
 * 第二个参数表示是否进入联想模式，实现自动上屏功能时，不能使用联想模式
 */
char           *_TableGetCandWord (int iIndex, Bool _bLegend)
{
    char           *pCandWord = NULL;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    
    if (!strcmp (strCodeInput, table->strSymbol))
	return TableGetFHCandWord (iIndex);

    bIsInLegend = False;

    if (!iCandWordCount)
	return NULL;

    if (iIndex > (iCandWordCount - 1))
	iIndex = iCandWordCount - 1;

    if (tableCandWord[iIndex].flag == CT_NORMAL)
	tbl.pCurCandRecord = tableCandWord[iIndex].candWord.record;
    else
	tbl.pCurCandRecord = (RECORD *)NULL;

    tbl.iTableChanged++;
    if (tbl.iTableChanged >= TABLE_AUTO_SAVE_AFTER)
	SaveTableDict ();

    switch (tableCandWord[iIndex].flag) {
    case CT_NORMAL:
	pCandWord = tableCandWord[iIndex].candWord.record->strHZ;
	break;
    case CT_AUTOPHRASE:
	if (table->iSaveAutoPhraseAfter) {
	    /* 当_bLegend为False时，不应该计算自动组词的频度，因此此时实际并没有选择这个词 */
	    if (table->iSaveAutoPhraseAfter >= tableCandWord[iIndex].candWord.autoPhrase->iSelected && _bLegend)
		tableCandWord[iIndex].candWord.autoPhrase->iSelected++;
	    if (table->iSaveAutoPhraseAfter == tableCandWord[iIndex].candWord.autoPhrase->iSelected)	//保存自动词组
		TableInsertPhrase (tableCandWord[iIndex].candWord.autoPhrase->strCode, tableCandWord[iIndex].candWord.autoPhrase->strHZ);
	}
	pCandWord = tableCandWord[iIndex].candWord.autoPhrase->strHZ;
	break;
    case CT_PYPHRASE:
	pCandWord = tableCandWord[iIndex].candWord.strPYPhrase;
	break;
    default:
	;
    }

    if (fcitxProfile.bUseLegend && _bLegend) {
	strcpy (tbl.strTableLegendSource, pCandWord);
	TableGetLegendCandWords (SM_FIRST);
    }
    else {
	if (table->bPromptTableCode) {
	    RECORD         *temp;

	    SetMessageCount(&messageUp, 0);
        AddMessageAtLast(&messageUp, MSG_INPUT, strCodeInput);

        SetMessageCount(&messageDown, 0);
        AddMessageAtLast(&messageDown, MSG_TIPS, pCandWord);
	    temp = tbl.tableSingleHZ[CalHZIndex (pCandWord)];
	    if (temp) {
        AddMessageAtLast(&messageDown, MSG_CODE, temp->strCode);
	    }
	}
	else {
	    SetMessageCount(&messageDown, 0);
	    SetMessageCount(&messageUp, 0);
	}
    }

    if (utf8_strlen (pCandWord) == 1)
	lastIsSingleHZ = 1;
    else
	lastIsSingleHZ = 0;

    return pCandWord;
}

INPUT_RETURN_VALUE TableGetPinyinCandWords (SEARCH_MODE mode)
{
    int             i;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    if (mode == SM_FIRST) {
	strcpy (strFindString, strCodeInput + 1);

	DoPYInput (0,0,-1);

	strCodeInput[0] = table->cPinyin;
	strCodeInput[1] = '\0';

	strcat (strCodeInput, strFindString);
	iCodeInputCount = strlen (strCodeInput);
    }
    else
	PYGetCandWords (mode);

    //下面将拼音的候选字表转换为码表输入法的样式
    for (i = 0; i < iCandWordCount; i++) {
	tableCandWord[i].flag = CT_PYPHRASE;
	PYGetCandText (i, tableCandWord[i].candWord.strPYPhrase);
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE TableGetCandWords (SEARCH_MODE mode)
{
    int             i;
    char            strTemp[3], *pstr;
    RECORD         *recTemp;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    if (strCodeInput[0] == '\0')
	return IRV_TO_PROCESS;

    if (bIsInLegend)
	return TableGetLegendCandWords (mode);
    if (!strcmp (strCodeInput, table->strSymbol))
	return TableGetFHCandWords (mode);

    if (strCodeInput[0] == table->cPinyin && table->bUsePY)
	TableGetPinyinCandWords (mode);
    else {
	if (mode == SM_FIRST) {
	    iCandWordCount = 0;
	    tbl.iTableCandDisplayed = 0;
	    tbl.iTableTotalCandCount = 0;
	    iCurrentCandPage = 0;
	    iCandPageCount = 0;
	    TableResetFlags ();
	    if (TableFindFirstMatchCode () == -1 && !tbl.iAutoPhrase) {
		SetMessageCount(&messageDown, 0);
		return IRV_DISPLAY_CANDWORDS;	//Not Found
	    }
	}
	else {
	    if (!iCandWordCount)
		return IRV_TO_PROCESS;

	    if (mode == SM_NEXT) {
		if (tbl.iTableCandDisplayed >= tbl.iTableTotalCandCount)
		    return IRV_DO_NOTHING;
	    }
	    else {
		if (tbl.iTableCandDisplayed == iCandWordCount)
		    return IRV_DO_NOTHING;

		tbl.iTableCandDisplayed -= iCandWordCount;
		TableSetCandWordsFlag (iCandWordCount, False);
	    }

	    TableFindFirstMatchCode ();
	}

	iCandWordCount = 0;
	if (mode == SM_PREV && table->bRule && table->bAutoPhrase && iCodeInputCount == table->iCodeLength) {
	    for (i = tbl.iAutoPhrase - 1; i >= 0; i--) {
		if (!TableCompareCode (strCodeInput, tbl.autoPhrase[i].strCode) && tbl.autoPhrase[i].flag) {
		    if (TableHasPhrase (tbl.autoPhrase[i].strCode, tbl.autoPhrase[i].strHZ))
			TableAddAutoCandWord (i, mode);
		}
	    }
	}

	if (iCandWordCount < fc.iMaxCandWord) {
	    while (tbl.currentRecord && tbl.currentRecord != tbl.recordHead) {
		if ((mode == SM_PREV) ^ (!tbl.currentRecord->flag)) {
		    if (!TableCompareCode (strCodeInput, tbl.currentRecord->strCode)) {
			if (mode == SM_FIRST)
			    tbl.iTableTotalCandCount++;
			TableAddCandWord (tbl.currentRecord, mode);
		    }
		}

		tbl.currentRecord = tbl.currentRecord->next;
	    }
	}

	if (table->bRule && table->bAutoPhrase && mode != SM_PREV && iCodeInputCount == table->iCodeLength) {
	    for (i = tbl.iAutoPhrase - 1; i >= 0; i--) {
		if (!TableCompareCode (strCodeInput, tbl.autoPhrase[i].strCode) && !tbl.autoPhrase[i].flag) {
		    if (TableHasPhrase (tbl.autoPhrase[i].strCode, tbl.autoPhrase[i].strHZ)) {
			if (mode == SM_FIRST)
			    tbl.iTableTotalCandCount++;
			TableAddAutoCandWord (i, mode);
		    }
		}
	    }
	}

	TableSetCandWordsFlag (iCandWordCount, True);

	if (mode != SM_PREV)
	    tbl.iTableCandDisplayed += iCandWordCount;

	//由于iCurrentCandPage和iCandPageCount用来指示是否显示上/下翻页图标，因此，此处需要设置一下
	iCurrentCandPage = (tbl.iTableCandDisplayed == iCandWordCount) ? 0 : 1;
	iCandPageCount = (tbl.iTableCandDisplayed >= tbl.iTableTotalCandCount) ? 1 : 2;
	if (iCandWordCount == tbl.iTableTotalCandCount)
	    iCandPageCount = 0;
	/* **************************************************** */
    }

    if (fc.bPointAfterNumber) {
	strTemp[1] = '.';
	strTemp[2] = '\0';
    }
    else
	strTemp[1] = '\0';

    SetMessageCount(&messageDown, 0);
    for (i = 0; i < iCandWordCount; i++) {
	strTemp[0] = i + 1 + '0';
	if (i == 9)
	    strTemp[0] = '0';
    AddMessageAtLast(&messageDown, MSG_INDEX, strTemp);

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
    AddMessageAtLast(&messageDown, mType, "%s", pMsg);

	if (tableCandWord[i].flag == CT_PYPHRASE) {
	    if (utf8_strlen (tableCandWord[i].candWord.strPYPhrase) == 1) {
		recTemp = tbl.tableSingleHZ[CalHZIndex (tableCandWord[i].candWord.strPYPhrase)];

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
		recTemp = tbl.tableSingleHZ[CalHZIndex (tableCandWord[i].candWord.record->strHZ)];
		if (!recTemp)
		    pstr = (char *) NULL;
		else
		    pstr = recTemp->strCode;
	    }
	    else
		pstr = (char *) NULL;
	}
	else if (HasMatchingKey ())
	    pstr = (tableCandWord[i].flag == CT_NORMAL) ? tableCandWord[i].candWord.record->strCode : tableCandWord[i].candWord.autoPhrase->strCode;
	else
	    pstr = ((tableCandWord[i].flag == CT_NORMAL) ? tableCandWord[i].candWord.record->strCode : tableCandWord[i].candWord.autoPhrase->strCode) + iCodeInputCount;

	if (pstr)
        AddMessageAtLast(&messageDown, MSG_CODE, pstr);
	else
        AddMessageAtLast(&messageDown, MSG_CODE, "");

	if (i != (iCandWordCount - 1)) {
	    MessageConcatLast(&messageDown, " ");
	}
    }

    return IRV_DISPLAY_CANDWORDS;
}

void TableAddAutoCandWord (INT16 which, SEARCH_MODE mode)
{
    int             i, j;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;

    if (mode == SM_PREV) {
	if (iCandWordCount == fc.iMaxCandWord) {
	    i = iCandWordCount - 1;
	    for (j = 0; j < iCandWordCount - 1; j++)
		tableCandWord[j].candWord.autoPhrase = tableCandWord[j + 1].candWord.autoPhrase;
	}
	else
	    i = iCandWordCount++;
	tableCandWord[i].flag = CT_AUTOPHRASE;
	tableCandWord[i].candWord.autoPhrase = &tbl.autoPhrase[which];
    }
    else {
	if (iCandWordCount == fc.iMaxCandWord)
	    return;

	tableCandWord[iCandWordCount].flag = CT_AUTOPHRASE;
	tableCandWord[iCandWordCount++].candWord.autoPhrase = &tbl.autoPhrase[which];
    }
}

void TableAddCandWord (RECORD * record, SEARCH_MODE mode)
{
    int             i = 0, j;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;

    switch (table->tableOrder) {
    case AD_NO:
	if (mode == SM_PREV) {
	    if (iCandWordCount == fc.iMaxCandWord)
		i = iCandWordCount - 1;
	    else {
		for (i = 0; i < iCandWordCount; i++) {
		    if (tableCandWord[i].flag != CT_NORMAL)
			break;
		}
	    }
	}
	else {
	    if (iCandWordCount == fc.iMaxCandWord)
		return;
	    tableCandWord[iCandWordCount].flag = CT_NORMAL;
	    tableCandWord[iCandWordCount++].candWord.record = record;
	    return;
	}
	break;
    case AD_FREQ:
	if (mode == SM_PREV) {
	    for (i = iCandWordCount - 1; i >= 0; i--) {
		if (tableCandWord[i].flag == CT_NORMAL) {
		    if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) < 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iHit >= record->iHit))
			break;
		}
	    }

	    if (iCandWordCount == fc.iMaxCandWord) {
		if (i < 0)
		    return;
	    }
	    else
		i++;
	}
	else {
	    for (; i < iCandWordCount; i++) {
		if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) > 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iHit < record->iHit))
		    break;
	    }
	    if (i == fc.iMaxCandWord)
		return;
	}
	break;

    case AD_FAST:
	if (mode == SM_PREV) {
	    for (i = iCandWordCount - 1; i >= 0; i--) {
		if (tableCandWord[i].flag == CT_NORMAL) {
		    if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) < 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iIndex >= record->iIndex))
			break;
		}
	    }

	    if (iCandWordCount == fc.iMaxCandWord) {
		if (i < 0)
		    return;
	    }
	    else
		i++;
	}
	else {
	    for (i = 0; i < iCandWordCount; i++) {
		if (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) > 0 || (strcmp (tableCandWord[i].candWord.record->strCode, record->strCode) == 0 && tableCandWord[i].candWord.record->iIndex < record->iIndex))
		    break;
	    }

	    if (i == fc.iMaxCandWord)
		return;
	}
	break;
    }

    if (mode == SM_PREV) {
	if (iCandWordCount == fc.iMaxCandWord) {
	    for (j = 0; j < i; j++) {
		tableCandWord[j].flag = tableCandWord[j + 1].flag;
		if (tableCandWord[j].flag == CT_NORMAL)
		    tableCandWord[j].candWord.record = tableCandWord[j + 1].candWord.record;
		else
		    tableCandWord[j].candWord.autoPhrase = tableCandWord[j + 1].candWord.autoPhrase;
	    }
	}
	else {
	    for (j = iCandWordCount; j > i; j--) {
		tableCandWord[j].flag = tableCandWord[j - 1].flag;
		if (tableCandWord[j].flag == CT_NORMAL)
		    tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
		else
		    tableCandWord[j].candWord.autoPhrase = tableCandWord[j - 1].candWord.autoPhrase;
	    }
	}
    }
    else {
	j = iCandWordCount;
	if (iCandWordCount == fc.iMaxCandWord)
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

    if (iCandWordCount != fc.iMaxCandWord)
	iCandWordCount++;
}

void TableResetFlags (void)
{
    RECORD         *record = tbl.recordHead->next;
    INT16           i;

    while (record != tbl.recordHead) {
	record->flag = False;
	record = record->next;
    }
    for (i = 0; i < tbl.iAutoPhrase; i++)
	tbl.autoPhrase[i].flag = False;
}

void TableSetCandWordsFlag (int iCount, Bool flag)
{
    int             i;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;

    for (i = 0; i < iCount; i++) {
	if (tableCandWord[i].flag == CT_NORMAL)
	    tableCandWord[i].candWord.record->flag = flag;
	else
	    tableCandWord[i].candWord.autoPhrase->flag = flag;
    }
}
Bool HasMatchingKey (void)
{
    char           *str;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    str = strCodeInput;
    while (*str) {
	if (*str++ == table->cMatchingKey)
	    return True;
    }
    return False;
}

int TableCompareCode (char *strUser, char *strDict)
{
    int             i;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

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
	    return -999;	//随意的一个值
    }

    return 0;
}

int TableFindFirstMatchCode (void)
{
    int             i = 0;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    if (!tbl.recordHead)
	return -1;

    if (table->bUseMatchingKey && (strCodeInput[0] == table->cMatchingKey))
	i = 0;
    else {
	while (strCodeInput[0] != tbl.recordIndex[i].cCode) {
	    if (!tbl.recordIndex[i].cCode)
		break;
	    i++;
	}
    }
    tbl.currentRecord = tbl.recordIndex[i].record;
    if (!tbl.currentRecord)
	return -1;

    while (tbl.currentRecord != tbl.recordHead) {
	if (!TableCompareCode (strCodeInput, tbl.currentRecord->strCode)) {
		return i;
	}
	tbl.currentRecord = tbl.currentRecord->next;
	i++;
    }

    return -1;			//Not found
}

/*
 * 反查编码
 * bMode=True表示用于组词，此时不查一、二级简码。但如果只有二级简码时返回二级简码，不查一级简码
 */
/*RECORD         *TableFindCode (char *strHZ, Bool bMode)
{
    RECORD         *recShort = NULL;	//记录二级简码的位置
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
void TableAdjustOrderByIndex (int iIndex)
{
    RECORD         *recTemp;
    int             iTemp;
    TABLECANDWORD*  tableCandWord = tbl.tableCandWord;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    if (tableCandWord[iIndex - 1].flag != CT_NORMAL)
	return;

    recTemp = tableCandWord[iIndex - 1].candWord.record;
    while (!strcmp (recTemp->strCode, recTemp->prev->strCode))
	recTemp = recTemp->prev;
    if (recTemp == tableCandWord[iIndex - 1].candWord.record)	//说明已经是第一个
	return;

    //将指定的字/词放到recTemp前
    tableCandWord[iIndex - 1].candWord.record->prev->next = tableCandWord[iIndex - 1].candWord.record->next;
    tableCandWord[iIndex - 1].candWord.record->next->prev = tableCandWord[iIndex - 1].candWord.record->prev;
    recTemp->prev->next = tableCandWord[iIndex - 1].candWord.record;
    tableCandWord[iIndex - 1].candWord.record->prev = recTemp->prev;
    recTemp->prev = tableCandWord[iIndex - 1].candWord.record;
    tableCandWord[iIndex - 1].candWord.record->next = recTemp;

    tbl.iTableChanged++;

    //需要的话，更新索引
    if (tableCandWord[iIndex - 1].candWord.record->strCode[1] == '\0') {
	for (iTemp = 0; iTemp < strlen (table->strInputCode); iTemp++) {
	    if (tbl.recordIndex[iTemp].cCode == tableCandWord[iIndex - 1].candWord.record->strCode[0]) {
		tbl.recordIndex[iTemp].record = tableCandWord[iIndex - 1].candWord.record;
		break;
	    }
	}
    }
}

/*
 * 根据序号删除词组，序号从1开始
 */
void TableDelPhraseByIndex (int iIndex)
{
    TABLECANDWORD *tableCandWord = tbl.tableCandWord;
    if (tableCandWord[iIndex - 1].flag != CT_NORMAL)
	return;

    if (utf8_strlen (tableCandWord[iIndex - 1].candWord.record->strHZ) <= 1)
	return;

    TableDelPhrase (tableCandWord[iIndex - 1].candWord.record);
}

/*
 * 根据字串删除词组
 */
void TableDelPhraseByHZ (char *strHZ)
{
    RECORD         *recTemp;

    recTemp = TableFindPhrase (strHZ);
    if (recTemp)
	TableDelPhrase (recTemp);
}

void TableDelPhrase (RECORD * record)
{
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);
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
RECORD         *TableHasPhrase (char *strCode, char *strHZ)
{
    RECORD         *recTemp;
    int             i;

    i = 0;
    while (strCode[0] != tbl.recordIndex[i].cCode)
	i++;

    recTemp = tbl.recordIndex[i].record;
    while (recTemp != tbl.recordHead) {
	if (!recTemp->bPinyin) {
	    if (strcmp (recTemp->strCode, strCode) > 0)
	        break;
	    else if (!strcmp (recTemp->strCode, strCode)) {
	        if (!strcmp (recTemp->strHZ, strHZ))	//该词组已经在词库中
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
RECORD         *TableFindPhrase (char *strHZ)
{
    RECORD         *recTemp;
    char            strTemp[UTF8_MAX_LENGTH + 1];
    int             i;

    //首先，先查找第一个汉字的编码
    strncpy(strTemp, strHZ, utf8_char_len(strHZ));
    strTemp[utf8_char_len(strHZ)] = '\0';

    recTemp = tbl.tableSingleHZ[CalHZIndex (strTemp)];
    if (!recTemp)
	return (RECORD *) NULL;

    //然后根据该编码找到检索的起始点
    i = 0;
    while (recTemp->strCode[0] != tbl.recordIndex[i].cCode)
	i++;

    recTemp = tbl.recordIndex[i].record;
    while (recTemp != tbl.recordHead) {	    
	if (recTemp->strCode[0] != tbl.recordIndex[i].cCode)
	    break;
	if (!strcmp (recTemp->strHZ, strHZ)) {
	    if (!recTemp->bPinyin)
		return recTemp;
        }

	recTemp = recTemp->next;
    }

    return (RECORD *) NULL;
}

void TableInsertPhrase (char *strCode, char *strHZ)
{
    RECORD         *insertPoint, *dictNew;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    insertPoint = TableHasPhrase (strCode, strHZ);

    if (!insertPoint)
	return;

    dictNew = (RECORD *) malloc (sizeof (RECORD));
    dictNew->strCode = (char *) malloc (sizeof (char) * (table->iCodeLength + 1));
    strcpy (dictNew->strCode, strCode);
    dictNew->strHZ = (char *) malloc (sizeof (char) * (strlen (strHZ) + 1));
    strcpy (dictNew->strHZ, strHZ);
    dictNew->iHit = 0;
    dictNew->iIndex = tbl.iTableIndex;

    dictNew->prev = insertPoint->prev;
    insertPoint->prev->next = dictNew;
    insertPoint->prev = dictNew;
    dictNew->next = insertPoint;

    table->iRecordCount++;
}

void TableCreateNewPhrase (void)
{
    int             i;

    SetMessageText(&messageDown, 0, "");
    for (i = tbl.iTableNewPhraseHZCount; i > 0; i--)
        MessageConcat(&messageDown , 0 ,tbl.hzLastInput[tbl.iHZLastInputCount - i].strHZ);

    TableCreatePhraseCode (messageDown.msg[0].strMsg);

    if (!tbl.bCanntFindCode)
        SetMessageText(&messageDown, 1, tbl.strNewPhraseCode);
    else
        SetMessageText(&messageDown, 0, "????");
}

void TableCreatePhraseCode (char *strHZ)
{
    unsigned char   i;
    unsigned char   i1, i2;
    size_t          iLen;
    char            strTemp[UTF8_MAX_LENGTH + 1] = {'\0', };
    RECORD         *recTemp;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    tbl.bCanntFindCode = False;
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

	recTemp = tbl.tableSingleHZ[CalHZIndex (strTemp)];

	if (!recTemp) {
	    tbl.bCanntFindCode = True;
	    break;
	}

	tbl.strNewPhraseCode[i1] = recTemp->strCode[table->rule[i].rule[i1].iIndex - 1];
    }
}

/*
 * 获取联想候选字列表
 */
INPUT_RETURN_VALUE TableGetLegendCandWords (SEARCH_MODE mode)
{
    char            strTemp[3];
    int             iLength;
    int             i;
    RECORD         *tableLegend = NULL;
    unsigned int    iTableTotalLengendCandCount = 0;
    TABLECANDWORD* tableCandWord = tbl.tableCandWord;

    if (!tbl.strTableLegendSource[0])
	return IRV_TO_PROCESS;

    iLength = strlen (tbl.strTableLegendSource);
    if (mode == SM_FIRST) {
	iCurrentLegendCandPage = 0;
	iLegendCandPageCount = 0;
	TableResetFlags();
    }
    else {
	if (!iLegendCandPageCount)
	    return IRV_TO_PROCESS;

	if (mode == SM_NEXT) {
	    if (iCurrentLegendCandPage == iLegendCandPageCount)
		return IRV_DO_NOTHING;

	    iCurrentLegendCandPage++;
	}
	else {
	    if (!iCurrentLegendCandPage)
		return IRV_DO_NOTHING;

	    TableSetCandWordsFlag (iLegendCandWordCount, False);
	    iCurrentLegendCandPage--;
	}
    }

    iLegendCandWordCount = 0;
    tableLegend = tbl.recordHead->next;
	
    while (tableLegend != tbl.recordHead) {
	if (((mode == SM_PREV) ^ (!tableLegend->flag)) && ((iLength + 2) == strlen (tableLegend->strHZ))) {
	    if (!strncmp (tableLegend->strHZ, tbl.strTableLegendSource, iLength) && tableLegend->strHZ[iLength]) {
		if (mode == SM_FIRST)
		    iTableTotalLengendCandCount++;
	
		TableAddLegendCandWord (tableLegend, mode);
	    }
	}

	tableLegend = tableLegend->next;
    }

    TableSetCandWordsFlag (iLegendCandWordCount, True);

    if (mode == SM_FIRST && fc.bDisablePagingInLegend)
	iLegendCandPageCount = iTableTotalLengendCandCount / fc.iMaxCandWord - ((iTableTotalLengendCandCount % fc.iMaxCandWord) ? 0 : 1);

    SetMessageCount(&messageUp, 0);
    AddMessageAtLast(&messageUp, MSG_TIPS, "联想：");
    AddMessageAtLast(&messageUp, MSG_INPUT, tbl.strTableLegendSource);

    if (fc.bPointAfterNumber) {
	strTemp[1] = '.';
	strTemp[2] = '\0';
    }
    else
	strTemp[1] = '\0';

    SetMessageCount(&messageDown, 0);
    for (i = 0; i < iLegendCandWordCount; i++) {
	if (i == 9)
	    strTemp[0] = '0';
	else
	    strTemp[0] = i + 1 + '0';
    AddMessageAtLast(&messageDown, MSG_INDEX, strTemp);

    AddMessageAtLast(&messageDown, ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER), tableCandWord[i].candWord.record->strHZ + strlen (tbl.strTableLegendSource));
	if (i != (iLegendCandWordCount - 1)) {
        MessageConcatLast(&messageDown, " ");
	}
    }

    bIsInLegend = (iLegendCandWordCount != 0);

    return IRV_DISPLAY_CANDWORDS;
}

void TableAddLegendCandWord (RECORD * record, SEARCH_MODE mode)
{
    int             i, j;
    TABLECANDWORD *tableCandWord = tbl.tableCandWord;
    
    if (mode == SM_PREV) {
	for (i = iLegendCandWordCount - 1; i >= 0; i--) {
	    if (tableCandWord[i].candWord.record->iHit >= record->iHit)
		break;
	}

	if (iLegendCandWordCount == fc.iMaxCandWord) {
	    if (i < 0)
		return;
	}
	else
	    i++;
    }
    else {
	for (i = 0; i < iLegendCandWordCount; i++) {
	    if (tableCandWord[i].candWord.record->iHit < record->iHit)
		break;
	}
	
	if (i == fc.iMaxCandWord)
	    return;
    }

    if (mode == SM_PREV) {
	if (iLegendCandWordCount == fc.iMaxCandWord) {
	    for (j = 0; j < i; j++)
		tableCandWord[j].candWord.record = tableCandWord[j + 1].candWord.record;
	}
	else {
	    for (j = iLegendCandWordCount; j > i; j--)
		tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
	}
    }
    else {
	j = iLegendCandWordCount;
	if (iLegendCandWordCount == fc.iMaxCandWord)
	    j--;
	for (; j > i; j--)
	    tableCandWord[j].candWord.record = tableCandWord[j - 1].candWord.record;
    }
    
    tableCandWord[i].flag = CT_NORMAL;
    tableCandWord[i].candWord.record = record;

    if (iLegendCandWordCount != fc.iMaxCandWord)
	iLegendCandWordCount++;
}

char           *TableGetLegendCandWord (int iIndex)
{
    TABLECANDWORD *tableCandWord = tbl.tableCandWord;
    if (iLegendCandWordCount) {
	if (iIndex > (iLegendCandWordCount - 1))
	    iIndex = iLegendCandWordCount - 1;

	tableCandWord[iIndex].candWord.record->iHit++;
	strcpy (tbl.strTableLegendSource, tableCandWord[iIndex].candWord.record->strHZ + strlen (tbl.strTableLegendSource));
	TableGetLegendCandWords (SM_FIRST);

	return tbl.strTableLegendSource;
    }
    return NULL;
}

INPUT_RETURN_VALUE TableGetFHCandWords (SEARCH_MODE mode)
{
    char            strTemp[3];
    int             i;

    if (!tbl.iFH)
	return IRV_DISPLAY_MESSAGE;

    if (fc.bPointAfterNumber) {
	strTemp[1] = '.';
	strTemp[2] = '\0';
    }
    else
	strTemp[1] = '\0';

    SetMessageCount(&messageDown, 0);

    if (mode == SM_FIRST) {
	iCandPageCount = tbl.iFH / fc.iMaxCandWord - ((tbl.iFH % fc.iMaxCandWord) ? 0 : 1);
	iCurrentCandPage = 0;
    }
    else {
	if (!iCandPageCount)
	    return IRV_TO_PROCESS;

	if (mode == SM_NEXT) {
	    if (iCurrentCandPage == iCandPageCount)
		return IRV_DO_NOTHING;

	    iCurrentCandPage++;
	}
	else {
	    if (!iCurrentCandPage)
		return IRV_DO_NOTHING;

	    iCurrentCandPage--;
	}
    }

    for (i = 0; i < fc.iMaxCandWord; i++) {
	strTemp[0] = i + 1 + '0';
	if (i == 9)
	    strTemp[0] = '0';
    AddMessageAtLast(&messageDown, MSG_INDEX, strTemp);
    AddMessageAtLast(&messageDown, ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER), tbl.fh[iCurrentCandPage * fc.iMaxCandWord + i].strFH);
	if (i != (fc.iMaxCandWord - 1)) {
        MessageConcatLast(&messageDown, " ");
	}
	if ((iCurrentCandPage * fc.iMaxCandWord + i) >= (tbl.iFH - 1)) {
	    i++;
	    break;
	}
    }

    iCandWordCount = i;
    return IRV_DISPLAY_CANDWORDS;
}

char           *TableGetFHCandWord (int iIndex)
{
    SetMessageCount(&messageDown, 0);

    if (iCandWordCount) {
	if (iIndex > (iCandWordCount - 1))
	    iIndex = iCandWordCount - 1;

	return tbl.fh[iCurrentCandPage * fc.iMaxCandWord + iIndex].strFH;
    }
    return NULL;
}

Bool TablePhraseTips (void)
{
    RECORD         *recTemp = NULL;
    char            strTemp[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1] = "\0", *ps;
    INT16           i, j;

    if (!tbl.recordHead)
	return False;

    //如果最近输入了一个词组，这个工作就不需要了
    if (lastIsSingleHZ != 1)
	return False;

    j = (tbl.iHZLastInputCount > PHRASE_MAX_LENGTH) ? tbl.iHZLastInputCount - PHRASE_MAX_LENGTH : 0;
    for (i = j; i < tbl.iHZLastInputCount; i++)
	strcat (strTemp, tbl.hzLastInput[i].strHZ);
    //如果只有一个汉字，这个工作也不需要了
    if (utf8_strlen (strTemp) < 2)
	return False;

    //首先要判断是不是已经在词库中
	ps = strTemp;
    for (i = 0; i < (tbl.iHZLastInputCount - j - 1); i++) {
	recTemp = TableFindPhrase (ps);
	if (recTemp) {
        SetMessageCount(&messageUp, 0);
        AddMessageAtLast(&messageUp, MSG_TIPS, "词库中有词组 ");
        AddMessageAtLast(&messageUp, MSG_INPUT, ps);

        SetMessageCount(&messageDown, 0);
        AddMessageAtLast(&messageDown, MSG_FIRSTCAND, "编码为 ");
        AddMessageAtLast(&messageDown, MSG_CODE, recTemp->strCode);
        AddMessageAtLast(&messageDown, MSG_TIPS, " ^DEL删除");
	    tbl.bTablePhraseTips = True;
	    inputWindow.bShowCursor = False;

	    return True;
	}
	ps = ps + utf8_char_len(ps);
    }

    return False;
}

void TableCreateAutoPhrase (INT8 iCount)
{
    char            *strHZ;
    INT16           i, j, k;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

    if (!tbl.autoPhrase)
	return;

    strHZ=(char *)malloc((table->iAutoPhrase * UTF8_MAX_LENGTH + 1)*sizeof(char));
    /*
     * 为了提高效率，此处只重新生成新录入字构成的词组
     */
    j = tbl.iHZLastInputCount - table->iAutoPhrase - iCount;
    if (j < 0)
	j = 0;

    for (; j < tbl.iHZLastInputCount - 1; j++) {
	for (i = table->iAutoPhrase; i >= 2; i--) {
	    if ((j + i - 1) > tbl.iHZLastInputCount)
		continue;

	    strcpy (strHZ, tbl.hzLastInput[j].strHZ);
	    for (k = 1; k < i; k++)
		strcat (strHZ, tbl.hzLastInput[j + k].strHZ);

	    //再去掉重复的词组
	    for (k = 0; k < tbl.iAutoPhrase; k++) {
		if (!strcmp (tbl.autoPhrase[k].strHZ, strHZ))
		    goto _next;
	    }
	    //然后去掉系统中已经有的词组
	    if (TableFindPhrase (strHZ))
	    	goto _next;

	    TableCreatePhraseCode (strHZ);
	    if (tbl.iAutoPhrase != AUTO_PHRASE_COUNT) {
		tbl.autoPhrase[tbl.iAutoPhrase].flag = False;
		strcpy (tbl.autoPhrase[tbl.iAutoPhrase].strCode, tbl.strNewPhraseCode);
		strcpy (tbl.autoPhrase[tbl.iAutoPhrase].strHZ, strHZ);
		tbl.autoPhrase[tbl.iAutoPhrase].iSelected = 0;
		tbl.iAutoPhrase++;
	    }
	    else {
		tbl.insertPoint->flag = False;
		strcpy (tbl.insertPoint->strCode, tbl.strNewPhraseCode);
		strcpy (tbl.insertPoint->strHZ, strHZ);
		tbl.insertPoint->iSelected = 0;
		tbl.insertPoint = tbl.insertPoint->next;
	    }

	  _next:
	    continue;
	}

    }

    free(strHZ);
}

void UpdateHZLastInput (char *str)
{
    int             i, j;
	char           *pstr;
    TABLE* table = (TABLE*) utarray_eltptr(tbl.table, tbl.iTableIMIndex);

	pstr = str;

    for (i = 0; i < utf8_strlen (str) ; i++) {
	if (tbl.iHZLastInputCount < PHRASE_MAX_LENGTH)
	    tbl.iHZLastInputCount++;
	else {
	    for (j = 0; j < (tbl.iHZLastInputCount - 1); j++) {
		strncpy(tbl.hzLastInput[j].strHZ, tbl.hzLastInput[j + 1].strHZ, utf8_char_len(tbl.hzLastInput[j + 1].strHZ));
	    }
	}
	strncpy(tbl.hzLastInput[tbl.iHZLastInputCount - 1].strHZ, pstr, utf8_char_len(pstr));
	tbl.hzLastInput[tbl.iHZLastInputCount - 1].strHZ[utf8_char_len(pstr)] = '\0';
	pstr = pstr + utf8_char_len(pstr);
    }

    if (table->bRule && table->bAutoPhrase)
	TableCreateAutoPhrase ((INT8) (utf8_strlen (str)));
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
