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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <iconv.h>

#include "xim.h"
#include "IC.h"
#include "tools.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "vk.h"

CONNECT_ID     *connectIDsHead = (CONNECT_ID *) NULL;
XIMS            ims;

//************************************************
CARD16          connect_id = 0;
CARD16          icid = 0;
CARD16          lastConnectID = 0;

//************************************************

long            filter_mask = KeyPressMask | KeyReleaseMask;
IC             *CurrentIC = NULL;
char            strLocale[201] = "zh_CN.GB18030,zh_CN.GB2312,zh_CN,zh_CN.GBK,zh_CN.UTF-8,zh_CN.UTF8,en_US.UTF-8,en_US.UTF8";

//该变量是GTK+ OverTheSpot光标跟随的临时解决方案
INT8            iOffsetY = 12;

//extern Bool     bLumaQQ;

extern IM      *im;
extern INT8     iIMIndex;

extern Display *dpy;
extern int      iScreen;
extern Window   mainWindow;
extern Window   inputWindow;
extern Window   VKWindow;

extern int      iMainWindowX;
extern int      iMainWindowY;
extern int      iInputWindowX;
extern int      iInputWindowY;
extern int      iTempInputWindowX;
extern int      iTempInputWindowY;
extern uint     iInputWindowHeight;
extern uint     iInputWindowWidth;
extern Bool     bTrackCursor;
extern Bool     bCenterInputWindow;
extern Bool     bIsUtf8;
extern int      iCodeInputCount;
extern iconv_t  convUTF8;
extern uint     uMessageDown;
extern Bool     bVK;

/* 计算打字速度
extern Bool     bStartRecordType;
extern uint     iHZInputed;
*/
extern Bool     bShowInputWindowTriggering;

extern Bool     bUseGBKT;

/*extern char	strUserLocale[];*/
//+++++++++++++++++++++++++++++++++
//extern INT8   iKeyPressed;

static XIMStyle Styles[] = {
//    XIMPreeditCallbacks | XIMStatusCallbacks, //OnTheSpot
//    XIMPreeditCallbacks | XIMStatusArea,      //OnTheSpot
//    XIMPreeditCallbacks | XIMStatusNothing,   //OnTheSpot
    XIMPreeditPosition | XIMStatusArea,	//OverTheSpot
    XIMPreeditPosition | XIMStatusNothing,	//OverTheSpot
    XIMPreeditPosition | XIMStatusNone,	//OverTheSpot
//    XIMPreeditArea | XIMStatusArea,           //OffTheSpot
//    XIMPreeditArea | XIMStatusNothing,        //OffTheSpot
//    XIMPreeditArea | XIMStatusNone,           //OffTheSpot
    XIMPreeditNothing | XIMStatusNothing,	//Root
    XIMPreeditNothing | XIMStatusNone,	//Root
    0
};

/* Trigger Keys List */
/*static XIMTriggerKey Trigger_Keys_Ctrl_Shift[] = {
    {XK_space, ControlMask, ControlMask},
    {XK_Shift_L, ControlMask, ControlMask},
    {0L, 0L, 0L}
};

XIMTriggerKey   Trigger_Keys1[] = {
    {XK_space, ControlMask, ControlMask},
    {0L, 0L, 0L}
};*/

XIMTriggerKey  *Trigger_Keys = (XIMTriggerKey *) NULL;
INT8            iTriggerKeyCount;

/* Supported Chinese Encodings */
static XIMEncoding zhEncodings[] = {
    "COMPOUND_TEXT",
    NULL
};

Bool MyOpenHandler (IMOpenStruct * call_data)
{
    CreateConnectID (call_data);

    return True;
}

Bool MyGetICValuesHandler (IMChangeICStruct * call_data)
{
    GetIC (call_data);

    return True;
}

