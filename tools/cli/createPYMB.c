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
#include <stdio.h>
#include <string.h>

#include "im/pinyin/pyParser.h"
#include "im/pinyin/pyMapTable.h"
#include "im/pinyin/PYFA.h"
#include "fcitx-utils/utf8.h"
#include "im/pinyin/pyconfig.h"
#include <im/pinyin/py.h>

FcitxPinyinConfig pyconfig;

FILE           *fps, *fpt, *fp1, *fp2;
boolean         bSingleHZMode = false;

typedef struct _PY {
    char            strPY[3];
    char            strHZ[UTF8_MAX_LENGTH + 1];

    struct _PY     *next, *prev;
} _PyStruct;

typedef struct _PyPhrase {
    char           *strPhrase;
    char           *strMap;

    struct _PyPhrase *next;
    unsigned int    uIndex;
} _PyPhrase;

typedef struct _PyBase {
    char            strHZ[UTF8_MAX_LENGTH + 1];

    struct _PyPhrase *phrase;
    int             iPhraseCount;
    unsigned int    iIndex;
} _PyBase;

typedef struct {
    char            strMap[3];

    struct _PyBase *pyBase;
    int             iHZCount;
    //char *strMohu;
} __PYFA;

int             iPYFACount;
__PYFA         *PYFAList;
int             YY[1000];
int             iAllCount;

static void Usage();

boolean LoadPY(void)
{
    FILE           *fp;
    int             i, j;
    int             iSW;;

    fp = fopen("pybase.mb", "r");

    if (!fp)
        return false;

    fcitx_utils_read_int32(fp, &iPYFACount);

    PYFAList = (__PYFA *) malloc(sizeof(__PYFA) * iPYFACount);

    for (i = 0; i < iPYFACount; i++) {
        fread(PYFAList[i].strMap, sizeof(char) * 2, 1, fp);
        PYFAList[i].strMap[2] = '\0';
        fcitx_utils_read_int32(fp, &(PYFAList[i].iHZCount));
        PYFAList[i].pyBase = (_PyBase *) malloc(sizeof(_PyBase) * PYFAList[i].iHZCount);

        for (j = 0; j < PYFAList[i].iHZCount; j++) {
            int8_t len;
            fread(&len, sizeof(int8_t) , 1, fp);
            fread(PYFAList[i].pyBase[j].strHZ, sizeof(char) * len, 1, fp);
            PYFAList[i].pyBase[j].strHZ[len] = '\0';
            PYFAList[i].pyBase[j].phrase = (_PyPhrase *) malloc(sizeof(_PyPhrase));
            PYFAList[i].pyBase[j].phrase->next = NULL;
            PYFAList[i].pyBase[j].iPhraseCount = 0;
        }
    }

    fclose(fp);

    i = 0;

    while (1) {
        iSW = 0;

        for (j = 0; j < iPYFACount; j++) {
            if (i < PYFAList[j].iHZCount) {
                PYFAList[j].pyBase[i].iIndex = iAllCount--;
                iSW = 1;
            }
        }

        if (!iSW)
            break;

        i++;
    }

    fp = fopen("pybase.mb", "w");

    if (!fp)
        return false;

    fcitx_utils_write_int32(fp, iPYFACount);

    for (i = 0; i < iPYFACount; i++) {
        fwrite(PYFAList[i].strMap, sizeof(char) * 2, 1, fp);
        fcitx_utils_write_int32(fp, PYFAList[i].iHZCount);

        for (j = 0; j < PYFAList[i].iHZCount; j++) {
            int8_t len = strlen(PYFAList[i].pyBase[j].strHZ);
            fwrite(&len, sizeof(int8_t), 1, fp);
            fwrite(PYFAList[i].pyBase[j].strHZ, sizeof(char) * len, 1, fp);
            fcitx_utils_write_int32(fp, PYFAList[i].pyBase[j].iIndex);
        }
    }

    fclose(fp);

    return true;
}

