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

CONNECT_ID     *connectIDsHead=(CONNECT_ID*)NULL;

//************************************************
CARD16		connect_id = 0;
//************************************************

long            filter_mask = KeyPressMask | KeyReleaseMask;
IC             *CurrentIC = NULL;
Bool            bBackground = True;
char 		strLocale[201]="zh_CN.GB18030,zh_CN.GB2312,zh_CN,zh_CN.GBK,zh_CN.UTF-8,zh_CN.UTF8,en_US.UTF-8,en_US.UTF8";
//int y=0;

extern Bool	bLumaQQ;

extern IM      *im;
extern INT8     iIMIndex;

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
/*extern char	strUserLocale[];*/
//+++++++++++++++++++++++++++++++++
//extern INT8	iKeyPressed;

static XIMStyle Styles[] = {
//    XIMPreeditCallbacks | XIMStatusCallbacks,	//OnTheSpot
//    XIMPreeditCallbacks | XIMStatusArea,	//OnTheSpot
//    XIMPreeditCallbacks | XIMStatusNothing,	//OnTheSpot
    XIMPreeditPosition | XIMStatusArea,		//OverTheSpot
    XIMPreeditPosition | XIMStatusNothing,	//OverTheSpot
    XIMPreeditPosition | XIMStatusNone,		//OverTheSpot
//    XIMPreeditArea | XIMStatusArea,		//OffTheSpot
//    XIMPreeditArea | XIMStatusNothing,	//OffTheSpot
//    XIMPreeditArea | XIMStatusNone,		//OffTheSpot
    XIMPreeditNothing | XIMStatusNothing,	//Root
    XIMPreeditNothing | XIMStatusNone,		//Root
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

Bool MyOpenHandler (XIMS ims, IMOpenStruct *call_data)
{
	CreateConnectID(call_data);
	
	return True;
}

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
		else if ((iX + iInputWindowWidth) > DisplayWidth (dpy, iScreen))
		    iX = DisplayWidth (dpy, iScreen) - iInputWindowWidth;

		if (iY < 0)
		    iY = 0;
		else if ((iY + iInputWindowHeight) > DisplayHeight (dpy, iScreen))
		    iY = DisplayHeight (dpy, iScreen) - iInputWindowHeight;
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
    connect_id = call_data->connect_id;

    if (ConnectIDGetState(call_data->connect_id) == IS_CHN && iCodeInputCount) {
	DisplayInputWindow ();
	if ( bTrackCursor )
    		XMoveWindow (dpy, inputWindow, iInputWindowX, iInputWindowY);
    }
    else
	XUnmapWindow (dpy, inputWindow);

    if (ConnectIDGetState(call_data->connect_id)!=IS_CLOSED) {
	IMPreeditStart (ims, (XPointer) call_data);
	EnterChineseMode();
    }
    else
	DisplayMainWindow ();

    if ( bLumaQQ && ConnectIDGetReset(connect_id) ) {
    	SendHZtoClient(ims,(IMForwardEventStruct *)call_data,"\0");
	ConnectIDSetReset(connect_id, False);
    }

    return True;
}

Bool MyUnsetFocusHandler (XIMS ims, IMChangeICStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);

    return True;
}

Bool MyCloseHandler (XIMS ims, IMOpenStruct * call_data)
{
    XUnmapWindow (dpy, inputWindow);
    DestroyConnectID(call_data->connect_id);

    return True;
}

Bool MyCreateICHandler (XIMS ims, IMChangeICStruct * call_data)
{
    CreateIC (call_data);

    if (!CurrentIC) {
	CurrentIC = (IC *) FindIC (call_data->icid);;
	connect_id = call_data->connect_id;
    }

    return True;
}

Bool MyDestroyICHandler (XIMS ims, IMChangeICStruct * call_data)
{
    if (CurrentIC == (IC *) FindIC (call_data->icid)) {
	XUnmapWindow (dpy, inputWindow);
//	CurrentIC = (IC *) NULL;
    }

    DestroyIC (call_data);

    return True;
}

void EnterChineseMode(void)
{
	ResetInput ();
	ResetInputWindow ();

	if (im[iIMIndex].ResetIM)
		im[iIMIndex].ResetIM();

	DisplayMainWindow ();
}