Bool MySetICValuesHandler (IMChangeICStruct * call_data)
{
    if (CurrentIC == NULL)
	return True;
    if (CurrentIC != (IC *) FindIC (call_data->icid))
	return True;

    if (bTrackCursor) {
	int             i;
	Window          window;
	XICAttribute   *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

	for (i = 0; i < (int) ((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
	    if (!strcmp (XNSpotLocation, pre_attr->name)) {
		if (CurrentIC->focus_win)
		    XTranslateCoordinates (dpy, CurrentIC->focus_win, RootWindow (dpy, iScreen), (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y, &iTempInputWindowX, &iTempInputWindowY, &window);
		else if (CurrentIC->client_win)
		    XTranslateCoordinates (dpy, CurrentIC->client_win, RootWindow (dpy, iScreen), (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y, &iTempInputWindowX, &iTempInputWindowY, &window);
		else
		    return True;

		if (iTempInputWindowX < 0)
		    iTempInputWindowX = 0;
		else if ((iTempInputWindowX + iInputWindowWidth) > DisplayWidth (dpy, iScreen))
		    iTempInputWindowX = DisplayWidth (dpy, iScreen) - iInputWindowWidth;

		if (iTempInputWindowY < 0)
		    iTempInputWindowY = 0;
		else if ((iTempInputWindowY + iInputWindowHeight) > DisplayHeight (dpy, iScreen))
		    iTempInputWindowY = DisplayHeight (dpy, iScreen) - iInputWindowHeight;
		else
		    iTempInputWindowY += iOffsetY;

		XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);

		ConnectIDSetTrackCursor (call_data->connect_id, True);
	    }
	}
    }
    else if (bCenterInputWindow) {
	iInputWindowX = (DisplayWidth (dpy, iScreen) - iInputWindowWidth) / 2;
	if (iInputWindowX < 0)
	    iInputWindowX = 0;
	XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);
    }

    SetIC (call_data);

    return True;
}

Bool MySetFocusHandler (IMChangeFocusStruct * call_data)
{
    CurrentIC = (IC *) FindIC (call_data->icid);
    connect_id = call_data->connect_id;
    icid = call_data->icid;

    /* It seems this section is useless
       if (bTrackCursor) {
       int             i;
       Window          window;
       XICAttribute   *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

       for (i = 0; i < (int) ((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
       if (!strcmp (XNSpotLocation, pre_attr->name)) {
       if (CurrentIC->focus_win)
       XTranslateCoordinates (dpy, CurrentIC->client_win, RootWindow (dpy, iScreen), (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y, &iTempInputWindowX, &iTempInputWindowY, &window);
       else if (CurrentIC->client_win)
       XTranslateCoordinates (dpy, CurrentIC->client_win, RootWindow (dpy, iScreen), (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y, &iTempInputWindowX, &iTempInputWindowY, &window);
       else
       return True;

       if (iTempInputWindowX < 0)
       iTempInputWindowX = 0;
       else if ((iTempInputWindowX + iInputWindowWidth) > DisplayWidth (dpy, iScreen))
       iTempInputWindowX = DisplayWidth (dpy, iScreen) - iInputWindowWidth;
       else
       iTempInputWindowX += 5;

       if (iTempInputWindowY < 0)
       iTempInputWindowY = 0;
       else if ((iTempInputWindowY + iInputWindowHeight) > DisplayHeight (dpy, iScreen))
       iTempInputWindowY = DisplayHeight (dpy, iScreen) - iInputWindowHeight;

       XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);

       ConnectIDSetTrackCursor (call_data->connect_id, True);
       }
       }
       } */
    /* ************************************************************************ */

    if (ConnectIDGetState (connect_id) != IS_CLOSED) {
	IMPreeditStart (ims, (XPointer) call_data);
	EnterChineseMode (lastConnectID == connect_id);
	if (ConnectIDGetState (connect_id) == IS_CHN) {
	    if (bVK)
		DisplayVKWindow ();
	    else if (uMessageDown > 1 && lastConnectID == connect_id)
		DisplayInputWindow ();
	}
	else {
	    XUnmapWindow (dpy, inputWindow);
	    XUnmapWindow (dpy, VKWindow);
	}

	if (ConnectIDGetTrackCursor (connect_id))
	    XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);
	else
	    XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);
    }
    else {
	XUnmapWindow (dpy, inputWindow);
	XUnmapWindow (dpy, VKWindow);
	DisplayMainWindow ();
    }

    lastConnectID = connect_id;
    /*if (bLumaQQ && ConnectIDGetReset (connect_id)) {
       SendHZtoClient ((IMForwardEventStruct *) call_data, "\0");
       ConnectIDSetReset (connect_id, False);
       } */

    //When application gets the focus, rerecord the time.
    /*
       bStartRecordType = False;
       iHZInputed = 0;
     */

    return True;
}

Bool MyUnsetFocusHandler (IMChangeICStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    XUnmapWindow (dpy, VKWindow);

    return True;
}

