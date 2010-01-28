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
/*
 * @file   ime.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  按键和输入法通用功能处理
 *
 *
 */

#include <ctype.h>
#include <time.h>

#ifdef _USE_XFT
#include <ft2build.h>
#include <X11/Xft/Xft.h>
#endif

#include "xim.h"
#include "ime.h"
#include "about.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"
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
#include "AutoEng.h"
#include "extra.h"

#include "DBus.h"

IM             *im = NULL;
INT8            iIMCount = 0;
INT8		iState = 0;

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
char		cLastIsAutoConvert = 0;	//上一次输入是不是符合数字后自动转换的符号，如'.'/','，0表示不是这样的符号
INT8            iInCap = 0;		//是不是处于大写后的英文状态,0--不，1--按下大写键，2--按下分号键

/*
Bool            bAutoHideInputWindow = False;	//是否自动隐藏输入条
*/
Bool            bEngPuncAfterNumber = True;	//数字后面输出半角符号(只对'.'/','有效)
Bool            bPhraseTips = True;
INT8            lastIsSingleHZ = 0;
Bool            bUseGBKT = False;

SEMICOLON_TO_DO semicolonToDo = K_SEMICOLON_QUICKPHRASE;
Bool            bEngAfterCap = True;
Bool            bConvertPunc = True;
Bool            bDisablePagingInLegend = True;

Bool            bVK = False;

int             i2ndSelectKey = 50;	//第二个候选词选择键，为扫描码-默认是SHIFT
int             i3rdSelectKey = 62;	//第三个候选词选择键，为扫描码

Time            lastKeyPressedTime;
unsigned int    iTimeInterval = 250;

KEY_RELEASED    keyReleased = KR_OTHER;
Bool            bDoubleSwitchKey = False;
KEY_CODE        switchKey = L_CTRL;

//热键定义
HOTKEYS         hkTrigger[HOT_KEY_COUNT] = { CTRL_SPACE, 0 };
HOTKEYS         hkGBK[HOT_KEY_COUNT] = { CTRL_M, 0 };
HOTKEYS         hkLegend[HOT_KEY_COUNT] = { CTRL_L, 0 };
HOTKEYS         hkCorner[HOT_KEY_COUNT] = { SHIFT_SPACE, 0 };	//全半角切换
HOTKEYS         hkPunc[HOT_KEY_COUNT] = { ALT_SPACE, 0 };	//中文标点
HOTKEYS         hkNextPage[HOT_KEY_COUNT] = { '.', 0 };	//下一页
HOTKEYS         hkPrevPage[HOT_KEY_COUNT] = { ',', 0 };	//上一页
HOTKEYS         hkTrack[HOT_KEY_COUNT] = { CTRL_K, 0 };
HOTKEYS         hkGBT[HOT_KEY_COUNT] = { CTRL_ALT_F, 0 };
HOTKEYS         hkHideMainWindow[HOT_KEY_COUNT] = { CTRL_ALT_H, 0 };
HOTKEYS         hkSaveAll[HOT_KEY_COUNT] = { CTRL_ALT_S, 0 };
HOTKEYS         hkVK[HOT_KEY_COUNT] = { CTRL_ALT_K, 0 };

#ifdef _ENABLE_RECORDING
HOTKEYS         hkRecording[HOT_KEY_COUNT] = { CTRL_ALT_J, 0 };
HOTKEYS		hkResetRecording[HOT_KEY_COUNT] = { CTRL_ALT_A, 0 };
#endif

Bool            bUseLegend = False;
Bool            bIsInLegend = False;

INT8            iIMIndex = 0;
Bool            bLocked = False;

// dgod extern im
char		strExternIM[PATH_MAX];

Bool            bPointAfterNumber = True;

/* 计算打字速度 */
time_t          timeStart;
Bool            bStartRecordType;
Bool            bShowUserSpeed = True;
Bool            bShowVersion = True;
uint            iHZInputed = 0;

char            strNameOfPinyin[41] = "智能拼音";
char            strNameOfShuangpin[41] = "智能双拼";
char            strNameOfQuwei[41] = "区位";

Bool		bCursorAuto=False;

int		inputMethods[INPUT_METHODS];

