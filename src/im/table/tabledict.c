#include "config.h"

#include <stdio.h>
#include <limits.h>
#include <libintl.h>
#include <unistd.h>
#include "fcitx/fcitx.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/utf8.h"
#include "tabledict.h"

#define TABLE_TEMP_FILE "table_XXXXXX"
const int iInternalVersion = INTERNAL_VERSION;

void UpdateTableMetaData(TableMetaData* tableMetaData)
{
    if (!tableMetaData->tableDict)
        return;
    TableDict* tableDict = tableMetaData->tableDict;
    if (tableMetaData->iTableAutoSendToClient == -1)
        tableMetaData->iTableAutoSendToClient = tableDict->iCodeLength;
    if (tableMetaData->iTableAutoSendToClientWhenNone == -1)
        tableMetaData->iTableAutoSendToClientWhenNone = tableDict->iCodeLength;
}

boolean LoadTableDict(TableMetaData* tableMetaData)
{
    char            strCode[MAX_CODE_LENGTH + 1];
    char           *strHZ = 0;
    FILE           *fpDict;
    RECORD         *recTemp;
    unsigned int    i = 0;
    uint32_t        iTemp, iTempCount;
    char            cChar = 0, cTemp;
    int8_t          iVersion = 1;
    int             iRecordIndex;
    TableDict      *tableDict;

    //读入码表
    FcitxLog(DEBUG, _("Loading Table Dict"));

    int reload = 0;
    do {
        boolean error = false;
        if (!reload) {
            /**
             * kcm saves a absolute path here but it is then interpreted as
             * a relative path?
             **/
            fpDict = FcitxXDGGetFileWithPrefix("table", tableMetaData->strPath,
                                               "r", NULL);
        } else {
            char *tablepath;
            char *path = fcitx_utils_get_fcitx_path("pkgdatadir");
            fcitx_utils_alloc_cat_str(tablepath, path, "/table/",
                                      tableMetaData->strPath);
            fpDict = fopen(tablepath, "r");
            free(tablepath);
        }
        if (!fpDict)
            return false;

        tableMetaData->tableDict = fcitx_utils_new(TableDict);

        tableDict = tableMetaData->tableDict;
        tableDict->pool = fcitx_memory_pool_create();
#define CHECK_LOAD_TABLE_ERROR(SIZE) if (size < (SIZE)) { error = true; goto table_load_error; }

        //先读取码表的信息
        //判断版本信息
        size_t size;
        size = fcitx_utils_read_uint32(fpDict, &iTemp);
        CHECK_LOAD_TABLE_ERROR(1);

        if (!iTemp) {
            size = fread(&iVersion, sizeof(int8_t), 1, fpDict);
            CHECK_LOAD_TABLE_ERROR(1);
            iVersion = (iVersion < INTERNAL_VERSION);
            size = fcitx_utils_read_uint32(fpDict, &iTemp);
            CHECK_LOAD_TABLE_ERROR(1);
        }

        tableDict->strInputCode = (char*)realloc(tableDict->strInputCode, sizeof(char) * (iTemp + 1));
        size = fread(tableDict->strInputCode, sizeof(char), iTemp + 1, fpDict);
        CHECK_LOAD_TABLE_ERROR(iTemp + 1);
        /*
         * 建立索引，加26是为了为拼音编码预留空间
         */
        size_t tmp_len = strlen(tableDict->strInputCode) + 26;
        tableDict->recordIndex = (RECORD_INDEX*)fcitx_memory_pool_alloc(tableDict->pool, tmp_len * sizeof(RECORD_INDEX));
        for (iTemp = 0; iTemp < tmp_len; iTemp++) {
            tableDict->recordIndex[iTemp].cCode = 0;
            tableDict->recordIndex[iTemp].record = NULL;
        }
        /********************************************************************/

        size = fread(&(tableDict->iCodeLength), sizeof(uint8_t), 1, fpDict);
        CHECK_LOAD_TABLE_ERROR(1);
        UpdateTableMetaData(tableMetaData);

        if (!iVersion) {
            size = fread(&(tableDict->iPYCodeLength), sizeof(uint8_t), 1, fpDict);
            CHECK_LOAD_TABLE_ERROR(1);
        }
        else
            tableDict->iPYCodeLength = tableDict->iCodeLength;

        size = fcitx_utils_read_uint32(fpDict, &iTemp);
        CHECK_LOAD_TABLE_ERROR(1);
        tableDict->strIgnoreChars = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (iTemp + 1));
        size = fread(tableDict->strIgnoreChars, sizeof(char), iTemp + 1, fpDict);
        CHECK_LOAD_TABLE_ERROR(iTemp + 1);

        size = fread(&(tableDict->bRule), sizeof(unsigned char), 1, fpDict);
        CHECK_LOAD_TABLE_ERROR(1);

        if (tableDict->bRule) { //表示有组词规则
            tableDict->rule = (RULE*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(RULE) * (tableDict->iCodeLength - 1));
            for (i = 0; i < tableDict->iCodeLength - 1; i++) {
                size = fread(&(tableDict->rule[i].iFlag), sizeof(unsigned char), 1, fpDict);
                CHECK_LOAD_TABLE_ERROR(1);
                size = fread(&(tableDict->rule[i].iWords), sizeof(unsigned char), 1, fpDict);
                CHECK_LOAD_TABLE_ERROR(1);
                tableDict->rule[i].rule = (RULE_RULE*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(RULE_RULE) * tableDict->iCodeLength);
                for (iTemp = 0; iTemp < tableDict->iCodeLength; iTemp++) {
                    size = fread(&(tableDict->rule[i].rule[iTemp].iFlag), sizeof(unsigned char), 1, fpDict);
                    CHECK_LOAD_TABLE_ERROR(1);
                    size = fread(&(tableDict->rule[i].rule[iTemp].iWhich), sizeof(unsigned char), 1, fpDict);
                    CHECK_LOAD_TABLE_ERROR(1);
                    size = fread(&(tableDict->rule[i].rule[iTemp].iIndex), sizeof(unsigned char), 1, fpDict);
                    CHECK_LOAD_TABLE_ERROR(1);
                }
            }
        }

        tableDict->recordHead = (RECORD*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(RECORD));
        tableDict->currentRecord = tableDict->recordHead;

        size = fcitx_utils_read_uint32(fpDict, &tableDict->iRecordCount);
        CHECK_LOAD_TABLE_ERROR(1);

        for (i = 0; i < SINGLE_HZ_COUNT; i++) {
            tableDict->tableSingleHZ[i] = (RECORD*)NULL;
            tableDict->tableSingleHZCons[i] = (RECORD*)NULL;
        }

        iRecordIndex = 0;
        size_t bufSize = 0;
        for (i = 0; i < tableDict->iRecordCount; i++) {
            size = fread(strCode, sizeof(int8_t), tableDict->iPYCodeLength + 1, fpDict);
            CHECK_LOAD_TABLE_ERROR(tableDict->iPYCodeLength + 1);
            size = fcitx_utils_read_uint32(fpDict, &iTemp);
            CHECK_LOAD_TABLE_ERROR(1);
            /* we don't actually have such limit, but sometimes, broken table
             * may break this, so we need to give a limitation.
             */
            if (iTemp > UTF8_MAX_LENGTH * 30) {
                error = true;
                goto table_load_error;
            }
            if (iTemp > bufSize) {
                bufSize = iTemp;
                strHZ = realloc(strHZ, bufSize);
            }
            size = fread(strHZ, sizeof(int8_t), iTemp, fpDict);
            CHECK_LOAD_TABLE_ERROR(iTemp);
            recTemp = (RECORD*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(RECORD));
            recTemp->strCode = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (tableDict->iPYCodeLength + 1));
            memset(recTemp->strCode, 0, sizeof(char) * (tableDict->iPYCodeLength + 1));
            strcpy(recTemp->strCode, strCode);
            recTemp->strHZ = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * iTemp);
            strcpy(recTemp->strHZ, strHZ);

            if (!iVersion) {
                size = fread(&cTemp, sizeof(int8_t), 1, fpDict);
                CHECK_LOAD_TABLE_ERROR(1);
                recTemp->type = cTemp;
            }

            size = fcitx_utils_read_uint32(fpDict, &recTemp->iHit);
            CHECK_LOAD_TABLE_ERROR(1);
            size = fcitx_utils_read_uint32(fpDict, &recTemp->iIndex);
            CHECK_LOAD_TABLE_ERROR(1);
            if (recTemp->iIndex > tableDict->iTableIndex)
                tableDict->iTableIndex = recTemp->iIndex;

            /* 建立索引 */
            if (cChar != recTemp->strCode[0]) {
                cChar = recTemp->strCode[0];
                tableDict->recordIndex[iRecordIndex].cCode = cChar;
                tableDict->recordIndex[iRecordIndex].record = recTemp;
                iRecordIndex++;
            }
            /******************************************************************/
            /** 为单字生成一个表   */
            if (fcitx_utf8_strlen(recTemp->strHZ) == 1 && !IsIgnoreChar(tableDict, strCode[0]))
            {
                RECORD** tableSingleHZ = NULL;
                if (recTemp->type == RECORDTYPE_NORMAL)
                    tableSingleHZ = tableDict->tableSingleHZ;
                else if (recTemp->type == RECORDTYPE_CONSTRUCT)
                    tableSingleHZ = tableDict->tableSingleHZCons;

                if (tableSingleHZ) {
                    iTemp = CalHZIndex(recTemp->strHZ);
                    if (iTemp < SINGLE_HZ_COUNT) {
                        if (tableSingleHZ[iTemp]) {
                            if (strlen(strCode) > strlen(tableDict->tableSingleHZ[iTemp]->strCode))
                                tableSingleHZ[iTemp] = recTemp;
                        } else
                            tableSingleHZ[iTemp] = recTemp;
                    }
                }
            }

            if (recTemp->type == RECORDTYPE_PINYIN)
                tableDict->bHasPinyin = true;

            if (recTemp->type == RECORDTYPE_PROMPT && strlen(recTemp->strCode) == 1)
                tableDict->promptCode[(uint8_t) recTemp->strCode[0]] = recTemp;

            tableDict->currentRecord->next = recTemp;
            recTemp->prev = tableDict->currentRecord;
            tableDict->currentRecord = recTemp;
        }
        if (strHZ) {
            free(strHZ);
            strHZ = NULL;
        }

        tableDict->currentRecord->next = tableDict->recordHead;
        tableDict->recordHead->prev = tableDict->currentRecord;

