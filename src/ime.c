#include "xim.h"
#include "ime.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "IC.h"
#include "punc.h"
#include "wbx.h"
#include "erbi.h"
#include "py.h"
#include "tools.h"

#include <ctype.h>

int             iMaxCandWord = 5;
int             iCandPageCount;
int             iCurrentCandPage;
int             iCandWordCount;

int             iLegendCandWordCount;
int             iLegendCandPageCount;
int             iCurrentLegendCandPage;

int             iCodeInputCount;

// *************************************************************
char            strCodeInput[MAX_USER_INPUT + 1];
char            strStringGet[MAX_USER_INPUT + 1];	//保存输入法返回的需要送到客户程序中的字串

// *************************************************************
int             iCursorPos;

ENTER_TO_DO     enterToDo = K_ENTER_SEND;

Bool            bCorner = False;	//全半角切换
Bool            bChnPunc = True;	//中英文标点切换
Bool            bUseGBK = False;	//是否支持GBK
Bool            bIsDoInputOnly = False;	//表明是否只由输入法来处理键盘
Bool            bLastIsNumber = False;	//上一次输入是不是阿拉伯数字
Bool            bInCap = False;	//是不是处于大写后的英文状态
Bool            bAutoHideInputWindow = True;	//是否自动隐藏输入条
Bool            bEngPuncAfterNumber = True;	//数字后面输出半角符号(只对'.'/','有效)
Bool            bPhraseTips = False;
INT8            lastIsSingleHZ = 0;

Bool            bEngAfterSemicolon = True;
Bool            bEngAfterCap = True;
Bool            bDisablePagingInLegend = True;

int             i2ndSelectKey = 0;	//第二个候选词选择键，为扫描码
int             i3rdSelectKey = 0;	//第三个候选词选择键，为扫描码

int             iLastKey = 0;	//上次输入的键值
Time            lastKeyPressedTime;

KEY_RELEASED    keyReleased = KR_OTHER;
KEYCODE         switchKey = L_CTRL;

INPUT_RETURN_VALUE (*DoInput) (int);
char           *(*GetCandWord) (int);

INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE);
Bool (*PhraseTips) (char *strPhrase);	//提示词库中是否已经有

//热键定义
HOTKEYS         hkGBK[HOT_KEY_COUNT] = { CTRL_M, 0 };
HOTKEYS         hkLegend[HOT_KEY_COUNT] = { CTRL_L, 0 };
HOTKEYS         hkCorner[HOT_KEY_COUNT] = { SHIFT_SPACE, 0 };	//全半角切换
HOTKEYS         hkPunc[HOT_KEY_COUNT] = { ALT_SPACE, 0 };	//中文标点
HOTKEYS         hkNextPage[HOT_KEY_COUNT] = { '.', 0 };	//下一页
HOTKEYS         hkPrevPage[HOT_KEY_COUNT] = { ',', 0 };	//上一页

HOTKEYS         hkERBINextPage[HOT_KEY_COUNT] = { ']', 0 };
HOTKEYS         hkERBIPrevPage[HOT_KEY_COUNT] = { '[', 0 };

SINGLE_HZ       hzLastInput[MAX_HZ_SAVED];	//记录最近输入的汉字
BYTE            iHZLastInputCount = 0;

Bool            bUseLegend = False;
SINGLE_HZ       legendCandWords[MAX_CAND_WORD];
Bool            bIsInLegend = False;

IME             imeIndex = IME_WUBI;

extern IC      *CurrentIC;
extern Display *dpy;
extern ChnPunc *chnPunc;

extern MESSAGE  messageUp[];
extern uint     uMessageUp;
extern MESSAGE  messageDown[];
extern uint     uMessageDown;
extern Bool     bShowPrev;
extern Bool     bShowNext;
extern Bool     bShowCursor;
extern Bool     bSingleHZMode;
extern Window   inputWindow;
extern Bool     bUseCtrlShift;
extern HIDE_MAINWINDOW hideMainWindow;
extern XIMTriggerKey Trigger_Keys[];
extern Window   mainWindow;

extern int      MAINWND_WIDTH;

/*******************************************************/
//Bool            bDebug = False;

