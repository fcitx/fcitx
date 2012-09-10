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
#ifdef FCITX_HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "fcitx-utils/utf8.h"
#include "fcitx/fcitx.h"
#include "fcitx-config/fcitx-config.h"
#include "im/table/tabledict.h"

#define CHECK_OPTION(str, x) ((strstr((str), strConst[x]) == (str)) || (strstr((str), strConstNew[x]) == (str)))
#define ADD_LENGTH(str, x) ((strstr((str), strConst[x]) == (str)) ? (strlen(strConst[x])) : (strlen(strConstNew[x])))

#define STR_KEYCODE 0
#define STR_CODELEN 1
#define STR_IGNORECHAR 2
#define STR_PINYIN 3
#define STR_PINYINLEN 4
#define STR_DATA 5
#define STR_RULE 6
#define STR_PROMPT 7
#define STR_CONSTRUCTPHRASE 8

#define CONST_STR_SIZE 9

#define MAX_CODE_LENGTH  30
#define FH_MAX_LENGTH  10
#define TABLE_AUTO_SAVE_AFTER 1024
#define AUTO_PHRASE_COUNT 10000
#define SINGLE_HZ_COUNT 66000

char* strConst[CONST_STR_SIZE] = { "键码=", "码长=", "规避字符=", "拼音=", "拼音长度=" , "[数据]", "[组词规则]", "提示=", "构词="};
char* strConstNew[CONST_STR_SIZE] = { "KeyCode=", "Length=", "InvalidChar=", "Pinyin=", "PinyinLength=" , "[Data]", "[Rule]", "Prompt=", "ConstructPhrase="};

char            strInputCode[100] = "\0";
char            strIgnoreChars[100] = "\0";
char            cPinyinKey = '\0';
char            cPromptKey = '&';
char            cPhraseKey = '^';

boolean IsValidCode(char cChar)
{
    char           *p;

    p = strInputCode;

    while (*p) {
        if (cChar == *p)
            return true;

        p++;
    }

    p = strIgnoreChars;

    while (*p) {
        if (cChar == *p)
            return true;

        p++;
    }

    if (cChar == cPinyinKey || cChar == cPhraseKey || cChar == cPromptKey)
        return true;

    return false;
}