table_load_error:
        fclose(fpDict);
        if (error) {
            fcitx_memory_pool_destroy(tableDict->pool);
            tableDict->pool = NULL;
            reload++;
        } else {
            break;
        }
    } while(reload < 2);

    if (!tableDict->pool)
        return false;

    FcitxLog(DEBUG, _("Load Table Dict OK"));

    //读取相应的特殊符号表
    fpDict = FcitxXDGGetFileWithPrefix("table", tableMetaData->strSymbolFile, "r", NULL);

    if (fpDict) {
        tableDict->iFH = fcitx_utils_calculate_record_number(fpDict);
        tableDict->fh = (FH*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(FH) * tableDict->iFH);

        char* strBuf = NULL;
        size_t bufLen = 0;
        for (i = 0; i < tableDict->iFH; i++) {
            if (getline(&strBuf, &bufLen, fpDict) == -1)
                break;

            if (!fcitx_utf8_check_string(strBuf))
                break;

            if (fcitx_utf8_strlen(strBuf) > FH_MAX_LENGTH)
                break;

            strcpy(tableDict->fh[i].strFH, strBuf);
        }
        fcitx_utils_free(strBuf);
        tableDict->iFH = i;

        fclose(fpDict);
    }

    tableDict->strNewPhraseCode = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (tableDict->iCodeLength + 1));
    tableDict->strNewPhraseCode[tableDict->iCodeLength] = '\0';

    tableDict->iAutoPhrase = 0;
    if (tableMetaData->bAutoPhrase) {
        tableDict->autoPhrase = (AUTOPHRASE*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(AUTOPHRASE) * AUTO_PHRASE_COUNT);

        //读取上次保存的自动词组信息
        FcitxLog(DEBUG, _("Loading Autophrase."));

        char *temppath;
        fcitx_utils_alloc_cat_str(temppath, tableMetaData->uniqueName,
                                  "_LastAutoPhrase.tmp");
        fpDict = FcitxXDGGetFileWithPrefix("table", temppath, "r", NULL);
        free(temppath);
        i = 0;
        if (fpDict) {
            size_t size = fcitx_utils_read_int32(fpDict, &tableDict->iAutoPhrase);
            if (size == 1) {
                for (; i < tableDict->iAutoPhrase; i++) {
                    tableDict->autoPhrase[i].strCode = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (tableDict->iCodeLength + 1));
                    tableDict->autoPhrase[i].strHZ = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1));
                    size = fread(tableDict->autoPhrase[i].strCode, tableDict->iCodeLength + 1, 1, fpDict);
                    if (size != 1) {
                        tableDict->iAutoPhrase = i;
                        break;
                    }
                    size = fread(tableDict->autoPhrase[i].strHZ, PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1, 1, fpDict);
                    tableDict->autoPhrase[i].strHZ[PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH] = 0;
                    if (size != 1 || !fcitx_utf8_check_string(tableDict->autoPhrase[i].strHZ)) {
                        tableDict->iAutoPhrase = i;
                        break;
                    }
                    size = fcitx_utils_read_uint32(fpDict, &iTempCount);
                    if (size != 1) {
                        tableDict->iAutoPhrase = i;
                        break;
                    }

                    tableDict->autoPhrase[i].iSelected = iTempCount;
                    if (i == AUTO_PHRASE_COUNT - 1)
                        tableDict->autoPhrase[i].next = &tableDict->autoPhrase[0];
                    else
                        tableDict->autoPhrase[i].next = &tableDict->autoPhrase[i + 1];
                }
            }
            fclose(fpDict);
        }

        for (; i < AUTO_PHRASE_COUNT; i++) {
            tableDict->autoPhrase[i].strCode = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (tableDict->iCodeLength + 1));
            tableDict->autoPhrase[i].strHZ = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1));
            tableDict->autoPhrase[i].iSelected = 0;
            if (i == AUTO_PHRASE_COUNT - 1)
                tableDict->autoPhrase[i].next = &tableDict->autoPhrase[0];
            else
                tableDict->autoPhrase[i].next = &tableDict->autoPhrase[i + 1];
        }

        if (i == AUTO_PHRASE_COUNT)
            tableDict->insertPoint = &tableDict->autoPhrase[0];
        else
            tableDict->insertPoint = &tableDict->autoPhrase[i - 1];

        FcitxLog(DEBUG, _("Load Autophrase OK"));
    } else
        tableDict->autoPhrase = (AUTOPHRASE *) NULL;

    return true;
}