void ResetInput (void)
{
    iCandPageCount = 0;
    iCurrentCandPage = 0;
    iCandWordCount = 0;
    iLegendCandWordCount = 0;
    iCurrentLegendCandPage = 0;
    iLegendCandPageCount = 0;
    iCursorPos = 0;

    strCodeInput[0] = '\0';
    iCodeInputCount = 0;

    bIsDoInputOnly = False;

    bShowPrev = False;
    bShowNext = False;

    bIsInLegend = False;
    bInCap = False;

    if (imeIndex == IME_PINYIN) {
	bSingleHZMode = False;
	ResetPYStatus ();
    }
    else {
	bShowCursor = False;
	ResetWBStatus ();
	ResetEBStatus ();
    }
}

void CloseIME (XIMS ims, IMForwardEventStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);

    if (CurrentIC->imeState == IS_ENG)
	CurrentIC->imeState = IS_CHN;
    else {
	CurrentIC->imeState = IS_CLOSED;
	IMPreeditEnd (ims, (XPointer) call_data);
    }

    DisplayMainWindow ();
}

//FILE           *fd;
void ProcessKey (XIMS ims, IMForwardEventStruct * call_data)
{
    KeySym          keysym;
    XKeyEvent      *kev;
    int             count;
    INPUT_RETURN_VALUE retVal;
    int             iKeyState;
    char           *strbuf;
    int             iKey;
    char           *pstr;

    kev = (XKeyEvent *) & call_data->event;

    strbuf = (char *) malloc (sizeof (char) * STRBUFLEN);
    memset (strbuf, 0, STRBUFLEN);
    count = XLookupString (kev, strbuf, STRBUFLEN, &keysym, NULL);
    free (strbuf);

    iKeyState = kev->state - (kev->state & KEY_NUMLOCK) - (kev->state & KEY_CAPSLOCK) - (kev->state & KEY_SCROLLLOCK);

    iKey = GetKey (keysym, iKeyState, count);

    //printf ("KEYSTATE:%d  %d KEYCODE:%d COUNT:%d KEY:%d\n", kev->state,iKeyState, (unsigned char) keysym, count, GetKey (keysym, iKeyState, count));
    if (!iKey)
	return;

    retVal = IRV_TO_PROCESS;
    if (call_data->event.type == KeyRelease) {
	if ((kev->time - lastKeyPressedTime) < 500 && (!bIsDoInputOnly)) {
	    if (iKeyState == KEY_CTRL_SHIFT_COMP && (iKey == 225 || iKey == 227)) {
		if (CurrentIC->imeState == IS_CHN)
		    SwitchIME (-1);
		else if (IsKey (ims, call_data, Trigger_Keys))
		    CloseIME (ims, call_data);
	    }
	    else if (iKey == CTRL_LSHIFT) {
		if (CurrentIC->imeState == IS_CHN)
		    SwitchIME (-1);
		else if (IsKey (ims, call_data, Trigger_Keys))
		    CloseIME (ims, call_data);
	    }
	    else if (kev->keycode == switchKey && keyReleased == KR_CTRL) {
		if (CurrentIC->imeState == IS_ENG) {
		    CurrentIC->imeState = IS_CHN;
		    DisplayInputWindow ();
		}
		else {
		    CurrentIC->imeState = IS_ENG;
		    ResetInput ();
		    ResetInputWindow ();
		    XUnmapWindow (dpy, inputWindow);
		}
		if (hideMainWindow != HM_HIDE)
		    DisplayMainWindow ();
	    }
	    else if (kev->keycode == i2ndSelectKey && keyReleased == KR_2ND_SELECTKEY) {
		if (!bIsInLegend) {
		    pstr = GetCandWord (1);
		    if (pstr) {
			strcpy (strStringGet, pstr);
			if (bIsInLegend)
			    retVal = IRV_GET_LEGEND;
			else
			    retVal = IRV_GET_CANDWORDS;
		    }
		    else if (iCandWordCount)
			retVal = IRV_DISPLAY_CANDWORDS;
		    else
			retVal = IRV_DONOT_PROCESS;
		}
	    }
	    else if (kev->keycode == i3rdSelectKey && keyReleased == KR_3RD_SELECTKEY) {
		if (!bIsInLegend) {
		    pstr = GetCandWord (2);
		    if (pstr) {
			strcpy (strStringGet, pstr);
			if (bIsInLegend)
			    retVal = IRV_GET_LEGEND;
			else
			    retVal = IRV_GET_CANDWORDS;
		    }
		    else if (iCandWordCount)
			retVal = IRV_DISPLAY_CANDWORDS;
		    else
			retVal = IRV_DONOT_PROCESS;
		}
	    }
	}

	keyReleased = KR_OTHER;

	if (retVal == IRV_TO_PROCESS)
	    IMForwardEvent (ims, (XPointer) call_data);
    }
    else if (call_data->event.type == KeyPress) {
	lastKeyPressedTime = kev->time;
	keyReleased = KR_OTHER;
	if (iKeyState == KEY_NONE) {
	    if (kev->keycode == switchKey)
		keyReleased = KR_CTRL;
	    else if (kev->keycode == i2ndSelectKey)
		keyReleased = KR_2ND_SELECTKEY;
	    else if (kev->keycode == i3rdSelectKey)
		keyReleased = KR_3RD_SELECTKEY;
	}

	if (iKey == CTRL_LSHIFT || iKey == SHIFT_LCTRL) {
	    //什么都不做
	}
	else if (IsKey (ims, call_data, Trigger_Keys))
	    CloseIME (ims, call_data);
	else {
	    if (CurrentIC->imeState != IS_CHN)
		IMForwardEvent (ims, (XPointer) call_data);
	    else {
		if (!bInCap && !bCorner) {
		    retVal = DoInput (iKey);
/*		    if (bDebug)
			fprintf (fd, "INPUT: %d  %c\n", iKey, iKey);*/
		}
/*		if (iKey == CTRL_D) {
		    fd = fopen ("/home/yuking/debug.txt", "wt");
		    fprintf (fd, "DEBUG________________________________\n");

		    bDebug = True;
		}
		if (iKey == CTRL_E) {
		    fclose (fd);
		    bDebug = False;
		}*/

		if (!bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
		    if ((iKey == CTRL_LSHIFT || iKey == CTRL_RSHIFT) && (bUseCtrlShift)) {
			CloseIME (ims, call_data);
			return;
		    }
		    else if (bCorner && (iKey >= 32 && iKey <= 126)) {
			sprintf (strStringGet, "%c%c", 0xa3, 0xa0 + iKey - 32);
			retVal = IRV_GET_CANDWORDS;
		    }
		    else if (iKey >= 'A' && iKey <= 'Z' && bEngAfterCap && !(kev->state & KEY_CAPSLOCK)) {
			bInCap = True;
			if (!bIsInLegend && iCandWordCount) {
			    pstr = GetCandWord (0);
			    iCandWordCount = 0;
			    if (pstr) {
				strcpy (strStringGet, pstr);
				UpdateHZLastInput ();
				SendHZtoClient (ims, call_data, strStringGet);
				iCodeInputCount = 0;
			    }
			}
		    }
		    else if (iKey == ';' && bEngAfterSemicolon && !iCodeInputCount)
			bInCap = True;
		    else if (IsHotKey (iKey, hkCorner))
			retVal = ChangeCorner ();
		    else if (IsHotKey (iKey, hkPunc))
			retVal = ChangePunc ();
		    else if (IsHotKey (iKey, hkGBK))
			retVal = ChangeGBK ();
//need to adjust here for PrevPage & NetxPage Hotkey of different IMEs
		    else if (IsHotKey (iKey, hkPrevPage)) {
			if (imeIndex != IME_ERBI)
			    retVal = GetCandWords (SM_PREV);
		    }
		    else if (IsHotKey (iKey, hkNextPage)) {
			if (imeIndex != IME_ERBI)
			    retVal = GetCandWords (SM_NEXT);
		    }
		    else if (IsHotKey (iKey, hkERBIPrevPage)) {
			if (imeIndex == IME_ERBI)
			    retVal = GetCandWords (SM_PREV);
		    }
		    else if (IsHotKey (iKey, hkERBINextPage)) {
			if (imeIndex == IME_ERBI)
			    retVal = GetCandWords (SM_NEXT);
		    }
		    else if (IsHotKey (iKey, hkLegend))
			retVal = ChangeLegend ();

		    if (retVal == IRV_TO_PROCESS) {
			if (bInCap) {
			    if (iKey == ' ' && iCodeInputCount == 0) {
				strcpy (strStringGet, "；");
				retVal = IRV_ENG;
				bInCap = False;
			    }
			    else if (isprint (iKey) && iKey < 128) {
				if (iCodeInputCount == MAX_USER_INPUT)
				    retVal = IRV_DO_NOTHING;
				else {
				    if (!bEngAfterSemicolon || !(bEngAfterSemicolon && (iCodeInputCount == 0 && iKey == ';'))) {
					strCodeInput[iCodeInputCount++] = iKey;
					strCodeInput[iCodeInputCount] = '\0';
				    }
				    retVal = IRV_DISPLAY_MESSAGE;
				}
			    }
			    else if (iKey == (XK_BackSpace & 0x00FF) && iCodeInputCount) {
				iCodeInputCount--;
				strCodeInput[iCodeInputCount] = '\0';
				retVal = IRV_DISPLAY_MESSAGE;
				if (!iCodeInputCount)
				    retVal = IRV_CLEAN;
			    }
			    uMessageUp = 1;
			    uMessageDown = 1;
			    if (bEngAfterSemicolon && !iCodeInputCount) {
				strcpy (messageUp[0].strMsg, "进入英文输入状态");
				strcpy (messageDown[0].strMsg, "空格输入；Enter输入;");
			    }
			    else {
				strcpy (messageUp[0].strMsg, strCodeInput);
				strcpy (messageDown[0].strMsg, "按 Enter 输入英文");
			    }
			    messageUp[0].type = MSG_INPUT;
			    messageDown[0].type = MSG_TIPS;
			}
			else if ((bLastIsNumber && bEngPuncAfterNumber) && (iKey == '.' || iKey == ',')) {
			    retVal = IRV_TO_PROCESS;
			    bLastIsNumber = False;
			}
			else {
			    if (bChnPunc) {
				int             iPunc;

				pstr = NULL;
				iPunc = IsPunc (iKey);
				if (iPunc != -1) {
				    strStringGet[0] = '\0';
				    if (!bIsInLegend)
					pstr = GetCandWord (0);
				    if (pstr) {
					strcpy (strStringGet, pstr);
					UpdateHZLastInput ();
				    }
				    strcat (strStringGet, chnPunc[iPunc].strChnPunc[chnPunc[iPunc].iWhich]);

				    uMessageUp = 1;
				    messageUp[0].strMsg[0] = iKey;
				    messageUp[0].strMsg[1] = '\0';
				    messageUp[0].type = MSG_INPUT;

				    uMessageDown = 1;
				    strcpy (messageDown[0].strMsg, chnPunc[iPunc].strChnPunc[chnPunc[iPunc].iWhich]);
				    messageDown[0].type = MSG_OTHER;

				    chnPunc[iPunc].iWhich++;
				    if (chnPunc[iPunc].iWhich >= chnPunc[iPunc].iCount)
					chnPunc[iPunc].iWhich = 0;

				    retVal = IRV_PUNC;
				}
				else if (isprint (iKey) && iKey < 128) {
				    if (iKey >= '0' && iKey <= '9')
					bLastIsNumber = True;
				    else {
					bLastIsNumber = False;
					if (iKey == ' ')
					    retVal = IRV_DONOT_PROCESS_CLEAN;	//为了与mozilla兼容
					else {
					    strStringGet[0] = '\0';
					    if (!bIsInLegend)
						pstr = GetCandWord (0);
					    if (pstr) {
						strcpy (strStringGet, pstr);
						UpdateHZLastInput ();
					    }
					    count = strlen (strStringGet);
					    uMessageDown = uMessageUp = 0;
					    strStringGet[count] = iKey;
					    strStringGet[count + 1] = '\0';
					    retVal = IRV_ENG;
					}

				    }
				}
			    }
			}
		    }
		}

		if (retVal == IRV_TO_PROCESS) {
		    if (iKey == ESC) {
			if (iCodeInputCount || bInCap || bIsInLegend)
			    retVal = IRV_CLEAN;
			else
			    retVal = IRV_DONOT_PROCESS_CLEAN;
		    }
		    else if (iKey == CTRL_5) {
			LoadConfig (False);
			retVal = IRV_DONOT_PROCESS_CLEAN;
		    }
		    else if (iKey == ENTER) {
			if (bInCap) {
			    if (bEngAfterSemicolon && !iCodeInputCount)
				strcpy (strStringGet, ";");
			    else
				strcpy (strStringGet, strCodeInput);
			    retVal = IRV_ENG;
			    bInCap = False;
			}
			else if (!iCodeInputCount)
			    retVal = IRV_DONOT_PROCESS;
			else {
			    switch (enterToDo) {
			    case K_ENTER_NOTHING:
				retVal = IRV_DO_NOTHING;
				break;
			    case K_ENTER_CLEAN:
				retVal = IRV_CLEAN;
				break;
			    case K_ENTER_SEND:
				uMessageDown = 1;
				strcpy (messageDown[0].strMsg, strCodeInput);
				strcpy (strStringGet, strCodeInput);
				retVal = IRV_ENG;
				break;
			    }
			}
		    }
		    else if (isprint (iKey) && iKey < 128)
			retVal = IRV_DONOT_PROCESS_CLEAN;
		    else
			retVal = IRV_DONOT_PROCESS;
		}
	    }
	}
    }
    else
	retVal = IRV_DONOT_PROCESS;

    switch (retVal) {
    case IRV_DO_NOTHING:
	break;
    case IRV_DONOT_PROCESS:
    case IRV_DONOT_PROCESS_CLEAN:
	IMForwardEvent (ims, (XPointer) call_data);
	if (retVal == IRV_DONOT_PROCESS)
	    break;
    case IRV_CLEAN:
	ResetInput ();
	ResetInputWindow ();
	if (bAutoHideInputWindow)
	    XUnmapWindow (dpy, inputWindow);
	else
	    DisplayInputWindow ();
	break;
    case IRV_DISPLAY_CANDWORDS:
	bShowNext = bShowPrev = False;
	if (bIsInLegend) {
	    if (iCurrentLegendCandPage > 0)
		bShowPrev = True;
	    if (iCurrentLegendCandPage < iLegendCandPageCount)
		bShowNext = True;
	}
	else {
	    if (imeIndex == IME_ERBI) {
		if (iCodeInputCount == 1 && strCodeInput[0] == '`' && !iCandWordCount) {
		    uMessageUp = 1;
		    messageUp[0].strMsg[0] = '`';
		    messageUp[0].strMsg[1] = '\0';
		    messageUp[0].type = MSG_INPUT;
		    uMessageDown = 1;
		    strcpy (messageDown[0].strMsg, strStringGet);
		    messageDown[0].type = MSG_TIPS;
		}
		else {
		    if (iCurrentCandPage > 0)
			bShowPrev = True;
		    if (iCurrentCandPage < iCandPageCount)
			bShowNext = True;
		}
	    }
	    else {
		if (iCodeInputCount == 1 && strCodeInput[0] == 'z' && !iCandWordCount) {
		    uMessageUp = 1;
		    messageUp[0].strMsg[0] = 'z';
		    messageUp[0].strMsg[1] = '\0';
		    messageUp[0].type = MSG_INPUT;
		    uMessageDown = 1;
		    strcpy (messageDown[0].strMsg, strStringGet);
		    messageDown[0].type = MSG_TIPS;
		}
		else {
		    if (iCurrentCandPage > 0)
			bShowPrev = True;
		    if (iCurrentCandPage < iCandPageCount)
			bShowNext = True;
		}
	    }
	}

	DisplayInputWindow ();
	break;
    case IRV_DISPLAY_MESSAGE:
	bShowNext = False;
	bShowPrev = False;
	DisplayInputWindow ();
	break;
    case IRV_GET_LEGEND:
	UpdateHZLastInput ();
	SendHZtoClient (ims, call_data, strStringGet);
	if (iLegendCandWordCount) {
	    bShowNext = bShowPrev = False;
	    if (iCurrentLegendCandPage > 0)
		bShowPrev = True;
	    if (iCurrentLegendCandPage < iLegendCandPageCount)
		bShowNext = True;
	    bLastIsNumber = False;
	    iCodeInputCount = 0;
	    DisplayInputWindow ();
	}
	else {
	    ResetInput ();
	    if (bAutoHideInputWindow)
		XUnmapWindow (dpy, inputWindow);
	    else
		DisplayInputWindow ();
	}

	break;
    case IRV_GET_CANDWORDS:
	UpdateHZLastInput ();
	if (bPhraseTips)
	    DoPhraseTips ();
    case IRV_ENG:
    case IRV_PUNC:
	ResetInput ();
	if (bAutoHideInputWindow && (retVal == IRV_PUNC || (!bPhraseTips || (bPhraseTips && !lastIsSingleHZ))))
	    XUnmapWindow (dpy, inputWindow);
	else
	    DisplayInputWindow ();
    case IRV_GET_CANDWORDS_NEXT:
	if (retVal == IRV_GET_CANDWORDS_NEXT || lastIsSingleHZ == -1) {
	    UpdateHZLastInput ();
	    DisplayInputWindow ();
	}
	SendHZtoClient (ims, call_data, strStringGet);
	bLastIsNumber = False;
	lastIsSingleHZ = 0;

	break;
    default:			//避免编译器给一个警告
	;
    }

    //不知道这个函数能不能解决双字母的问题 ----- 看来是不行的了 -----暂时保留
    fflush (NULL);
}

