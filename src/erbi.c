#include "erbi.h"

#include <stdio.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "tools.h"
#include "InputWindow.h"
#include "SetLocale.h"
#include "py.h"
#include "pyParser.h"

EBRECORD       *erbiDictCurrent = NULL, *erbiDictHead = NULL;
EBRECORD       *EBCandWord[MAX_CAND_WORD];
EBRECORD       *EBLegend = NULL;
char            strEBLegendSource[EB_PHRASE_MAX_LENGTH * 2 + 1] = "";

EBFH           *ebfh;
int             iEBFH = 0;

Bool            bIsEBDelPhrase = False;
HOTKEYS         hkEBDelPhrase[HOT_KEY_COUNT] = { CTRL_7, 0 };
Bool            bIsEBAdjustOrder = False;
HOTKEYS         hkEBAdjustOrder[HOT_KEY_COUNT] = { CTRL_6, 0 };
Bool            bIsEBAddPhrase = False;
HOTKEYS         hkEBAddPhrase[HOT_KEY_COUNT] = { CTRL_8, 0 };

BYTE            iEBChanged = 0;
BYTE            iEBNewPhraseHZCount;
Bool            bCanntFindErBi;	//记录新组成的词能否生成五笔编码--一般情况下都是可以的
char            strNewPhraseEBCode[EB_CODE_LENGTH + 1];

Bool            bEBAutoAdjustOrder = False;
Bool            bEBAutoSendToClient = True;	//4键自动上屏
extern Bool     bUseZPY;	//用Z键输入拼音
extern Bool     bWBUseZ;	//是否用Z来模糊匹配
Bool            bEBExactMatch = False;

Bool            bEBDictLoaded = False;	//需要用的时候再读入五笔码表
Bool            bPromptEBCode = True;	//输入完毕后是否提示五笔编码--用Z输入拼音时总有提示

extern Display *dpy;
extern Window   inputWindow;

extern SINGLE_HZ legendCandWords[];
extern char     strCodeInput[];
extern Bool     bRunLocal;
extern Bool     bIsDoInputOnly;
extern int      iCandPageCount;
extern int      iCurrentCandPage;
extern int      iCandWordCount;
extern int      iLegendCandWordCount;
extern int      iLegendCandPageCount;
extern int      iCurrentLegendCandPage;
extern int      iCodeInputCount;
extern int      iMaxCandWord;
extern char     strStringGet[];
extern MESSAGE  messageUp[];
extern uint     uMessageUp;
extern MESSAGE  messageDown[];
extern uint     uMessageDown;

extern SINGLE_HZ hzLastInput[];
extern BYTE     iHZLastInputCount;

extern Bool     bUseLegend;
extern Bool     bIsInLegend;
extern INT8     lastIsSingleHZ;
extern Bool     bDisablePagingInLegend;

//----------------------------------------
extern PYFA    *PYFAList;
extern PYCandWord PYCandWords[];
extern Bool     bSingleHZMode;
extern char     strFindString[];
extern ParsePYStruct findMap;

//----------------------------------------

Bool LoadEBDict (void)
{
    char            strCode[EB_CODE_LENGTH + 1];
    char            strHZ[EB_PHRASE_MAX_LENGTH * 2 + 1];
    FILE           *fpDict;
    EBRECORD       *recTemp;
    char            strPath[PATH_MAX];

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");
    strcat (strPath, ERBI_DICT_FILENAME);

    //读入五笔码表
    if (access (strPath, 0)) {
	if (bRunLocal) {
	    strcpy (strPath, (char *) getenv ("HOME"));
	    strcat (strPath, "/fcitx/");
	}
	else
	    strcpy (strPath, DATA_DIR);
	strcat (strPath, ERBI_DICT_FILENAME);
	fpDict = fopen (strPath, "rt");
    }
    else
	fpDict = fopen (strPath, "rt");
    if (!fpDict)
	return False;

    erbiDictHead = (EBRECORD *) malloc (sizeof (EBRECORD));
    erbiDictCurrent = erbiDictHead;

    for (;;) {
	if (EOF == fscanf (fpDict, "%s %s\n", strCode, strHZ))
	    break;
	recTemp = (EBRECORD *) malloc (sizeof (EBRECORD));
	erbiDictCurrent->next = recTemp;
	recTemp->prev = erbiDictCurrent;
	erbiDictCurrent = recTemp;
	strcpy (recTemp->strCode, strCode);
	/****************************************/
	recTemp->strHZ = (char *) malloc (sizeof (char) * strlen (strHZ) + 1);
	/****************************************/
	strcpy (recTemp->strHZ, strHZ);
    }
    erbiDictCurrent->next = erbiDictHead;
    erbiDictHead->prev = erbiDictCurrent;

    fclose (fpDict);

    //读取五笔特殊符号表
    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");
    strcat (strPath, ERBI_FH_FILENAME);

    if (access (strPath, 0)) {
	if (bRunLocal) {
	    strcpy (strPath, (char *) getenv ("HOME"));
	    strcat (strPath, "/fcitx/");
	}
	else
	    strcpy (strPath, DATA_DIR);
	strcat (strPath, ERBI_FH_FILENAME);
	fpDict = fopen (strPath, "rt");
    }
 
    fpDict = fopen (strPath, "rt");
    if (fpDict) {
	iEBFH = CalculateRecordNumber (fpDict);
	ebfh = (EBFH *) malloc (sizeof (EBFH) * iEBFH);

	int             i = 0;

	for (i = 0; i < iEBFH; i++) {
	    if (EOF == fscanf (fpDict, "%s\n", ebfh[i].strEBFH))
		break;
	}
	iEBFH = i;
	
	fclose (fpDict);
    }

    strNewPhraseEBCode[4] = '\0';
    bEBDictLoaded = True;

    return True;
}