void SaveTableDict(TableMetaData *tableMetaData)
{
    RECORD         *recTemp;
    char           *pstr, *tempfile;
    FILE           *fpDict;
    uint32_t    iTemp;
    unsigned int    i;
    int             fd;
    int8_t          cTemp;
    TableDict      *tableDict = tableMetaData->tableDict;

    if (!tableDict->iTableChanged)
        return;

    // make ~/.config/fcitx/table/ dir
    FcitxXDGGetFileUserWithPrefix("table", "", "w", NULL);
    FcitxXDGGetFileUserWithPrefix("table", TABLE_TEMP_FILE, NULL, &tempfile);
    fd = mkstemp(tempfile);
    fpDict = NULL;

    if (fd > 0)
        fpDict = fdopen(fd, "w");

    if (!fpDict) {
        FcitxLog(ERROR, _("Save dict error"));
        free(tempfile);
        return;
    }

    // write version number
    fcitx_utils_write_uint32(fpDict, 0);
    fwrite(&iInternalVersion, sizeof(char), 1, fpDict);

    iTemp = strlen(tableDict->strInputCode);
    fcitx_utils_write_uint32(fpDict, iTemp);
    fwrite(tableDict->strInputCode, sizeof(char), iTemp + 1, fpDict);
    fwrite(&(tableDict->iCodeLength), sizeof(char), 1, fpDict);
    fwrite(&(tableDict->iPYCodeLength), sizeof(char), 1, fpDict);
    iTemp = strlen(tableDict->strIgnoreChars);
    fcitx_utils_write_uint32(fpDict, iTemp);
    fwrite(tableDict->strIgnoreChars, sizeof(char), iTemp + 1, fpDict);

    fwrite(&(tableDict->bRule), sizeof(unsigned char), 1, fpDict);
    if (tableDict->bRule) { // table contains rule
        for (i = 0; i < tableDict->iCodeLength - 1; i++) {
            fwrite(&(tableDict->rule[i].iFlag), sizeof(unsigned char), 1, fpDict);
            fwrite(&(tableDict->rule[i].iWords), sizeof(unsigned char), 1, fpDict);
            for (iTemp = 0; iTemp < tableDict->iCodeLength; iTemp++) {
                fwrite(&(tableDict->rule[i].rule[iTemp].iFlag), sizeof(unsigned char), 1, fpDict);
                fwrite(&(tableDict->rule[i].rule[iTemp].iWhich), sizeof(unsigned char), 1, fpDict);
                fwrite(&(tableDict->rule[i].rule[iTemp].iIndex), sizeof(unsigned char), 1, fpDict);
            }
        }
    }

    fcitx_utils_write_uint32(fpDict, tableDict->iRecordCount);
    recTemp = tableDict->recordHead->next;
    while (recTemp != tableDict->recordHead) {
        fwrite(recTemp->strCode, sizeof(char), tableDict->iPYCodeLength + 1, fpDict);

        iTemp = strlen(recTemp->strHZ) + 1;
        fcitx_utils_write_uint32(fpDict, iTemp);
        fwrite(recTemp->strHZ, sizeof(char), iTemp, fpDict);

        cTemp = recTemp->type;
        fwrite(&cTemp, sizeof(int8_t), 1, fpDict);
        fcitx_utils_write_uint32(fpDict, recTemp->iHit);
        fcitx_utils_write_uint32(fpDict, recTemp->iIndex);
        recTemp = recTemp->next;
    }

    fclose(fpDict);
    fpDict = FcitxXDGGetFileUserWithPrefix("table", tableMetaData->strPath, NULL, &pstr);
    if (access(pstr, 0))
        unlink(pstr);
    rename(tempfile, pstr);
    free(pstr);
    free(tempfile);

    FcitxLog(DEBUG, _("Rename OK"));

    tableDict->iTableChanged = 0;

    if (tableDict->autoPhrase) {
        // Save auto phrase
        // make ~/.config/fcitx/table/ dir
        FcitxXDGGetFileUserWithPrefix("table", "", "w", NULL);
        FcitxXDGGetFileUserWithPrefix("table", TABLE_TEMP_FILE, NULL, &tempfile);
        fd = mkstemp(tempfile);
        fpDict = NULL;

        if (fd > 0)
            fpDict = fdopen(fd, "w");

        if (fpDict) {
            fcitx_utils_write_uint32(fpDict, tableDict->iAutoPhrase);
            for (i = 0; i < tableDict->iAutoPhrase; i++) {
                fwrite(tableDict->autoPhrase[i].strCode, tableDict->iCodeLength + 1, 1, fpDict);
                fwrite(tableDict->autoPhrase[i].strHZ, PHRASE_MAX_LENGTH * UTF8_MAX_LENGTH + 1, 1, fpDict);
                fcitx_utils_write_int32(
                    fpDict, tableDict->autoPhrase[i].iSelected);
            }
            fclose(fpDict);
        }

        char *strPath;
        fcitx_utils_alloc_cat_str(strPath, tableMetaData->uniqueName,
                                  "_LastAutoPhrase.tmp");
        fpDict = FcitxXDGGetFileUserWithPrefix("table", strPath, NULL, &pstr);
        free(strPath);
        if (access(pstr, F_OK))
            unlink(pstr);
        rename(tempfile, pstr);
        free(pstr);
        free(tempfile);
    }
}

