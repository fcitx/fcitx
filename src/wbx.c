#include "wbx.h"

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

WBRECORD       *wubiDictCurrent = NULL, *wubiDictHead = NULL;
WBRECORD       *WBCandWord[MAX_CAND_WORD];
WBRECORD       *WBLegend = NULL;
char            strWBLegendSource[WB_PHRASE_MAX_LENGTH * 2 + 1] = "";

WBFH           *wbfh;
int             iWBFH = 0;

Bool            bIsWBDelPhrase = False;
HOTKEYS         hkWBDelPhrase[HOT_KEY_COUNT] = { CTRL_7, 0 };
Bool            bIsWBAdjustOrder = False;
HOTKEYS         hkWBAdjustOrder[HOT_KEY_COUNT] = { CTRL_6, 0 };
Bool            bIsWBAddPhrase = False;
HOTKEYS         hkWBAddPhrase[HOT_KEY_COUNT] = { CTRL_8, 0 };

BYTE            iWBChanged = 0;
BYTE            iWBNewPhraseHZCount;
Bool            bCanntFindWuBi;	//记录新组成的词能否生成五笔编码--一般情况下都是可以的
char            strNewPhraseWBCode[WB_CODE_LENGTH + 1];

Bool            bWBAutoAdjustOrder = False;
Bool            bWBAutoSendToClient = True;	//4键自动上屏
Bool            bUseZPY = True;	//用Z键输入拼音
Bool            bWBUseZ = True;	//是否用Z来模糊匹配
Bool            bWBExactMatch = True;

Bool            bWBDictLoaded = False;	//需要用的时候再读入五笔码表
Bool            bPromptWBCode = True;	//输入完毕后是否提示五笔编码--用Z输入拼音时总有提示

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

Bool LoadWBDict (void)
{
    char            strCode[WB_CODE_LENGTH + 1];
    char            strHZ[WB_PHRASE_MAX_LENGTH * 2 + 1];
    FILE           *fpDict;
    WBRECORD       *recTemp;
    char            strPath[PATH_MAX];
    int             i = 0;

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");
    strcat (strPath, WUBI_DICT_FILENAME);

    //读入五笔码表
    if (access (strPath, 0)) {
	if (bRunLocal) {
	    strcpy (strPath, (char *) getenv ("HOME"));
	    strcat (strPath, "/fcitx/");
	}
	else
	    strcpy (strPath, DATA_DIR);
	strcat (strPath, WUBI_DICT_FILENAME);
	fpDict = fopen (strPath, "rt");
    }
    else
	fpDict = fopen (strPath, "rt");
    if (!fpDict)
	return False;

    wubiDictHead = (WBRECORD *) malloc (sizeof (WBRECORD));
    wubiDictCurrent = wubiDictHead;

    for (;;) {
	if (EOF == fscanf (fpDict, "%s %s\n", strCode, strHZ))
	    break;
	recTemp = (WBRECORD *) malloc (sizeof (WBRECORD));
	wubiDictCurrent->next = recTemp;
	recTemp->prev = wubiDictCurrent;
	wubiDictCurrent = recTemp;
	strcpy (recTemp->strCode, strCode);
	/****************************************/
	recTemp->strHZ = (char *) malloc (sizeof (char) * strlen (strHZ) + 1);
	/****************************************/
	strcpy (recTemp->strHZ, strHZ);
    }
    wubiDictCurrent->next = wubiDictHead;
    wubiDictHead->prev = wubiDictCurrent;

    fclose (fpDict);

    //读取五笔特殊符号表
    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/");
    strcat (strPath, WUBI_FH_FILENAME);

    if (access (strPath, 0)) {
	if (bRunLocal) {
	    strcpy (strPath, (char *) getenv ("HOME"));
	    strcat (strPath, "/fcitx/");
	}
	else
	    strcpy (strPath, DATA_DIR);
	strcat (strPath, WUBI_FH_FILENAME);
	fpDict = fopen (strPath, "rt");
    }

    fpDict = fopen (strPath, "rt");
    if (fpDict) {
	iWBFH = CalculateRecordNumber (fpDict);
	wbfh = (WBFH *) malloc (sizeof (WBFH) * iWBFH);

	for (i = 0; i < iWBFH; i++) {
	    if (EOF == fscanf (fpDict, "%s\n", wbfh[i].strWBFH))
		break;
	}
	iWBFH = i;

	fclose (fpDict);
    }

    strNewPhraseWBCode[4] = '\0';
    bWBDictLoaded = True;

    return True;
}

