#include "punc.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "ime.h"
#include "tools.h"

ChnPunc        *chnPunc = NULL;

extern int      iCodeInputCount;
extern Bool     bRunLocal;

int LoadPuncDict (void)
{
    FILE           *fpDict;
    int             iRecordNo;
    char            strText[11];
    char            strPath[PATH_MAX];
    char           *pstr;
    int             i;

    if (bRunLocal) {
	strcpy (strPath, (char *) getenv ("HOME"));
	strcat (strPath, "/fcitx/");
    }
    else
	strcpy (strPath, DATA_DIR);
    strcat (strPath, PUNC_DICT_FILENAME);
    fpDict = fopen (strPath, "rt");

    if (!fpDict)
	return False;

    iRecordNo = CalculateRecordNumber (fpDict);
    chnPunc = (ChnPunc *) malloc (sizeof (ChnPunc) * (iRecordNo + 1));

    iRecordNo = 0;
    for (;;) {
	if (!fgets (strText, 10, fpDict))
	    break;
	i = strlen (strText) - 1;

	while ((strText[i] == '\n') || (strText[i] == ' ')) {
	    if (!i)
		break;
	    i--;
	}

	if (i) {
	    strText[i + 1] = '\0';
	    pstr = strText;
	    while (*pstr != ' ')
		chnPunc[iRecordNo].ASCII = *pstr++;

	    while (*pstr == ' ')
		pstr++;

	    chnPunc[iRecordNo].iCount = 0;
	    chnPunc[iRecordNo].iWhich = 0;
	    while (*pstr) {
		i = 0;
		while (*pstr != ' ' && *pstr)
		    chnPunc[iRecordNo].strChnPunc[chnPunc[iRecordNo].iCount][i++] = *pstr++;
		chnPunc[iRecordNo].strChnPunc[chnPunc[iRecordNo].iCount][i] = '\0';
		while (*pstr == ' ')
		    pstr++;
		chnPunc[iRecordNo].iCount++;
	    }
	    iRecordNo++;
	}
    }

    chnPunc[iRecordNo].ASCII = '\0';

    fclose (fpDict);
    return True;
}

/*
 * 判断字符是不是标点符号
 * 是则返回标点符号的索引，否则返回-1
 */
int IsPunc (int iKey)
{
    int             iIndex = 0;

    if ( !chnPunc )
	return -1;
    
    while (chnPunc[iIndex].ASCII) {
	if (chnPunc[iIndex].ASCII == iKey)
	    return iIndex;
	iIndex++;
    }

    return -1;
}