Bool MyCloseHandler (IMOpenStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    XUnmapWindow (dpy, VKWindow);
    DestroyConnectID (call_data->connect_id);
    connect_id = 0;
    icid = 0;

    return True;
}

Bool MyCreateICHandler (IMChangeICStruct * call_data)
{
    CreateIC (call_data);

    if (!CurrentIC) {
	CurrentIC = (IC *) FindIC (call_data->icid);;
	connect_id = call_data->connect_id;
	icid = call_data->icid;
    }

    return True;
}

Bool MyDestroyICHandler (IMChangeICStruct * call_data)
{
    if (CurrentIC == (IC *) FindIC (call_data->icid)) {
	XUnmapWindow (dpy, inputWindow);
	XUnmapWindow (dpy, VKWindow);
    }

    DestroyIC (call_data);

    return True;
}

void EnterChineseMode (Bool bState)
{
    if (!bState) {
	ResetInput ();
	ResetInputWindow ();

	if (im[iIMIndex].ResetIM)
	    im[iIMIndex].ResetIM ();
    }

    DisplayMainWindow ();
}

Bool MyTriggerNotifyHandler (IMTriggerNotifyStruct * call_data)
{
    if (call_data->flag == 0) {	/* on key */
	/*
	 * Here, the start of preediting is notified from IMlibrary, which
	 * * is the only way to start preediting in case of Dynamic Event
	 * * Flow, because ON key is mandatary for Dynamic Event Flow.
	 */
	SetConnectID (call_data->connect_id, IS_CHN);
	EnterChineseMode (False);

	if (ConnectIDGetTrackCursor (connect_id))
	    XMoveWindow (dpy, inputWindow, iTempInputWindowX, iTempInputWindowY);
	else
	    XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);

	if (bShowInputWindowTriggering)
	    DisplayInputWindow ();
    }

    return True;
}

/*
#define _PRINT_MESSAGE
*/
Bool MyProtoHandler (XIMS _ims, IMProtocol * call_data)
{
    switch (call_data->major_code) {
    case XIM_OPEN:
#ifdef _PRINT_MESSAGE
	printf ("XIM_OPEN\n");
#endif
	return MyOpenHandler ((IMOpenStruct *) call_data);
    case XIM_CLOSE:
#ifdef _PRINT_MESSAGE
	printf ("XIM_CLOSE\n");
#endif
	return MyCloseHandler ((IMOpenStruct *) call_data);
    case XIM_CREATE_IC:
#ifdef _PRINT_MESSAGE
	printf ("XIM_CREATE_IC\n");
#endif
	return MyCreateICHandler ((IMChangeICStruct *) call_data);
    case XIM_DESTROY_IC:
#ifdef _PRINT_MESSAGE
	printf ("XIM_DESTROY_IC:%d\n", ((IMForwardEventStruct *) call_data)->icid);
#endif
	return MyDestroyICHandler ((IMChangeICStruct *) call_data);
    case XIM_SET_IC_VALUES:
#ifdef _PRINT_MESSAGE
//      printf ("XIM_SET_IC_VALUES:%d\n", ((IMForwardEventStruct *) call_data)->icid);
#endif
	return MySetICValuesHandler ((IMChangeICStruct *) call_data);
    case XIM_GET_IC_VALUES:
#ifdef _PRINT_MESSAGE
	printf ("XIM_GET_IC_VALUES\n");
#endif
	return MyGetICValuesHandler ((IMChangeICStruct *) call_data);
    case XIM_FORWARD_EVENT:
#ifdef _PRINT_MESSAGE
	printf ("XIM_FORWARD_EVENT: %d  %d\n", ((IMForwardEventStruct *) call_data)->icid, ((IMForwardEventStruct *) call_data)->connect_id);
#endif
	//if ( connect_id == ((IMForwardEventStruct *)call_data)->connect_id )
	ProcessKey ((IMForwardEventStruct *) call_data);
	XSync (dpy, False);

	return True;
    case XIM_SET_IC_FOCUS:
#ifdef _PRINT_MESSAGE
	printf ("XIM_SET_IC_FOCUS:conn=%d   ic=%d\n", ((IMForwardEventStruct *) call_data)->icid, ((IMForwardEventStruct *) call_data)->connect_id);
#endif
	return MySetFocusHandler ((IMChangeFocusStruct *) call_data);
    case XIM_UNSET_IC_FOCUS:
#ifdef _PRINT_MESSAGE
	printf ("XIM_UNSET_IC_FOCUS:%d\n", ((IMForwardEventStruct *) call_data)->icid);
#endif
	return MyUnsetFocusHandler ((IMChangeICStruct *) call_data);;
    case XIM_RESET_IC:
#ifdef _PRINT_MESSAGE
	printf ("XIM_RESET_IC\n");
#endif
	return True;
    case XIM_TRIGGER_NOTIFY:
#ifdef _PRINT_MESSAGE
	printf ("XIM_TRIGGER_NOTIFY\n");
#endif
	return MyTriggerNotifyHandler ((IMTriggerNotifyStruct *) call_data);
    default:
	return True;
    }
}

