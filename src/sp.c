#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>

#include "main.h"
#include "sp.h"
#include "pyMapTable.h"
#include "pyParser.h"
#include "MainWindow.h"

SP_C            SPMap_C[] = {
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

    {"\0", '\0'}
};

SP_S            SPMap_S[] = {
    {"ch", 'i'}
    ,
    {"sh", 'u'}
    ,
    {"zh", 'v'}
    ,
    {"\0", '\0'}
};

Bool            bSP_UseSemicolon = False;
Bool            bSP = False;
char            cNonS = 'o';

void LoadSPData (void)
{
    FILE           *fp;
    char            strPath[PATH_MAX];
    char            str[20], strS[5], *pstr;
    int             i;

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");

    if (access (strPath, 0))
	mkdir (strPath, S_IRWXU);

    strcat (strPath, "sp.dat");
    fp = fopen (strPath, "rt");

    if (!fp)
	return;

    while (1) {
	if (!fgets (str, 20, fp))
	    break;

	i = strlen (str) - 1;
	while (str[i] == ' ' || str[i] == '\n')
	    str[i--] = '\0';
	pstr = str;
	if (*pstr == ' ' || *pstr == '\t')
	    pstr++;
	if (!strlen (pstr) || pstr[0] == '#')
	    continue;

	if (pstr[0] == '=')	//是零声母设置
	    cNonS = tolower (pstr[1]);
	else {
	    i = 0;
	    while (pstr[i]) {
		if (pstr[i] == '=') {
		    strncpy (strS, pstr, i);
		    strS[i] = '\0';

		    pstr += i;
		    i = GetSPIndexQP_S (strS);
		    if (i != -1)
			SPMap_S[i].cJP = tolower (pstr[1]);
		    else {
			i = GetSPIndexQP_C (strS);
			if (i != -1)
			    SPMap_C[i].cJP = tolower (pstr[1]);
		    }
		    break;
		}
		i++;
	    }
	}
    }

    fclose (fp);

    //下面判断是否使用了';'
    i = 0;
    while (SPMap_C[i].strQP[0]) {
	if (SPMap_C[i++].cJP == ';')
	    bSP_UseSemicolon = True;
    }
    if (!bSP_UseSemicolon) {
	i = 0;
	while (SPMap_S[i].strQP[0]) {
	    if (SPMap_S[i++].cJP == ';')
		bSP_UseSemicolon = True;
	}
    }
    if (!bSP_UseSemicolon) {
	if (cNonS == ';')
	    bSP_UseSemicolon = True;
    }
}

/*
 * 将一个全拼转换为双拼
 * strQP只能是一个标准的全拼，本函数不检查错误
 */
/*void QP2SP (char *strQP, char *strSP)
{
    //IsConsonant
    int             iIndex;

    strSP[2] = '\0';
    //"ang"是唯一一个可单独使用、长为3的拼音，单独处理
    if (!strcmp (strQP, "ang")) {
	strSP[0] = cNonS;
	strSP[1] = SPMap_C[GetSPIndexQP_C (strQP)].cJP;
	return;
    }
    if (strlen (strQP) == 1) {
	strSP[0] = cNonS;
	strSP[1] = *strQP;
	return;
    }
    if (strlen (strQP) == 2) {
	iIndex = GetSPIndexQP_C (strQP);
	if (iIndex == -1)
	    strcpy (strSP, strQP);
	else {
	    strSP[0] = cNonS;
	    strSP[1] = SPMap_C[iIndex].cJP;
	}

	return;
    }

    iIndex = IsSyllabary (strQP, True);

    strQP += strlen (syllabaryMapTable[iIndex].strPY);
    if (*strQP) {
	if (strlen (syllabaryMapTable[iIndex].strPY) == 1)
	    strSP[0] = syllabaryMapTable[iIndex].strPY[0];
	else
	    strSP[0] = SPMap_S[GetSPIndexQP_S (syllabaryMapTable[iIndex].strPY)].cJP;
	if (strlen (strQP) == 1)
	    strSP[1] = strQP[0];
	else
	    strSP[1] = SPMap_C[GetSPIndexQP_C (strQP)].cJP;
    }
    else {
	strSP[0] = cNonS;
	strSP[1] = SPMap_C[GetSPIndexQP_C (syllabaryMapTable[iIndex].strPY)].cJP;
    }
}
*/
/*
 * 此处只转换单个双拼，并且不检查错误
 */
void SP2QP (char *strSP, char *strQP)
{
    int             iIndex1 = 0, iIndex2 = 0;
    char            strTmp[2];
    char            str_QP[MAX_PY_LENGTH + 1];

    strTmp[1] = '\0';
    strQP[0] = '\0';

    if (strSP[0] != cNonS) {
	iIndex1 = GetSPIndexJP_S (*strSP);
	if (iIndex1 == -1) {
	    strTmp[0] = strSP[0];
	    strcat (strQP, strTmp);
	}
	else
	    strcat (strQP, SPMap_S[iIndex1].strQP);
    }
    else if (!strSP[1])
	strcpy (strQP, strSP);

    if (strSP[1]) {
	iIndex2 = -1;
	while (1) {
	    iIndex2 = GetSPIndexJP_C (strSP[1], iIndex2 + 1);
	    if (iIndex2 == -1) {
		strTmp[0] = strSP[1];
		strcat (strQP, strTmp);
		break;
	    }

	    strcpy (str_QP, strQP);
	    strcat (strQP, SPMap_C[iIndex2].strQP);
	    if (FindPYFAIndex (strQP, False) != -1)
		break;

	    strcpy (strQP, str_QP);
	}
    }

    if (FindPYFAIndex (strQP, False) != -1)
	iIndex2 = 0;		//这只是将iIndex2置为非-1,以免后面的判断

    strTmp[0] = strSP[0];
    strTmp[1] = '\0';
    if ((iIndex1 == -1 && !(IsSyllabary (strTmp, 0))) || iIndex2 == -1) {
	iIndex1 = FindPYFAIndex (strSP, False);
	if (iIndex1 != -1)
	    strcpy (strQP, strSP);
    }
}

int GetSPIndexQP_S (char *str)
{
    int             i;

    i = 0;
    while (SPMap_S[i].strQP[0]) {
	if (!strcmp (str, SPMap_S[i].strQP))
	    return i;

	i++;
    }

    return -1;
}

int GetSPIndexQP_C (char *str)
{
    int             i;

    i = 0;
    while (SPMap_C[i].strQP[0]) {
	if (!strcmp (str, SPMap_C[i].strQP))
	    return i;
	i++;
    }

    return -1;
}

int GetSPIndexJP_S (char c)
{
    int             i;

    i = 0;
    while (SPMap_S[i].strQP[0]) {
	if (c == SPMap_S[i].cJP)
	    return i;

	i++;
    }

    return -1;
}

int GetSPIndexJP_C (char c, int iStart)
{
    int             i;

    i = iStart;
    while (SPMap_C[i].strQP[0]) {
	if (c == SPMap_C[i].cJP)
	    return i;
	i++;
    }

    return -1;
}

void SwitchSP (void)
{
    bSP = !bSP;
    DisplayMainWindow ();

    if (bSP)
	LoadSPData ();
}
