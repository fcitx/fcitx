#include <X11/Xlib.h>
#include <iconv.h>

#include "xim.h"
#include "IC.h"
#include "tools.h"
#include "ime.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "wbx.h"

long            filter_mask = KeyPressMask | KeyReleaseMask;
IC             *CurrentIC;
Bool            bRunLocal = False;
char           *strDefaultLocale;
Bool            bUseCtrlShift = False;
Bool            bBackground = True;

extern Display *dpy;
extern int      iScreen;
extern Window   mainWindow;
extern Window   inputWindow;

extern int      iMainWindowX;
extern int      iMainWindowY;
extern int      iInputWindowX;
extern int      iInputWindowY;
extern uint     iInputWindowHeight;
extern uint     iInputWindowWidth;
extern Bool     bTrackCursor;
extern Bool     bIsUtf8;
extern int      iCodeInputCount;
extern iconv_t  convUTF8;

static XIMStyle Styles[] = {
    XIMPreeditCallbacks | XIMStatusCallbacks,	//OnTheSpot
    XIMPreeditCallbacks | XIMStatusArea,	//OnTheSpot
    XIMPreeditCallbacks | XIMStatusNothing,	//OnTheSpot
    XIMPreeditPosition | XIMStatusArea,	//OverTheSpot
    XIMPreeditPosition | XIMStatusNothing,	//OverTheSpot
    XIMPreeditPosition | XIMStatusNone,	//OverTheSpot
    XIMPreeditArea | XIMStatusArea,	//OffTheSpot
    XIMPreeditArea | XIMStatusNothing,	//OffTheSpot
    XIMPreeditArea | XIMStatusNone,	//OffTheSpot
    XIMPreeditNothing | XIMStatusNothing,	//Root
    XIMPreeditNothing | XIMStatusNone,	//Root
    0
};

/* Trigger Keys List */
static XIMTriggerKey Trigger_Keys_Ctrl_Shift[] = {
    {XK_space, ControlMask, ControlMask},
    {XK_Shift_L, ControlMask, ControlMask},
    {0L, 0L, 0L}
};

XIMTriggerKey   Trigger_Keys[] = {
    {XK_space, ControlMask, ControlMask},
    {0L, 0L, 0L}
};

/* Supported Chinese Encodings */
static XIMEncoding zhEncodings[] = {
    "COMPOUND_TEXT",
    NULL
};

Bool MyGetICValuesHandler (XIMS ims, IMChangeICStruct * call_data)
{
    GetIC (call_data);
    return True;
}

Bool MySetICValuesHandler (XIMS ims, IMChangeICStruct * call_data)
{
    int             iX, iY;

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
		    XTranslateCoordinates (dpy, CurrentIC->focus_win, RootWindow (dpy, iScreen), (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y, &iX, &iY, &window);
		else if (CurrentIC->client_win)
		    XTranslateCoordinates (dpy, CurrentIC->client_win, RootWindow (dpy, iScreen), (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y, &iX, &iY, &window);
		else
		    return True;

		if (iX < 0)
		    iX = 0;
		else if ((iX + iInputWindowWidth) > DisplayWidth (dpy, iScreen)) {
		    iX = DisplayWidth (dpy, iScreen) - iInputWindowWidth;
		}

		if (iY < 0)
		    iY = 0;
		else if ((iY + iInputWindowHeight) > DisplayHeight (dpy, iScreen)) {
		    iY = DisplayHeight (dpy, iScreen) - iInputWindowHeight;
		}
		else
		    iY += 3;

		XMoveWindow (dpy, inputWindow, iX, iY);
	    }
	}
    }

    SetIC (call_data);

    return True;
}

Bool MySetFocusHandler (XIMS ims, IMChangeFocusStruct * call_data)
{
    CurrentIC = (IC *) FindIC (call_data->icid);
    if (!CurrentIC)
	return True;

    XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);

    DisplayMainWindow ();
    if (CurrentIC->imeState == IS_CHN && iCodeInputCount)
	DisplayInputWindow ();
    else
	XUnmapWindow (dpy, inputWindow);

    return True;
}

Bool MyUnsetFocusHandler (XIMS ims, IMChangeICStruct * call_data)
{
    if (CurrentIC != (IC *) FindIC (call_data->icid))
	XUnmapWindow (dpy, inputWindow);
    return True;
}

Bool MyOpenHandler (XIMS ims, IMOpenStruct * call_data)
{
    return True;
}

Bool MyCloseHandler (XIMS ims, IMOpenStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    return True;
}

Bool MyCreateICHandler (XIMS ims, IMChangeICStruct * call_data)
{
    CreateIC (call_data);
    CurrentIC = (IC *) FindIC (call_data->icid);
    return True;
}

Bool MyDestroyICHandler (XIMS ims, IMChangeICStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    DestroyIC (call_data);

    return True;
}