void FreeTableDict(TableMetaData* tableMetaData)
{
    TableDict      *tableDict = tableMetaData->tableDict;

    if (!tableDict->recordHead)
        return;

    if (tableDict->iTableChanged)
        SaveTableDict(tableMetaData);

    fcitx_memory_pool_destroy(tableDict->pool);
    free(tableDict);
    tableMetaData->tableDict = NULL;
}

/*
 *根据字串判断词库中是否有某个字/词，注意该函数会忽略拼音词组
 */
RECORD         *TableFindPhrase(const TableDict* tableDict, const char *strHZ)
{
    RECORD         *recTemp;
    char            strTemp[UTF8_MAX_LENGTH + 1];
    int             i;

    //首先，先查找第一个汉字的编码
    strncpy(strTemp, strHZ, fcitx_utf8_char_len(strHZ));
    strTemp[fcitx_utf8_char_len(strHZ)] = '\0';

    recTemp = tableDict->tableSingleHZ[CalHZIndex(strTemp)];
    if (!recTemp)
        return (RECORD *) NULL;

    //然后根据该编码找到检索的起始点
    i = 0;
    while (recTemp->strCode[0] != tableDict->recordIndex[i].cCode)
        i++;

    recTemp = tableDict->recordIndex[i].record;
    while (recTemp != tableDict->recordHead) {
        if (recTemp->strCode[0] != tableDict->recordIndex[i].cCode)
            break;
        if (!strcmp(recTemp->strHZ, strHZ)) {
            if (recTemp->type != RECORDTYPE_PINYIN)
                return recTemp;
        }

        recTemp = recTemp->next;
    }

    return (RECORD *) NULL;
}

