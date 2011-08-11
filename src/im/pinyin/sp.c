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
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>

#include "fcitx/fcitx.h"

#include "py.h"
#include "sp.h"
#include "pyMapTable.h"
#include "pyParser.h"
#include "fcitx-config/xdg.h"
#include "pyconfig.h"
#include <string.h>

const SP_C            SPMap_C_Template[] =
{
    {"ai", 'l'}
    ,
    {"an", 'j'}
    ,
    {"ang", 'h'}
    ,
    {"ao", 'k'}
    ,
    {"ei", 'z'}
    ,
    {"en", 'f'}
    ,
    {"eng", 'g'}
    ,
    {"er", 'r'}
    ,
    {"ia", 'w'}
    ,
    {"ian", 'm'}
    ,
    {"iang", 'd'}
    ,
    {"iao", 'c'}
    ,
    {"ie", 'x'}
    ,
    {"in", 'n'}
    ,
    {"ing", 'y'}
    ,
    {"iong", 's'}
    ,
    {"iu", 'q'}
    ,
    {"ng", 'g'}
    ,
    {"ong", 's'}
    ,
    {"ou", 'b'}
    ,
    {"ua", 'w'}
    ,
    {"uai", 'y'}
    ,
    {"uan", 'r'}
    ,
    {"uang", 'd'}
    ,
    {"ue", 't'}
    ,
    {"ui", 'v'}
    ,
    {"un", 'p'}
    ,
    {"uo", 'o'}
    ,
    {"ve", 't'}
    ,
    {"v", 'v'}
    ,
    {"\0", '\0'}
};

const SP_S SPMap_S_Template[] =
{
    {"ch", 'i'}
    ,
    {"sh", 'u'}
    ,
    {"zh", 'v'}
    ,
    {"\0", '\0'}
};

#define STR_SPCONF_NAME 0

#define cstr(b) (strConstSPConf[STR_SPCONF_##b])
#define cstrlen(b) (strlen(cstr(b)))

char* strConstSPConf[] =
{
    "方案名称="
};

boolean SPInit(void *arg)
{
    FcitxPinyinState *pystate = (FcitxPinyinState*)arg;
    pystate->bSP = true;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;
    memcpy(pyconfig->SPMap_S, SPMap_S_Template, sizeof(SPMap_S_Template));
    memcpy(pyconfig->SPMap_C, SPMap_C_Template, sizeof(SPMap_C_Template));

    LoadSPData(pystate);
    return true;
}

void LoadSPData(FcitxPinyinState *pystate)
{
    FILE           *fp;
    char            str[100], strS[5], *pstr;
    int             i;
    boolean            bIsDefault = false;
    FcitxPinyinConfig* pyconfig = &pystate->pyconfig;
    SP_C* SPMap_C = pyconfig->SPMap_C;
    SP_S* SPMap_S = pyconfig->SPMap_S;

    /* reset work around */
    i = 0;

    while (SPMap_C[i].strQP[0])
    {
        if (strlen(SPMap_C[i].strQP) == 1)
            SPMap_C[i].cJP = SPMap_C[i].strQP[0];

        i ++ ;
    }

    fp = GetXDGFileWithPrefix("pinyin", "sp.dat", "rt", NULL);

    while (1)
    {
        if (!fgets(str, 100, fp))
            break;

        i = strlen(str) - 1;

        while ((i >= 0) && (str[i] == ' ' || str[i] == '\n'))
            str[i--] = '\0';

        pstr = str;

        if (*pstr == ' ' || *pstr == '\t')
            pstr++;

        if (!strlen(pstr) || pstr[0] == '#')
            continue;

        if (!strncmp(pstr, cstr(NAME), cstrlen(NAME)))
        {
            pstr += cstrlen(NAME);

            if (*pstr == ' ' || *pstr == '\t')
                pstr++;

            bIsDefault = !(strcmp(pyconfig->strDefaultSP, pstr));

            continue;
        }

        if (!bIsDefault)
            continue;

        if (pstr[0] == '=') //是零声母设置
            pyconfig->cNonS = tolower(pstr[1]);
        else
        {
            i = 0;

            while (pstr[i])
            {
                if (pstr[i] == '=')
                {
                    strncpy(strS, pstr, i);
                    strS[i] = '\0';

                    pstr += i;
                    i = GetSPIndexQP_S(pyconfig, strS);

                    if (i != -1)
                        SPMap_S[i].cJP = tolower(pstr[1]);
                    else
                    {
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

    //下面判断是否使用了';'
    i = 0;

    while (SPMap_C[i].strQP[0])
    {
        if (SPMap_C[i++].cJP == ';')
            pystate->bSP_UseSemicolon = true;
    }

    if (!pystate->bSP_UseSemicolon)
    {
        i = 0;

        while (SPMap_S[i].strQP[0])
        {
            if (SPMap_S[i++].cJP == ';')
                pystate->bSP_UseSemicolon = true;
        }
    }

    if (!pystate->bSP_UseSemicolon)
    {
        if (pyconfig->cNonS == ';')
            pystate->bSP_UseSemicolon = true;
    }
}

/*
 * 此处只转换单个双拼，并且不检查错误
 */
void SP2QP(FcitxPinyinConfig* pyconfig, char *strSP, char *strQP)
{
    int             iIndex1 = 0, iIndex2 = 0;
    char            strTmp[2];
    char            str_QP[MAX_PY_LENGTH + 1];
    SP_C* SPMap_C = pyconfig->SPMap_C;
    SP_S* SPMap_S = pyconfig->SPMap_S;

    strTmp[1] = '\0';
    strQP[0] = '\0';

    if (strSP[0] != pyconfig->cNonS)
    {
        iIndex1 = GetSPIndexJP_S(pyconfig, *strSP);

        if (iIndex1 == -1)
        {
            strTmp[0] = strSP[0];
            strcat(strQP, strTmp);
        }
        else
            strcat(strQP, SPMap_S[iIndex1].strQP);
    }
    else
        if (!strSP[1])
            strcpy(strQP, strSP);

    if (strSP[1])
    {
        iIndex2 = -1;

        while (1)
        {
            iIndex2 = GetSPIndexJP_C(pyconfig, strSP[1], iIndex2 + 1);

            if (iIndex2 == -1)
            {
                strTmp[0] = strSP[1];
                strcat(strQP, strTmp);
                break;
            }

            strcpy(str_QP, strQP);

            strcat(strQP, SPMap_C[iIndex2].strQP);

            if (FindPYFAIndex(pyconfig, strQP, false) != -1)
                break;

            strcpy(strQP, str_QP);
        }
    }

    if (FindPYFAIndex(pyconfig, strQP, false) != -1)
        iIndex2 = 0;        //这只是将iIndex2置为非-1,以免后面的判断

    strTmp[0] = strSP[0];

    strTmp[1] = '\0';

    if ((iIndex1 == -1 && !(IsSyllabary(strTmp, 0))) || iIndex2 == -1)
    {
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

    while (SPMap_S[i].strQP[0])
    {
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

    while (SPMap_C[i].strQP[0])
    {
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

    while (SPMap_S[i].strQP[0])
    {
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

    while (SPMap_C[i].strQP[0])
    {
        if (c == SPMap_C[i].cJP)
            return i;

        i++;
    }

    return -1;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0; 