int main(int argc, char *argv[])
{
    FILE           *fpDict, *fpNew;
    RECORD         *temp, *head, *newRec, *current;
    uint32_t        s = 0;
    int             i;
    uint32_t        iTemp;
    char           *pstr = 0;
    char            strTemp[10];
    unsigned char   bRule;
    RULE           *rule = NULL;
    unsigned int    l;

    unsigned char   iCodeLength = 0;
    unsigned char   iPYCodeLength = 0;

    int8_t          type;

    if (argc != 3) {
        printf("\nUsage: txt2mb <Source File> <IM File>\n\n");
        exit(1);
    }

    fpDict = fopen(argv[1], "r");

    if (!fpDict) {
        printf("\nCan not read source file!\n\n");
        exit(2);
    }

    head = (RECORD *) malloc(sizeof(RECORD));
    head->next = head;
    head->prev = head;
    current = head;

    bRule = 0;
    l = 0;

    char* buf = NULL, *buf1 = NULL;
    size_t len;
    for (;;) {
        l++;

        if (getline(&buf, &len, fpDict) == -1)
            break;

        i = strlen(buf) - 1;

        while ((i >= 0) && (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r'))
            buf[i--] = '\0';

        pstr = buf;

        if (*pstr == ' ')
            pstr++;

        if (pstr[0] == '#')
            continue;

        if (CHECK_OPTION(pstr, STR_KEYCODE)) {
            pstr += ADD_LENGTH(pstr, STR_KEYCODE);
            strcpy(strInputCode, pstr);
        } else if (CHECK_OPTION(pstr, STR_CODELEN)) {
            pstr += ADD_LENGTH(pstr, STR_CODELEN);
            iCodeLength = atoi(pstr);

            if (iCodeLength > MAX_CODE_LENGTH) {
                iCodeLength = MAX_CODE_LENGTH;
                printf("Max Code Length is %d\n", MAX_CODE_LENGTH);
            }
        } else if (CHECK_OPTION(pstr, STR_IGNORECHAR)) {
            pstr += ADD_LENGTH(pstr, STR_IGNORECHAR);
            strcpy(strIgnoreChars, pstr);
        } else if (CHECK_OPTION(pstr, STR_PINYIN)) {
            pstr += ADD_LENGTH(pstr, STR_PINYIN);

            while (*pstr == ' ' && *pstr != '\0')
                pstr++;

            cPinyinKey = *pstr;
        } else if (CHECK_OPTION(pstr, STR_PROMPT)) {
            pstr += ADD_LENGTH(pstr, STR_PROMPT);

            while (*pstr == ' ' && *pstr != '\0')
                pstr++;

            cPromptKey = *pstr;
        } else if (CHECK_OPTION(pstr, STR_CONSTRUCTPHRASE)) {
            pstr += ADD_LENGTH(pstr, STR_CONSTRUCTPHRASE);

            while (*pstr == ' ' && *pstr != '\0')
                pstr++;

            cPhraseKey = *pstr;
        } else if (CHECK_OPTION(pstr, STR_PINYINLEN)) {
            pstr += ADD_LENGTH(pstr, STR_PINYINLEN);
            iPYCodeLength = atoi(pstr);
        }

        else if (CHECK_OPTION(pstr, STR_DATA))
            break;
        else if (CHECK_OPTION(pstr, STR_RULE)) {
            bRule = 1;
            break;
        }
    }

    if (iCodeLength <= 0 || !strInputCode[0]) {
        printf("Source File Format Error!\n");
        exit(1);
    }

    if (bRule) {
        /*
         * 组词规则数应该比键码长度小1
         */
        rule = (RULE *) malloc(sizeof(RULE) * (iCodeLength - 1));

        for (iTemp = 0; iTemp < (iCodeLength - 1); iTemp++) {
            l++;

            if (getline(&buf, &len, fpDict) == -1)
                break;

            rule[iTemp].rule = (RULE_RULE *) malloc(sizeof(RULE_RULE) * iCodeLength);

            i = strlen(buf) - 1;

            while ((i >= 0) && (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r'))
                buf[i--] = '\0';

            pstr = buf;

            if (*pstr == ' ')
                pstr++;

            if (pstr[0] == '#')
                continue;

            if (CHECK_OPTION(pstr, STR_DATA))
                break;

            switch (*pstr) {

            case 'e':

            case 'E':
                rule[iTemp].iFlag = 0;
                break;

            case 'a':

            case 'A':
                rule[iTemp].iFlag = 1;
                break;

            default:
                printf("2   Phrase rules are not suitable!\n");
                printf("\t\t%s\n", buf);
                exit(1);
            }

            pstr++;

            char* p = pstr;

            while (*p && *p != '=')
                p++;

            if (!(*p)) {
                printf("3   Phrase rules are not suitable!\n");
                printf("\t\t%s\n", buf);
                exit(1);
            }

            strncpy(strTemp, pstr, p - pstr);

            strTemp[p - pstr] = '\0';
            rule[iTemp].iWords = atoi(strTemp);

            p++;

            for (i = 0; i < iCodeLength; i++) {
                while (*p == ' ')
                    p++;

                switch (*p) {

                case 'p':

                case 'P':
                    rule[iTemp].rule[i].iFlag = 1;
                    break;

                case 'n':

                case 'N':
                    rule[iTemp].rule[i].iFlag = 0;
                    break;

                default:
                    printf("4   Phrase rules are not suitable!\n");
                    printf("\t\t%s\n", buf);
                    exit(1);
                }

                p++;

                rule[iTemp].rule[i].iWhich = *p++ - '0';
                rule[iTemp].rule[i].iIndex = *p++ - '0';

                while (*p == ' ')
                    p++;

                if (i != (iCodeLength - 1)) {
                    if (*p != '+') {
                        printf("5   Phrase rules are not suitable!\n");
                        printf("\t\t%s  %d\n", buf, iCodeLength);
                        exit(1);
                    }

                    p++;
                }
            }
        }

        if (iTemp != iCodeLength - 1) {
            printf("6  Phrase rules are not suitable!\n");
            exit(1);
        }

        for (iTemp = 0; iTemp < (iCodeLength - 1); iTemp++) {
            l++;

            if (getline(&buf, &len, fpDict) == -1)
                break;

            i = strlen(buf) - 1;

            while ((i >= 0) && (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r'))
                buf[i--] = '\0';

            pstr = buf;

            if (*pstr == ' ')
                pstr++;

            if (pstr[0] == '#')
                continue;

            if (CHECK_OPTION(pstr, STR_DATA))
                break;
        }
    }

    if (iPYCodeLength < iCodeLength)
        iPYCodeLength = iCodeLength;

    if (!CHECK_OPTION(pstr, STR_DATA)) {
        printf("Source File Format Error!\n");
        exit(1);
    }

    while (getline(&buf, &len, fpDict) != -1) {
        l++;
        if (buf1)
            free(buf1);
        buf1 = fcitx_utils_trim(buf);
        char *p = buf1;

        while (*p && !isspace(*p))
            p ++;

        if (*p == '\0')
            continue;

        while (isspace(*p)) {
            *p = '\0';
            p ++;
        }

        char* strHZ = p;

        if (!IsValidCode(buf1[0])) {
            printf("Invalid Format: Line-%d  %s %s\n", l, buf1, strHZ);

            exit(1);
        }

        if (((buf1[0] != cPinyinKey) && (strlen(buf1) > iCodeLength))
            || ((buf1[0] == cPinyinKey) && (strlen(buf1) > (iPYCodeLength + 1)))
            || ((buf1[0] == cPhraseKey) && (strlen(buf1) > (iCodeLength + 1)))
            || ((buf1[0] == cPromptKey) && (strlen(buf1) > (iPYCodeLength + 1)))
        ) {
            printf("Delete:  %s %s, Too long\n", buf1, strHZ);
            continue;
        }

        size_t hzLen = fcitx_utf8_strlen(strHZ);
        if (buf1[0] == cPhraseKey && hzLen != 1) {
            printf("Delete:  %s %s, Too long\n", buf1, strHZ);
            continue;
        }

        type = RECORDTYPE_NORMAL;

        pstr = buf1;

        if (buf1[0] == cPinyinKey) {
            pstr ++;
            type = RECORDTYPE_PINYIN;
        } else if (buf1[0] == cPhraseKey) {
            pstr ++;
            type = RECORDTYPE_CONSTRUCT;
        } else if (buf1[0] == cPromptKey) {
            pstr ++;
            type = RECORDTYPE_PROMPT;
        }

        //查找是否重复
        temp = current;

        if (temp != head) {
            if (strcmp(temp->strCode, pstr) >= 0) {
                while (temp != head && strcmp(temp->strCode, pstr) >= 0) {
                    if (!strcmp(temp->strHZ, strHZ) && !strcmp(temp->strCode, pstr) && temp->type == type) {
                        printf("Delete:  %s %s\n", pstr, strHZ);
                        goto _next;
                    }

                    temp = temp->prev;
                }

                if (temp == head)
                    temp = temp->next;

                while (temp != head && strcmp(temp->strCode, pstr) <= 0)
                    temp = temp->next;
            } else {
                while (temp != head && strcmp(temp->strCode, pstr) <= 0) {
                    if (!strcmp(temp->strHZ, strHZ) && !strcmp(temp->strCode, pstr) && temp->type == type) {
                        printf("Delete:  %s %s\n", pstr, strHZ);
                        goto _next;
                    }

                    temp = temp->next;
                }
            }
        }

        //插在temp的前面
        newRec = (RECORD *) fcitx_utils_malloc0(sizeof(RECORD));

        newRec->strCode = (char *) fcitx_utils_malloc0(sizeof(char) * (iPYCodeLength + 1));

        newRec->strHZ = (char *) fcitx_utils_malloc0(sizeof(char) * strlen(strHZ) + 1);

        strcpy(newRec->strCode, pstr);

        strcpy(newRec->strHZ, strHZ);

        newRec->type = type;

        newRec->iHit = 0;

        newRec->iIndex = 0;

        temp->prev->next = newRec;

        newRec->next = temp;

        newRec->prev = temp->prev;

        temp->prev = newRec;

        current = newRec;

        s++;

    _next:
        continue;

    }


    if (buf)
        free(buf);
    if (buf1)
        free(buf1);

    fclose(fpDict);

    printf("\nReading %d records.\n\n", s);

    fpNew = fopen(argv[2], "w");

    if (!fpNew) {
        printf("\nCan not create target file!\n\n");
        exit(3);
    }

    int8_t iInternalVersion = INTERNAL_VERSION;

    //写入版本号--如果第一个字为0,表示后面那个字节为版本号
    fcitx_utils_write_uint32(fpNew, 0);
    fwrite(&iInternalVersion, sizeof(int8_t), 1, fpNew);

    iTemp = (uint32_t)strlen(strInputCode);
    fcitx_utils_write_uint32(fpNew, iTemp);
    fwrite(strInputCode, sizeof(char), iTemp + 1, fpNew);
    fwrite(&iCodeLength, sizeof(unsigned char), 1, fpNew);
    fwrite(&iPYCodeLength, sizeof(unsigned char), 1, fpNew);
    iTemp = (uint32_t)strlen(strIgnoreChars);
    fcitx_utils_write_uint32(fpNew, iTemp);
    fwrite(strIgnoreChars, sizeof(char), iTemp + 1, fpNew);

    fwrite(&bRule, sizeof(unsigned char), 1, fpNew);

    if (bRule) {
        for (i = 0; i < iCodeLength - 1; i++) {
            fwrite(&(rule[i].iFlag), sizeof(unsigned char), 1, fpNew);
            fwrite(&(rule[i].iWords), sizeof(unsigned char), 1, fpNew);

            for (iTemp = 0; iTemp < iCodeLength; iTemp++) {
                fwrite(&(rule[i].rule[iTemp].iFlag), sizeof(unsigned char), 1, fpNew);
                fwrite(&(rule[i].rule[iTemp].iWhich), sizeof(unsigned char), 1, fpNew);
                fwrite(&(rule[i].rule[iTemp].iIndex), sizeof(unsigned char), 1, fpNew);
            }
        }
    }

    fcitx_utils_write_uint32(fpNew, s);

    current = head->next;

    while (current != head) {
        fwrite(current->strCode, sizeof(char), iPYCodeLength + 1, fpNew);
        s = strlen(current->strHZ) + 1;
        fcitx_utils_write_uint32(fpNew, s);
        fwrite(current->strHZ, sizeof(char), s, fpNew);
        fwrite(&(current->type), sizeof(int8_t), 1, fpNew);
        fcitx_utils_write_uint32(fpNew, current->iHit);
        fcitx_utils_write_uint32(fpNew, current->iIndex);
        current = current->next;
    }

    fclose(fpNew);

    return 0;
}

// kate: indent-mode cstyle; space-indent on; indent-width 4;
