#include "pyParser.h"
#include "ime.h"

#include <stdio.h>
#include <string.h>

#include "pyMapTable.h"
#include "PYFA.h"
#include "sp.h"

extern PYTABLE  PYTable[];
extern ConsonantMap consonantMapTable[];
extern SyllabaryMap syllabaryMapTable[];
extern int      iIMEIndex;
extern Bool     bSP;
extern MHPY     MHPY_C[];
extern Bool     bFullPY;

int IsSyllabary (char *strPY, Bool bMode)
{
    register int    i;

    for (i = 0; syllabaryMapTable[i].cMap; i++) {
	if (bMode) {
	    if (!strncmp (strPY, syllabaryMapTable[i].strPY, strlen (syllabaryMapTable[i].strPY)))
		return i;
	}
	else {
	    if (!strcmp (strPY, syllabaryMapTable[i].strPY))
		return i;
	}
    }

    return -1;
}

int IsConsonant (char *strPY, Bool bMode)
{
    register int    i;

    for (i = 0; consonantMapTable[i].cMap; i++) {
	if (bMode) {
	    if (!strncmp (strPY, consonantMapTable[i].strPY, strlen (consonantMapTable[i].strPY)))
		return i;
	}
	else {
	    if (!strcmp (strPY, consonantMapTable[i].strPY))
		return i;
	}
    }

    return -1;
}

int FindPYFAIndex (char *strPY, Bool bMode)
{
    int             i;
    int             iTemp;

    iTemp = -1;
    for (i = 0; PYTable[i].strPY[0] != '\0'; i++) {
	if (bMode) {
	    if (!strncmp (strPY, PYTable[i].strPY, strlen (PYTable[i].strPY))) {
		if (!PYTable[i].pMH)
		    return i;
		else if (*(PYTable[i].pMH))
		    return i;
	    }
	}
	else {
	    if (!strcmp (strPY, PYTable[i].strPY)) {
		if (!PYTable[i].pMH)
		    return i;
		else if (*(PYTable[i].pMH))
		    return i;
	    }
	}
    }

    return -1;
}