void TableCreateAutoPhrase(TableMetaData* tableMetaData, char iCount)
{
    char            *strHZ;
    short           i, j, k;
    TableDict      *tableDict = tableMetaData->tableDict;

    if (!tableDict->autoPhrase)
        return;

    strHZ = (char*)fcitx_utils_malloc0((tableMetaData->iAutoPhraseLength * UTF8_MAX_LENGTH + 1) * sizeof(char));
    /*
     * 为了提高效率，此处只重新生成新录入字构成的词组
     */
    j = tableDict->iHZLastInputCount - tableMetaData->iAutoPhraseLength - iCount;
    if (j < 0)
        j = 0;

    for (; j < tableDict->iHZLastInputCount - 1; j++) {
        for (i = tableMetaData->iAutoPhraseLength; i >= 2; i--) {
            if ((j + i - 1) > tableDict->iHZLastInputCount)
                continue;

            strcpy(strHZ, tableDict->hzLastInput[j].strHZ);
            for (k = 1; k < i; k++)
                strcat(strHZ, tableDict->hzLastInput[j + k].strHZ);

            //再去掉重复的词组
            for (k = 0; k < tableDict->iAutoPhrase; k++) {
                if (!strcmp(tableDict->autoPhrase[k].strHZ, strHZ))
                    goto _next;
            }
            //然后去掉系统中已经有的词组
            if (TableFindPhrase(tableDict, strHZ))
                goto _next;

            TableCreatePhraseCode(tableDict, strHZ);
            if (tableDict->iAutoPhrase != AUTO_PHRASE_COUNT) {
                strcpy(tableDict->autoPhrase[tableDict->iAutoPhrase].strCode, tableDict->strNewPhraseCode);
                strcpy(tableDict->autoPhrase[tableDict->iAutoPhrase].strHZ, strHZ);
                tableDict->autoPhrase[tableDict->iAutoPhrase].iSelected = 0;
                tableDict->iAutoPhrase++;
            } else {
                strcpy(tableDict->insertPoint->strCode, tableDict->strNewPhraseCode);
                strcpy(tableDict->insertPoint->strHZ, strHZ);
                tableDict->insertPoint->iSelected = 0;
                tableDict->insertPoint = tableDict->insertPoint->next;
            }
            tableDict->iTableChanged++;

        _next:
            continue;
        }

    }

    free(strHZ);
}