void CreatePYPhrase(void)
{
    char            strPY[256];
    char            strPhrase[256];
    char            strMap[256];
    ParsePYStruct   strTemp;
    int32_t         iIndex, i, s1, s2, j, k;
    _PyPhrase      *phrase, *t, *tt;
    FILE           *f = fopen("pyERROR", "w");
    FILE           *fg = fopen("pyPhrase.ok", "w");
    int             kkk;
    uint32_t        uIndex;
    FcitxPinyinConfig pyconfig;

    memset(&pyconfig, 0 , sizeof(pyconfig));
    InitMHPY(&pyconfig.MHPY_C, MHPY_C_TEMPLATE);
    InitMHPY(&pyconfig.MHPY_S, MHPY_S_TEMPLATE);
    InitPYTable(&pyconfig);

    s1 = 0;
    s2 = 0;
    uIndex = 0;
    printf("Start Loading Phrase...\n");

    while (!feof(fpt)) {
        fscanf(fpt, "%s", strPY);
        fscanf(fpt, "%s\n", strPhrase);

        if (strlen(strPhrase) < 3)
            continue;

        ParsePY(&pyconfig, strPY, &strTemp, PY_PARSE_INPUT_SYSTEM, false);

        s2++;

        kkk = 0;

        if (strTemp.iHZCount != (int)fcitx_utf8_strlen(strPhrase) ||
            (strTemp.iMode & PARSE_ABBR)) {
            fprintf(f, "%s %s\n", strPY, strPhrase);
            continue;
        }

        strMap[0] = '\0';

        for (iIndex = 0; iIndex < strTemp.iHZCount; iIndex++)
            strcat(strMap, strTemp.strMap[iIndex]);

        for (iIndex = 0; iIndex < iPYFACount; iIndex++) {
            if (!strncmp(PYFAList[iIndex].strMap, strMap, 2)) {
                for (i = 0; i < PYFAList[iIndex].iHZCount; i++) {
                    if (!fcitx_utf8_strncmp(PYFAList[iIndex].pyBase[i].strHZ, strPhrase, 1)) {
                        t = PYFAList[iIndex].pyBase[i].phrase;

                        for (j = 0; j < PYFAList[iIndex].pyBase[i].iPhraseCount; j++) {
                            tt = t;
                            t = t->next;

                            if (!strcmp(t->strMap, strMap + 2) && !strcmp(t->strPhrase, strPhrase + fcitx_utf8_char_len(strPhrase))) {
                                printf("\n\t%d: %s %s ----->deleted.\n", s2, strPY, strPhrase);
                                goto _next;
                            }

                            if (strcmp(t->strMap, strMap + 2) > 0) {
                                t = tt;
                                break;
                            }
                        }

                        phrase = (_PyPhrase *) malloc(sizeof(_PyPhrase));

                        phrase->strPhrase = (char *) malloc(sizeof(char) * (strlen(strPhrase) - fcitx_utf8_char_len(strPhrase) + 1));
                        phrase->strMap = (char *) malloc(sizeof(char) * ((strTemp.iHZCount - 1) * 2 + 1));
                        phrase->uIndex = uIndex++;
                        strcpy(phrase->strPhrase, strPhrase + fcitx_utf8_char_len(strPhrase));
                        strcpy(phrase->strMap, strMap + 2);

                        tt = t->next;
                        t->next = phrase;
                        phrase->next = tt;
                        PYFAList[iIndex].pyBase[i].iPhraseCount++;
                        s1++;
                        kkk = 1;

                    _next:
                        ;
                    }
                }
            }
        }

        if (!kkk)
            fprintf(f, "%s %s %s\n", strPY, strPhrase, (char *)(strTemp.strPYParsed));
        else
            fprintf(fg, "%s %s\n", strPY, strPhrase);
    }

    printf("%d Phrases, %d Converted!\nWriting Phrase file ...", s2, s1);

    for (i = 0; i < iPYFACount; i++) {
        for (j = 0; j < PYFAList[i].iHZCount; j++) {
            iIndex = PYFAList[i].pyBase[j].iPhraseCount;

            if (iIndex) {
                int8_t clen = strlen(PYFAList[i].pyBase[j].strHZ);
                fcitx_utils_write_int32(fp2, i);
                fwrite(&clen, sizeof(int8_t), 1, fp2);
                fwrite(PYFAList[i].pyBase[j].strHZ, sizeof(char) * clen, 1, fp2);

                fcitx_utils_write_int32(fp2, iIndex);
                t = PYFAList[i].pyBase[j].phrase->next;

                for (k = 0; k < PYFAList[i].pyBase[j].iPhraseCount; k++) {
                    int32_t slen = strlen(t->strPhrase);
                    iIndex = strlen(t->strMap);
                    fcitx_utils_write_int32(fp2, iIndex);
                    fwrite(t->strMap, sizeof(char), iIndex, fp2);
                    fcitx_utils_write_int32(fp2, slen);
                    fwrite(t->strPhrase, sizeof(char), strlen(t->strPhrase), fp2);
                    fcitx_utils_write_uint32(fp2, uIndex - 1 - t->uIndex);
                    t = t->next;
                }
            }
        }
    }

    printf("\nOK!\n");

    fclose(fp2);
    fclose(fpt);
}