void ResetWBStatus (void)
{
    bIsWBAddPhrase = False;
    bIsWBDelPhrase = False;
    bIsWBAdjustOrder = False;
    bIsDoInputOnly = False;
}

void SaveWubiDict (void)
{
    WBRECORD       *recTemp;
    char            strPath[PATH_MAX];
    FILE           *fp;

    strcpy (strPath, (char *) getenv ("HOME"));
    strcat (strPath, "/.fcitx/wbx.mb");

    fp = fopen (strPath, "wt");
    if (!fp) {
	fprintf (stderr, "Cannot create WuBi table file\n");
	return;
    }

    recTemp = wubiDictHead->next;

    while (recTemp != wubiDictHead) {
	fprintf (fp, "%s %s\n", recTemp->strCode, recTemp->strHZ);
	recTemp = recTemp->next;
    }

    fclose (fp);
}

INPUT_RETURN_VALUE DoWBInput (int iKey)
{
    INPUT_RETURN_VALUE retVal;

    if (!bWBDictLoaded)
	LoadWBDict ();

    retVal = IRV_DO_NOTHING;
    if (iKey >= 'a' && iKey <= 'z') {
	bIsInLegend = False;

	if (!bIsWBAddPhrase && !bIsWBDelPhrase && !bIsWBAdjustOrder) {
	    if (strCodeInput[0] == 'z') {
		if (iCodeInputCount != (MAX_PY_LENGTH + 1)) {
		    strCodeInput[iCodeInputCount++] = iKey;
		    strCodeInput[iCodeInputCount] = '\0';
		    retVal = WBGetCandWords (SM_FIRST);
		}
		else
		    retVal = IRV_DO_NOTHING;
	    }
	    else {
		if (iCodeInputCount < WB_CODE_LENGTH) {
		    strCodeInput[iCodeInputCount++] = iKey;
		    strCodeInput[iCodeInputCount] = '\0';

		    if (iCodeInputCount == 1 && strCodeInput[0] == 'z') {
			iCandWordCount = 0;	
			retVal = IRV_DISPLAY_CANDWORDS;
			}
		    else {
			retVal = WBGetCandWords (SM_FIRST);
			if (bWBAutoSendToClient && (iCodeInputCount == WB_CODE_LENGTH)) {
			    if (iCandWordCount == 1) {	//如果只有一个候选词，则送到客户程序中
				strcpy (strStringGet, WBGetCandWord (0));
				iCandWordCount = 0;
				retVal = IRV_GET_CANDWORDS;
			    }
			}
		    }
		}
		else {
		    if (bWBAutoSendToClient) {
			if (iCandWordCount) {
			    strcpy (strStringGet, WBCandWord[0]->strHZ);
			    retVal = IRV_GET_CANDWORDS_NEXT;
			}
			else
			    retVal = IRV_DISPLAY_CANDWORDS;

			iCodeInputCount = 1;
			strCodeInput[0] = iKey;
			strCodeInput[1] = '\0';

			WBGetCandWords (SM_FIRST);
		    }
		    else
			retVal = IRV_DO_NOTHING;
		}
	    }
	}
    }
    else {
	if (bIsWBAddPhrase) {
	    switch (iKey) {
	    case LEFT:
		if (iWBNewPhraseHZCount < iHZLastInputCount) {
		    iWBNewPhraseHZCount++;
		    WBCreateNewPhrase ();
		}
		break;
	    case RIGHT:
		if (iWBNewPhraseHZCount > 2) {
		    iWBNewPhraseHZCount--;
		    WBCreateNewPhrase ();
		}
		break;
	    case ENTER:
		if (!bCanntFindWuBi)
		    InsertWBPhrase (messageDown[1].strMsg, messageDown[0].strMsg);
	    case ESC:
		bIsWBAddPhrase = False;
		bIsDoInputOnly = False;
		return IRV_CLEAN;
	    default:
		return IRV_DO_NOTHING;
	    }

	    return IRV_DISPLAY_MESSAGE;
	}
	if (IsHotKey (iKey, hkWBAddPhrase)) {
	    if (!bIsWBAddPhrase) {
		if (iHZLastInputCount < 2)	//词组最少为两个汉字
		    return IRV_DO_NOTHING;

		iWBNewPhraseHZCount = 2;
		bIsWBAddPhrase = True;
		bIsDoInputOnly = True;

		uMessageUp = 1;
		strcpy (messageUp[0].strMsg, "左/右键增加/减少，ENTER确定，ESC取消");
		messageUp[0].type = MSG_TIPS;

		uMessageDown = 2;
		messageDown[0].type = MSG_FIRSTCAND;
		messageDown[1].type = MSG_CODE;

		WBCreateNewPhrase ();
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
	    if (bIsWBDelPhrase || bIsWBAdjustOrder) {
		ResetWBStatus ();
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
		    if (bIsWBDelPhrase) {
			DelWBPhraseByIndex (iKey);
			retVal = WBGetCandWords (SM_FIRST);
//                      retVal = IRV_DISPLAY_MESSAGE;
		    }
		    else if (bIsWBAdjustOrder) {
			AdjustWBOrderByIndex (iKey);
			retVal = WBGetCandWords (SM_FIRST);
//                      retVal = IRV_DISPLAY_MESSAGE;
		    }
		    else {
			strcpy (strStringGet, WBGetCandWord (iKey - 1));
			if (bIsInLegend)
			    retVal = IRV_GET_LEGEND;
			else
			    retVal = IRV_GET_CANDWORDS;
		    }
		}
	    }
	    else {
		strcpy (strStringGet, WBGetLegendCandWord (iKey - 1));
		retVal = IRV_GET_LEGEND;
	    }
	}
	else if (!bIsWBDelPhrase && !bIsWBAdjustOrder) {
	    if (IsHotKey (iKey, hkWBAdjustOrder)) {
		if ((iCurrentCandPage == 0 && iCandWordCount < 2) || bIsInLegend)
		    return IRV_DO_NOTHING;

		bIsWBAdjustOrder = True;
		uMessageUp = 1;
		strcpy (messageUp[0].strMsg, "选择需要提前的词组序号，ESC取消");
		messageUp[0].type = MSG_TIPS;
		retVal = IRV_DISPLAY_MESSAGE;
	    }
	    else if (IsHotKey (iKey, hkWBDelPhrase)) {
		if (!iCandWordCount || bIsInLegend)
		    return IRV_DO_NOTHING;

		bIsWBDelPhrase = True;
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

		if (!strcmp (strCodeInput, "z")) {
			iCandWordCount = 0;
		    retVal = IRV_DISPLAY_CANDWORDS;
		}
		else if (iCodeInputCount)
		    retVal = WBGetCandWords (SM_FIRST);
		else
		    retVal = IRV_CLEAN;
	    }
	    else if (iKey == ' ') {
		if (!bIsInLegend) {
		    if (!(iCodeInputCount == 1 && strCodeInput[0] == 'z')) {
			if (!iCandWordCount) {
			    iCodeInputCount = 0;
			    return IRV_CLEAN;
			}
			strcpy (strStringGet, WBGetCandWord (0));
		    }
		    if (bIsInLegend)
			retVal = IRV_GET_LEGEND;
		    else
			retVal = IRV_GET_CANDWORDS;
		}
		else {
		    strcpy (strStringGet, WBGetLegendCandWord (0));
		    retVal = IRV_GET_LEGEND;
		}
	    }
	    else
		return IRV_TO_PROCESS;
	}
    }

    if (!bIsInLegend) {
	if (!bIsWBDelPhrase && !bIsWBAdjustOrder) {
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
	if (bPromptWBCode) {
	    WBRECORD       *temp;

	    strcpy (messageDown[0].strMsg, strStringGet);
	    messageDown[0].type = MSG_TIPS;
	    temp = FindWuBiCode (strStringGet, False);
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

char           *WBGetCandWord (int iIndex)
{
    char           *pCandWord;

    if (!strcmp (strCodeInput, "zzzz"))
	return WBGetFHCandWord (iIndex);

    bIsInLegend = False;

    if (iCandWordCount) {
	if (iIndex > (iCandWordCount - 1))
	    iIndex = iCandWordCount - 1;

	pCandWord = WBCandWord[iIndex]->strHZ;
	if (bWBAutoAdjustOrder)
	    AdjustWBOrderByIndex (iIndex + 1);

	if (bUseLegend) {
	    strcpy (strWBLegendSource, WBCandWord[iIndex]->strHZ);
	    WBGetLegendCandWords (SM_FIRST);
	}

	if (strlen (pCandWord) == 2)
	    lastIsSingleHZ = 1;
	else
	    lastIsSingleHZ = 0;

	return pCandWord;
    }

    return NULL;
}

INPUT_RETURN_VALUE WBGetPinyinCandWords (SEARCH_MODE mode)
{
    int             i;
    WBRECORD       *pWB;

    if (mode == SM_FIRST) {
	bSingleHZMode = True;
	strcpy (strFindString, strCodeInput + 1);

	DoPYInput (-1);

	strCodeInput[0] = 'z';
	strCodeInput[1] = '\0';

	strcat (strCodeInput, strFindString);
	iCodeInputCount = strlen (strCodeInput);
    }
    else
	PYGetCandWords (mode);

    //下面将拼音的候选字表转换为五笔的样式
    for (i = 0; i < iCandWordCount; i++) {
	pWB = FindWuBiCode (PYFAList[PYCandWords[i].cand.base.iPYFA].pyBase[PYCandWords[i].cand.base.iBase].strHZ, False);
	if (pWB)
	    WBCandWord[i] = pWB;
	else
	    WBCandWord[i] = wubiDictHead->next;
    }

    return IRV_DISPLAY_CANDWORDS;
}

INPUT_RETURN_VALUE WBGetCandWords (SEARCH_MODE mode)
{
    int             i;
    char            strTemp[2], *pstr;

    if (bIsInLegend)
	return WBGetLegendCandWords (mode);
    if (!strcmp (strCodeInput, "zzzz"))
	return WBGetFHCandWords (mode);
    if (strCodeInput[0] == 'z' && bUseZPY)
	WBGetPinyinCandWords (mode);
    else {
	WBRECORD       *recTemp;

	if (mode == SM_FIRST) {
	    iCandPageCount = 0;
	    iCurrentCandPage = 0;
	    if (WBFindFirstMatchCode () == -1) {
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
	    WBFindFirstMatchCode ();
	    for (i = 0; i < iCurrentCandPage; i++) {
		iCandWordCount = 0;
		for (;;) {
		    for (;;) {
			if (!CompareWBCode (strCodeInput, wubiDictCurrent->strCode) && CheckLocale (wubiDictCurrent->strHZ)) {
			    iCandWordCount++;
			    break;
			}
			wubiDictCurrent = wubiDictCurrent->next;
		    }
		    wubiDictCurrent = wubiDictCurrent->next;
		    if (iCandWordCount == iMaxCandWord)
			break;
		}
	    }
	    iCandWordCount = 0;
	    for (;;) {
		for (;;) {
		    if (!CompareWBCode (strCodeInput, wubiDictCurrent->strCode) && CheckLocale (wubiDictCurrent->strHZ)) {
			WBCandWord[iCandWordCount++] = wubiDictCurrent;
			break;
		    }

		    wubiDictCurrent = wubiDictCurrent->next;
		}
		wubiDictCurrent = wubiDictCurrent->next;
		if (iCandWordCount == iMaxCandWord)
		    break;
	    }
	}
	else {
	    iCandWordCount = 0;

	    for (;;) {
		for (;;) {
		    if (!CompareWBCode (strCodeInput, wubiDictCurrent->strCode) && CheckLocale (wubiDictCurrent->strHZ)) {
			WBCandWord[iCandWordCount++] = wubiDictCurrent;
			break;
		    }

		    wubiDictCurrent = wubiDictCurrent->next;
		    if (wubiDictCurrent == wubiDictHead)
			break;
		}

		if (wubiDictCurrent == wubiDictHead)
		    break;
		wubiDictCurrent = wubiDictCurrent->next;
		if (wubiDictCurrent == wubiDictHead)
		    break;
		if (iCandWordCount == iMaxCandWord)
		    break;
	    }
	}

	if (mode != SM_PREV) {
	    recTemp = wubiDictCurrent;

	    if (iCurrentCandPage == iCandPageCount) {
		if (wubiDictCurrent != wubiDictHead) {
		    while (1) {
			if (!CompareWBCode (strCodeInput, recTemp->strCode) && CheckLocale (recTemp->strHZ)) {
			    iCandPageCount++;
			    break;
			}
			recTemp = recTemp->next;
			if (recTemp == wubiDictHead)
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

	strcpy (messageDown[uMessageDown].strMsg, WBCandWord[i]->strHZ);
	messageDown[uMessageDown++].type = ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER);
	if (HasZ ())
	    pstr = WBCandWord[i]->strCode;
	else
	    pstr = WBCandWord[i]->strCode + iCodeInputCount;
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

Bool HasZ (void)
{
    char           *str;

    str = strCodeInput;
    while (*str) {
	if (*str++ == 'z')
	    return True;
    }

    return False;
}

int CompareWBCode (char *strUser, char *strDict)
{
    int             i;

    for (i = 0; i < strlen (strUser); i++) {
	if (!strDict[i])
	    return strUser[i];
	if (strUser[i] != 'z' || !bWBUseZ) {
	    if (strUser[i] != strDict[i])
		return (strUser[i] - strDict[i]);
	}
    }

    if (bWBExactMatch) {
	if (strlen (strUser) != strlen (strDict))
	    return -999;	//随意的一个值
    }

    return 0;
}

int WBFindFirstMatchCode (void)
{
    int             i = 0;

    if (!wubiDictHead)
	return -1;

    wubiDictCurrent = wubiDictHead->next;
    while (wubiDictCurrent != wubiDictHead) {
	if (!CompareWBCode (strCodeInput, wubiDictCurrent->strCode)) {
	    if (CheckLocale (wubiDictCurrent->strHZ))
		return i;
	}
	wubiDictCurrent = wubiDictCurrent->next;
	i++;
    }

    return -1;			//Not found
}

/*
 * 反查五笔编码
 * bMode=True表示用于组词，此时不查一、二级简码
 */
WBRECORD       *FindWuBiCode (char *strHZ, Bool bMode)
{
    WBRECORD       *recTemp;

    if (!wubiDictHead)
	return NULL;

    recTemp = wubiDictHead->next;
    while (recTemp != wubiDictHead) {
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
void AdjustWBOrderByIndex (int iIndex)
{
    WBRECORD       *recTemp;

    recTemp = WBCandWord[iIndex - 1];
    while (!strcmp (recTemp->strCode, recTemp->prev->strCode))
	recTemp = recTemp->prev;
    if (recTemp == WBCandWord[iIndex - 1])	//说明已经是第一个
	return;

    //将指定的字/词放到recTemp前
    WBCandWord[iIndex - 1]->prev->next = WBCandWord[iIndex - 1]->next;
    WBCandWord[iIndex - 1]->next->prev = WBCandWord[iIndex - 1]->prev;
    recTemp->prev->next = WBCandWord[iIndex - 1];
    WBCandWord[iIndex - 1]->prev = recTemp->prev;
    recTemp->prev = WBCandWord[iIndex - 1];
    WBCandWord[iIndex - 1]->next = recTemp;

    iWBChanged++;
    if (iWBChanged == 5) {
	iWBChanged = 0;
	SaveWubiDict ();
    }
}

/*
 * 根据序号删除五笔词组，序号从1开始
 */
void DelWBPhraseByIndex (int iIndex)
{
    WBRECORD       *recTemp;

    if (strlen (WBCandWord[iIndex - 1]->strHZ) <= 2)
	return;

    recTemp = WBCandWord[iIndex - 1];

    recTemp->prev->next = recTemp->next;
    recTemp->next->prev = recTemp->prev;

    free (recTemp->strHZ);
    free (recTemp);

    SaveWubiDict ();
}

void InsertWBPhrase (char *strCode, char *strHZ)
{
    WBRECORD       *wubiDictInsertPoint, *wubiDictNew, *recTemp;
    Bool            bNew;

    wubiDictInsertPoint = wubiDictHead;
    recTemp = wubiDictHead->next;
    bNew = True;

    while (recTemp != wubiDictHead) {
	if (strcmp (recTemp->strCode, strCode) > 0) {
	    wubiDictInsertPoint = recTemp;
	    break;
	}
	else if (!strcmp (recTemp->strCode, strCode)) {
	    wubiDictInsertPoint = recTemp;
	    if (!strcmp (recTemp->strHZ, strHZ))	//该词组已经在词库中
		bNew = False;
	    break;
	}

	recTemp = recTemp->next;
    }

    if (bNew) {
	wubiDictNew = (WBRECORD *) malloc (sizeof (WBRECORD));
	strcpy (wubiDictNew->strCode, strCode);
	wubiDictNew->strHZ = (char *) malloc (sizeof (char) * strlen (strHZ) + 1);
	strcpy (wubiDictNew->strHZ, strHZ);

	wubiDictNew->prev = wubiDictInsertPoint->prev;
	wubiDictInsertPoint->prev->next = wubiDictNew;
	wubiDictInsertPoint->prev = wubiDictNew;
	wubiDictNew->next = wubiDictInsertPoint;

	SaveWubiDict ();
    }
}

void WBCreateNewPhrase (void)
{
    int             i;

    messageDown[0].strMsg[0] = '\0';
    for (i = iWBNewPhraseHZCount; i > 0; i--)
	strcat (messageDown[0].strMsg, hzLastInput[iHZLastInputCount - i].strHZ);

    CreatePhraseWBCode ();

    if (!bCanntFindWuBi)
	strcpy (messageDown[1].strMsg, strNewPhraseWBCode);
    else
	strcpy (messageDown[1].strMsg, "????");
}

void CreatePhraseWBCode (void)
{
    char           *str1, *str2, *str3, *str4;

    bCanntFindWuBi = False;
    str1 = (char *) FindWuBiCode (hzLastInput[iHZLastInputCount - iWBNewPhraseHZCount].strHZ, True);
    str2 = (char *) FindWuBiCode (hzLastInput[iHZLastInputCount - iWBNewPhraseHZCount + 1].strHZ, True);
    if (!str1 || !str2)
	bCanntFindWuBi = True;
    else {
	if (iWBNewPhraseHZCount == 2) {
	    strNewPhraseWBCode[0] = str1[0];
	    strNewPhraseWBCode[1] = str1[1];
	    strNewPhraseWBCode[2] = str2[0];
	    strNewPhraseWBCode[3] = str2[1];
	}
	else if (iWBNewPhraseHZCount == 3) {
	    str3 = (char *) FindWuBiCode (hzLastInput[iHZLastInputCount - 1].strHZ, True);
	    if (!str3)
		bCanntFindWuBi = True;
	    else {
		strNewPhraseWBCode[0] = str1[0];
		strNewPhraseWBCode[1] = str2[0];
		strNewPhraseWBCode[2] = str3[0];
		strNewPhraseWBCode[3] = str3[1];
	    }
	}
	else {
	    str3 = (char *)
		FindWuBiCode (hzLastInput[iHZLastInputCount - iWBNewPhraseHZCount + 2].strHZ, True);
	    str4 = (char *) FindWuBiCode (hzLastInput[iHZLastInputCount - 1].strHZ, True);
	    if (!str3 || !str4)
		bCanntFindWuBi = True;
	    else {
		strNewPhraseWBCode[0] = str1[0];
		strNewPhraseWBCode[1] = str2[0];
		strNewPhraseWBCode[2] = str3[0];
		strNewPhraseWBCode[3] = str4[0];
	    }
	}
    }
}

/*
 * 获取联想候选字列表
 */
INPUT_RETURN_VALUE WBGetLegendCandWords (SEARCH_MODE iMode)
{
    char            strTemp[2];
    int             iLength;
    int             i;
    WBRECORD       *recTemp;

    if (!strWBLegendSource[0])
	return IRV_TO_PROCESS;

    iLength = strlen (strWBLegendSource);

    if (iMode == SM_FIRST) {
	iLegendCandPageCount = 0;
	iLegendCandWordCount = 0;
	iCurrentLegendCandPage = 0;
	WBLegend = wubiDictHead->next;
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
		if ((iLength + 2) == strlen (WBLegend->strHZ)) {
		    if (!strncmp (WBLegend->strHZ, strWBLegendSource, iLength)) {
			if (WBLegend->strHZ[iLength]) {
			    iLegendCandWordCount--;
			    if (!iLegendCandWordCount) {
				WBLegend = WBLegend->prev;
				break;
			    }
			}
		    }
		}
		WBLegend = WBLegend->prev;
	    }

	    recTemp = WBLegend;
	    while (iLegendCandWordCount < iMaxCandWord) {
		if ((iLength + 2) == strlen (WBLegend->strHZ)) {
		    if (!strncmp (WBLegend->strHZ, strWBLegendSource, iLength)) {
			if (WBLegend->strHZ[iLength]) {
			    legendCandWords[iMaxCandWord - iLegendCandWordCount - 1].strHZ[0] = WBLegend->strHZ[iLength];
			    legendCandWords[iMaxCandWord - iLegendCandWordCount - 1].strHZ[1] = WBLegend->strHZ[iLength + 1];
			    iLegendCandWordCount++;
			}
		    }
		}
		WBLegend = WBLegend->prev;
	    }
	    WBLegend = recTemp;
	}
    }

    if (iMode != SM_PREV) {
	WBLegend = WBLegend->next;
	while (WBLegend != wubiDictHead) {
	    if ((iLength + 2) == strlen (WBLegend->strHZ)) {
		if (!strncmp (WBLegend->strHZ, strWBLegendSource, iLength)) {
		    if (WBLegend->strHZ[iLength]) {
			legendCandWords[iLegendCandWordCount].strHZ[0] = WBLegend->strHZ[iLength];
			legendCandWords[iLegendCandWordCount++].strHZ[1] = WBLegend->strHZ[iLength + 1];
			if (iLegendCandWordCount == iMaxCandWord)
			    break;
		    }
		}
	    }
	    WBLegend = WBLegend->next;
	}

    if (!bDisablePagingInLegend && iCurrentLegendCandPage == iLegendCandPageCount) {
	    if (WBLegend != wubiDictHead) {
		recTemp = WBLegend->next;
		while (recTemp != wubiDictHead) {
		    if ((iLength + 2) == strlen (recTemp->strHZ)) {
			if (!strncmp (recTemp->strHZ, strWBLegendSource, iLength)) {
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

	if (WBLegend == wubiDictHead)
	    WBLegend = WBLegend->prev;
    }

    uMessageUp = 2;
    strcpy (messageUp[0].strMsg, "联想：");
    messageUp[0].type = MSG_TIPS;
    strcpy (messageUp[1].strMsg, strWBLegendSource);
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

char           *WBGetLegendCandWord (int iIndex)
{
    if (iLegendCandWordCount) {
	if (iIndex > (iLegendCandWordCount - 1))
	    iIndex = iLegendCandWordCount - 1;
	strcpy (strWBLegendSource, legendCandWords[iIndex].strHZ);
	WBGetLegendCandWords (SM_FIRST);

	return strWBLegendSource;
    }

    return NULL;
}

INPUT_RETURN_VALUE WBGetFHCandWords (SEARCH_MODE mode)
{
    char            strTemp[2];
    int             i;

    if (!iWBFH)
	return IRV_DO_NOTHING;

    strTemp[1] = '\0';
    uMessageDown = 0;

    if (mode == SM_FIRST) {
	iCandPageCount = iWBFH / iMaxCandWord - ((iWBFH % iMaxCandWord) ? 0 : 1);
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

    for (i = 0; i < iMaxCandWord; i++) {
	strTemp[0] = i + 1 + '0';
	if (i == 9)
	    strTemp[0] = '0';
	strcpy (messageDown[uMessageDown].strMsg, strTemp);
// #ifdef _USE_XFT
	strcat (messageDown[uMessageDown].strMsg, ".");
// #endif
	messageDown[uMessageDown++].type = MSG_INDEX;
	strcpy (messageDown[uMessageDown].strMsg, wbfh[iCurrentCandPage * iMaxCandWord + i].strWBFH);
	if (i != (iMaxCandWord - 1)) {
#ifdef _USE_XFT
	    strcat (messageDown[uMessageDown].strMsg, "  ");
#else
	    strcat (messageDown[uMessageDown].strMsg, " ");
#endif
	}
	messageDown[uMessageDown++].type = ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER);
	if ((iCurrentCandPage * iMaxCandWord + i) >= (iWBFH - 1)) {
	    i++;
	    break;
	}
    }

    iCandWordCount = i;
    return IRV_DISPLAY_CANDWORDS;
}

char           *WBGetFHCandWord (int iIndex)
{
    //bIsInLegend = False;

    if (iCandWordCount) {
	if (iIndex > (iCandWordCount - 1))
	    iIndex = iCandWordCount - 1;

	return wbfh[iCurrentCandPage * iMaxCandWord + iIndex].strWBFH;
    }

    return NULL;
}

Bool WBPhraseTips (char *strPhrase)
{
    WBRECORD       *recTemp = NULL;
    INT8            i;

    if (!wubiDictHead)
	return False;

    //如果最近输入了一个词组，这个工作就不需要了
    if (lastIsSingleHZ != 1)
	return False;

    //如果strPhrase只有一个汉字，这个工作也不需要了
    if (strlen (strPhrase) < 4)
	return False;

    //首先要判断是不是已经在词库中
    for (i = 0; i < (strlen (strPhrase) - 2); i += 2) {
	recTemp = wubiDictHead->next;
	while (recTemp != wubiDictHead) {
	    if (!strcmp (recTemp->strHZ, strPhrase + i))
		goto _HIT;
	    recTemp = recTemp->next;
	}
    }
    if (recTemp == wubiDictHead)
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