boolean TableCreatePhraseCode(TableDict* tableDict, char *strHZ)
{
    unsigned char   i;
    unsigned char   i1, i2;
    size_t          iLen;
    char            strTemp[UTF8_MAX_LENGTH + 1] = {'\0', };
    RECORD         *recTemp;
    boolean bCanntFindCode = false;

    iLen = fcitx_utf8_strlen(strHZ);
    if (iLen >= tableDict->iCodeLength) {
        i2 = tableDict->iCodeLength;
        i1 = 1;
    } else {
        i2 = iLen;
        i1 = 0;
    }

    for (i = 0; i < tableDict->iCodeLength - 1; i++) {
        if (tableDict->rule[i].iWords == i2 && tableDict->rule[i].iFlag == i1)
            break;
    }

    if (i == tableDict->iCodeLength - 1)
        return true;

    int codeIdx = 0;
    for (i1 = 0; i1 < tableDict->iCodeLength; i1++) {
        int clen;
        char* ps;
        if (tableDict->rule[i].rule[i1].iFlag) {
            ps = fcitx_utf8_get_nth_char(strHZ, tableDict->rule[i].rule[i1].iWhich - 1);
            clen = fcitx_utf8_char_len(ps);
            strncpy(strTemp, ps, clen);
        } else {
            ps = fcitx_utf8_get_nth_char(strHZ, iLen - tableDict->rule[i].rule[i1].iWhich);
            clen = fcitx_utf8_char_len(ps);
            strncpy(strTemp, ps, clen);
        }

        int hzIndex = CalHZIndex(strTemp);

        if (tableDict->tableSingleHZ[hzIndex]) {
            if (tableDict->tableSingleHZCons[hzIndex])
                recTemp = tableDict->tableSingleHZCons[hzIndex];
            else
                recTemp = tableDict->tableSingleHZ[hzIndex];
        }
        else {
            bCanntFindCode = true;
            break;
        }

        if (strlen(recTemp->strCode) >= tableDict->rule[i].rule[i1].iIndex) {
            tableDict->strNewPhraseCode[codeIdx] = recTemp->strCode[tableDict->rule[i].rule[i1].iIndex - 1];
            codeIdx++;
        }

    }

    return bCanntFindCode;
}