void ParsePY (char *strPY, ParsePYStruct * parsePY, PYPARSEINPUTMODE mode)
{
    char           *strP;
    int             iIndex;
    int             iTemp;
    char            str_Map[3];
    char            strTemp[7];

    parsePY->iMode = PARSE_SINGLEHZ;
    strP = strPY;
    parsePY->iHZCount = 0;

    if (bSP) {
	char            strQP[7];
	char            strJP[3];

	strJP[2] = '\0';

	while (*strP) {
	    strJP[0] = *strP++;
	    strJP[1] = *strP;
	    SP2QP (strJP, strQP);
	    MapPY (strQP, str_Map, mode);

	    if (!*strP) {
		strcpy (parsePY->strMap[parsePY->iHZCount], str_Map);
		strcpy (parsePY->strPYParsed[parsePY->iHZCount++], strJP);
		break;
	    }

	    iIndex = FindPYFAIndex (strQP, 0);
	    if (iIndex == -1) {
		strJP[1] = '\0';
		SP2QP (strJP, strQP);
		if (!MapPY (strQP, str_Map, mode))
		    strcpy (parsePY->strMap[parsePY->iHZCount], strJP);
		else
		    strcpy (parsePY->strMap[parsePY->iHZCount], str_Map);
		strcpy (parsePY->strPYParsed[parsePY->iHZCount++], strJP);
	    }
	    else {
		strcpy (parsePY->strMap[parsePY->iHZCount], str_Map);
		strcpy (parsePY->strPYParsed[parsePY->iHZCount++], strJP);
		strP++;
	    }
	}
    }
    else {
	Bool            bSeperator = False;

	do {
	    iIndex = FindPYFAIndex (strP, 1);
	    if (iIndex != -1) {
		strTemp[0] = PYTable[iIndex].strPY[strlen (PYTable[iIndex].strPY) - 1];
		iTemp = -1;
		if (strTemp[0] == 'g' || strTemp[0] == 'n') {
		    strncpy (strTemp, strP, strlen (PYTable[iIndex].strPY) - 1);
		    strTemp[strlen (PYTable[iIndex].strPY) - 1] = '\0';

		    iTemp = FindPYFAIndex (strTemp, 0);
		    if (iTemp != -1) {
			iTemp = FindPYFAIndex (strP + strlen (PYTable[iTemp].strPY), 1);
			if (iTemp != -1) {
			    if (strlen (PYTable[iTemp].strPY) == 1 || !strcmp ("ng", PYTable[iTemp].strPY))
				iTemp = -1;
			}
			if (iTemp != -1) {
			    strncpy (strTemp, strP, strlen (PYTable[iIndex].strPY) - 1);
			    strTemp[strlen (PYTable[iIndex].strPY) - 1] = '\0';
			}
		    }
		}
		if (iTemp == -1)
		    strcpy (strTemp, PYTable[iIndex].strPY);
		MapPY (strTemp, str_Map, mode);
		strcpy (parsePY->strMap[parsePY->iHZCount], str_Map);
		strP += strlen (strTemp);

		if (bSeperator) {
		    bSeperator = False;
		    parsePY->strPYParsed[parsePY->iHZCount][0] = PY_SEPERATOR;
		    parsePY->strPYParsed[parsePY->iHZCount][1] = '\0';
		}
		else
		    parsePY->strPYParsed[parsePY->iHZCount][0] = '\0';
		strcat (parsePY->strPYParsed[parsePY->iHZCount++], strTemp);
	    }
	    else {
		if (bFullPY && *strP != PY_SEPERATOR)
		    parsePY->iMode = PARSE_ERROR;

		iIndex = IsConsonant (strP, 1);
		if (-1 != iIndex) {
		    parsePY->iMode = PARSE_ERROR;

		    if (bSeperator) {
			bSeperator = False;
			parsePY->strPYParsed[parsePY->iHZCount][0] = PY_SEPERATOR;
			parsePY->strPYParsed[parsePY->iHZCount][1] = '\0';
		    }
		    else
			parsePY->strPYParsed[parsePY->iHZCount][0] = '\0';
		    strcat (parsePY->strPYParsed[parsePY->iHZCount], consonantMapTable[iIndex].strPY);
		    MapPY (consonantMapTable[iIndex].strPY, str_Map, mode);
		    strcpy (parsePY->strMap[parsePY->iHZCount++], str_Map);
		    strP += strlen (consonantMapTable[iIndex].strPY);
		}
		else {
		    iIndex = IsSyllabary (strP, 1);
		    if (-1 != iIndex) {
			if (bSeperator) {
			    bSeperator = False;
			    parsePY->strPYParsed[parsePY->iHZCount][0] = PY_SEPERATOR;
			    parsePY->strPYParsed[parsePY->iHZCount][1] = '\0';
			}
			else
			    parsePY->strPYParsed[parsePY->iHZCount][0] = '\0';
			strcat (parsePY->strPYParsed[parsePY->iHZCount], syllabaryMapTable[iIndex].strPY);
			MapPY (syllabaryMapTable[iIndex].strPY, str_Map, mode);
			strcpy (parsePY->strMap[parsePY->iHZCount++], str_Map);

			strP += strlen (syllabaryMapTable[iIndex].strPY);
			if (parsePY->iMode != PARSE_ERROR)
			    parsePY->iMode = PARSE_ABBR;
		    }
		    else {	//必定是分隔符
			strP++;
			bSeperator = True;
			parsePY->strPYParsed[parsePY->iHZCount][0] = PY_SEPERATOR;
			parsePY->strPYParsed[parsePY->iHZCount][1] = '\0';
			parsePY->strMap[parsePY->iHZCount][0] = '0';
			parsePY->strMap[parsePY->iHZCount][1] = '0';
			parsePY->strMap[parsePY->iHZCount][2] = '\0';
		    }
		}
	    }
	} while (*strP);
    }

    if (strPY[strlen (strPY) - 1] == PY_SEPERATOR)
	parsePY->iHZCount++;

    if (parsePY->iMode != PARSE_ERROR) {
	parsePY->iMode = parsePY->iMode & PARSE_ABBR;
	if (parsePY->iHZCount > 1)
	    parsePY->iMode = parsePY->iMode | PARSE_PHRASE;
	else
	    parsePY->iMode = parsePY->iMode | PARSE_SINGLEHZ;
    }
}

/*
 * 将一个拼音(包括仅为声母或韵母)转换为拼音映射
 * 返回True为转换成功，否则为False(一般是因为strPY不是一个标准的拼音)
 */
