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
#include <ctype.h>
#include <time.h>

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
#include "vk.h"
#include "QuickPhrase.h"

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
INT8            iInCap = 0;	//是不是处于大写后的英文状态,0--不，1--按下大写键，2--按下分号键
Bool            bAutoHideInputWindow = False;	//是否自动隐藏输入条
Bool            bEngPuncAfterNumber = True;	//数字后面输出半角符号(只对'.'/','有效)
Bool            bPhraseTips = True;
INT8            lastIsSingleHZ = 0;
Bool            bUseGBKT = False;

SEMICOLON_TO_DO semicolonToDo = K_SEMICOLON_QUICKPHRASE;
Bool            bEngAfterCap = True;
Bool            bConvertPunc = True;
Bool            bDisablePagingInLegend = True;

Bool            bVK = False;

int             i2ndSelectKey = 0;	//第二个候选词选择键，为扫描码
int             i3rdSelectKey = 0;	//第三个候选词选择键，为扫描码

Time            lastKeyPressedTime;
unsigned int    iTimeInterval = 250;

KEY_RELEASED    keyReleased = KR_OTHER;
Bool            bDoubleSwitchKey = False;
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
Bool            bUseSP = True;
Bool            bUseQW = True;
Bool            bUseTable = True;

/* 新的LumaQQ已经不需要特意支持了
Bool            bLumaQQ = False;
*/
Bool            bPointAfterNumber = True;

/* 计算打字速度的功能没有什么用处，去掉好了 */
/* 计算打字速度 */
/*
time_t          timeStart;
Bool            bStartRecordType;
Bool            bShowUserSpeed = True;
uint            iHZInputed = 0;
*/

/* ************************ */

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

extern Window   VKWindow;
extern VKS      vks[];
extern unsigned char iCurrentVK;
extern Bool     bVK;

extern int      MAINWND_WIDTH;
extern Bool     bLocked;
extern Bool     bCompactMainWindow;
extern Bool     bShowVK;

extern INT8     iTableChanged;
extern INT8     iNewPYPhraseCount;
extern INT8     iOrderCount;
extern INT8     iNewFreqCount;
extern INT16    iTableOrderChanged;

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
    iInCap = 0;

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
    XUnmapWindow (dpy, VKWindow);
    IMPreeditEnd (ims, (XPointer) call_data);
    SetConnectID (call_data->connect_id, IS_CLOSED);
    bVK = False;
    SwitchIM (-2);
    DisplayMainWindow ();
}

void ChangeIMState (CARD16 _connect_id)
{
    if (ConnectIDGetState (_connect_id) == IS_ENG) {
	SetConnectID (_connect_id, IS_CHN);
	if (bVK)
	    DisplayVKWindow ();
	else
	    DisplayInputWindow ();

	if (ConnectIDGetTrackCursor (_connect_id))
	    XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);
	else
	    XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);
    }
    else {
	SetConnectID (_connect_id, IS_ENG);
	ResetInput ();
	ResetInputWindow ();
	XUnmapWindow (dpy, inputWindow);
	XUnmapWindow (dpy, VKWindow);
    }

    if (hideMainWindow != HM_HIDE)
	DisplayMainWindow ();
}

/*
 * 转换strStringGet中的标点为中文标点
 */