extern XIMS     ims;
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

extern Window   inputWindow;
extern HIDE_MAINWINDOW hideMainWindow;
extern XIMTriggerKey *Trigger_Keys;
extern Window   mainWindow;
extern int      iCursorPos;

extern Window   VKWindow;
extern VKS      vks[];
extern unsigned char iCurrentVK;
extern Bool     bVK;

extern Window   aboutWindow;

extern int      MAINWND_WIDTH;
extern Bool     bCompactMainWindow;
extern Bool     bShowVK;

extern INT8     iTableChanged;
extern INT8     iNewPYPhraseCount;
extern INT8     iOrderCount;
extern INT8     iNewFreqCount;

extern TABLE   *table;
extern INT8     iTableCount;

extern Bool     bTrigger;

extern int      iInputWindowX;
extern int      iInputWindowY;

extern Bool     bShowInputWindowTriggering;
extern Bool	bMainWindow_Hiden;
extern char    *strFullCorner;

extern Bool	bUseDBus;
extern CARD16	connect_id;

#ifdef _ENABLE_DBUS
extern Property logo_prop;
extern Property state_prop;
extern Property punc_prop;
extern Property corner_prop;
extern Property gbk_prop;
extern Property gbkt_prop;
extern Property legend_prop;
#endif


#ifdef _USE_XFT
extern XftFont *xftMainWindowFont;
#else
extern XFontSet fontSetMainWindow;
#endif

#ifdef _ENABLE_RECORDING
extern FILE	*fpRecord;
extern Bool	bRecording;
extern Bool 	bWrittenRecord;
#endif

#ifdef _ENABLE_TRAY
extern tray_win_t tray;
extern Bool	tray_mapped;
#endif

void ResetInput (void)
{
    iCandPageCount = 0;
    iCurrentCandPage = 0;
    iCandWordCount = 0;
    iLegendCandWordCount = 0;
    iCurrentLegendCandPage = 0;
    iLegendCandPageCount = 0;
    iCursorPos = 0;
    uMessageUp = uMessageDown = 0;

    strCodeInput[0] = '\0';
    iCodeInputCount = 0;

    bIsDoInputOnly = False;

    bShowPrev = False;
    bShowNext = False;

    bIsInLegend = False;
    iInCap = 0;

    if (!IsIM (strNameOfPinyin))
	bShowCursor = False;

    if (im[iIMIndex].ResetIM)
	im[iIMIndex].ResetIM ();
}

void CloseIM (IMForwardEventStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    
    if (!bUseDBus)
	XUnmapWindow (dpy, VKWindow);

    IMPreeditEnd (ims, (XPointer) call_data);
    SetConnectID (call_data->connect_id, IS_CLOSED);
    icidSetIMState(call_data->icid, IS_CLOSED);
    bVK = False;
    SwitchIM (-2);
    
    if (!bUseDBus) {
	DrawMainWindow ();

#ifdef _ENABLE_TRAY
	DrawTrayWindow (INACTIVE_ICON);
#endif
    }
#ifdef _ENABLE_DBUS
    else
	updateProperty(&state_prop);
#endif
}