Bool IsKey (XIMS ims, IMForwardEventStruct * call_data, XIMTriggerKey * trigger)
{				/* Searching for these keys */
    char            strbuf[STRBUFLEN];
    KeySym          keysym;
    int             i;
    int             modifier;
    int             modifier_mask;
    XKeyEvent      *kev;

    memset (strbuf, 0, STRBUFLEN);
    kev = (XKeyEvent *) & call_data->event;
    XLookupString (kev, strbuf, STRBUFLEN, &keysym, NULL);

    for (i = 0; trigger[i].keysym != 0; i++) {
	modifier = trigger[i].modifier;
	modifier_mask = trigger[i].modifier_mask;
	if (((KeySym) trigger[i].keysym == keysym) && ((kev->state & modifier_mask) == modifier))
	    return True;
    }
    return False;
}

/*extern Bool bDebug;
extern FILE *fd;*/
Bool MyForwardEventHandler (XIMS ims, IMForwardEventStruct * call_data)
{
    if (CurrentIC == NULL)
	return True;

    /*if ( bDebug )
       fprintf(fd,"PROCESS   %d\n",call_data->event.type); */
    ProcessKey (ims, call_data);

    return True;
}

Bool MyTriggerNotifyHandler (XIMS ims, IMTriggerNotifyStruct * call_data)
{
    if (call_data->flag == 0) {	/* on key */
	/*
	 * Here, the start of preediting is notified from IMlibrary, which
	 * * is the only way to start preediting in case of Dynamic Event
	 * * Flow, because ON key is mandatary for Dynamic Event Flow.
	 */
	ResetInput ();
	ResetInputWindow ();
	ResetWBStatus ();
	CurrentIC->imeState = IS_CHN;
	DisplayInputWindow ();
	DisplayMainWindow ();

	return True;
    }
//      else if (use_offkey && call_data->flag == 1)
//      {                       /* off key */
    /*
     * Here, the end of preediting is notified from the IMlibrary, which
     * * happens only if OFF key, which is optional for Dynamic Event Flow,
     * * has been registered by IMOpenIM or IMSetIMValues, otherwise,
     * * the end of preediting must be notified from the IMserver to the
     * * IMlibrary.
     */
//              return True;
//      }
//    else {
    /*
     * never happens
     */
//      return False;
//    }
    return False;
}

Bool MyPreeditStartReplyHandler (XIMS ims, IMPreeditCBStruct * call_data)
{
    return True;
}

Bool MyPreeditCaretReplyHandler (XIMS ims, IMPreeditCBStruct * call_data)
{
    return True;
}

Bool MyProtoHandler (XIMS ims, IMProtocol * call_data)
{
    /*switch (call_data->major_code) {
       case XIM_OPEN:
       fprintf (stderr, "XIM_OPEN:\n");
       return MyOpenHandler (ims, call_data);
       case XIM_CLOSE:
       fprintf (stderr, "XIM_CLOSE:\n");
       return MyCloseHandler (ims, call_data);
       case XIM_CREATE_IC:
       fprintf (stderr, "XIM_CREATE_IC:\n");
       return MyCreateICHandler (ims, call_data);
       case XIM_DESTROY_IC:
       fprintf (stderr, "XIM_DESTROY_IC.\n");
       return MyDestroyICHandler (ims, call_data);
       case XIM_SET_IC_VALUES:
       fprintf (stderr, "XIM_SET_IC_VALUES:\n");
       return MySetICValuesHandler (ims, call_data);
       case XIM_GET_IC_VALUES:
       fprintf (stderr, "XIM_GET_IC_VALUES:\n");
       return MyGetICValuesHandler (ims, call_data);
       case XIM_FORWARD_EVENT:
       return MyForwardEventHandler (ims, call_data);
       case XIM_SET_IC_FOCUS:
       fprintf (stderr, "XIM_SET_IC_FOCUS()\n");
       return MySetFocusHandler (ims, (IMChangeFocusStruct *) call_data);
       case XIM_UNSET_IC_FOCUS:
       fprintf (stderr, "XIM_UNSET_IC_FOCUS:\n");
       return MyUnsetFocusHandler (ims, (IMChangeICStruct *) call_data);;
       case XIM_RESET_IC:
       fprintf (stderr, "XIM_RESET_IC_FOCUS:\n");
       return True;
       case XIM_TRIGGER_NOTIFY:
       fprintf (stderr, "XIM_TRIGGER_NOTIFY:\n");
       return MyTriggerNotifyHandler (ims, call_data);
       case XIM_PREEDIT_START_REPLY:
       fprintf (stderr, "XIM_PREEDIT_START_REPLY:\n");
       return MyPreeditStartReplyHandler (ims, call_data);
       case XIM_PREEDIT_CARET_REPLY:
       fprintf (stderr, "XIM_PREEDIT_CARET_REPLY:\n");
       return MyPreeditCaretReplyHandler (ims, call_data);
       default:
       fprintf (stderr, "Unknown IMDKit Protocol message type\n");
       break;
       } */
    switch (call_data->major_code) {
    case XIM_OPEN:
	return MyOpenHandler (ims, (IMOpenStruct *) call_data);
    case XIM_CLOSE:
	return MyCloseHandler (ims, (IMOpenStruct *) call_data);
    case XIM_CREATE_IC:
	return MyCreateICHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_DESTROY_IC:
	return MyDestroyICHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_SET_IC_VALUES:
	return MySetICValuesHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_GET_IC_VALUES:
	return MyGetICValuesHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_FORWARD_EVENT:
	return MyForwardEventHandler (ims, (IMForwardEventStruct *) call_data);
    case XIM_SET_IC_FOCUS:
	return MySetFocusHandler (ims, (IMChangeFocusStruct *) call_data);
    case XIM_UNSET_IC_FOCUS:
	return MyUnsetFocusHandler (ims, (IMChangeICStruct *) call_data);;
    case XIM_RESET_IC:
	return True;
    case XIM_TRIGGER_NOTIFY:
	return MyTriggerNotifyHandler (ims, (IMTriggerNotifyStruct *) call_data);
    case XIM_PREEDIT_START_REPLY:
	return MyPreeditStartReplyHandler (ims, (IMPreeditCBStruct *) call_data);
    case XIM_PREEDIT_CARET_REPLY:
	return MyPreeditCaretReplyHandler (ims, (IMPreeditCBStruct *) call_data);
    default:
	return True;
    }
}

