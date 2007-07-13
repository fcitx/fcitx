#include <ctype.h>

#ifdef _USE_XFT
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#endif

#include "xim.h"
#include "ime.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "IC.h"
#include "punc.h"
#include "py.h"
#include "sp.h"
#include "qw.h"
#include "table.h"
#include "tools.h"
#include "ui.h"

IM             *im = NULL;
INT8            iIMCount = 0;

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

Bool            bEngAfterSemicolon = False;
Bool            bEngAfterCap = True;
Bool            bDisablePagingInLegend = True;

int             i2ndSelectKey = 0;	//第二个候选词选择键，为扫描码
int             i3rdSelectKey = 0;	//第三个候选词选择键，为扫描码

Time            lastKeyPressedTime;

KEY_RELEASED    keyReleased = KR_OTHER;
Bool		bDoubleSwitchKey = False;
KEYCODE         switchKey = L_CTRL;

//热键定义
HOTKEYS         hkTrigger[HOT_KEY_COUNT] = { CTRL_SPACE, 0 };
HOTKEYS         hkGBK[HOT_KEY_COUNT] = { CTRL_M, 0 };
HOTKEYS         hkLegend[HOT_KEY_COUNT] = { CTRL_L, 0 };
HOTKEYS         hkCorner[HOT_KEY_COUNT] = { SHIFT_SPACE, 0 };	//全半角切换
HOTKEYS         hkPunc[HOT_KEY_COUNT] = { ALT_SPACE, 0 };	//中文标点
HOTKEYS         hkNextPage[HOT_KEY_COUNT] = { '.', 0 };	//下一页
HOTKEYS         hkPrevPage[HOT_KEY_COUNT] = { ',', 0 };	//上一页
HOTKEYS         hkTrack[HOT_KEY_COUNT] = { CTRL_K, 0 };

Bool            bUseLegend = False;
Bool            bIsInLegend = False;

INT8            iIMIndex = 0;
Bool            bUsePinyin = True;
Bool            bUseSP = False;
Bool            bUseQW = True;
Bool            bUseTable = True;

Bool            bLumaQQ = False;

//++++++++++++++++++++++++++++++++++++++++
/*
INT8		iKeyPressed = 0;
Bool		bRelease = True;
*/

extern XIMS     ims;
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
extern Bool     bTrackCursor;
extern Bool     bSingleHZMode;
extern Window   inputWindow;
extern HIDE_MAINWINDOW hideMainWindow;
extern XIMTriggerKey *Trigger_Keys;
extern Window   mainWindow;
extern int      iCursorPos;

extern int      MAINWND_WIDTH;
extern Bool     bLocked;
extern Bool     bCompactMainWindow;

extern INT8     iTableChanged;
extern INT8     iNewPYPhraseCount;
extern INT8     iOrderCount;
extern INT8     iNewFreqCount;
extern INT8     iTableOrderChanged;

extern TABLE   *table;
extern INT8     iTableCount;

extern Bool     bTrigger;

extern int      iInputWindowX;
extern int      iInputWindowY;
extern int      iTempInputWindowX;
extern int      iTempInputWindowY;

#ifdef _USE_XFT
extern XftFont *xftMainWindowFont;
#else
extern XFontSet fontSetMainWindow;
#endif
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

    if (IsIM (NAME_OF_PINYIN))
	bSingleHZMode = False;
    else
	bShowCursor = False;

    if (im[iIMIndex].ResetIM)
	im[iIMIndex].ResetIM ();
}

void CloseIM (IMForwardEventStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    IMPreeditEnd (ims, (XPointer) call_data);
    SetConnectID (call_data->connect_id, IS_CLOSED);
    DisplayMainWindow ();
}

void ChangeIMState(IMForwardEventStruct * call_data)
{
    if (ConnectIDGetState (call_data->connect_id) == IS_ENG) {
	SetConnectID (call_data->connect_id, IS_CHN);
	DisplayInputWindow ();
	
	if ( ConnectIDGetTrackCursor (call_data->connect_id) )
	    XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);
	else
	    XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);
    }
    else {
	SetConnectID (call_data->connect_id, IS_ENG);
	ResetInput ();
	ResetInputWindow ();
	XUnmapWindow (dpy, inputWindow);
    }
    if (hideMainWindow != HM_HIDE)
	DisplayMainWindow ();
}