Bool MyTriggerNotifyHandler (XIMS ims, IMTriggerNotifyStruct * call_data)
{
    if (call_data->flag == 0) {	/* on key */
	/*
	 * Here, the start of preediting is notified from IMlibrary, which
	 * * is the only way to start preediting in case of Dynamic Event
	 * * Flow, because ON key is mandatary for Dynamic Event Flow.
	 */
	SetConnectID(call_data->connect_id, IS_CHN);
	EnterChineseMode();
	DisplayInputWindow ();
    }

    return True;
}

/*
#define _PRINT_MESSAGE
*/
Bool MyProtoHandler (XIMS ims, IMProtocol * call_data)
{
    switch (call_data->major_code) {
    case XIM_OPEN:
#ifdef _PRINT_MESSAGE
	printf ("XIM_OPEN\n");
#endif
	return MyOpenHandler (ims, (IMOpenStruct *) call_data);
    case XIM_CLOSE:
#ifdef _PRINT_MESSAGE
	printf ("XIM_CLOSE\n");
#endif
	return MyCloseHandler (ims, (IMOpenStruct *) call_data);
    case XIM_CREATE_IC:
#ifdef _PRINT_MESSAGE
	printf("XIM_CREATE_IC\n");
#endif
	return MyCreateICHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_DESTROY_IC:
#ifdef _PRINT_MESSAGE
	printf ("XIM_DESTROY_IC:%d\n", ((IMForwardEventStruct *) call_data)->icid);
#endif
	return MyDestroyICHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_SET_IC_VALUES:
#ifdef _PRINT_MESSAGE
//	printf ("XIM_SET_IC_VALUES:%d\n", ((IMForwardEventStruct *) call_data)->icid);
#endif
	return MySetICValuesHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_GET_IC_VALUES:
#ifdef _PRINT_MESSAGE
	printf ("XIM_GET_IC_VALUES\n");
#endif
	return MyGetICValuesHandler (ims, (IMChangeICStruct *) call_data);
    case XIM_FORWARD_EVENT:
#ifdef _PRINT_MESSAGE
	printf ("XIM_FORWARD_EVENT: %d  %d\n", ((IMForwardEventStruct *) call_data)->icid, ((IMForwardEventStruct *) call_data)->connect_id);
#endif
	//if ( connect_id == ((IMForwardEventStruct *)call_data)->connect_id )
		ProcessKey(ims, (IMForwardEventStruct *) call_data);
	XSync(dpy, False);
	
	return True;
    case XIM_SET_IC_FOCUS:
#ifdef _PRINT_MESSAGE
	printf ("XIM_SET_IC_FOCUS:conn=%d   ic=%d\n", ((IMForwardEventStruct *) call_data)->icid,((IMForwardEventStruct *) call_data)->connect_id);
#endif
	return MySetFocusHandler (ims, (IMChangeFocusStruct *) call_data);
    case XIM_UNSET_IC_FOCUS:
#ifdef _PRINT_MESSAGE
	printf ("XIM_UNSET_IC_FOCUS:%d\n", ((IMForwardEventStruct *) call_data)->icid);
#endif
	return MyUnsetFocusHandler (ims, (IMChangeICStruct *) call_data);;
    case XIM_RESET_IC:
#ifdef _PRINT_MESSAGE
	printf ("XIM_RESET_IC\n");
#endif
	return True;
    case XIM_TRIGGER_NOTIFY:
#ifdef _PRINT_MESSAGE
	printf ("XIM_TRIGGER_NOTIFY\n");
#endif
	return MyTriggerNotifyHandler (ims, (IMTriggerNotifyStruct *) call_data);
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
    XIMStyles      *input_styles;
    XIMTriggerKeys *on_keys;
    XIMEncodings   *encodings;
    char *p;

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

    on_keys = (XIMTriggerKeys *) malloc (sizeof (XIMTriggerKeys));
    on_keys->count_keys = iTriggerKeyCount+1;
    on_keys->keylist = Trigger_Keys;

    encodings = (XIMEncodings *) malloc (sizeof (XIMEncodings));
    encodings->count_encodings = sizeof (zhEncodings) / sizeof (XIMEncoding) - 1;
    encodings->supported_encodings = zhEncodings;

    /*ims = IMOpenIM (dpy, IMModifiers, "Xi18n", IMServerWindow, im_window, IMServerName, imname, IMLocale, (strUserLocale[0])? strUserLocale:"zh_CN.GB18030,zh_CN.GB2312,zh_CN,zh_CN.GBK,zh_CN.UTF-8,en_US.UTF-8", IMServerTransport, "X/", IMInputStyles, input_styles, NULL);*/
    p=getenv("LC_CTYPE");
    if ( !p ) {
    	p=getenv("LC_ALL");
	if ( !p)
		p=getenv("LANG");
    }
    if ( p ) {
    	if ( !strstr(strLocale, p) ) {
		strcat(strLocale,",");
		strcat(strLocale,p);
	}
    }

    ims = IMOpenIM (dpy, IMModifiers, "Xi18n", IMServerWindow, im_window, IMServerName, imname, IMLocale, strLocale, IMServerTransport, "X/", IMInputStyles, input_styles, NULL);
    if (ims == (XIMS) NULL) {
	fprintf (stderr, "已经存在另一个同名服务程序 %s\n", imname);
	return False;
    }

    IMSetIMValues (ims, IMOnKeysList, on_keys, NULL);
    IMSetIMValues (ims, IMEncodingList, encodings, IMProtocolHandler, MyProtoHandler, IMFilterEventMask, filter_mask, NULL);

    return True;
}

void CreateConnectID(IMOpenStruct *call_data)
{
    CONNECT_ID	*temp, *connectIDNew;

    temp=connectIDsHead;
    while (temp) {
	if ( !temp->next )
	    break;
	temp=temp->next;
    }

    connectIDNew=(CONNECT_ID *)malloc(sizeof(CONNECT_ID));
    connectIDNew->next=(CONNECT_ID *)NULL;
    connectIDNew->connect_id=call_data->connect_id;
    connectIDNew->imState = IS_CLOSED;
    connectIDNew->bReset = !bLumaQQ;
    //connectIDNew->strLocale=(char *)malloc(sizeof(char)*(call_data->lang.length+1));
    //strcpy(connectIDNew->strLocale,call_data->lang.name);

    if ( !temp )
	connectIDsHead=connectIDNew;
    else
	temp->next=connectIDNew;
}

void DestroyConnectID(CARD16 connect_id)
{
    CONNECT_ID *temp,*last;

    last=temp=connectIDsHead;
    while (temp) {
	if ( temp->connect_id==connect_id )
	    break;
	
	last=temp;;
	temp=temp->next;
    }

    if ( !temp ) //按说这种情况不会发生的
	return;

    if ( temp==connectIDsHead )
	connectIDsHead=temp->next;
    else
	last->next=temp->next;

    /*if (temp->strLocale)
	free(temp->strLocale);*/
    free(temp);
}

void SetConnectID(CARD16 connect_id, IME_STATE	imState)
{
    CONNECT_ID *temp;

    temp=connectIDsHead;
    while (temp) {
	if ( temp->connect_id==connect_id ) {
	    temp->imState = imState;
	    return;
	}

	temp=temp->next;
    }
}

IME_STATE ConnectIDGetState(CARD16 connect_id)
{
    CONNECT_ID *temp;

    temp=connectIDsHead;

    while (temp) {
	if ( temp->connect_id==connect_id )
	    return temp->imState;

	temp=temp->next;
    }

    return IS_CLOSED;
}

Bool ConnectIDGetReset(CARD16 connect_id)
{
    CONNECT_ID *temp;

    temp=connectIDsHead;

    while (temp) {
	if ( temp->connect_id==connect_id )
	    return temp->bReset;

	temp=temp->next;
    }

    return False;
}

void ConnectIDSetReset(CARD16 connect_id, Bool bReset)
{
    CONNECT_ID *temp;

    temp=connectIDsHead;

    while (temp) {
	if ( temp->connect_id==connect_id ) {
	    temp->bReset = bReset;
	    return;
	}

	temp=temp->next;
    }
}

void ConnectIDResetReset(void)
{
    CONNECT_ID *temp;

    temp=connectIDsHead;

    while (temp) {
	temp->bReset = True;
	temp=temp->next;
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
