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
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

#include "fcitx/fcitx.h"
#include "fcitx/context.h"
#include "fcitx-config/xdg.h"

#include "py.h"
#include "sp.h"
#include "spdata.h"
#include "pyMapTable.h"
#include "pyParser.h"
#include "pyconfig.h"

#define STR_SPCONF_NAME 0

#define cstr(b) (strConstSPConf[STR_SPCONF_##b])
#define cstrlen(b) (strlen(cstr(b)))

char* strConstSPConf[] = {
    "方案名称="
};

void LoadSPData(FcitxPinyinState *pystate)
{
    FILE           *fp;
    char            str[100], strS[5], *pstr;
    int             i;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;
    boolean            bIsDefault = false;
    SP_C* SPMap_C = pyconfig->SPMap_C;
    SP_S* SPMap_S = pyconfig->SPMap_S;
    char            nonS = 'o';

    const SP_C* SPMap_C_source = NULL;
    const SP_S* SPMap_S_source = NULL;
    switch (pyconfig->spscheme) {
    case SP_ZIRANMA:
        SPMap_C_source = SPMap_C_Ziranma;
        SPMap_S_source = SPMap_S_Ziranma;
        break;
    case SP_MS:
        SPMap_C_source = SPMap_C_MS;
        SPMap_S_source = SPMap_S_MS;
        break;
    case SP_ZIGUANG:
        SPMap_C_source = SPMap_C_Ziguang;
        SPMap_S_source = SPMap_S_Ziguang;
        break;
    case SP_PINYINJIAJIA:
        SPMap_C_source = SPMap_C_PinyinJiaJia;
        SPMap_S_source = SPMap_S_PinyinJiaJia;
        break;
    case SP_ZHONGWENZHIXING:
        SPMap_C_source = SPMap_C_Zhongwenzhixing;
        SPMap_S_source = SPMap_S_Zhongwenzhixing;
        break;
    case SP_ABC:
        SPMap_C_source = SPMap_C_ABC;
        SPMap_S_source = SPMap_S_ABC;
        break;
    case SP_XIAOHE:
        nonS = '*';
        SPMap_C_source = SPMap_C_XIAOHE;
        SPMap_S_source = SPMap_S_XIAOHE;
        break;
    default: {
        /* reset work around */
        i = 0;

        while (SPMap_C[i].strQP[0]) {
            if (!SPMap_C[i].strQP[1])
                SPMap_C[i].cJP = SPMap_C[i].strQP[0];
            i ++ ;
        }

        fp = FcitxXDGGetFileWithPrefix("pinyin", "sp.dat", "r", NULL);

        while (1) {
            if (!fgets(str, 100, fp))
                break;

            i = strlen(str) - 1;

            while ((i >= 0) && (str[i] == ' ' || str[i] == '\n'))
                str[i--] = '\0';

            pstr = str;

            if (*pstr == ' ' || *pstr == '\t')
                pstr++;

            if (*pstr == '\0' || *pstr == '#')
                continue;

            if (!strncmp(pstr, cstr(NAME), cstrlen(NAME))) {
                pstr += cstrlen(NAME);

                if (*pstr == ' ' || *pstr == '\t')
                    pstr++;

                if (strcmp(pstr, "自然码") != 0
                        && strcmp(pstr, "微软") != 0
                        && strcmp(pstr, "紫光") != 0
                        && strcmp(pstr, "拼音加加") != 0
                        && strcmp(pstr, "中文之星") != 0
                        && strcmp(pstr, "智能ABC") != 0
                        && strcmp(pstr, "小鹤") != 0) {
                    bIsDefault = true;
                }

                continue;
            }
            if (!bIsDefault)
                continue;

            if (pstr[0] == '=') //是零声母设置
                pyconfig->cNonS = tolower(pstr[1]);
            else {
                i = 0;

                while (pstr[i]) {
                    if (pstr[i] == '=') {
                        strncpy(strS, pstr, i);
                        strS[i] = '\0';

                        pstr += i;
                        i = GetSPIndexQP_S(pyconfig, strS);

                        if (i != -1)
                            SPMap_S[i].cJP = tolower(pstr[1]);
                        else {
                            i = GetSPIndexQP_C(pyconfig, strS);

                            if (i != -1)
                                SPMap_C[i].cJP = tolower(pstr[1]);
                        }

                        break;
                    }

                    i++;
                }
            }
        }

        fclose(fp);
    }
    break;
    }

    if (SPMap_C_source && SPMap_S_source) {
        pyconfig->cNonS = nonS;
        memcpy(pyconfig->SPMap_S, SPMap_S_source, 4 * sizeof(SP_S));
        memcpy(pyconfig->SPMap_C, SPMap_C_source, 31 * sizeof(SP_C));
    }

    //下面判断是否使用了';'
    i = 0;

    while (SPMap_C[i].strQP[0]) {
        if (SPMap_C[i++].cJP == ';')
            pystate->bSP_UseSemicolon = true;
    }

    if (!pystate->bSP_UseSemicolon) {
        i = 0;

        while (SPMap_S[i].strQP[0]) {
            if (SPMap_S[i++].cJP == ';')
                pystate->bSP_UseSemicolon = true;
        }
    }

    if (!pystate->bSP_UseSemicolon) {
        if (pyconfig->cNonS == ';')
            pystate->bSP_UseSemicolon = true;
    }
}