/*
 *判断某个词组是不是已经在词库中,有返回NULL，无返回插入点
 */
RECORD         *TableHasPhrase(const TableDict* tableDict, const char *strCode, const char *strHZ)
{
    RECORD         *recTemp;
    int             i;

    i = 0;
    while (strCode[0] != tableDict->recordIndex[i].cCode)
        i++;

    recTemp = tableDict->recordIndex[i].record;
    while (recTemp != tableDict->recordHead) {
        if (recTemp->type != RECORDTYPE_PINYIN) {
            if (strcmp(recTemp->strCode, strCode) > 0)
                break;
            else if (!strcmp(recTemp->strCode, strCode)) {
                if (!strcmp(recTemp->strHZ, strHZ))     //该词组已经在词库中
                    return NULL;
            }
        }
        recTemp = recTemp->next;
    }

    return recTemp;
}

void TableInsertPhrase(TableDict* tableDict, const char *strCode, const char *strHZ)
{
    RECORD         *insertPoint, *dictNew;

    insertPoint = TableHasPhrase(tableDict, strCode, strHZ);

    if (!insertPoint)
        return;

    dictNew = (RECORD*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(RECORD));
    dictNew->strCode = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (tableDict->iCodeLength + 1));
    dictNew->type = RECORDTYPE_NORMAL;
    strcpy(dictNew->strCode, strCode);
    dictNew->strHZ = (char*)fcitx_memory_pool_alloc(tableDict->pool, sizeof(char) * (strlen(strHZ) + 1));
    strcpy(dictNew->strHZ, strHZ);
    dictNew->iHit = 0;
    dictNew->iIndex = tableDict->iTableIndex;

    dictNew->prev = insertPoint->prev;
    insertPoint->prev->next = dictNew;
    insertPoint->prev = dictNew;
    dictNew->next = insertPoint;

    tableDict->iRecordCount++;
    tableDict->iTableChanged++;
}

/*
 * 根据字串删除词组
 */
void TableDelPhraseByHZ(TableDict* tableDict, const char *strHZ)
{
    RECORD         *recTemp;

    recTemp = TableFindPhrase(tableDict, strHZ);
    if (recTemp)
        TableDelPhrase(tableDict, recTemp);
}

void TableDelPhrase(TableDict* tableDict, RECORD * record)
{
    record->prev->next = record->next;
    record->next->prev = record->prev;

    /*
     * since we use memory pool, don't free record
     * though free list is currently not supported, but it's ok
     * people will not delete phrase so many times
     */

    tableDict->iRecordCount--;
    tableDict->iTableChanged++;
}