void ChangeIMState (CARD16 _connect_id)
{
    if (ConnectIDGetState (_connect_id) == IS_ENG) {
	SetConnectID (_connect_id, IS_CHN);
	iState = IS_CHN;

	if (!bUseDBus) {
	    if (bVK)
		DisplayVKWindow ();
	    else
		DisplayInputWindow ();
	}
    }
    else {
	SetConnectID (_connect_id, IS_ENG);
	iState = IS_ENG;
	ResetInput ();
	ResetInputWindow ();

	XUnmapWindow (dpy, inputWindow);
	if (!bUseDBus)
	    XUnmapWindow (dpy, VKWindow);
    }
    
    if (!bUseDBus) {
	if (hideMainWindow != HM_HIDE)
	    DrawMainWindow ();
    }

#ifdef _ENABLE_DBUS 
    if (bUseDBus) {
	updateMessages();
	updateProperty(&state_prop);
    }
#endif
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

    if (!iKey) {
	IMForwardEvent (ims, (XPointer) call_data);
	return;
    }

    /*
     * 原意是为了解决xine-ui中候选字自动选中的问题
     * xine-ui每秒钟产生一个左SHIFT键的释放事件
     * 但这段代码对新的xine-ui已经不起作用了
     */
     /*if (kev->same_screen && (kev->keycode == switchKey || kev->keycode == i2ndSelectKey || kev->keycode == i3rdSelectKey)) {
	IMForwardEvent (ims, (XPointer) call_data);
	return;
     }*/

    retVal = IRV_TO_PROCESS;

#ifdef _DEBUG
    printf ("KeyRelease=%d  iKeyState=%d  KEYCODE=%d  keyCount=%d  iKey=%d\n", (call_data->event.type == KeyRelease), iKeyState, kev->keycode, keyCount, iKey);
#endif

    /* Added by hubert_star AT forum.ubuntu.com.cn */
    if ( (call_data->event.type == KeyRelease) && (((iKey >= 32 ) && (iKey <= 126)) && (iKey != (i2ndSelectKey ^ 0xFF)) && (iKey != (i3rdSelectKey ^ 0xFF))))
        return;

    /* ******************************************* */

    if (call_data->event.type == KeyRelease) {
	if (ConnectIDGetState (call_data->connect_id) != IS_CLOSED) {
	    if ((kev->time - lastKeyPressedTime) < 500 && (!bIsDoInputOnly)) {
		if (iKeyState == KEY_CTRL_SHIFT_COMP && (iKey == 225 || iKey == 227)) {
		    if ( !bLocked ) {
		        if (ConnectIDGetState (call_data->connect_id) == IS_CHN)
			    SwitchIM (-1);
		        else if (IsHotKey (iKey, hkTrigger))
			    CloseIM (call_data);
		    }
		    else if ( bVK )
		        ChangVK();
		}
		else if (iKey == CTRL_LSHIFT) {
		    if ( !bLocked ) {
		        if (ConnectIDGetState (call_data->connect_id) == IS_CHN)
			    SwitchIM (-1);
		        else if (IsHotKey (iKey, hkTrigger))
			    CloseIM (call_data);
		    }
		    else if ( bVK )
		        ChangVK();
		}
		else if (kev->keycode == switchKey && keyReleased == KR_CTRL && !bDoubleSwitchKey) {
		    /* 按下临时英文键会把已经输入的英文送到客户程序中，但实际使用发现，这种方式不具人性化
		     * if (iCodeInputCount) {
			strcpy (strStringGet, strCodeInput);
			retVal = IRV_ENG;
		    }
		    else
			retVal = IRV_TO_PROCESS;
		    */
		    keyReleased = KR_OTHER;
		    ChangeIMState (call_data->connect_id);
		    retVal = IRV_DONOT_PROCESS;
	        }
		else if ((kev->keycode == i2ndSelectKey && keyReleased == KR_2ND_SELECTKEY)
			 || (iKey == (i2ndSelectKey ^ 0xFF) && keyReleased == KR_2ND_SELECTKEY_OTHER)) {
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

		    keyReleased = KR_OTHER;
		}
		else if ((kev->keycode == i3rdSelectKey && keyReleased == KR_3RD_SELECTKEY)
			 || (iKey == (i3rdSelectKey ^ 0xFF) && keyReleased == KR_3RD_SELECTKEY_OTHER) ) {
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

		    keyReleased = KR_OTHER;
		}
	    }
	}
	else
	    call_data->event.type = KeyPress;
    }

    if (retVal == IRV_TO_PROCESS) {
	if (call_data->event.type == KeyPress) {
	    if (kev->keycode != switchKey)
		keyReleased = KR_OTHER;
	    else {
		if ((keyReleased == KR_CTRL) && (kev->time - lastKeyPressedTime < iTimeInterval) && bDoubleSwitchKey) {
	    	    SendHZtoClient(call_data, strCodeInput);
		    ChangeIMState (call_data->connect_id);
	        }
	    }

	    lastKeyPressedTime = kev->time;
	    if (kev->keycode == switchKey) {
		keyReleased = KR_CTRL;
		retVal = IRV_DO_NOTHING;
	    }
	    else if (IsHotKey (iKey, hkTrigger)) {
		if (ConnectIDGetState (call_data->connect_id) == IS_ENG) {
		    SetConnectID (call_data->connect_id, IS_CHN);

		    EnterChineseMode (False);
		    if (!bUseDBus)
			DrawMainWindow ();

		    if (bShowInputWindowTriggering && !bCorner) {
			if (!bUseDBus)
			    DisplayInputWindow ();
		    }
		    else
		        MoveInputWindow(call_data->connect_id);
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
			    if (kev->keycode == i2ndSelectKey) {
				keyReleased = KR_2ND_SELECTKEY;
				return;
			    }
			    else if (kev->keycode == i3rdSelectKey) {
				keyReleased = KR_3RD_SELECTKEY;
				return;
			    }
			    else if (iKey == (i2ndSelectKey ^ 0xFF)) {
				if (iCandWordCount >= 2) {
				    keyReleased = KR_2ND_SELECTKEY_OTHER;
				    return;
				}
			    }
			    else if (iKey == (i3rdSelectKey ^ 0xFF)) {
				if (iCandWordCount >= 2) {
				    keyReleased = KR_3RD_SELECTKEY_OTHER;
				    return;
				}
			    }
			}

			if (iKey == CTRL_LSHIFT || iKey == SHIFT_LCTRL) {
			    if (bLocked)
				retVal = IRV_TO_PROCESS;
			}
			else {
			    //调用输入法模块
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
			    else {
				if (!iInCap) {
				    char            strTemp[MAX_USER_INPUT];

				    retVal = im[iIMIndex].DoInput (iKey);
				    if (!bCursorAuto && !IsIM (strNameOfPinyin) && !IsIM (strNameOfShuangpin))
					iCursorPos = iCodeInputCount;

				    //为了实现自动英文转换
				    strcpy (strTemp, strCodeInput);
				    if (retVal == IRV_TO_PROCESS) {
					strTemp[strlen (strTemp) + 1] = '\0';
					strTemp[strlen (strTemp)] = iKey;
				    }

				    if (SwitchToEng (strTemp)) {
					iInCap = 3;
					if (retVal != IRV_TO_PROCESS) {
					    iCodeInputCount--;
					    retVal = IRV_TO_PROCESS;
					}
				    }

				    if (iKey!= (XK_BackSpace & 0x00FF))
					cLastIsAutoConvert = 0;
				}
				else if (iInCap == 2 && semicolonToDo == K_SEMICOLON_QUICKPHRASE && !iLegendCandWordCount)
				    retVal = QuickPhraseDoInput (iKey);

				if (!bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
				    if (!iInCap && iKey >= 'A' && iKey <= 'Z' && bEngAfterCap && !(kev->state & KEY_CAPSLOCK)) {
					iInCap = 1;
					if (!bIsInLegend && iCandWordCount) {
					    pstr = im[iIMIndex].GetCandWord (0);
					    iCandWordCount = 0;
					    if (pstr) {
						SendHZtoClient (call_data, pstr);
						strcpy (strStringGet, pstr);
						//粗略统计字数
						iHZInputed += (int) (strlen (strStringGet) / 2);
						iCodeInputCount = 0;
					    }
					}
				    }
				    else if (iKey == ';' && semicolonToDo != K_SEMICOLON_NOCHANGE && !iCodeInputCount) {
					if (iInCap != 2)
					    iInCap = 2;
					else
					    iKey = ' ';	//使用第2个分号输入中文分号
				    }
				    else if (!iInCap) {
					if (IsHotKey (iKey, hkPrevPage))
					    retVal = im[iIMIndex].GetCandWords (SM_PREV);
					else if (IsHotKey (iKey, hkNextPage))
					    retVal = im[iIMIndex].GetCandWords (SM_NEXT);
				    }

				    if (retVal == IRV_TO_PROCESS) {
					if (iInCap) {
					    if ((iKey == ' ') && (iCodeInputCount == 0)) {
						strcpy (strStringGet, "；");
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
						else if (iKey == (XK_BackSpace & 0x00FF) || iKey == CTRL_H) {
						    if (iCodeInputCount)
							iCodeInputCount--;
						    strCodeInput[iCodeInputCount] = '\0';
						    iCursorPos = iCodeInputCount;
						    if (!iCodeInputCount)
							retVal = IRV_CLEAN;
						    else if (semicolonToDo == K_SEMICOLON_QUICKPHRASE && iInCap == 2)
							retVal = QuickPhraseGetCandWords (SM_FIRST);
						    else
							retVal = IRV_DISPLAY_MESSAGE;
						}

						uMessageUp = 1;
						if (iInCap == 2) {
						    if (semicolonToDo == K_SEMICOLON_ENG) {
							strcpy (messageUp[0].strMsg, "英文输入 ");
							iCursorPos += 9;
						    }
						    else {
							strcpy (messageUp[0].strMsg, "自定义输入 ");
							iCursorPos += 11;
						    }

						    if (iCodeInputCount) {
							uMessageUp = 2;
							//strcat (messageUp[0].strMsg, "  ");
							strcpy (messageUp[1].strMsg, strCodeInput);
							messageUp[1].type = MSG_INPUT;
							iCursorPos += iCodeInputCount;
						    }

						    if (retVal != IRV_DISPLAY_CANDWORDS) {
							if (iCodeInputCount)
							    strcpy (messageDown[0].strMsg, "按 Enter 输入英文");
							else
							    strcpy (messageDown[0].strMsg, "空格输入；Enter输入;");
							uMessageDown = 1;
							messageDown[0].type = MSG_TIPS;
						    }
						    messageUp[0].type = MSG_TIPS;
						}
						else {
						    uMessageDown = 1;
						    messageDown[0].type = MSG_TIPS;
						    strcpy (messageUp[0].strMsg, strCodeInput);
						    strcpy (messageDown[0].strMsg, "按 Enter 输入英文");
						    messageUp[0].type = MSG_INPUT;
						}
					    }
					}
					else if ((bLastIsNumber && bEngPuncAfterNumber) && (iKey == '.' || iKey == ',' || iKey == ':') && !iCandWordCount) {
					    cLastIsAutoConvert = iKey;
					    bLastIsNumber = False;
					    retVal = IRV_TO_PROCESS;
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
						else if ((iKey == (XK_BackSpace & 0x00FF) || iKey == CTRL_H) && cLastIsAutoConvert ) {
						    char *pPunc;

						    IMForwardEvent (ims, (XPointer) call_data);
						    pPunc = GetPunc(cLastIsAutoConvert);
						    if ( pPunc )
						    	SendHZtoClient(call_data, pPunc);

						    retVal = IRV_DO_NOTHING;
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
					    cLastIsAutoConvert = 0;
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

					if (!bUseDBus) {
					    if (!mainWindow)
					        CreateMainWindow();
					    if (hideMainWindow != HM_HIDE) {
						DisplayMainWindow ();
						DrawMainWindow ();
					    }
					    if (!tray.window) {
					        CreateTrayWindow();
					        DrawTrayWindow (INACTIVE_ICON);
					    }
					    if (!aboutWindow)
						CreateAboutWindow();
					    InitGC (inputWindow);
					    InitMainWindowColor ();
					    InitInputWindowColor ();
					    InitVKWindowColor ();
					}
					else {
					    XUnmapWindow(dpy, mainWindow);
					    XDestroyWindow(dpy,tray.window);
					    tray.window = (Window) NULL;
					    tray_mapped = False;
					}

					SetIM ();
					if (!bUseDBus) {
					    CreateFont ();
					    CalculateInputWindowHeight ();
					}

					FreeQuickPhrase ();
					LoadQuickPhrase ();

					FreeAutoEng ();
					LoadAutoEng ();

					FreePunc ();
					LoadPuncDict ();
					SwitchIM(-2);
					if (!bUseDBus)
					    DrawMainWindow();

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
		    else if (IsHotKey (iKey, hkGBT))
			retVal = ChangeGBKT ();
		    else if (IsHotKey (iKey, hkHideMainWindow)) {
			if (bMainWindow_Hiden) {
			    bMainWindow_Hiden = False;
			    if (!bUseDBus) {
				DisplayMainWindow();
				DrawMainWindow();
			    }
		    	}
			else {
			    bMainWindow_Hiden = True;
			    if (!bUseDBus) 
				XUnmapWindow(dpy,mainWindow);
			}
			retVal = IRV_DO_NOTHING;
		    }
		    else if (IsHotKey (iKey, hkSaveAll)) {
			SaveIM();
			uMessageDown = 1;
			strcpy(messageDown[0].strMsg,"词库已保存");
			messageDown[0].type = MSG_TIPS;
			retVal = IRV_DISPLAY_MESSAGE;
		    }
		    else if (IsHotKey (iKey, hkVK) ) 
		        SwitchVK ();
#ifdef _ENABLE_RECORDING
		    else if (IsHotKey (iKey, hkRecording) )
		        ChangeRecording ();
		    else if (IsHotKey (iKey, hkResetRecording) )
		        ResetRecording ();
#endif
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
	IMForwardEvent (ims, (XPointer) call_data);

	if (retVal != IRV_DONOT_PROCESS_CLEAN)
	    return;
    case IRV_CLEAN:
	ResetInput ();
	ResetInputWindow ();
	XUnmapWindow (dpy, inputWindow);
#ifdef _ENABLE_DBUS
	if (bUseDBus)
	    updateMessages();
#endif

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

	if (!bUseDBus) {
	    DisplayInputWindow ();
	    DrawInputWindow ();
	}
#ifdef _ENABLE_DBUS
	else {
	    MoveInputWindow(call_data->connect_id);
	    updateMessages();
	}
#endif

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

	if (!bUseDBus) {
	    DisplayInputWindow ();
	    DrawInputWindow ();
	}
#ifdef _ENABLE_DBUS
	else
	    updateMessages();
#endif

	break;
    case IRV_GET_LEGEND:
	SendHZtoClient (call_data, strStringGet);
	iHZInputed += (int) (strlen (strStringGet) / 2);	//粗略统计字数
	if (iLegendCandWordCount) {
	    bShowNext = bShowPrev = False;
	    if (iCurrentLegendCandPage > 0)
		bShowPrev = True;
	    if (iCurrentLegendCandPage < iLegendCandPageCount)
		bShowNext = True;
	    bLastIsNumber = False;
	    iCodeInputCount = 0;
	    if (!bUseDBus) {
		DisplayInputWindow ();
		DrawInputWindow ();
	    }
#ifdef _ENABLE_DBUS
	    else
		updateMessages();
#endif
	}
	else {
	    ResetInput ();
	    if (!bUseDBus)
		XUnmapWindow (dpy, inputWindow);
#ifdef _ENABLE_DBUS
	    else
		updateMessages();
#endif
	}

	break;
    case IRV_GET_CANDWORDS:
	SendHZtoClient (call_data, strStringGet);
	bLastIsNumber = False;
	if (bPhraseTips && im[iIMIndex].PhraseTips && !bVK)
	    DoPhraseTips ();
	iHZInputed += (int) (strlen (strStringGet) / 2);	

	if (bVK || (!uMessageDown && (!bPhraseTips || (bPhraseTips && !lastIsSingleHZ)))) {
		XUnmapWindow (dpy, inputWindow);
		uMessageUp = 0;
		uMessageDown = 0;
	}
	else {
	    if (!bUseDBus) {
		DisplayInputWindow ();
		DrawInputWindow ();
	    }
	}
#ifdef _ENABLE_DBUS
	if (bUseDBus)
	    updateMessages();
#endif

	ResetInput ();
        lastIsSingleHZ = 0;
	break;
    case IRV_ENG:
	//如果处于中文标点模式，应该将其中的标点转换为全角
	if (bChnPunc && bConvertPunc)
	    ConvertPunc ();
    case IRV_PUNC:
	iHZInputed += (int) (strlen (strStringGet) / 2);	//粗略统计字数
	ResetInput ();
	if (!uMessageDown)
	    XUnmapWindow (dpy, inputWindow);
    case IRV_GET_CANDWORDS_NEXT:
	SendHZtoClient (call_data, strStringGet);
	bLastIsNumber = False;
	lastIsSingleHZ = 0;

	if (retVal == IRV_GET_CANDWORDS_NEXT || lastIsSingleHZ == -1) {
	    iHZInputed += (int) (strlen (strStringGet) / 2);	//粗略统计字数
	    if (!bUseDBus)
		DisplayInputWindow ();
	}
#ifdef _ENABLE_DBUS
	if (bUseDBus)
	    updateMessages();
#endif

	break;
    default:
	;
    }

    //计算打字速度的功能
    if (retVal == IRV_DISPLAY_MESSAGE || retVal == IRV_DISPLAY_CANDWORDS || retVal == IRV_PUNC) {
	if (!bStartRecordType) {
	    bStartRecordType = True;
	    timeStart = time (NULL);
	}
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

    SwitchIM(iIMIndex);
    
    if (!bUseDBus) {
	DrawMainWindow ();
    	XUnmapWindow (dpy, inputWindow);
    }
#ifdef _ENABLE_DBUS
    else
	updateProperty(&corner_prop);
#endif

    SaveProfile ();

    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE ChangePunc (void)
{
    bChnPunc = !bChnPunc;
    
    if (!bUseDBus) 
    	DrawMainWindow ();
    
    SaveProfile ();

#ifdef _ENABLE_DBUS
    if (bUseDBus)
	updateProperty(&punc_prop);
#endif

    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE ChangeGBK (void)
{
    bUseGBK = !bUseGBK;
    ResetInput ();
    ResetInputWindow ();

    if (!bUseDBus)
	DrawMainWindow ();
    XUnmapWindow (dpy, inputWindow);

    SaveProfile ();

#ifdef _ENABLE_DBUS
    if (bUseDBus)
	updateProperty(&gbk_prop);
#endif

    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeGBKT (void)
{
    bUseGBKT = !bUseGBKT;
    ResetInput ();
    ResetInputWindow ();

    if (!bUseDBus) {
	DrawMainWindow ();
    	XUnmapWindow (dpy, inputWindow);
    }

    SaveProfile ();

#ifdef _ENABLE_DBUS
    if (bUseDBus)
	updateProperty(&gbkt_prop);
#endif

    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeLegend (void)
{
    bUseLegend = !bUseLegend;
    ResetInput ();
    
    if (!bUseDBus) {
	ResetInputWindow ();
	
	DrawMainWindow ();
    	XUnmapWindow (dpy, inputWindow);
    }

    SaveProfile ();

#ifdef _ENABLE_DBUS
    if (bUseDBus)
	updateProperty(&legend_prop);
#endif
    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeTrack (void)
{
    bTrackCursor = !bTrackCursor;
    SaveProfile ();

    return IRV_DO_NOTHING;;
}

void ChangeLock (void)
{
    bLocked = !bLocked;
    
    if (!bUseDBus)
	DrawMainWindow ();

    SaveProfile ();
}

#ifdef _ENABLE_RECORDING
void ChangeRecording (void)
{
    bRecording =! bRecording;
    ResetInput ();
    ResetInputWindow ();
    XUnmapWindow (dpy, inputWindow);

    CloseRecording();
    if ( bRecording )
        OpenRecording(True);

    SaveProfile ();
}

void ResetRecording (void)
{
    if (fpRecord) {
        fclose(fpRecord);
        fpRecord = NULL;
        bWrittenRecord = False;
    }

    if ( bRecording )
        OpenRecording(False);
}
#endif

void SwitchIM (INT8 index)
{
    INT8        iLastIM;
    char	*str;

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
	else {
		iIMIndex = index;
	}
    }

    if (bVK)
        str = vks[iCurrentVK].strName;
    else if (bCorner)
	str = strFullCorner;
    else
	str = im[iIMIndex].strName;

    if (!bUseDBus) {
#ifdef _USE_XFT
	MAINWND_WIDTH = ((bCompactMainWindow) ? _MAINWND_WIDTH_COMPACT : _MAINWND_WIDTH) + StringWidth (str, xftMainWindowFont) + 4;
#else
	MAINWND_WIDTH = ((bCompactMainWindow) ? _MAINWND_WIDTH_COMPACT : _MAINWND_WIDTH) + StringWidth (str, fontSetMainWindow) + 4;
#endif
	if (!bShowVK && bCompactMainWindow)
	    MAINWND_WIDTH -= 24;

	XResizeWindow (dpy, mainWindow, MAINWND_WIDTH, MAINWND_HEIGHT);

	DrawMainWindow ();
    }

    if (index != (INT8) - 2) {
	if (im[iLastIM].Destroy)
	    im[iLastIM].Destroy ();
	if (im[iIMIndex].Init)
	    im[iIMIndex].Init ();
    }

    ResetInput ();
	XUnmapWindow (dpy, inputWindow);

    SaveProfile ();

#ifdef _ENABLE_DBUS
    if (bUseDBus) {
	char* need_free = NULL;
	
	if ((index == (INT8)-2) || (index == (INT8)-2)) {
	    logo_prop.label = "Fcitx";
	    iState = IS_ENG;
	}
	else {
	    int iIndex = ConnectIDGetState(connect_id);
	    
	    if (iIndex == IS_CHN) {
		logo_prop.label =(need_free = g2u(im[iIMIndex].strName));
		iState = IS_CHN;
	    }
	}
	
	updateProperty(&logo_prop);
	updateProperty(&state_prop);
	updateMessages();
	if (need_free)
	    free(need_free);
    }
#endif
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
    if (iTableChanged)
	SaveTableDict ();
    SavePY();
}

void SetIM (void)
{
    INT8            i, j, k=0, l;
    Bool	    bFlag[INPUT_METHODS];

    if (im)
	free (im);

    if (inputMethods[IM_TABLE])
	LoadTableInfo ();

    iIMCount = iTableCount;
    if (inputMethods[IM_PY])
	iIMCount++;
    if (inputMethods[IM_SP])
	iIMCount++;
    if (inputMethods[IM_QW])
	iIMCount++;

    iIMCount += EIM_MAX;
    if (!iIMCount)
	iIMCount = 1;

    im = (IM *) malloc (sizeof (IM) * iIMCount);
    iIMCount = 0;

    /* 对输入法的次序进行添加 */
    for ( i = 0; i < INPUT_METHODS; i++ )
        bFlag[i] = False;

    for ( i = 0; i < INPUT_METHODS; i++ ) {
	l = 0;
	for ( j = 0; j < INPUT_METHODS; j++ ) {
	    if ( !bFlag[j] ) {
	        k = j;
	        l = 1;
	    }
        }

        l = 0;
	for ( j = (INPUT_METHODS-1); j>=0; j-- ) {
	    if ( (inputMethods[k] >= inputMethods[j]) && !bFlag[j] ) {
	        k = j;
	        l = 1;
	    }
	}

	if ( !l )
	    break;

	bFlag[k] = True;
	if ( inputMethods[k]>0 ) {
	    switch (k) {
	    case IM_PY:
		RegisterNewIM (strNameOfPinyin, ResetPYStatus, DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL, PYInit, SavePY);
		break;
	    case IM_SP:
		RegisterNewIM (strNameOfShuangpin, ResetPYStatus, DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL, SPInit, SavePY);
		break;
	    case IM_QW:
		RegisterNewIM (strNameOfQuwei, NULL, DoQWInput, QWGetCandWords, QWGetCandWord, NULL, NULL, NULL, NULL);
		break;
	    case IM_TABLE:for (l = 0; l < iTableCount; l++) {
		    RegisterNewIM (table[l].strName, TableResetStatus, DoTableInput, TableGetCandWords, TableGetCandWord, TableGetLegendCandWord, TablePhraseTips, TableInit, FreeTableIM);
		    table[l].iIMIndex = iIMCount - 1;
	        }
	    default:
	        break;
	    }
	}
    }

    if ( (!inputMethods[IM_SP] && (!inputMethods[IM_TABLE] || !iTableCount)) && !iIMCount )	 //至少应该有一种输入法
	RegisterNewIM (strNameOfPinyin, ResetPYStatus, DoPYInput, PYGetCandWords, PYGetCandWord, PYGetLegendCandWord, NULL, PYInit, NULL);

    if (strExternIM[0] && strExternIM[1])
	LoadExtraIM(strExternIM);

    SwitchIM (iIMIndex);
}