Bool IsHotKey (int iKey, HOTKEYS * hotkey)
{
    if (iKey == hotkey[0] || iKey == hotkey[1])
	return True;
    return False;
}

void UpdateHZLastInput (void)
{
    int             iCount;
    int             i, j;

    iCount = strlen (strStringGet) / 2;

    for (i = 0; i < iCount; i++) {
	if (iHZLastInputCount < MAX_HZ_SAVED)
	    iHZLastInputCount++;
	else {
	    for (j = 0; j < (iHZLastInputCount - 1); j++) {
		hzLastInput[j].strHZ[0] = hzLastInput[j + 1].strHZ[0];
		hzLastInput[j].strHZ[1] = hzLastInput[j + 1].strHZ[1];
	    }
	}
	hzLastInput[iHZLastInputCount - 1].strHZ[0] = strStringGet[2 * i];
	hzLastInput[iHZLastInputCount - 1].strHZ[1] = strStringGet[2 * i + 1];
	hzLastInput[iHZLastInputCount - 1].strHZ[2] = '\0';
    }
}

INPUT_RETURN_VALUE ChangeCorner (void)
{
    ResetInput ();
    ResetInputWindow ();

    bCorner = !bCorner;
    DisplayMainWindow ();

    SaveProfile ();

    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE ChangePunc (void)
{
    bChnPunc = !bChnPunc;
    DisplayMainWindow ();

    SaveProfile ();

    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE ChangeGBK (void)
{
    bUseGBK = !bUseGBK;
    ResetInput ();
    ResetInputWindow ();

    DisplayMainWindow ();
    DisplayInputWindow ();

    SaveProfile ();

    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeLegend (void)
{
    bUseLegend = !bUseLegend;
    DisplayMainWindow ();

    SaveProfile ();

    return IRV_CLEAN;
}

void SwitchIME (BYTE index)
{
    if (index == (BYTE) - 1) {
	if (imeIndex == IME_ERBI)
	    imeIndex = IME_WUBI;
	else
	    imeIndex++;
    }
    else
	imeIndex = index;

    switch (imeIndex) {
    case IME_WUBI:
	DoInput = DoWBInput;
	GetCandWords = WBGetCandWords;
	GetCandWord = WBGetCandWord;
	PhraseTips = WBPhraseTips;
	break;
    case IME_PINYIN:
	DoInput = DoPYInput;
	GetCandWords = PYGetCandWords;
	GetCandWord = PYGetCandWord;
	PhraseTips = NULL;
	break;
    case IME_ERBI:
	DoInput = DoEBInput;
	GetCandWords = EBGetCandWords;
	GetCandWord = EBGetCandWord;
	PhraseTips = EBPhraseTips;
	break;
    }

    if (imeIndex == IME_PINYIN)
	MAINWND_WIDTH = _MAINWND_WIDTH + 18;
    else
	MAINWND_WIDTH = _MAINWND_WIDTH;

    XResizeWindow (dpy, mainWindow, MAINWND_WIDTH, MAINWND_HEIGHT);
    DisplayMainWindow();

    ResetInput ();
    XUnmapWindow (dpy, inputWindow);

    SaveProfile ();
}

void DoPhraseTips (void)
{
    char            strTemp[MAX_HZ_SAVED * 2 + 1];
    INT8            count;

    if (!PhraseTips)
	return;

    strTemp[0] = '\0';
    for (count = 0; count < iHZLastInputCount; count++)
	strcat (strTemp, hzLastInput[count].strHZ);

    if (PhraseTips (strTemp))
	lastIsSingleHZ = -1;
    else
	lastIsSingleHZ = 0;
}