void ResetEBStatus (void)
{
    bIsEBAddPhrase = False;
    bIsEBDelPhrase = False;
    bIsEBAdjustOrder = False;
    bIsDoInputOnly = False;
}

void SaveErbiDict (void)
{
    EBRECORD       *recTemp;
    char            strPath[PATH_MAX];
    FILE           *fp;

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/erbi.mb");

    fp = fopen (strPath, "wt");
    if (!fp) {
	fprintf (stderr, "Cannot create ErBi table file\n");
	return;
    }

    recTemp = erbiDictHead->next;

    while (recTemp != erbiDictHead) {
	fprintf (fp, "%s %s\n", recTemp->strCode, recTemp->strHZ);
	recTemp = recTemp->next;
    }

    fclose (fp);
}

INPUT_RETURN_VALUE DoEBInput (int iKey)
{
    INPUT_RETURN_VALUE retVal;

    if (!bEBDictLoaded)
	LoadEBDict ();

    retVal = IRV_DO_NOTHING;
    if ((iKey >= 'a' && iKey <= 'z') || iKey == ',' || iKey == '.' || iKey == '/' || iKey == ';' || iKey == '`' || (iKey == '[' && iCurrentCandPage==0 && iCodeInputCount == 0)) {
	bIsInLegend = False;
//added 2 new lines for "["
	if (iKey =='[')
		iKey = '`';
	 
	  
	if (!bIsEBAddPhrase && !bIsEBDelPhrase && !bIsEBAdjustOrder) {
	    if (strCodeInput[0] == '`') {
		if (iCodeInputCount != (MAX_PY_LENGTH + 1)) {
		    strCodeInput[iCodeInputCount++] = iKey;
		    strCodeInput[iCodeInputCount] = '\0';
		    retVal = EBGetCandWords (SM_FIRST);
		}
		else
		    retVal = IRV_DO_NOTHING;
	    }
//added 3 new lines for , .
		else if ((iKey == ',' || iKey == '.') && (( iCodeInputCount == 0 )||( iCodeInputCount == 4 ))){
			retVal = IRV_TO_PROCESS;
		}
	    else {
		if (iCodeInputCount < EB_CODE_LENGTH) {
			
		    strCodeInput[iCodeInputCount++] = iKey;
		    strCodeInput[iCodeInputCount] = '\0';

		    if (iCodeInputCount == 1 && strCodeInput[0] == '`') {
			iCandWordCount = 0;
			retVal = IRV_DISPLAY_CANDWORDS;
			}
		    else {
			retVal = EBGetCandWords (SM_FIRST);
//added new lines for i second 简码
			if (iCodeInputCount == 2 && strCodeInput[0] == 'i'){
				strcpy (strStringGet, EBGetCandWord (0));
				iCandWordCount = 0;
				retVal = IRV_GET_CANDWORDS;
			}
			
//added new lines for / second 简码
			if (iCodeInputCount == 2 && strCodeInput[0] == '/'){
				strcpy (strStringGet, EBGetCandWord (0));
				iCandWordCount = 0;
				retVal = IRV_GET_CANDWORDS;
			}
			
			if (bEBAutoSendToClient && (iCodeInputCount == EB_CODE_LENGTH)) {
			    if (iCandWordCount == 1) {	//如果只有一个候选词，则送到客户程序中
				strcpy (strStringGet, EBGetCandWord (0));
				iCandWordCount = 0;
				retVal = IRV_GET_CANDWORDS;
			    }
			}
		    }
		}
		else {
		    if (bEBAutoSendToClient) {
			if (iCandWordCount) {
			    strcpy (strStringGet, EBCandWord[0]->strHZ);
			    retVal = IRV_GET_CANDWORDS_NEXT;
			}
			else
			    retVal = IRV_DISPLAY_CANDWORDS;

			iCodeInputCount = 1;
			strCodeInput[0] = iKey;
			strCodeInput[1] = '\0';

			EBGetCandWords (SM_FIRST);
		    }
		    else
			retVal = IRV_DO_NOTHING;
		}
	    }
	}
    }
    else {
	if (bIsEBAddPhrase) {
	    switch (iKey) {
	    case LEFT:
		if (iEBNewPhraseHZCount < iHZLastInputCount) {
		    iEBNewPhraseHZCount++;
		    EBCreateNewPhrase ();
		}
		break;
	    case RIGHT:
		if (iEBNewPhraseHZCount > 2) {
		    iEBNewPhraseHZCount--;
		    EBCreateNewPhrase ();
		}
		break;
	    case ENTER:
		if (!bCanntFindErBi)
		    InsertEBPhrase (messageDown[1].strMsg, messageDown[0].strMsg);
	    case ESC:
		bIsEBAddPhrase = False;
		bIsDoInputOnly = False;
		return IRV_CLEAN;
	    default:
		return IRV_DO_NOTHING;
	    }

	    return IRV_DISPLAY_MESSAGE;
	}
	if (IsHotKey (iKey, hkEBAddPhrase)) {
	    if (!bIsEBAddPhrase) {
		if (iHZLastInputCount < 2)	//词组最少为两个汉字
		    return IRV_DO_NOTHING;

		iEBNewPhraseHZCount = 2;
		bIsEBAddPhrase = True;
		bIsDoInputOnly = True;

		uMessageUp = 1;
		strcpy (messageUp[0].strMsg, "左/右键增加/减少，ENTER确定，ESC取消");
		messageUp[0].type = MSG_TIPS;

		uMessageDown = 2;
		messageDown[0].type = MSG_FIRSTCAND;
		messageDown[1].type = MSG_CODE;

		EBCreateNewPhrase ();
		DisplayInputWindow ();

		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else
		retVal = IRV_TO_PROCESS;

	    return retVal;
	}

	if (!iCodeInputCount && !bIsInLegend)
	    return IRV_TO_PROCESS;

	if (iKey == ESC) {
	    if (bIsEBDelPhrase || bIsEBAdjustOrder) {
		ResetEBStatus ();
		retVal = IRV_DISPLAY_CANDWORDS;
	    }
	    else
		return IRV_CLEAN;
	}
	else if (iKey >= '0' && iKey <= '9') {
	    if (!iCandWordCount)
		return IRV_TO_PROCESS;

	    iKey -= '0';
	    if (iKey == 0)
		iKey = 10;

	    if (!bIsInLegend) {
		if (iKey > iCandWordCount)
		    return IRV_DO_NOTHING;
		else {
		    if (bIsEBDelPhrase) {
			DelEBPhraseByIndex (iKey);
			retVal = EBGetCandWords (SM_FIRST);
//                      retVal = IRV_DISPLAY_MESSAGE;
		    }
		    else if (bIsEBAdjustOrder) {
			AdjustEBOrderByIndex (iKey);
			retVal = EBGetCandWords (SM_FIRST);
//                      retVal = IRV_DISPLAY_MESSAGE;
		    }
		    else {
			strcpy (strStringGet, EBGetCandWord (iKey - 1));
			if (bIsInLegend)
			    retVal = IRV_GET_LEGEND;
			else
			    retVal = IRV_GET_CANDWORDS;
		    }
		}
	    }
	    else {
		strcpy (strStringGet, EBGetLegendCandWord (iKey - 1));
		retVal = IRV_GET_LEGEND;
	    }
	}
	else if (!bIsEBDelPhrase && !bIsEBAdjustOrder) {
	    if (IsHotKey (iKey, hkEBAdjustOrder)) {
		if ((iCurrentCandPage == 0 && iCandWordCount < 2) || bIsInLegend)
		    return IRV_DO_NOTHING;

		bIsEBAdjustOrder = True;
		uMessageUp = 1;
		strcpy (messageUp[0].strMsg, "选择需要提前的词组序号，ESC取消");
		messageUp[0].type = MSG_TIPS;
		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else if (IsHotKey (iKey, hkEBDelPhrase)) {
		if (!iCandWordCount || bIsInLegend)
		    return IRV_DO_NOTHING;

		bIsEBDelPhrase = True;
		uMessageUp = 1;
		strcpy (messageUp[0].strMsg, "选择需要删除的词组序号，ESC取消");
		messageUp[0].type = MSG_TIPS;
		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else if (iKey == (XK_BackSpace & 0x00FF)) {
		if (!iCodeInputCount) {
		    bIsInLegend = False;
		    return IRV_DONOT_PROCESS_CLEAN;
		}

		iCodeInputCount--;
		strCodeInput[iCodeInputCount] = '\0';

		if (!strcmp (strCodeInput, "`")) {
			iCandWordCount = 0;
		    retVal = IRV_DISPLAY_CANDWORDS;
		}
		else if (iCodeInputCount)
		    retVal = EBGetCandWords (SM_FIRST);
		else
		    retVal = IRV_CLEAN;
	    }
	    else if (iKey == ' ') {
		if (!bIsInLegend) {
		    if (!(iCodeInputCount == 1 && strCodeInput[0] == '`')) {
			if (!iCandWordCount) {
			    iCodeInputCount = 0;
			    return IRV_CLEAN;
			}
			strcpy (strStringGet, EBGetCandWord (0));
		    }
		    if (bIsInLegend)
			retVal = IRV_GET_LEGEND;
		    else
			retVal = IRV_GET_CANDWORDS;
		}
		else {
		    strcpy (strStringGet, EBGetLegendCandWord (0));
		    retVal = IRV_GET_LEGEND;
		}
	    }
	    else
		return IRV_TO_PROCESS;
	}
    }

    if (!bIsInLegend) {
	if (!bIsEBDelPhrase && !bIsEBAdjustOrder) {
	    if (iCodeInputCount) {
		uMessageUp = 1;
		strcpy (messageUp[0].strMsg, strCodeInput);
		messageUp[0].type = MSG_INPUT;
	    }
	    else
		uMessageUp = 0;
	}
	else
	    bIsDoInputOnly = True;
    }

    if (retVal == IRV_GET_CANDWORDS) {
	if (bPromptEBCode) {
	    EBRECORD       *temp;

	    strcpy (messageDown[0].strMsg, strStringGet);
	    messageDown[0].type = MSG_TIPS;
	    temp = FindErBiCode (strStringGet, False);
	    if (temp) {
		strcpy (messageDown[1].strMsg, temp->strCode);
		messageDown[1].type = MSG_CODE;
		uMessageDown = 2;
	    }
	    else
		uMessageDown = 1;
	    iCodeInputCount = 0;
	}
	else {
	    uMessageDown = 0;
	    uMessageUp = 0;
	}
    }

    return retVal;
}

char           *EBGetCandWord (int iIndex)
{
    if (!strcmp (strCodeInput, "````"))
	return EBGetFHCandWord (iIndex);

    char           *pCandWord;

    bIsInLegend = False;

    if (iCandWordCount) {
	if (iIndex > (iCandWordCount - 1))
	    iIndex = iCandWordCount - 1;

	pCandWord = EBCandWord[iIndex]->strHZ;
	if (bEBAutoAdjustOrder)
	    AdjustEBOrderByIndex (iIndex + 1);

	if (bUseLegend) {
	    strcpy (strEBLegendSource, EBCandWord[iIndex]->strHZ);
	    EBGetLegendCandWords (SM_FIRST);
	}

	if (strlen (pCandWord) == 2)
	    lastIsSingleHZ = 1;
	else
	    lastIsSingleHZ = 0;

	return pCandWord;
    }

    return NULL;
}

INPUT_RETURN_VALUE EBGetPinyinCandWords (SEARCH_MODE mode)
{
    if (mode == SM_FIRST) {
	bSingleHZMode = True;
	strcpy (strFindString, strCodeInput + 1);

	DoPYInput (-1);

	strCodeInput[0] = '`';
	strCodeInput[1] = '\0';

	strcat (strCodeInput, strFindString);
	iCodeInputCount = strlen (strCodeInput);
    }
    else
	PYGetCandWords (mode);

    //下面将拼音的候选字表转换为五笔的样式
    int             i;
    EBRECORD       *pEB;
 
    for (i = 0; i < iCandWordCount; i++) {
	pEB = FindErBiCode (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase[PYCandWords[i].cand.base.iBase].strHZ, False);
	if (pEB)
	    EBCandWord[i] = pEB;
	else
	    EBCandWord[i] = erbiDictHead->next;
    }
    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE EBGetCandWords (SEARCH_MODE mode)
{
    int             i;
    char            strTemp[2], *pstr;
	

    if (bIsInLegend)
	return EBGetLegendCandWords (mode);
    if (!strcmp (strCodeInput, "````"))
	return EBGetFHCandWords (mode);
    if (strCodeInput[0] == '`' && bUseZPY)
	EBGetPinyinCandWords (mode);
    else {
	EBRECORD       *recTemp;

	if (mode == SM_FIRST) {
	    iCandPageCount = 0;
	    iCurrentCandPage = 0;

	    if (EBFindFirstMatchCode () == -1) {
		iCandWordCount = 0;
		uMessageDown = 0;
		return IRV_DISPLAY_CANDWORDS;	//Not Found
	    }
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

	if (mode == SM_PREV) {
	    EBFindFirstMatchCode ();
	    for (i = 0; i < iCurrentCandPage; i++) {
		iCandWordCount = 0;
		for (;;) {
		    for (;;) {
			if (!CompareEBCode (strCodeInput, erbiDictCurrent->strCode) && CheckLocale (erbiDictCurrent->strHZ)) {
			    iCandWordCount++;
			    break;
			}
			erbiDictCurrent = erbiDictCurrent->next;
		    }
		    erbiDictCurrent = erbiDictCurrent->next;
		    if (iCandWordCount == iMaxCandWord)
			break;
		}
	    }
	    iCandWordCount = 0;
	    for (;;) {
		for (;;) {
		    if (!CompareEBCode (strCodeInput, erbiDictCurrent->strCode) && CheckLocale (erbiDictCurrent->strHZ)) {
			EBCandWord[iCandWordCount++] = erbiDictCurrent;
			break;
		    }

		    erbiDictCurrent = erbiDictCurrent->next;
		}
		erbiDictCurrent = erbiDictCurrent->next;
		if (iCandWordCount == iMaxCandWord)
		    break;
	    }
	}
	else {
	    iCandWordCount = 0;

	    for (;;) {
		for (;;) {
		    if (!CompareEBCode (strCodeInput, erbiDictCurrent->strCode) && CheckLocale (erbiDictCurrent->strHZ)) {
			EBCandWord[iCandWordCount++] = erbiDictCurrent;
			break;
		    }

		    erbiDictCurrent = erbiDictCurrent->next;
		    if (erbiDictCurrent == erbiDictHead)
			break;
		}

		if (erbiDictCurrent == erbiDictHead)
		    break;
		erbiDictCurrent = erbiDictCurrent->next;
		if (erbiDictCurrent == erbiDictHead)
		    break;
		if (iCandWordCount == iMaxCandWord)
		    break;
	    }
	}

	if (mode != SM_PREV) {
	    recTemp = erbiDictCurrent;

	    if (iCurrentCandPage == iCandPageCount) {
		if (erbiDictCurrent != erbiDictHead) {
		    while (1) {
			if (!CompareEBCode (strCodeInput, recTemp->strCode) && CheckLocale (recTemp->strHZ)) {
			    iCandPageCount++;
			    break;
			}
			recTemp = recTemp->next;
			if (recTemp == erbiDictHead)
			    break;
		    }
		}
	    }
	}
    }

    strTemp[1] = '\0';
    uMessageDown = 0;

    for (i = 0; i < iCandWordCount; i++) {
	strTemp[0] = i + 1 + '0';
	if (i == 9)
	    strTemp[0] = '0';
	strcpy (messageDown[uMessageDown].strMsg, strTemp);
#ifdef _USE_XFT
	strcat (messageDown[uMessageDown].strMsg, " ");
#endif
	messageDown[uMessageDown++].type = MSG_INDEX;

	strcpy (messageDown[uMessageDown].strMsg, EBCandWord[i]->strHZ);
	messageDown[uMessageDown++].type = ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER);
	if (EBHasZ ())
	    pstr = EBCandWord[i]->strCode;
	else
	    pstr = EBCandWord[i]->strCode + iCodeInputCount;
	strcpy (messageDown[uMessageDown].strMsg, pstr);
	if (i != (iCandWordCount - 1)) {
#ifdef _USE_XFT
	    strcat (messageDown[uMessageDown].strMsg, "  ");
#else
	    strcat (messageDown[uMessageDown].strMsg, " ");
#endif
	}
	messageDown[uMessageDown++].type = MSG_CODE;
    }

    return IRV_DISPLAY_CANDWORDS;
}

Bool EBHasZ (void)
{
    char           *str;

    str = strCodeInput;
    while (*str) {
	if (*str++ == '`')
	    return True;
    }

    return False;
}

int CompareEBCode (char *strUser, char *strDict)
{
    int             i;

    for (i = 0; i < strlen (strUser); i++) {
	if (!strDict[i])
	    return strUser[i];
	if (strUser[i] != '`' || !bWBUseZ) {
	    if (strUser[i] != strDict[i])
		return (strUser[i] - strDict[i]);
	}
    }

    if (bEBExactMatch) {
	if (strlen (strUser) != strlen (strDict))
	    return -999;	//随意的一个值
    }

    return 0;
}

int EBFindFirstMatchCode (void)
{
    if (!erbiDictHead)
	return -1;

    int             i = 0;

    erbiDictCurrent = erbiDictHead->next;
    while (erbiDictCurrent != erbiDictHead) {
	if (!CompareEBCode (strCodeInput, erbiDictCurrent->strCode)) {
	    if (CheckLocale (erbiDictCurrent->strHZ))
		return i;
	}
	erbiDictCurrent = erbiDictCurrent->next;
	i++;
    }

    return -1;			//Not found
}

/*
 * 反查五笔编码
 * bMode=True表示用于组词，此时不查一、二级简码
 */
EBRECORD       *FindErBiCode (char *strHZ, Bool bMode)
{
    if (!erbiDictHead)
	return NULL;

    EBRECORD       *recTemp;

    recTemp = erbiDictHead->next;
    while (recTemp != erbiDictHead) {
	if (!strcmp (recTemp->strHZ, strHZ)) {
	    if ((bMode && strlen (recTemp->strCode) > 2) || !bMode)
		return recTemp;
	}
	recTemp = recTemp->next;
    }

    return NULL;
}

/*
 * 根据序号调整五笔词组顺序，序号从1开始
 * 将指定的字/词调整到同样编码的最前面
 */
void AdjustEBOrderByIndex (int iIndex)
{
    EBRECORD       *recTemp;

    recTemp = EBCandWord[iIndex - 1];
    while (!strcmp (recTemp->strCode, recTemp->prev->strCode))
	recTemp = recTemp->prev;
    if (recTemp == EBCandWord[iIndex - 1])	//说明已经是第一个
	return;

    //将指定的字/词放到recTemp前
    EBCandWord[iIndex - 1]->prev->next = EBCandWord[iIndex - 1]->next;
    EBCandWord[iIndex - 1]->next->prev = EBCandWord[iIndex - 1]->prev;
    recTemp->prev->next = EBCandWord[iIndex - 1];
    EBCandWord[iIndex - 1]->prev = recTemp->prev;
    recTemp->prev = EBCandWord[iIndex - 1];
    EBCandWord[iIndex - 1]->next = recTemp;

    iEBChanged++;
    if (iEBChanged == 5) {
	iEBChanged = 0;
	SaveErbiDict ();
    }
}

/*
 * 根据序号删除五笔词组，序号从1开始
 */
void DelEBPhraseByIndex (int iIndex)
{
    EBRECORD       *recTemp;

    if (strlen (EBCandWord[iIndex - 1]->strHZ) <= 2)
	return;

    recTemp = EBCandWord[iIndex - 1];

    recTemp->prev->next = recTemp->next;
    recTemp->next->prev = recTemp->prev;

    free (recTemp->strHZ);
    free (recTemp);

    SaveErbiDict ();
}

void InsertEBPhrase (char *strCode, char *strHZ)
{
    EBRECORD       *erbiDictInsertPoint, *erbiDictNew, *recTemp;
    Bool            bNew;

    erbiDictInsertPoint = erbiDictHead;
    recTemp = erbiDictHead->next;
    bNew = True;

    while (recTemp != erbiDictHead) {
	if (strcmp (recTemp->strCode, strCode) > 0) {
	    erbiDictInsertPoint = recTemp;
	    break;
	}
	else if (!strcmp (recTemp->strCode, strCode)) {
	    erbiDictInsertPoint = recTemp;
	    if (!strcmp (recTemp->strHZ, strHZ))	//该词组已经在词库中
		bNew = False;
	    break;
	}

	recTemp = recTemp->next;
    }

    if (bNew) {
	erbiDictNew = (EBRECORD *) malloc (sizeof (EBRECORD));
	strcpy (erbiDictNew->strCode, strCode);
	erbiDictNew->strHZ = (char *) malloc (sizeof (char) * strlen (strHZ) + 1);
	strcpy (erbiDictNew->strHZ, strHZ);

	erbiDictNew->prev = erbiDictInsertPoint->prev;
	erbiDictInsertPoint->prev->next = erbiDictNew;
	erbiDictInsertPoint->prev = erbiDictNew;
	erbiDictNew->next = erbiDictInsertPoint;

	SaveErbiDict ();
    }
}

void EBCreateNewPhrase (void)
{
    int             i;

    messageDown[0].strMsg[0] = '\0';
    for (i = iEBNewPhraseHZCount; i > 0; i--)
	strcat (messageDown[0].strMsg, hzLastInput[iHZLastInputCount - i].strHZ);

    CreatePhraseEBCode ();

    if (!bCanntFindErBi)
	strcpy (messageDown[1].strMsg, strNewPhraseEBCode);
    else
	strcpy (messageDown[1].strMsg, "????");
}

void CreatePhraseEBCode (void)
{
    char           *str1, *str2, *str3, *str4;

    bCanntFindErBi = False;
    str1 = (char *) FindErBiCode (hzLastInput[iHZLastInputCount - iEBNewPhraseHZCount].strHZ, True);
    str2 = (char *) FindErBiCode (hzLastInput[iHZLastInputCount - iEBNewPhraseHZCount + 1].strHZ, True);
    if (!str1 || !str2)
	bCanntFindErBi = True;
    else {
	if (iEBNewPhraseHZCount == 2) {
	    strNewPhraseEBCode[0] = str1[0];
	    strNewPhraseEBCode[1] = str1[1];
	    strNewPhraseEBCode[2] = str2[0];
	    strNewPhraseEBCode[3] = str2[1];
	}
	else if (iEBNewPhraseHZCount == 3) {
	    str3 = (char *) FindErBiCode (hzLastInput[iHZLastInputCount - 1].strHZ, True);
	    if (!str3)
		bCanntFindErBi = True;
	    else {
		strNewPhraseEBCode[0] = str1[0];
		strNewPhraseEBCode[1] = str1[1];
		strNewPhraseEBCode[2] = str2[0];
		strNewPhraseEBCode[3] = str3[0];
	    }
	}
	else {
	    str3 = (char *)
		FindErBiCode (hzLastInput[iHZLastInputCount - iEBNewPhraseHZCount + 2].strHZ, True);
	    str4 = (char *) FindErBiCode (hzLastInput[iHZLastInputCount - 1].strHZ, True);
	    if (!str3 || !str4)
		bCanntFindErBi = True;
	    else {
		strNewPhraseEBCode[0] = str1[0];
		strNewPhraseEBCode[1] = str2[0];
		strNewPhraseEBCode[2] = str3[0];
		strNewPhraseEBCode[3] = str4[0];
	    }
	}
    }
}

/*
 * 获取联想候选字列表
 */
INPUT_RETURN_VALUE EBGetLegendCandWords (SEARCH_MODE iMode)
{
    char            strTemp[2];
    int             iLength;
    int             i;
    EBRECORD       *recTemp;

    if (!strEBLegendSource[0])
	return IRV_TO_PROCESS;

    iLength = strlen (strEBLegendSource);

    if (iMode == SM_FIRST) {
	iLegendCandPageCount = 0;
	iLegendCandWordCount = 0;
	iCurrentLegendCandPage = 0;
	EBLegend = erbiDictHead->next;
    }
    else {
	if (!iLegendCandPageCount)
	    return IRV_TO_PROCESS;

	if (iMode == SM_NEXT) {
	    if (iCurrentLegendCandPage == iLegendCandPageCount)
		return IRV_DO_NOTHING;

	    iLegendCandWordCount = 0;
	    iCurrentLegendCandPage++;
	}
	else {
	    if (!iCurrentLegendCandPage)
		return IRV_DO_NOTHING;

	    iCurrentLegendCandPage--;
	    while (1) {
		if ((iLength + 2) == strlen (EBLegend->strHZ)) {
		    if (!strncmp (EBLegend->strHZ, strEBLegendSource, iLength)) {
			if (EBLegend->strHZ[iLength]) {
			    iLegendCandWordCount--;
			    if (!iLegendCandWordCount) {
				EBLegend = EBLegend->prev;
				break;
			    }
			}
		    }
		}
		EBLegend = EBLegend->prev;
	    }

	    recTemp = EBLegend;
	    while (iLegendCandWordCount < iMaxCandWord) {
		if ((iLength + 2) == strlen (EBLegend->strHZ)) {
		    if (!strncmp (EBLegend->strHZ, strEBLegendSource, iLength)) {
			if (EBLegend->strHZ[iLength]) {
			    legendCandWords[iMaxCandWord - iLegendCandWordCount - 1].strHZ[0] = EBLegend->strHZ[iLength];
			    legendCandWords[iMaxCandWord - iLegendCandWordCount - 1].strHZ[1] = EBLegend->strHZ[iLength + 1];
			    iLegendCandWordCount++;
			}
		    }
		}
		EBLegend = EBLegend->prev;
	    }
	    EBLegend = recTemp;
	}
    }

    if (iMode != SM_PREV) {
	EBLegend = EBLegend->next;
	while (EBLegend != erbiDictHead) {
	    if ((iLength + 2) == strlen (EBLegend->strHZ)) {
		if (!strncmp (EBLegend->strHZ, strEBLegendSource, iLength)) {
		    if (EBLegend->strHZ[iLength]) {
			legendCandWords[iLegendCandWordCount].strHZ[0] = EBLegend->strHZ[iLength];
			legendCandWords[iLegendCandWordCount++].strHZ[1] = EBLegend->strHZ[iLength + 1];
			if (iLegendCandWordCount == iMaxCandWord)
			    break;
		    }
		}
	    }
	    EBLegend = EBLegend->next;
	}

    if (!bDisablePagingInLegend && iCurrentLegendCandPage == iLegendCandPageCount) {
	    if (EBLegend != erbiDictHead) {
		recTemp = EBLegend->next;
		while (recTemp != erbiDictHead) {
		    if ((iLength + 2) == strlen (recTemp->strHZ)) {
			if (!strncmp (recTemp->strHZ, strEBLegendSource, iLength)) {
			    if (recTemp->strHZ[iLength]) {
				iLegendCandPageCount++;
				break;
			    }
			}
		    }
		    recTemp = recTemp->next;
		}
	    }
	}

	if (EBLegend == erbiDictHead)
	    EBLegend = EBLegend->prev;
    }

    uMessageUp = 2;
    strcpy (messageUp[0].strMsg, "联想：");
    messageUp[0].type = MSG_TIPS;
    strcpy (messageUp[1].strMsg, strEBLegendSource);
    messageUp[1].type = MSG_INPUT;

    strTemp[1] = '\0';
    uMessageDown = 0;
    for (i = 0; i < iLegendCandWordCount; i++) {
	strTemp[0] = i + 1 + '0';
	if (i == 9)
	    strTemp[0] = '0';
	strcpy (messageDown[uMessageDown].strMsg, strTemp);
#ifdef _USE_XFT
	strcat (messageDown[uMessageDown].strMsg, " ");
#endif
	messageDown[uMessageDown++].type = MSG_INDEX;

	strcpy (messageDown[uMessageDown].strMsg, legendCandWords[i].strHZ);
	if (i != (iLegendCandWordCount - 1)) {
#ifdef _USE_XFT
	    strcat (messageDown[uMessageDown].strMsg, "  ");
#else
	    strcat (messageDown[uMessageDown].strMsg, " ");
#endif
	}
	messageDown[uMessageDown++].type = ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER);
    }

    bIsInLegend = (iLegendCandWordCount != 0);

    return IRV_DISPLAY_CANDWORDS;
}

char           *EBGetLegendCandWord (int iIndex)
{
    if (iLegendCandWordCount) {
	if (iIndex > (iLegendCandWordCount - 1))
	    iIndex = iLegendCandWordCount - 1;
	strcpy (strEBLegendSource, legendCandWords[iIndex].strHZ);
	EBGetLegendCandWords (SM_FIRST);

	return strEBLegendSource;
    }

    return NULL;
}

INPUT_RETURN_VALUE EBGetFHCandWords (SEARCH_MODE mode)
{
    if (!iEBFH)
	return IRV_DO_NOTHING;

    char            strTemp[2];

    strTemp[1] = '\0';
    uMessageDown = 0;

    if (mode == SM_FIRST) {
	iCandPageCount = iEBFH / iMaxCandWord - ((iEBFH % iMaxCandWord) ? 0 : 1);
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

    int             i;

    for (i = 0; i < iMaxCandWord; i++) {
	strTemp[0] = i + 1 + '0';
	if (i == 9)
	    strTemp[0] = '0';
	strcpy (messageDown[uMessageDown].strMsg, strTemp);
// #ifdef _USE_XFT
	strcat (messageDown[uMessageDown].strMsg, ".");
// #endif
	messageDown[uMessageDown++].type = MSG_INDEX;
	strcpy (messageDown[uMessageDown].strMsg, ebfh[iCurrentCandPage * iMaxCandWord + i].strEBFH);
	if (i != (iMaxCandWord - 1)) {
#ifdef _USE_XFT
	    strcat (messageDown[uMessageDown].strMsg, "  ");
#else
	    strcat (messageDown[uMessageDown].strMsg, " ");
#endif
	}
	messageDown[uMessageDown++].type = ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER);
	if ((iCurrentCandPage * iMaxCandWord + i) >= (iEBFH - 1)) {
	    i++;
	    break;
	}
    }

    iCandWordCount = i;
    return IRV_DISPLAY_CANDWORDS;
}

char           *EBGetFHCandWord (int iIndex)
{
    //bIsInLegend = False;

    if (iCandWordCount) {
	if (iIndex > (iCandWordCount - 1))
	    iIndex = iCandWordCount - 1;

	return ebfh[iCurrentCandPage * iMaxCandWord + iIndex].strEBFH;
    }

    return NULL;
}

Bool EBPhraseTips (char *strPhrase)
{
    if (!erbiDictHead)
	return False;

    //如果最近输入了一个词组，这个工作就不需要了
    if (lastIsSingleHZ != 1)
	return False;

    //如果strPhrase只有一个汉字，这个工作也不需要了
    if (strlen (strPhrase) < 4)
	return False;

    //首先要判断是不是已经在词库中
    EBRECORD       *recTemp = NULL;
    INT8            i;

    for (i = 0; i < (strlen (strPhrase) - 2); i += 2) {
	recTemp = erbiDictHead->next;
	while (recTemp != erbiDictHead) {
	    if (!strcmp (recTemp->strHZ, strPhrase + i))
		goto _HIT;
	    recTemp = recTemp->next;
	}
    }
    if (recTemp == erbiDictHead)
	return False;

  _HIT:
    strcpy (messageUp[0].strMsg, "词库中有词组 ");
    messageUp[0].type = MSG_TIPS;
    strcpy (messageUp[1].strMsg, strPhrase + i);
    messageUp[1].type = MSG_INPUT;
    uMessageUp = 2;

    strcpy (messageDown[0].strMsg, "编码为：");
    messageDown[0].type = MSG_TIPS;
    strcpy (messageDown[1].strMsg, recTemp->strCode);
    messageDown[1].type = MSG_CODE;
    uMessageDown = 2;

    return True;
}