void ConvertPunc (void)
{
    char            strTemp[MAX_USER_INPUT + 1] = "\0";
    char           *s1, *s2, *pPunc;

    s1 = strTemp;
    s2 = strStringGet;

    while (*s2) {
	pPunc = GetPunc (*s2);
	if (pPunc) {
	    strcat (s1, pPunc);
	    s1 += strlen (pPunc);
	}
	else
	    *s1++ = *s2;
	s2++;
    }
    *s2 = '\0';

    strcpy (strStringGet, strTemp);
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

    kev = (XKeyEvent *) & call_data->event;
    memset (strbuf, 0, STRBUFLEN);
    keyCount = XLookupString (kev, strbuf, STRBUFLEN, &keysym, NULL);

    iKeyState = kev->state - (kev->state & KEY_NUMLOCK) - (kev->state & KEY_CAPSLOCK) - (kev->state & KEY_SCROLLLOCK);
    iKey = GetKey (keysym, iKeyState, keyCount);

    if (!iKey)
	return;

    /*
     * 解决xine中候选字自动选中的问题
     * xine每秒钟产生一个左SHIFT键的释放事件
     */
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
		ChangeIMState (call_data->connect_id);
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
		    strcpy (strStringGet, " ");
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
		    strcpy (strStringGet, "　");
		    uMessageDown = 0;
		    retVal = IRV_GET_CANDWORDS;
		}
	    }
	}
    }

    if (retVal == IRV_TO_PROCESS) {
	if (call_data->event.type == KeyPress) {
	    if (kev->keycode != switchKey)
		keyReleased = KR_OTHER;
	    else {
		if ((keyReleased == KR_CTRL) && (kev->time - lastKeyPressedTime < iTimeInterval) && bDoubleSwitchKey)
		    ChangeIMState (call_data->connect_id);
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

		    if (ConnectIDGetTrackCursor (call_data->connect_id))
			XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);
		    else
			XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);

		    if (hideMainWindow != HM_HIDE)
			DisplayMainWindow ();

		}
		else
		    CloseIM (call_data);

		retVal = IRV_DO_NOTHING;
	    }
	}

	if (retVal == IRV_TO_PROCESS) {
	    if (call_data->event.type == KeyPress) {
		if (ConnectIDGetState (call_data->connect_id) == IS_CHN) {
		    if (bVK)
			retVal = DoVKInput (iKey);
		    else {
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
			    //调用输入法模块
			    if (!iInCap) {
				retVal = im[iIMIndex].DoInput (iKey);
				if (!IsIM (NAME_OF_PINYIN) && !IsIM (NAME_OF_SHUANGPIN))
				    iCursorPos = iCodeInputCount;
			    }
			    else if (iInCap == 2 && semicolonToDo == K_SEMICOLON_QUICKPHRASE) {
				retVal = QuickPhraseDoInput (iKey);
			    }

			    if (!bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
				if (iKey >= 'A' && iKey <= 'Z' && bEngAfterCap && !(kev->state & KEY_CAPSLOCK)) {
				    iInCap = 1;
				    if (!bIsInLegend && iCandWordCount) {
					pstr = im[iIMIndex].GetCandWord (0);
					iCandWordCount = 0;
					if (pstr) {
					    strcpy (strStringGet, pstr);
					    //粗略统计字数
					    // iHZInputed += (int) (strlen (strStringGet) / 2);
					    SendHZtoClient (call_data, strStringGet);
					    iCodeInputCount = 0;
					}
				    }
				}
				else if (iKey == ';' && semicolonToDo != K_SEMICOLON_NOCHANGE && !iCodeInputCount)
				    iInCap = 2;
				else if (IsHotKey (iKey, hkPrevPage)) {
				    if (iInCap == 2 && semicolonToDo == K_SEMICOLON_QUICKPHRASE)
					retVal = QuickPhraseGetCandWords (SM_PREV);
				    else
					retVal = im[iIMIndex].GetCandWords (SM_PREV);
				}
				else if (IsHotKey (iKey, hkNextPage)) {
				    if (iInCap == 2 && semicolonToDo == K_SEMICOLON_QUICKPHRASE)
					retVal = QuickPhraseGetCandWords (SM_NEXT);
				    else
					retVal = im[iIMIndex].GetCandWords (SM_NEXT);
				}

				if (retVal == IRV_TO_PROCESS) {
				    if (iInCap) {
					if (iKey == ' ') {
					    if (iCodeInputCount == 0)
						strcpy (strStringGet, "；");
					    else
						strcpy (strStringGet, strCodeInput);
					    retVal = IRV_ENG;
					    uMessageUp = uMessageDown = 0;
					    iInCap = 0;
					}
					else {
					    if (isprint (iKey) && iKey < 128) {
						if (iCodeInputCount == MAX_USER_INPUT)
						    retVal = IRV_DO_NOTHING;
						else {
						    if (!(iInCap == 2 && !iCodeInputCount && iKey == ';')) {
							strCodeInput[iCodeInputCount++] = iKey;
							strCodeInput[iCodeInputCount] = '\0';
							bShowCursor = True;
							iCursorPos = iCodeInputCount;
							if (semicolonToDo == K_SEMICOLON_QUICKPHRASE && iInCap == 2)
							    retVal = QuickPhraseGetCandWords (SM_FIRST);
							else
							    retVal = IRV_DISPLAY_MESSAGE;
						    }
						    else
							retVal = IRV_DISPLAY_MESSAGE;
						}
					    }
					    else if ((iKey == (XK_BackSpace & 0x00FF) || iKey == CTRL_H) && iCodeInputCount) {
						iCodeInputCount--;
						strCodeInput[iCodeInputCount] = '\0';
						iCursorPos = iCodeInputCount;
						retVal = IRV_DISPLAY_MESSAGE;
						if (!iCodeInputCount)
						    retVal = IRV_CLEAN;
						else if (semicolonToDo == K_SEMICOLON_QUICKPHRASE && iInCap == 2)
						    retVal = QuickPhraseGetCandWords (SM_FIRST);
					    }

					    uMessageUp = 1;
					    if (iInCap == 2) {
						strcpy (messageUp[0].strMsg, strCodeInput);
						if (semicolonToDo == K_SEMICOLON_ENG)
						    strcat (messageUp[0].strMsg, "英文输入：");
						else {
						    if (iCodeInputCount)
							strcat (messageUp[0].strMsg, "  ");
						    strcat (messageUp[0].strMsg, "自定义输入");
						}
						if (!iCodeInputCount) {
						    strcpy (messageDown[0].strMsg, "空格输入；Enter输入;");
						    uMessageDown = 1;
						    messageDown[0].type = MSG_TIPS;
						}
					    }
					    else {
						uMessageDown = 1;
						messageDown[0].type = MSG_TIPS;
						strcpy (messageUp[0].strMsg, strCodeInput);
						strcpy (messageDown[0].strMsg, "按 Enter/空格 输入英文");
					    }
					    messageUp[0].type = MSG_INPUT;
					}
				    }
				    else if ((bLastIsNumber && bEngPuncAfterNumber) && (iKey == '.' || iKey == ',') && !iCandWordCount) {
					retVal = IRV_TO_PROCESS;
					bLastIsNumber = False;
				    }
				    else {
					if (bChnPunc) {
					    char           *pPunc;

					    pstr = NULL;
					    pPunc = GetPunc (iKey);
					    if (pPunc) {
						strStringGet[0] = '\0';
						if (!bIsInLegend)
						    pstr = im[iIMIndex].GetCandWord (0);
						if (pstr)
						    strcpy (strStringGet, pstr);
						strcat (strStringGet, pPunc);
						uMessageDown = uMessageUp = 0;

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
				    if (iCodeInputCount || iInCap || bIsInLegend)
					retVal = IRV_CLEAN;
				    else
					retVal = IRV_DONOT_PROCESS;
				}
				else if (iKey == CTRL_5) {
				    LoadConfig (False);
				    SetIM ();

				    /*if (bLumaQQ)
				       ConnectIDResetReset (); */

				    CreateFont ();
				    SwitchIM (iIMIndex);
				    CalculateInputWindowHeight ();

				    FreeQuickPhrase ();
				    LoadQuickPhrase ();

				    FreePunc ();
				    LoadPuncDict ();

				    retVal = IRV_DO_NOTHING;
				}
				else if (iKey == ENTER) {
				    if (iInCap) {
					if (!iCodeInputCount)
					    strcpy (strStringGet, ";");
					else
					    strcpy (strStringGet, strCodeInput);
					retVal = IRV_PUNC;
					uMessageUp = uMessageDown = 0;
					iInCap = 0;
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
					    uMessageDown = uMessageUp = 0;
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
		else {
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
		}

		if (retVal == IRV_TO_PROCESS || retVal == IRV_DONOT_PROCESS) {
		    if (IsHotKey (iKey, hkCorner))
			retVal = ChangeCorner ();
		    else if (IsHotKey (iKey, hkPunc))
			retVal = ChangePunc ();
		    else if (IsHotKey (iKey, hkGBK))
			retVal = ChangeGBK ();
		    else if (IsHotKey (iKey, hkLegend))
			retVal = ChangeLegend ();
		    else if (IsHotKey (iKey, hkTrack))
			retVal = ChangeTrack ();
		}
	    }
	    else
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
	    // if (!bLumaQQ && (!keyCount || (!iKeyState && (iKey == ESC || iKey == ENTER))))
	    if (!keyCount || (!iKeyState && (iKey == ESC || iKey == ENTER)))
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
	// iHZInputed += (int) (strlen (strStringGet) / 2);     //粗略统计字数
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
	if (bPhraseTips && im[iIMIndex].PhraseTips && !bVK)
	    DoPhraseTips ();
    case IRV_ENG:
	//如果处于中文标点模式，应该将其中的标点转换为全角
	if (retVal != IRV_GET_CANDWORDS && bChnPunc && bConvertPunc)
	    ConvertPunc ();
    case IRV_PUNC:
	//iHZInputed += (int) (strlen (strStringGet) / 2);      //粗略统计字数
	ResetInput ();
	if (bVK || (!(uMessageDown && retVal == IRV_GET_CANDWORDS)
		    && bAutoHideInputWindow && (retVal == IRV_PUNC || (!bPhraseTips || (bPhraseTips && !lastIsSingleHZ)))))
	    XUnmapWindow (dpy, inputWindow);
	else if (ConnectIDGetState (call_data->connect_id) == IS_CHN)
	    DisplayInputWindow ();
    case IRV_GET_CANDWORDS_NEXT:
	if (retVal == IRV_GET_CANDWORDS_NEXT || lastIsSingleHZ == -1) {
	    // iHZInputed += (int) (strlen (strStringGet) / 2);  //粗略统计字数
	    DisplayInputWindow ();
	}

	SendHZtoClient (call_data, strStringGet);
	bLastIsNumber = False;
	lastIsSingleHZ = 0;

	break;
    default:
	;
    }

    /* 计算打字速度的功能
       if (retVal == IRV_DISPLAY_MESSAGE || retVal == IRV_DISPLAY_CANDWORDS || retVal == IRV_PUNC) {
       if (!bStartRecordType) {
       bStartRecordType = True;
       timeStart = time (NULL);
       }
       }
     */
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

INPUT_RETURN_VALUE ChangeGBKT (void)
{
    bUseGBKT = !bUseGBKT;
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

    if (index != (INT8) - 2 && bVK)
	return;

    iLastIM = (iIMIndex >= iIMCount) ? (iIMCount - 1) : iIMIndex;
    if (index == (INT8) - 1) {
	if (iIMIndex == (iIMCount - 1))
	    iIMIndex = 0;
	else
	    iIMIndex++;
    }
    else if (index != (INT8) - 2) {
	if (index >= iIMCount)
	    iIMIndex = iIMCount - 1;
    }

#ifdef _USE_XFT
    MAINWND_WIDTH = ((bCompactMainWindow) ? _MAINWND_WIDTH_COMPACT : _MAINWND_WIDTH) + StringWidth ((bVK) ? vks[iCurrentVK].strName : im[iIMIndex].strName, xftMainWindowFont) + 4;
#else
    MAINWND_WIDTH = ((bCompactMainWindow) ? _MAINWND_WIDTH_COMPACT : _MAINWND_WIDTH) + StringWidth ((bVK) ? vks[iCurrentVK].strName : im[iIMIndex].strName, fontSetMainWindow) + 4;
#endif
    if (!bShowVK && bCompactMainWindow)
	MAINWND_WIDTH -= 24;

    XResizeWindow (dpy, mainWindow, MAINWND_WIDTH, MAINWND_HEIGHT);

    DisplayMainWindow ();

    if (im[iIMIndex].Init)
	im[iIMIndex].Init ();

    if (iIMCount == 1)
	return;

    if (im[iLastIM].Destroy != NULL)
	im[iLastIM].Destroy ();

    ResetInput ();
    XUnmapWindow (dpy, inputWindow);

    SaveProfile ();
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
