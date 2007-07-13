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
#include "punc.h"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "ime.h"
#include "tools.h"

ChnPunc        *chnPunc = (ChnPunc *)NULL;

extern int      iCodeInputCount;

int LoadPuncDict (void)
{
    FILE           *fpDict;
    int             iRecordNo;
    char            strText[11];
    char            strPath[PATH_MAX];
    char           *pstr;
    int             i;

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");
    strcat (strPath, PUNC_DICT_FILENAME);

    if (access (strPath, 0)) {
	strcpy (strPath, PKGDATADIR "/data/");
	strcat (strPath, PUNC_DICT_FILENAME);
    }
    
    fpDict = fopen (strPath, "rt");

    if (!fpDict) {	
	printf ("Can't open Chinese punc file: %s\n", strPath);
	return False;
    }

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
	    while (*pstr == ' ')
		pstr++;
	    chnPunc[iRecordNo].ASCII = *pstr++;
	    while (*pstr == ' ')
		pstr++;

	    chnPunc[iRecordNo].iCount = 0;
	    chnPunc[iRecordNo].iWhich = 0;
	    while (*pstr) {
		i = 0;
		while (*pstr != ' ' && *pstr) {
		    chnPunc[iRecordNo].strChnPunc[chnPunc[iRecordNo].iCount][i] = *pstr;
		    i++;
		    pstr++;
		}

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

void		FreePunc (void)
{
    if (!chnPunc )
	return;
    
    free(chnPunc);    
    chnPunc = (ChnPunc *)NULL;
}

/*
 * 根据字符得到相应的标点符号
 * 如果该字符不在标点符号集中，则返回NULL
 */
char *GetPunc (int iKey)
{
    int          iIndex = 0;
    char	*pPunc;

    if (!chnPunc)
	    return (char*)NULL;
    
    while (chnPunc[iIndex].ASCII) {
    	if (chnPunc[iIndex].ASCII == iKey) {
		pPunc=chnPunc[iIndex].strChnPunc[chnPunc[iIndex].iWhich];
		chnPunc[iIndex].iWhich++;
		if (chnPunc[iIndex].iWhich >= chnPunc[iIndex].iCount)
			chnPunc[iIndex].iWhich = 0;
	    	return pPunc;
	}
	iIndex++;
    }

    return (char*)NULL;
}