Bool MapPY (char *strPY, char strMap[3], PYPARSEINPUTMODE mode)
{
    char            str[5];
    int             iIndex;

    //特殊处理eng
    if (!strcmp (strPY, "eng") && MHPY_C[1].bMode) {
	strcpy (strMap, "X0");
	return True;
    }

    strMap[2] = '\0';
    iIndex = IsSyllabary (strPY, 0);
    if (-1 != iIndex) {
	strMap[0] = syllabaryMapTable[iIndex].cMap;
	strMap[1] = mode;
	return True;
    }
    iIndex = IsConsonant (strPY, 0);

    if (-1 != iIndex) {
	strMap[0] = mode;
	strMap[1] = consonantMapTable[iIndex].cMap;
	return True;
    }

    str[0] = strPY[0];
    str[1] = '\0';

    if (strPY[1] == 'h' || strPY[1] == 'g') {
	str[0] = strPY[0];
	str[1] = strPY[1];
	str[2] = '\0';
	iIndex = IsSyllabary (str, 0);
	strMap[0] = consonantMapTable[iIndex].cMap;
	iIndex = IsConsonant (strPY + 2, 0);
	strMap[1] = consonantMapTable[iIndex].cMap;
    }
    else {
	str[0] = strPY[0];
	str[1] = '\0';
	iIndex = IsSyllabary (str, 0);
	if (iIndex == -1)
	    return False;
	strMap[0] = consonantMapTable[iIndex].cMap;
	iIndex = IsConsonant (strPY + 1, 0);
	if (iIndex == -1)
	    return False;
	strMap[1] = consonantMapTable[iIndex].cMap;
    }

    return True;
}

/*
 * 将一个完整的拼音映射转换为拼音，返回False表示失败，
 * 一般原因是拼音映射不正确
 */
/*Bool MapToPY (char strMap[3], char *strPY)
{
    int             i;

    strPY[0] = '\0';
    if (strMap[0] != '0') {
	i = 0;
	while (syllabaryMapTable[i].cMap) {
	    if (syllabaryMapTable[i].cMap == strMap[0]) {
		strcpy (strPY, syllabaryMapTable[i].strPY);
		break;
	    }
	    i++;
	}
	if (!strlen (strPY))
	    return False;
    }

    if (strMap[1] != '0') {
	i = 0;
	while (consonantMapTable[i].cMap) {
	    if (consonantMapTable[i].cMap == strMap[1]) {
		strcat (strPY, consonantMapTable[i].strPY);
		return True;
	    }
	    i++;
	}
    }

    return False;
}*/

/*
 * 比较一位拼音映射
 * 0表示相等
 * b指示是声母还是韵母，True表示声母
 */
int Cmp1Map (char map1, char map2, Bool b)
{
    int             iVal1, iVal2;

    if (map2 == '0' || map1 == '0') {
	if (map1 == ' ' || map2 == ' ' || !bFullPY || bSP)
	    return 0;
    }
    else {
	if (b) {
	    iVal1 = GetMHIndex_S (map1);
	    iVal2 = GetMHIndex_S (map2);
	}
	else {
	    iVal1 = GetMHIndex_C (map1);
	    iVal2 = GetMHIndex_C (map2);
	}
	if (iVal1 == iVal2)
	    if (iVal1 >= 0)
		return 0;
    }

    return (map1 - map2);
}

/*
 * 比较两位拼音映射
 * 0表示相等
 * >0表示前者大
 * <0表示后者大
 */
int Cmp2Map (char map1[3], char map2[3])
{
    int             i;

    i = Cmp1Map (map1[0], map2[0], True);
    if (i)
	return i;

    return Cmp1Map (map1[1], map2[1], False);
}

/*
 * 判断strMap2是否与strMap1相匹配
 * 是 返回值为0
 * 否 返回值不为0
 * *iMatchedLength 记录了二者能够匹配的长度
 */
int CmpMap (char *strMap1, char *strMap2, int *iMatchedLength)
{
    int             val;

    *iMatchedLength = 0;
    for (;;) {
	if (!strMap2[*iMatchedLength])
	    return (strMap1[*iMatchedLength] - strMap2[*iMatchedLength]);
	val = Cmp1Map (strMap1[*iMatchedLength], strMap2[*iMatchedLength], (*iMatchedLength + 1) % 2);
	if (val)
	    return Cmp1Map (strMap1[*iMatchedLength], strMap2[*iMatchedLength], (*iMatchedLength + 1) % 2);
	(*iMatchedLength)++;
    }

    return 0;
}