//FILE           *fd;
void ProcessKey (IMForwardEventStruct * call_data)
{
    KeySym          keysym;
    XKeyEvent      *kev;
    int             keyCount;
    INPUT_RETURN_VALUE retVal;
    int             iKeyState;
    char            strbuf[STRBUFLEN];
    int             iKey;
    char           *pstr;
    int             iLen;
 //   IC		   *ic;

    kev = (XKeyEvent *) & call_data->event;
    memset (strbuf, 0, STRBUFLEN);
    keyCount = XLookupString (kev, strbuf, STRBUFLEN, &keysym, NULL);

    iKeyState = kev->state - (kev->state & KEY_NUMLOCK) - (kev->state & KEY_CAPSLOCK) - (kev->state & KEY_SCROLLLOCK);

    iKey = GetKey (keysym, iKeyState, keyCount);

    //printf("COUNT:%d STATE:%d KEYSYM:%d IKEY: %d(%c)\n",keyCount,iKeyState,(char)keysym,iKey,iKey);

    if (!iKey)
	return;

    /*
     * 解决xine中候选字自动选中的问题
     * xine每秒钟产生一个左SHIFT键的释放事件
     */
    /*ic=(IC *)FindIC (call_data->icid);
    /printf("%d %d %d %d %d %d\n",kev->same_screen,ic->focus_win,ic->client_win,kev->window,kev->root,kev->subwindow); */
    if (kev->same_screen && (kev->keycode == switchKey || kev->keycode == i2ndSelectKey || kev->keycode == i3rdSelectKey))
    	return;
    
    retVal = IRV_TO_PROCESS;

    if (call_data->event.type == KeyRelease) {
	if ((kev->time - lastKeyPressedTime) < 500 && (!bIsDoInputOnly)) {
	    if (!bLocked && iKeyState == KEY_CTRL_SHIFT_COMP && (iKey == 225 || iKey == 227)) {
		if (ConnectIDGetState (call_data->connect_id) == IS_CHN)
		    SwitchIM (-1);
		else if (IsHotKey (iKey, hkTrigger))
		    CloseIM (call_data);
	    }
	    else if (!bLocked && iKey == CTRL_LSHIFT) {
		if (ConnectIDGetState (call_data->connect_id) == IS_CHN)
		    SwitchIM (-1);
		else if (IsHotKey (iKey, hkTrigger))
		    CloseIM (call_data);
	    }
	    else if (kev->keycode == switchKey && keyReleased == KR_CTRL && !bDoubleSwitchKey) {
	        ChangeIMState(call_data);
		retVal = IRV_DO_NOTHING;
	    }
	    else if (kev->keycode == i2ndSelectKey && keyReleased == KR_2ND_SELECTKEY) {
		if (!bIsInLegend) {
		    pstr = im[iIMIndex].GetCandWord (1);
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
			retVal = IRV_TO_PROCESS;
		}
		else {
			strcpy(strStringGet, " ");
			uMessageDown = 0;
			retVal = IRV_GET_CANDWORDS;
		}
	    }
	    else if (kev->keycode == i3rdSelectKey && keyReleased == KR_3RD_SELECTKEY) {
		if (!bIsInLegend) {
		    pstr = im[iIMIndex].GetCandWord (2);
		    if (pstr) {
			strcpy (strStringGet, pstr);
			if (bIsInLegend)
			    retVal = IRV_GET_LEGEND;
			else
			    retVal = IRV_GET_CANDWORDS;
		    }
		    else if (iCandWordCount)
			retVal = IRV_DISPLAY_CANDWORDS;
		}
		else {
			strcpy(strStringGet, "　");
			uMessageDown = 0;
			retVal = IRV_GET_CANDWORDS;
		}
	    }
	}
    }

    if (retVal == IRV_TO_PROCESS) {
	if (call_data->event.type == KeyPress) {
	    if (kev->keycode != switchKey )
		keyReleased = KR_OTHER;
	    else {
	        if ( (keyReleased == KR_CTRL) && (kev->time-lastKeyPressedTime<250) && bDoubleSwitchKey) 
		    ChangeIMState(call_data);
	    }
	    
	    lastKeyPressedTime = kev->time;
	    if (kev->keycode == switchKey) {
		keyReleased = KR_CTRL;
		//retVal = (bDoubleSwitchKey)? IRV_CLEAN:IRV_DO_NOTHING;
		retVal = IRV_DO_NOTHING;
	    }
	    else if (IsHotKey (iKey, hkTrigger)) {
		if (ConnectIDGetState (call_data->connect_id) == IS_ENG) {
		    SetConnectID (call_data->connect_id, IS_CHN);
		    DisplayInputWindow ();
		    
		    if ( ConnectIDGetTrackCursor (call_data->connect_id) )
			XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);
		    else
			XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);

		    if (hideMainWindow != HM_HIDE)
			DisplayMainWindow ();

		}
		else {
			CloseIM (call_data);
			retVal = IRV_DO_NOTHING;
		}
	    }
	}
	if (retVal == IRV_TO_PROCESS && (ConnectIDGetState (call_data->connect_id) == IS_CHN)) {
	    if (call_data->event.type == KeyPress) {
		if (iKeyState == KEY_NONE) {
		    if (kev->keycode == i2ndSelectKey)
			keyReleased = KR_2ND_SELECTKEY;
		    else if (kev->keycode == i3rdSelectKey)
			keyReleased = KR_3RD_SELECTKEY;
		}

		if (iKey == CTRL_LSHIFT || iKey == SHIFT_LCTRL) {
		    if (bLocked)
			retVal = IRV_TO_PROCESS;
		}
		else {
		    if (!bInCap && !bCorner) {
			retVal = im[iIMIndex].DoInput (iKey);
			if (!IsIM (NAME_OF_PINYIN) && !IsIM (NAME_OF_SHUANGPIN))
			    iCursorPos = iCodeInputCount;
		    }

		    if (!bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
			if (bCorner && (iKey >= 32 && iKey <= 126)) {
			    //有人报 空格 的全角不对，正确的是0xa1 0xa1
			    //但查资料却说全角符号总是以0xa3开始。
			    //由于0xa3 0xa0可能会显示乱码，因此采用0xa1 0xa1的方式
			    if (iKey == ' ')
				sprintf (strStringGet, "%c%c", 0xa1, 0xa1);
			    else
				sprintf (strStringGet, "%c%c", 0xa3, 0xa0 + iKey - 32);
			    retVal = IRV_GET_CANDWORDS;
			}
			else if (iKey >= 'A' && iKey <= 'Z' && bEngAfterCap && !(kev->state & KEY_CAPSLOCK)) {
			    bInCap = True;
			    if (!bIsInLegend && iCandWordCount) {
				pstr = im[iIMIndex].GetCandWord (0);
				iCandWordCount = 0;
				if (pstr) {
				    strcpy (strStringGet, pstr);
				    SendHZtoClient (call_data, strStringGet);
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
			else if (IsHotKey (iKey, hkPrevPage))
			    retVal = im[iIMIndex].GetCandWords (SM_PREV);
			else if (IsHotKey (iKey, hkNextPage))
			    retVal = im[iIMIndex].GetCandWords (SM_NEXT);
			else if (IsHotKey (iKey, hkLegend))
			    retVal = ChangeLegend ();
			else if (IsHotKey (iKey, hkTrack))
			    retVal = ChangeTrack ();

			if (retVal == IRV_TO_PROCESS) {
			    if (bInCap) {
				if (iKey == ' ') {
				    if (iCodeInputCount == 0)
					strcpy (strStringGet, "；");
				    else
					strcpy(strStringGet,strCodeInput);
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
					    bShowCursor = True;
					    iCursorPos = iCodeInputCount;
					}
					retVal = IRV_DISPLAY_MESSAGE;
				    }
				}
				else if (iKey == (XK_BackSpace & 0x00FF)
					 && iCodeInputCount) {
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
				    strcpy (messageDown[0].strMsg, "按 Enter/空格 输入英文");
				}
				messageUp[0].type = MSG_INPUT;
				messageDown[0].type = MSG_TIPS;
			    }
			    else if ((bLastIsNumber && bEngPuncAfterNumber)
				     && (iKey == '.' || iKey == ',')
				     && !iCandWordCount) {
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
					    pstr = im[iIMIndex].GetCandWord (0);
					if (pstr)
					    strcpy (strStringGet, pstr);
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
						    pstr = im[iIMIndex].GetCandWord (0);
						if (pstr)
						    strcpy (strStringGet, pstr);
						iLen = strlen (strStringGet);
						uMessageDown = uMessageUp = 0;
						strStringGet[iLen] = iKey;
						strStringGet[iLen + 1] = '\0';
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
				retVal = IRV_DONOT_PROCESS;
			}
			else if (iKey == CTRL_5) {
			    SetIM ();
			    LoadConfig (False);

			    if (bLumaQQ)
				ConnectIDResetReset ();

			    retVal = IRV_DO_NOTHING;
			}
			else if (iKey == ENTER) {
			    if (bInCap) {
				if (bEngAfterSemicolon && !iCodeInputCount)
				    strcpy (strStringGet, ";");
				else
				    strcpy (strStringGet, strCodeInput);
				retVal = IRV_ENG;
				uMessageUp = uMessageDown = 0;
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
	    else if (call_data->event.type != KeyPress)
		retVal = IRV_DONOT_PROCESS;
	}
    }

    switch (retVal) {
    case IRV_DO_NOTHING:
	break;
    case IRV_TO_PROCESS:
    case IRV_DONOT_PROCESS:
    case IRV_DONOT_PROCESS_CLEAN:
	if (call_data->event.type == KeyRelease) {
	    if (!bLumaQQ && (!keyCount || (!iKeyState && (iKey == ESC || iKey == ENTER))))
		IMForwardEvent (ims, (XPointer) call_data);
	}
	else
	    IMForwardEvent (ims, (XPointer) call_data);

	if (retVal != IRV_DONOT_PROCESS_CLEAN)
	    return;
    case IRV_CLEAN:
	ResetInput ();
	ResetInputWindow ();
	if (bAutoHideInputWindow)
	    XUnmapWindow (dpy, inputWindow);
	else
	    DisplayInputWindow ();

	return;
    case IRV_DISPLAY_CANDWORDS:
	bShowNext = bShowPrev = False;
	if (bIsInLegend) {
	    if (iCurrentLegendCandPage > 0)
		bShowPrev = True;
	    if (iCurrentLegendCandPage < iLegendCandPageCount)
		bShowNext = True;
	}
	else {
	    if (iCurrentCandPage > 0)
		bShowPrev = True;
	    if (iCurrentCandPage < iCandPageCount)
		bShowNext = True;
	}

	DisplayInputWindow ();
	break;
    case IRV_DISPLAY_LAST:
	bShowNext = bShowPrev = False;
	uMessageUp = 1;
	messageUp[0].strMsg[0] = strCodeInput[0];
	messageUp[0].strMsg[1] = '\0';
	messageUp[0].type = MSG_INPUT;
	uMessageDown = 1;
	strcpy (messageDown[0].strMsg, strStringGet);
	messageDown[0].type = MSG_TIPS;
	DisplayInputWindow ();
	break;
    case IRV_DISPLAY_MESSAGE:
	bShowNext = False;
	bShowPrev = False;
	DisplayInputWindow ();
	break;
    case IRV_GET_LEGEND:
	SendHZtoClient (call_data, strStringGet);
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
	if (bPhraseTips && im[iIMIndex].PhraseTips)
	    DoPhraseTips ();
    case IRV_ENG:
    case IRV_PUNC:
	ResetInput ();
	if (!(uMessageDown && retVal == IRV_GET_CANDWORDS)
	    && bAutoHideInputWindow && (retVal == IRV_PUNC || (!bPhraseTips || (bPhraseTips && !lastIsSingleHZ))))
	    XUnmapWindow (dpy, inputWindow);
	else
	    DisplayInputWindow ();
    case IRV_GET_CANDWORDS_NEXT:
	if (retVal == IRV_GET_CANDWORDS_NEXT || lastIsSingleHZ == -1)
	    DisplayInputWindow ();
	SendHZtoClient (call_data, strStringGet);
	bLastIsNumber = False;
	lastIsSingleHZ = 0;

	break;
    default:
	;
    }
}

Bool IsHotKey (int iKey, HOTKEYS * hotkey)
{
    if (iKey == hotkey[0] || iKey == hotkey[1])
	return True;
    return False;
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
    XUnmapWindow (dpy, inputWindow);

    SaveProfile ();

    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeLegend (void)
{
    bUseLegend = !bUseLegend;
    ResetInput ();
    ResetInputWindow ();

    DisplayMainWindow ();
    XUnmapWindow (dpy, inputWindow);

    SaveProfile ();

    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeTrack (void)
{
    bTrackCursor = !bTrackCursor;
    SaveProfile ();

    return IRV_DO_NOTHING;;
}

void SwitchIM (INT8 index)
{
    INT8            iLastIM;

    iLastIM = (iIMIndex >= iIMCount) ? (iIMCount - 1) : iIMIndex;

    if (index == (INT8) - 1) {
	if (iIMIndex == (iIMCount - 1))
	    iIMIndex = 0;
	else
	    iIMIndex++;
    }
    else {
	if (index >= iIMCount)
	    iIMIndex = iIMCount - 1;
    }

#ifdef _USE_XFT
    MAINWND_WIDTH = ((bCompactMainWindow) ? _MAINWND_WIDTH_COMPACT : _MAINWND_WIDTH) + StringWidth (im[iIMIndex].strName, xftMainWindowFont) + 4;
#else
    MAINWND_WIDTH = ((bCompactMainWindow) ? _MAINWND_WIDTH_COMPACT : _MAINWND_WIDTH) + StringWidth (im[iIMIndex].strName, fontSetMainWindow) + 4;
#endif
    XResizeWindow (dpy, mainWindow, MAINWND_WIDTH, MAINWND_HEIGHT);

    DisplayMainWindow ();

    if (iIMCount == 1)
	return;

    if (im[iLastIM].Destroy != NULL)
	im[iLastIM].Destroy ();

    ResetInput ();
    XUnmapWindow (dpy, inputWindow);

    SaveProfile ();

    if (im[iIMIndex].Init)
	im[iIMIndex].Init ();
}

void DoPhraseTips (void)
{
    if (!bPhraseTips)
	return;

    if (im[iIMIndex].PhraseTips ())
	lastIsSingleHZ = -1;
    else
	lastIsSingleHZ = 0;
}

/*
#define _DEBUG
*/
void RegisterNewIM (char *strName, void (*ResetIM) (void),
		    INPUT_RETURN_VALUE (*DoInput) (int), INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE), char *(*GetCandWord) (int), char *(*GetLegendCandWord) (int), Bool (*PhraseTips) (void), void (*Init) (void), void (*Destroy) (void))
{
#ifdef _DEBUG
    printf ("REGISTER %s\n", strName);
#endif
    strcpy (im[iIMCount].strName, strName);
    im[iIMCount].ResetIM = ResetIM;
    im[iIMCount].DoInput = DoInput;
    im[iIMCount].GetCandWords = GetCandWords;
    im[iIMCount].GetCandWord = GetCandWord;
    im[iIMCount].GetLegendCandWord = GetLegendCandWord;
    im[iIMCount].PhraseTips = PhraseTips;
    im[iIMCount].Init = Init;
    im[iIMCount].Destroy = Destroy;

    iIMCount++;
}

Bool IsIM (char *strName)
{
    if (strstr (im[iIMIndex].strName, strName))
	return True;

    return False;
}

void SaveIM (void)
{
    if (iTableChanged || iTableOrderChanged)
	SaveTableDict ();
    if (iNewPYPhraseCount)
	SavePYUserPhrase ();
    if (iOrderCount)
	SavePYIndex ();
    if (iNewFreqCount)
	SavePYFreq ();
}

void SetIM (void)
{
    INT8            i;

    if (im)
	free (im);

    if (bUseTable)
	LoadTableInfo ();

    iIMCount = iTableCount;
    if (bUsePinyin)
	iIMCount++;
    if (bUseSP)
	iIMCount++;
    if (bUseQW)
	iIMCount++;

    im = (IM *) malloc (sizeof (IM) * iIMCount);
    iIMCount = 0;

    /* 加入输入法 */
    if (bUsePinyin || (!bUseSP && (!bUseTable || !iTableCount)))	//至少应该有一种输入法
	RegisterNewIM (NAME_OF_PINYIN, ResetPYStatus, DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL, PYInit, NULL);
    if (bUseSP)
	RegisterNewIM (NAME_OF_SHUANGPIN, ResetPYStatus, DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL, SPInit, NULL);
    if (bUseQW)
	RegisterNewIM (NAME_OF_QUWEI, NULL, DoQWInput, QWGetCandWords, QWGetCandWord, NULL, NULL, NULL, NULL);
    if (bUseTable) {
	for (i = 0; i < iTableCount; i++) {
	    RegisterNewIM (table[i].strName, TableResetStatus, DoTableInput, TableGetCandWords, TableGetCandWord, TableGetLegendCandWord, TablePhraseTips, TableInit, FreeTableIM);
	    table[i].iIMIndex = iIMCount - 1;
	}
    }

    SwitchIM (iIMIndex);
}