void SendHZtoClient (IMForwardEventStruct * call_data, char *strHZ)
{
    XTextProperty   tp;
    char            strOutput[300];
    char           *ps;
    char           *pS2T = (char *) NULL;

    if (bUseGBKT)
	pS2T = strHZ = ConvertGBKSimple2Tradition (strHZ);

    if (bIsUtf8) {
	size_t          l1, l2;

	ps = strOutput;
	l1 = strlen (strHZ);
	l2 = 299;
	l1 = iconv (convUTF8, (ICONV_CONST char **) (&strHZ), &l1, &ps, &l2);
	*ps = '\0';
	ps = strOutput;
    }
    else
	ps = strHZ;

    XmbTextListToTextProperty (dpy, (char **) &ps, 1, XCompoundTextStyle, &tp);
    ((IMCommitStruct *) call_data)->flag |= XimLookupChars;
    ((IMCommitStruct *) call_data)->commit_string = (char *) tp.value;
    IMCommitString (ims, (XPointer) call_data);
    XFree (tp.value);
    if (bUseGBKT)
	free (pS2T);
}

Bool InitXIM (char *imname, Window im_window)
{
    XIMStyles      *input_styles;
    XIMTriggerKeys *on_keys;
    XIMEncodings   *encodings;
    char           *p;

    if (!imname) {
	imname = getenv ("XMODIFIERS");
	if (imname) {
	    if (strstr (imname, "@im="))
		imname += 4;
	    else {
		fprintf (stderr, "XMODIFIERS Error...\n");
		imname = DEFAULT_IMNAME;
	    }
	}
	else {
	    fprintf (stderr, "Please set XMODIFIERS...\n");
	    imname = DEFAULT_IMNAME;
	}
    }

    input_styles = (XIMStyles *) malloc (sizeof (XIMStyles));
    input_styles->count_styles = sizeof (Styles) / sizeof (XIMStyle) - 1;
    input_styles->supported_styles = Styles;

    on_keys = (XIMTriggerKeys *) malloc (sizeof (XIMTriggerKeys));
    on_keys->count_keys = iTriggerKeyCount + 1;
    on_keys->keylist = Trigger_Keys;

    encodings = (XIMEncodings *) malloc (sizeof (XIMEncodings));
    encodings->count_encodings = sizeof (zhEncodings) / sizeof (XIMEncoding) - 1;
    encodings->supported_encodings = zhEncodings;

    /*ims = IMOpenIM (dpy, IMModifiers, "Xi18n", IMServerWindow, im_window, IMServerName, imname, IMLocale, (strUserLocale[0])? strUserLocale:"zh_CN.GB18030,zh_CN.GB2312,zh_CN,zh_CN.GBK,zh_CN.UTF-8,en_US.UTF-8", IMServerTransport, "X/", IMInputStyles, input_styles, NULL); */
    p = getenv ("LC_CTYPE");
    if (!p) {
	p = getenv ("LC_ALL");
	if (!p)
	    p = getenv ("LANG");
    }
    if (p) {
	if (!strstr (strLocale, p)) {
	    strcat (strLocale, ",");
	    strcat (strLocale, p);
	}
    }

    ims = IMOpenIM (dpy, IMModifiers, "Xi18n", IMServerWindow, im_window, IMServerName, imname, IMLocale, strLocale, IMServerTransport, "X/", IMInputStyles, input_styles, NULL);
    if (ims == (XIMS) NULL) {
	fprintf (stderr, "Start FCITX error. Another XIM daemon named %s is running?\n", imname);
	return False;
    }

    IMSetIMValues (ims, IMOnKeysList, on_keys, NULL);
    IMSetIMValues (ims, IMEncodingList, encodings, IMProtocolHandler, MyProtoHandler, IMFilterEventMask, filter_mask, NULL);

    return True;
}