void TableUpdateHitFrequency(TableMetaData* tableMetaData, RECORD * record)
{
    if (tableMetaData->tableOrder != AD_NO) {
        tableMetaData->tableDict->iTableChanged++;
        record->iHit++;
        record->iIndex = ++tableMetaData->tableDict->iTableIndex;
    }
}

int TableCompareCode(const TableMetaData* tableMetaData, const char *strUser, const char *strDict, boolean exactMatch)
{
    int             i;

    size_t tmp_len = strlen(strUser);
    for (i = 0; i < tmp_len; i++) {
        if (!strDict[i])
            return strUser[i];
        if (strUser[i] != tableMetaData->cMatchingKey || !tableMetaData->bUseMatchingKey) {
            if (strUser[i] != strDict[i])
                return (strUser[i] - strDict[i]);
        }
    }
    if (exactMatch) {
        if (strlen(strUser) != strlen(strDict))
            return -999;    //随意的一个值
    }

    return 0;
}

int TableFindFirstMatchCode(TableMetaData* tableMetaData, const char* strCodeInput, boolean exactMatch, boolean cacheCurrentRecord)
{
    int             i = 0;
    TableDict      *tableDict = tableMetaData->tableDict;

    if (!tableDict->recordHead)
        return -1;

    if (tableMetaData->bUseMatchingKey && (strCodeInput[0] == tableMetaData->cMatchingKey))
        i = 0;
    else {
        while (strCodeInput[0] != tableDict->recordIndex[i].cCode) {
            if (!tableDict->recordIndex[i].cCode)
                break;
            i++;
        }
    }
    RECORD* record = NULL;
    RECORD** pRecord = &record;
    if (cacheCurrentRecord)
        pRecord = &tableDict->currentRecord;

    *pRecord = tableDict->recordIndex[i].record;
    if (!*pRecord)
        return -1;

    while (*pRecord != tableDict->recordHead) {
        if (!TableCompareCode(tableMetaData, strCodeInput, (*pRecord)->strCode, exactMatch)) {
            return i;
        }
        (*pRecord) = (*pRecord)->next;
        i++;
    }

    return -1;          //Not found
}

boolean IsInputKey(const TableDict* tableDict, int iKey)
{
    char           *p;

    p = tableDict->strInputCode;
    if (!p)
        return false;

    while (*p) {
        if (iKey == *p)
            return true;
        p++;
    }

    if (tableDict->bHasPinyin) {
        if (iKey >= 'a' && iKey <= 'z')
            return true;
    }

    return false;
}

boolean IsEndKey(const TableMetaData* tableMetaData, char cChar)
{
    char           *p;

    p = tableMetaData->strEndCode;
    if (!p)
        return false;

    while (*p) {
        if (cChar == *p)
            return true;
        p++;
    }

    return false;
}

boolean IsIgnoreChar(const TableDict* tableDict, char cChar)
{
    char           *p;

    p = tableDict->strIgnoreChars;
    while (*p) {
        if (cChar == *p)
            return true;
        p++;
    }

    return false;
}

#include "utf8_in_gb18030.h"

static int cmpi(const void * a, const void *b)
{
    return (*((int*)a)) - (*((int*)b));
}

unsigned int CalHZIndex(char *strHZ)
{
    unsigned int iutf = 0;
    int l = fcitx_utf8_char_len(strHZ);
    unsigned char* utf = (unsigned char*) strHZ;
    unsigned int *res;
    int idx;

    if (l == 2) {
        iutf = *utf++ << 8;
        iutf |= *utf++;
    } else if (l == 3) {
        iutf = *utf++ << 16;
        iutf |= *utf++ << 8;
        iutf |= *utf++;
    } else if (l == 4) {
        iutf = *utf++ << 24;

        iutf |= *utf++ << 16;
        iutf |= *utf++ << 8;
        iutf |= *utf++;
    }

    res = bsearch(&iutf, fcitx_utf8_in_gb18030, 63360, sizeof(int), cmpi);
    if (res)
        idx = res - fcitx_utf8_in_gb18030;
    else
        idx = 63361;
    return idx;
}

boolean HasMatchingKey(const TableMetaData* tableMetaData, const char* strCodeInput)
{
    const char *str;

    str = strCodeInput;
    while (*str) {
        if (*str++ == tableMetaData->cMatchingKey)
            return true;
    }
    return false;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