/*
 * 此处只转换单个双拼，并且不检查错误
 */
void SP2QP(FcitxPinyinConfig* pyconfig, const char *strSP, char *strQP)
{
    int             iIndex1 = 0, iIndex2 = 0;
    char            strTmp[2];
    char            str_QP[MAX_PY_LENGTH + 1];
    SP_C* SPMap_C = pyconfig->SPMap_C;
    SP_S* SPMap_S = pyconfig->SPMap_S;

    strTmp[1] = '\0';
    const char aeiou[] = "aeiou";

    /*
     * this part of code is to check xiaohe special rule
     * First, Xiaohe's C rule it like this.
     * if C length is 1, nonS is same as C, for example, aa is a
     * if C length is 2, type it as the same of QuanPin, for example, an is an
     * no nonS case for C length is 3.
     */
    boolean checkXiaoheNonS = false;
    do {
        if (pyconfig->cNonS != '*')
            break;
        if (!strchr(aeiou, strSP[0]))
            break;
        if (!strSP[1])
            break;

        if (strchr(aeiou, strSP[1])) {
             if (strSP[0] == strSP[1])
                 checkXiaoheNonS = true;
        } else {
            int idx = -1;
            while (1) {
                idx = GetSPIndexJP_C(pyconfig, strSP[1], idx + 1);
                if (idx == -1)
                    break;
                if (SPMap_C[idx].strQP[0] == strSP[0]) {
                    checkXiaoheNonS = true;
                    break;
                }
            }
        }
    } while(0);

recheck_sp:
    strQP[0] = '\0';
    if (strSP[0] != pyconfig->cNonS
        && !checkXiaoheNonS) {
        iIndex1 = GetSPIndexJP_S(pyconfig, *strSP);

        if (iIndex1 == -1) {
            strTmp[0] = strSP[0];
            strcat(strQP, strTmp);
        } else
            strcat(strQP, SPMap_S[iIndex1].strQP);
    } else if (!strSP[1])
        strcpy(strQP, strSP);

    if (strSP[1]) {
        iIndex2 = -1;

        while (1) {
            iIndex2 = GetSPIndexJP_C(pyconfig, strSP[1], iIndex2 + 1);

            if (iIndex2 == -1) {
                strTmp[0] = strSP[1];
                strcat(strQP, strTmp);
                break;
            }

            if (checkXiaoheNonS && SPMap_C[iIndex2].strQP[0] != strSP[0])
                continue;

            strcpy(str_QP, strQP);

            strcat(strQP, SPMap_C[iIndex2].strQP);

            if (FindPYFAIndex(pyconfig, strQP, false) != -1)
                break;

            strcpy(strQP, str_QP);
        }
    }

    if (FindPYFAIndex(pyconfig, strQP, false) != -1) {
        iIndex2 = 0;        //这只是将iIndex2置为非-1,以免后面的判断
    }
    else {
        if (checkXiaoheNonS) {
            checkXiaoheNonS = false;
            goto recheck_sp;
        }
    }

    strTmp[0] = strSP[0];

    strTmp[1] = '\0';

    if ((iIndex1 == -1 && !(IsSyllabary(strTmp, 0))) || iIndex2 == -1) {
        iIndex1 = FindPYFAIndex(pyconfig, strSP, false);

        if (iIndex1 != -1)
            strcpy(strQP, strSP);
    }
}

int GetSPIndexQP_S(FcitxPinyinConfig* pyconfig, char *str)
{
    int             i;
    SP_S* SPMap_S = pyconfig->SPMap_S;

    i = 0;

    while (SPMap_S[i].strQP[0]) {
        if (!strcmp(str, SPMap_S[i].strQP))
            return i;

        i++;
    }

    return -1;
}

int GetSPIndexQP_C(FcitxPinyinConfig* pyconfig, char *str)
{
    int             i;
    SP_C* SPMap_C = pyconfig->SPMap_C;

    i = 0;

    while (SPMap_C[i].strQP[0]) {
        if (!strcmp(str, SPMap_C[i].strQP))
            return i;

        i++;
    }

    return -1;
}

int GetSPIndexJP_S(FcitxPinyinConfig* pyconfig, char c)
{
    int             i;
    SP_S* SPMap_S = pyconfig->SPMap_S;

    i = 0;

    while (SPMap_S[i].strQP[0]) {
        if (c == SPMap_S[i].cJP)
            return i;

        i++;
    }

    return -1;
}

int GetSPIndexJP_C(FcitxPinyinConfig* pyconfig, char c, int iStart)
{
    int             i;
    SP_C* SPMap_C = pyconfig->SPMap_C;

    i = iStart;

    while (SPMap_C[i].strQP[0]) {
        if (c == SPMap_C[i].cJP)
            return i;

        i++;
    }

    return -1;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