void CreatePYBase(void)
{
    _PyStruct      *head, *pyList, *temp, *t;
    char            strPY[7], strHZ[UTF8_MAX_LENGTH * 80 + 1], strMap[3];
    int32_t         iIndex, iCount, i;
    int32_t         iBaseCount;
    int32_t         s = 0;
    int32_t         tt = 0;

    head = (_PyStruct *) malloc(sizeof(_PyStruct));
    head->prev = head;
    head->next = head;

    iBaseCount = 0;

    while (PYTable_template[iBaseCount].strPY[0] != '\0')
        iBaseCount++;

    for (iIndex = 0; iIndex < iBaseCount; iIndex++)
        YY[iIndex] = 0;

    iIndex = 0;

    while (!feof(fps)) {
        fscanf(fps, "%s", strPY);
        fscanf(fps, "%s\n", strHZ);

        if (MapPY(&pyconfig, strPY, strMap, PY_PARSE_INPUT_SYSTEM)) {
            for (i = 0; i < iBaseCount; i++)
                if ((!strcmp(PYTable_template[i].strPY, strPY)) && PYTable_template[i].control == PYTABLE_NONE)
                    YY[i] += 1;

            iIndex++;

            if (fcitx_utf8_strlen(strHZ) > 1) {
                int8_t charLen = fcitx_utf8_char_len(strHZ);
                fprintf(stderr, "%s length is larger that 1, truncated to ", strHZ);
                strHZ[charLen] = '\0';
                fprintf(stderr, "%s.\n", strHZ);
            }

            temp = (_PyStruct *) malloc(sizeof(_PyStruct));

            strcpy(temp->strHZ, strHZ);
            strcpy(temp->strPY, strMap);
            pyList = head->prev;

            while (pyList != head) {
                if (strcmp(pyList->strPY, strMap) <= 0)
                    break;

                pyList = pyList->prev;
            }

            temp->next = pyList->next;

            temp->prev = pyList;
            pyList->next->prev = temp;
            pyList->next = temp;
        } else
            fprintf(stderr, "%s Error!!!!\n", strPY);
    }

    iCount = 0;

    for (i = 0; i < iBaseCount; i++) {
        if (YY[i])
            iCount++;
    }

    fcitx_utils_write_uint32(fp1, iCount);

    printf("Groups: %d\n", iCount);
    iAllCount = iIndex;

    pyList = head->next;

    strcpy(strPY, pyList->strPY);
    iCount = 0;
    t = pyList;

    while (pyList != head) {
        if (!strcmp(strPY, pyList->strPY)) {
            iCount++;
        } else {
            tt++;
            fwrite(strPY, sizeof(char) * 2, 1, fp1);
            fcitx_utils_write_uint32(fp1, iCount);

            for (i = 0; i < iCount; i++) {
                int8_t len = strlen(t->strHZ);
                fwrite(&len, sizeof(int8_t), 1, fp1);
                fwrite(t->strHZ, sizeof(char) * len , 1, fp1);

                t = t->next;
            }

            s += iCount;

            t = pyList;
            iCount = 1;
            strcpy(strPY, pyList->strPY);
        }

        pyList = pyList->next;
    }

    fwrite(strPY, sizeof(char) * 2, 1, fp1);

    fcitx_utils_write_uint32(fp1, iCount);

    for (i = 0; i < iCount; i++) {
        int8_t len = strlen(t->strHZ);
        fwrite(&len, sizeof(int8_t), 1, fp1);
        fwrite(t->strHZ, sizeof(char) * len , 1, fp1);
        t = t->next;
    }

    fclose(fp1);
    fclose(fps);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        Usage();
        exit(1);
    }

    fps = fopen(argv[1], "r");

    fpt = fopen(argv[2], "r");
    fp1 = fopen("pybase.mb", "w");
    fp2 = fopen("pyphrase.mb", "w");

    if (fps && fpt && fp1 && fp2) {
        CreatePYBase();
        LoadPY();
        CreatePYPhrase();
    }

    return 0;
}

void Usage()
{
    printf("Usage: createPYMB <pyfile> <phrasefile>\n");
}

// kate: indent-mode cstyle; space-indent on; indent-width 4;