void SetIMState (Bool bState)
{
    IMChangeFocusStruct call_data;

    if (!CurrentIC)
	return;

    if (connect_id && CurrentIC->id) {
	call_data.connect_id = connect_id;
	call_data.icid = CurrentIC->id;

	if (bState) {
	    IMPreeditStart (ims, (XPointer) & call_data);
	    SetConnectID (connect_id, IS_CHN);
	}
	else {
	    IMPreeditEnd (ims, (XPointer) & call_data);
	    SetConnectID (connect_id, IS_CLOSED);
	    bVK = False;
	    XUnmapWindow (dpy, inputWindow);
	    XUnmapWindow (dpy, VKWindow);

	    SwitchIM (-2);
	}

	DisplayMainWindow ();
    }
}

void CreateConnectID (IMOpenStruct * call_data)
{
    CONNECT_ID     *temp, *connectIDNew;

    temp = connectIDsHead;
    while (temp) {
	if (!temp->next)
	    break;
	temp = temp->next;
    }

    connectIDNew = (CONNECT_ID *) malloc (sizeof (CONNECT_ID));
    connectIDNew->next = (CONNECT_ID *) NULL;
    connectIDNew->connect_id = call_data->connect_id;
    connectIDNew->imState = IS_CLOSED;
    //connectIDNew->bReset = !bLumaQQ;
    connectIDNew->bTrackCursor = False;
    //connectIDNew->strLocale=(char *)malloc(sizeof(char)*(call_data->lang.length+1));
    //strcpy(connectIDNew->strLocale,call_data->lang.name);

    if (!temp)
	connectIDsHead = connectIDNew;
    else
	temp->next = connectIDNew;
}

void DestroyConnectID (CARD16 connect_id)
{
    CONNECT_ID     *temp, *last;

    last = temp = connectIDsHead;
    while (temp) {
	if (temp->connect_id == connect_id)
	    break;

	last = temp;;
	temp = temp->next;
    }

    if (!temp)
	return;

    if (temp == connectIDsHead)
	connectIDsHead = temp->next;
    else
	last->next = temp->next;

    /*if (temp->strLocale)
       free(temp->strLocale); */
    free (temp);
}

void SetConnectID (CARD16 connect_id, IME_STATE imState)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;
    while (temp) {
	if (temp->connect_id == connect_id) {
	    temp->imState = imState;
	    return;
	}

	temp = temp->next;
    }
}

IME_STATE ConnectIDGetState (CARD16 connect_id)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;

    while (temp) {
	if (temp->connect_id == connect_id)
	    return temp->imState;

	temp = temp->next;
    }

    return IS_CLOSED;
}

/*New Lumaqq need not be supported specially
Bool ConnectIDGetReset (CARD16 connect_id)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;

    while (temp) {
	if (temp->connect_id == connect_id)
	    return temp->bReset;

	temp = temp->next;
    }

    return False;
}

void ConnectIDSetReset (CARD16 connect_id, Bool bReset)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;

    while (temp) {
	if (temp->connect_id == connect_id) {
	    temp->bReset = bReset;
	    return;
	}

	temp = temp->next;
    }
}

void ConnectIDResetReset (void)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;

    while (temp) {
	temp->bReset = True;
	temp = temp->next;
    }
}
*/
Bool ConnectIDGetTrackCursor (CARD16 connect_id)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;

    while (temp) {
	if (temp->connect_id == connect_id)
	    return temp->bTrackCursor;

	temp = temp->next;
    }

    return False;
}

void ConnectIDSetTrackCursor (CARD16 connect_id, Bool bTrackCursor)
{
    CONNECT_ID     *temp;

    temp = connectIDsHead;

    while (temp) {
	if (temp->connect_id == connect_id) {
	    temp->bTrackCursor = bTrackCursor;
	    return;
	}

	temp = temp->next;
    }
}

/*
char *ConnectIDGetLocale(CARD16 connect_id)
{
    CONNECT_ID *temp;
    static char strDefault[]="";

    temp=connectIDsHead;

    while (temp) {
	if ( temp->connect_id==connect_id ) {
	    if ( temp->strLocale )
	    	return temp->strLocale;
	    return strDefault;
	}
	
	temp=temp->next;
    }

    return strDefault;
}
*/