void SendHZtoClient (XIMS ims, IMForwardEventStruct * call_data, char *strHZ)
{
    XTextProperty   tp;
    Display        *display = ims->core.display;
    char            strOutput[300];
    char           *ps;

    if (bIsUtf8) {
	int             l1, l2;

	ps = strOutput;
	l1 = strlen (strHZ);
	l2 = 299;
	iconv (convUTF8, (char **) (&strHZ), (size_t *) & l1, &ps, (size_t *) & l2);
	*ps = '\0';
	ps = strOutput;
    }
    else
	ps = strHZ;

    XmbTextListToTextProperty (display, (char **) &ps, 1, XCompoundTextStyle, &tp);
    ((IMCommitStruct *) call_data)->flag |= XimLookupChars;
    ((IMCommitStruct *) call_data)->commit_string = (char *) tp.value;
    IMCommitString (ims, (XPointer) call_data);
    XFree (tp.value);
}

Bool InitXIM (char *imname, Window im_window)
{
    XIMS            ims;
    XIMStyles      *input_styles;	//, *styles2;
    XIMTriggerKeys *on_keys;
    XIMEncodings   *encodings;	//, *encoding2;
    char            transport[80];	/* enough */

    if (!imname) {
	imname = getenv ("XMODIFIERS");
	if (imname) {
	    if (strstr (imname, "@im="))
		imname += 4;
	    else {
		fprintf (stderr, "XMODIFIERS设置不正确！\n");
		imname = DEFAULT_IMNAME;
	    }
	}
	else {
	    fprintf (stderr, "没有设置XMODIFIERS！\n");
	    imname = DEFAULT_IMNAME;
	}
    }

    input_styles = (XIMStyles *) malloc (sizeof (XIMStyles));
    input_styles->count_styles = sizeof (Styles) / sizeof (XIMStyle) - 1;
    input_styles->supported_styles = Styles;

    encodings = (XIMEncodings *) malloc (sizeof (XIMEncodings));
    encodings->count_encodings = sizeof (zhEncodings) / sizeof (XIMEncoding) - 1;
    encodings->supported_encodings = zhEncodings;
    strcpy (transport, "X/");

    ims = IMOpenIM (dpy, IMModifiers, "Xi18n", IMServerWindow, im_window, IMServerName, imname, IMLocale, (strcasecmp (strDefaultLocale, "zh_CN.gb2312")) ? strDefaultLocale : "zh_CN", IMServerTransport, transport, IMInputStyles, input_styles, NULL);
    if (ims == (XIMS) NULL) {
	fprintf (stderr, "已经存在另一个同名服务程序 %s\n", imname);
	return False;
    }

    on_keys = (XIMTriggerKeys *) malloc (sizeof (XIMTriggerKeys));

    if (bUseCtrlShift) {
	on_keys->count_keys = sizeof (Trigger_Keys_Ctrl_Shift) / sizeof (XIMTriggerKey) - 1;
	on_keys->keylist = Trigger_Keys_Ctrl_Shift;
    }
    else {
	on_keys->count_keys = sizeof (Trigger_Keys) / sizeof (XIMTriggerKey) - 1;
	on_keys->keylist = Trigger_Keys;
    }

    IMSetIMValues (ims, IMOnKeysList, on_keys, NULL);
    IMSetIMValues (ims, IMEncodingList, encodings, IMProtocolHandler, MyProtoHandler, IMFilterEventMask, filter_mask, NULL);

    return True;
}
