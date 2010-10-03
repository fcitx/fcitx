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
#include "core/fcitx.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <iconv.h>
#include <string.h>
#include <limits.h>

#include "core/xim.h"
#include "core/IC.h"
#include "tools/tools.h"
#include "ui/MainWindow.h"
#include "ui/MessageWindow.h"
#include "ui/InputWindow.h"
#ifdef _ENABLE_TRAY
#include "ui/TrayWindow.h"
#endif
#include "im/special/vk.h"
#include "ui/ui.h"

#include "interface/DBus.h"
#include "fcitx-config/profile.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/cutils.h"

#define CHECK_ENV(env, value, icase) (!getenv(env) \
        || (icase ? \
            (0 != strcmp(getenv(env), (value))) \
            : (0 != strcasecmp(getenv(env), (value)))))

CONNECT_ID *connectIDsHead = (CONNECT_ID *) NULL;
ICID *icidsHead = (ICID *) NULL;
XIMS ims;
Window ximWindow;

//************************************************
CARD16 connect_id = 0;
CARD16 lastConnectID = 0;
//************************************************

IC *CurrentIC = NULL;
char strLocale[201] = "zh_CN.GB18030,zh_CN.GB2312,zh_CN,zh_CN.GBK,zh_CN.UTF-8,zh_CN.UTF8,en_US.UTF-8,en_US.UTF8";

int iClientCursorX = 0;
int iClientCursorY = 0;

#ifdef _ENABLE_RECORDING
FILE *fpRecord = NULL;
Bool bWrittenRecord = False;    //是否写入过记录
#endif

extern IM *im;

extern Display *dpy;
extern int iScreen;
extern VKWindow vkWindow;

extern uint iInputWindowHeight;
extern uint iInputWindowWidth;
extern int iCodeInputCount;
extern uint uMessageDown;
extern uint uMessageUp;
extern Bool bVK;

//计算打字速度
extern Bool bStartRecordType;
extern uint iHZInputed;

extern CARD16 g_last_connect_id;

#ifdef _ENABLE_DBUS
extern Property logo_prop;
extern Property state_prop;
extern Property punc_prop;
extern Property corner_prop;
extern Property gbk_prop;
extern Property gbkt_prop;
extern Property legend_prop;
#endif

#ifdef _DEBUG
char strXModifiers[100];
#endif

static XIMStyle Styles[] = {
    XIMPreeditPosition | XIMStatusArea, //OverTheSpot
    XIMPreeditPosition | XIMStatusNothing,      //OverTheSpot
    XIMPreeditPosition | XIMStatusNone, //OverTheSpot
    XIMPreeditNothing | XIMStatusNothing,       //Root
    XIMPreeditNothing | XIMStatusNone,  //Root
/*    XIMPreeditCallbacks | XIMStatusCallbacks,	//OnTheSpot
    XIMPreeditCallbacks | XIMStatusArea,		//OnTheSpot
    XIMPreeditCallbacks | XIMStatusNothing,		//OnTheSpot
    XIMPreeditArea | XIMStatusArea,			//OffTheSpot
    XIMPreeditArea | XIMStatusNothing,		//OffTheSpot
    XIMPreeditArea | XIMStatusNone,			//OffTheSpot */
    0
};

XIMTriggerKey *Trigger_Keys = (XIMTriggerKey *) NULL;
INT8 iTriggerKeyCount;

/* Supported Chinese Encodings */
static XIMEncoding zhEncodings[] = {
    "COMPOUND_TEXT",
    NULL
};

Bool MyOpenHandler(IMOpenStruct * call_data)
{
    CreateConnectID(call_data);

    return True;
}

void SetTrackPos(IMChangeICStruct * call_data)
{
    if (CurrentIC == NULL)
        return;
    if (CurrentIC != (IC *) FindIC(call_data->icid))
        return;

    if (fcitxProfile.bTrackCursor) {
        int i;
        Window window;
        XICAttribute *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

        for (i = 0; i < (int) ((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
            if (!strcmp(XNSpotLocation, pre_attr->name)) {
                if (CurrentIC->focus_win)
                    XTranslateCoordinates(dpy, CurrentIC->focus_win, RootWindow(dpy, iScreen),
                                          (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y,
                                          &iClientCursorX, &iClientCursorY, &window);
                else if (CurrentIC->client_win)
                    XTranslateCoordinates(dpy, CurrentIC->client_win, RootWindow(dpy, iScreen),
                                          (*(XPoint *) pre_attr->value).x, (*(XPoint *) pre_attr->value).y,
                                          &iClientCursorX, &iClientCursorY, &window);
                else
                    return;

                ConnectIDSetTrackCursor(call_data->connect_id, True);
            }
        }
    }

    MoveInputWindow(call_data->connect_id);
}

Bool MyGetICValuesHandler(IMChangeICStruct * call_data)
{
    GetIC(call_data);

    return True;
}

Bool MySetICValuesHandler(IMChangeICStruct * call_data)
{
    SetIC(call_data);
    SetTrackPos(call_data);

    return True;
}

Bool MySetFocusHandler(IMChangeFocusStruct * call_data)
{
    CurrentIC = (IC *) FindIC(call_data->icid);
    connect_id = call_data->connect_id;

    if (ConnectIDGetState(connect_id) != IS_CLOSED) {
        if (icidGetIMState(call_data->icid) == IS_CLOSED)
            IMPreeditStart(ims, (XPointer) call_data);

        EnterChineseMode(lastConnectID == connect_id);

        if (!fc.bUseDBus)
            DrawMainWindow();

        if (ConnectIDGetState(connect_id) == IS_CHN) {
            if (bVK)
                DisplayVKWindow();
        } else {
            CloseInputWindow();
            XUnmapWindow(dpy, vkWindow.window);
        }
    } else {
        if (icidGetIMState(call_data->icid) != IS_CLOSED)
            IMPreeditEnd(ims, (XPointer) call_data);

        CloseInputWindow();
        XUnmapWindow(dpy, vkWindow.window);

        if (!fc.bUseDBus) {
#ifdef _ENABLE_TRAY
            DrawTrayWindow(INACTIVE_ICON, 0, 0, tray.size, tray.size);
#endif
            if (fc.hideMainWindow == HM_SHOW) {
                DisplayMainWindow();
                DrawMainWindow();
            } else
                XUnmapWindow(dpy, mainWindow.window);
        }
#ifdef _ENABLE_DBUS
        if (fc.bUseDBus)
            updatePropertyByConnectID(connect_id);
#endif

        icidSetIMState(call_data->icid, ConnectIDGetState(connect_id));
        lastConnectID = connect_id;
        //When application gets the focus, re-record the time.
        bStartRecordType = False;
        iHZInputed = 0;

        if (ConnectIDGetTrackCursor(connect_id) && fcitxProfile.bTrackCursor) {
            position *pos = ConnectIDGetPos(connect_id);

            if (pos) {
                iClientCursorX = pos->x;
                iClientCursorY = pos->y;
                if (!fc.bUseDBus)
                    XMoveWindow(dpy, inputWindow.window, iClientCursorX, iClientCursorY);
#ifdef _ENABLE_DBUS
                else
                    KIMUpdateSpotLocation(iClientCursorX, iClientCursorY);
#endif
            }
        }
#ifdef _ENABLE_DBUS
        if (fc.bUseDBus)
            registerProperties();
#endif
    }

    return True;
}

Bool MyUnsetFocusHandler(IMChangeICStruct * call_data)
{
    if (call_data->connect_id == connect_id) {
        CloseInputWindow();
        XUnmapWindow(dpy, vkWindow.window);
    }

    return True;
}

Bool MyCloseHandler(IMOpenStruct * call_data)
{
    CloseInputWindow();

    XUnmapWindow(dpy, vkWindow.window);

    DestroyConnectID(call_data->connect_id);

    SaveIM();

#ifdef _ENABLE_RECORDING
    CloseRecording();
    if (fcitxProfile.bRecording)
        OpenRecording(True);
#endif

    return True;
}

Bool MyCreateICHandler(IMChangeICStruct * call_data)
{
    CreateIC(call_data);

    if (!CurrentIC) {
        CurrentIC = (IC *) FindIC(call_data->icid);
        connect_id = call_data->connect_id;
    }
#ifdef _ENABLE_DBUS
    if (fc.bUseDBus)
        updatePropertyByConnectID(connect_id);
#endif

    CreateICID(call_data);

    return True;
}

Bool MyDestroyICHandler(IMChangeICStruct * call_data)
{
    if (CurrentIC == (IC *) FindIC(call_data->icid)) {
        CloseInputWindow();
        XUnmapWindow(dpy, vkWindow.window);
    }

    DestroyIC(call_data);
    DestroyICID(call_data->icid);

#ifdef _ENABLE_DBUS
    if (fc.bUseDBus) {
        strcpy(logo_prop.label, "Fcitx");
        updateProperty(&logo_prop);
    }
#endif

    return True;
}

void EnterChineseMode(Bool bState)
{
    if (!bState) {
        ResetInput();
        ResetInputWindow();

        if (im[gs.iIMIndex].ResetIM)
            im[gs.iIMIndex].ResetIM();
    }

    if (!fc.bUseDBus) {
        DisplayMainWindow();
#ifdef _ENABLE_TRAY
        DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size, tray.size);
#endif
    }
#ifdef _ENABLE_DBUS
    if (fc.bUseDBus)
        updatePropertyByConnectID(connect_id);
#endif
}

Bool MyTriggerNotifyHandler(IMTriggerNotifyStruct * call_data)
{
    if (call_data->flag == 0) {
        /* Mainwindow always shows wrong input status, so fix it here */
        CurrentIC = (IC *) FindIC(call_data->icid);
        connect_id = call_data->connect_id;

        SetConnectID(call_data->connect_id, IS_CHN);
        icidSetIMState(call_data->icid, IS_CHN);

        EnterChineseMode(False);
        if (!fc.bUseDBus)
            DrawMainWindow();
    }

    SetTrackPos((IMChangeICStruct *) call_data);
    if (fc.bShowInputWindowTriggering && !fcitxProfile.bCorner) {
        DisplayInputWindow();

#ifdef _ENABLE_TRAY
        if (!fc.bUseDBus)
            DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size, tray.size);
#endif
    } else
        return False;

    return True;
}

Bool MyProtoHandler(XIMS _ims, IMProtocol * call_data)
{
    switch (call_data->major_code) {
    case XIM_OPEN:
#ifdef _DEBUG
        printf("XIM_OPEN:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyOpenHandler((IMOpenStruct *) call_data);
    case XIM_CLOSE:
#ifdef _DEBUG
        printf("XIM_CLOSE:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyCloseHandler((IMOpenStruct *) call_data);
    case XIM_CREATE_IC:
#ifdef _DEBUG
        printf("XIM_CREATE_IC:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyCreateICHandler((IMChangeICStruct *) call_data);
    case XIM_DESTROY_IC:
#ifdef _DEBUG
        printf("XIM_DESTROY_IC:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyDestroyICHandler((IMChangeICStruct *) call_data);
    case XIM_SET_IC_VALUES:
#ifdef _DEBUG
        printf("XIM_SET_IC_VALUES:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MySetICValuesHandler((IMChangeICStruct *) call_data);
    case XIM_GET_IC_VALUES:
#ifdef _DEBUG
        printf("XIM_GET_IC_VALUES:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyGetICValuesHandler((IMChangeICStruct *) call_data);
    case XIM_FORWARD_EVENT:
#ifdef _DEBUG
        printf("XIM_FORWARD_EVENT:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        ProcessKey((IMForwardEventStruct *) call_data);

        return True;
    case XIM_SET_IC_FOCUS:
#ifdef _DEBUG
        printf("XIM_SET_IC_FOCUS:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MySetFocusHandler((IMChangeFocusStruct *) call_data);
    case XIM_UNSET_IC_FOCUS:
#ifdef _DEBUG
        printf("XIM_UNSET_IC_FOCUS:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyUnsetFocusHandler((IMChangeICStruct *) call_data);;
    case XIM_RESET_IC:
#ifdef _DEBUG
        printf("XIM_RESET_IC:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return True;
    case XIM_TRIGGER_NOTIFY:
#ifdef _DEBUG
        printf("XIM_TRIGGER_NOTIFY:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return MyTriggerNotifyHandler((IMTriggerNotifyStruct *) call_data);
    default:
#ifdef _DEBUG
        printf("XIM_DEFAULT:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
#endif
        return True;
    }
}

void MyIMForwardEvent(CARD16 connectId, CARD16 icId, int keycode)
{
    IMForwardEventStruct forwardEvent;
    XEvent xEvent;

    memset(&forwardEvent, 0, sizeof(IMForwardEventStruct));
    forwardEvent.connect_id = connectId;
    forwardEvent.icid = icId;
    forwardEvent.major_code = XIM_FORWARD_EVENT;
    forwardEvent.sync_bit = 0;
    forwardEvent.serial_number = 0L;

    xEvent.xkey.type = KeyPress;
    xEvent.xkey.display = dpy;
    xEvent.xkey.serial = 0L;
    xEvent.xkey.send_event = False;
    xEvent.xkey.x = xEvent.xkey.y = xEvent.xkey.x_root = xEvent.xkey.y_root = 0;
    xEvent.xkey.same_screen = False;
    xEvent.xkey.subwindow = None;
    xEvent.xkey.window = None;
    xEvent.xkey.root = DefaultRootWindow(dpy);
    xEvent.xkey.state = 0;
    if (CurrentIC->focus_win)
        xEvent.xkey.window = CurrentIC->focus_win;
    else if (CurrentIC->client_win)
        xEvent.xkey.window = CurrentIC->client_win;

    xEvent.xkey.keycode = keycode;
    memcpy(&(forwardEvent.event), &xEvent, sizeof(forwardEvent.event));
    IMForwardEvent(ims, (XPointer) (&forwardEvent));
}

#ifdef _ENABLE_RECORDING
Bool OpenRecording(Bool bMode)
{
    if (!fpRecord) {
        if (fc.strRecordingPath[0] == '\0') {
            char *pbuf;

            GetXDGFileData("record.dat", NULL, &pbuf);
            if (fc.strRecordingPath)
                free(fc.strRecordingPath);
            fc.strRecordingPath = strdup(pbuf);
            free(pbuf);
        } else if (fc.strRecordingPath[0] != '/') {     //应该是个绝对路径
#ifdef _DEBUG
            FcitxLog(DEBUG, _("Recording file must be an absolute path."));
#endif
            fc.strRecordingPath[0] = '\0';
        }

        if (fc.strRecordingPath[0] != '\0')
            fpRecord = fopen(fc.strRecordingPath, (bMode) ? "a+" : "wt");
    }

    return (fpRecord ? True : False);
}

void CloseRecording(void)
{
    if (fpRecord) {
        if (bWrittenRecord) {
            fprintf(fpRecord, "\n\n");
            bWrittenRecord = False;
        }
        fclose(fpRecord);
        fpRecord = NULL;
    }
}
#endif

void SendHZtoClient(IMForwardEventStruct * call_data, char *strHZ)
{
    XTextProperty tp;
    IMCommitStruct cms;
    char *ps;
    char *pS2T = (char *) NULL;

#ifdef _DEBUG
    FcitxLog(DEBUG, _("Sending %s  icid=%d connectid=%d\n"), strHZ, CurrentIC->id, connect_id);
#endif

    /* avoid Seg fault */
    if (!call_data && !CurrentIC)
        return;

#ifdef _ENABLE_RECORDING
    if (fcitxProfile.bRecording) {
        if (OpenRecording(True)) {
            if (!bWrittenRecord) {
                char buf[20];
                struct tm *ts;
                time_t now;

                now = time(NULL);
                ts = localtime(&now);
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ts);

                fprintf(fpRecord, "%s\n", buf);
            }
            fprintf(fpRecord, "%s", strHZ);
            bWrittenRecord = True;
        }
    }
#endif

    if (fcitxProfile.bUseGBKT)
        pS2T = strHZ = ConvertGBKSimple2Tradition(strHZ);

    ps = strHZ;

    Xutf8TextListToTextProperty(dpy, (char **) &ps, 1, XCompoundTextStyle, &tp);

    memset(&cms, 0, sizeof(cms));
    cms.major_code = XIM_COMMIT;
    if (call_data) {
        cms.icid = call_data->icid;
        cms.connect_id = call_data->connect_id;
    } else {
        cms.icid = CurrentIC->id;
        cms.connect_id = connect_id;
    }
    cms.flag = XimLookupChars;
    cms.commit_string = (char *) tp.value;
    IMCommitString(ims, (XPointer) & cms);
    XFree(tp.value);

    if (fcitxProfile.bUseGBKT)
        free(pS2T);
}

Bool InitXIM(char *imname)
{
    XIMStyles *input_styles;
    XIMTriggerKeys *on_keys;
    XIMEncodings *encodings;
    char *p;

    ximWindow = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 1, 0, 0);
    if (ximWindow == (Window) NULL) {
        FcitxLog(FATAL, _("Can't Create imWindow"));
        exit(1);
    }

    if (!imname) {
        imname = getenv("XMODIFIERS");
        if (imname) {
            if (strstr(imname, "@im="))
                imname += 4;
            else {
                FcitxLog(WARNING, _("XMODIFIERS Error."));
                imname = DEFAULT_IMNAME;
            }
        } else {
            FcitxLog(WARNING, _("Please set XMODIFIERS."));
            imname = DEFAULT_IMNAME;
        }
    }

    if (fc.bHintWindow) {
        char strTemp[PATH_MAX];
        snprintf(strTemp, PATH_MAX, "@im=%s", imname);
        strTemp[PATH_MAX - 1] = '\0';
        if ((getenv("XMODIFIERS") != NULL && CHECK_ENV("XMODIFIERS", strTemp, True)) ||
            CHECK_ENV("GTK_IM_MODULE", "xim", False) || CHECK_ENV("QT_IM_MODULE", "xim", False)) {
            char *msg[6];
            msg[0] = _("Please check your environment varibles.");
            msg[1] = _("You need to set environment varibles below to make fcitx work correctly.");
            msg[2] = "export XMODIFIERS=\"@im=fcitx\"";
            msg[3] = "export QT_IM_MODULE=xim";
            msg[4] = "export GTK_IM_MODULE=xim";
            msg[5] = _("You can set those variables in ~/.bashrc .");
            DrawMessageWindow(_("Setting Hint"), msg, 6);
            DisplayMessageWindow();
        }
    }
#ifdef _DEBUG
    strcpy(strXModifiers, imname);
#endif

    input_styles = (XIMStyles *) malloc(sizeof(XIMStyles));
    input_styles->count_styles = sizeof(Styles) / sizeof(XIMStyle) - 1;
    input_styles->supported_styles = Styles;

    on_keys = (XIMTriggerKeys *) malloc(sizeof(XIMTriggerKeys));
    on_keys->count_keys = iTriggerKeyCount + 1;
    on_keys->keylist = Trigger_Keys;

    encodings = (XIMEncodings *) malloc(sizeof(XIMEncodings));
    encodings->count_encodings = sizeof(zhEncodings) / sizeof(XIMEncoding) - 1;
    encodings->supported_encodings = zhEncodings;

    p = getenv("LC_CTYPE");
    if (!p) {
        p = getenv("LC_ALL");
        if (!p)
            p = getenv("LANG");
    }
    if (p) {
        if (!strcasestr(strLocale, p)) {
            strcat(strLocale, ",");
            strcat(strLocale, p);
        }
    }

    ims = IMOpenIM(dpy,
                   IMModifiers, "Xi18n",
                   IMServerWindow, ximWindow,
                   IMServerName, imname,
                   IMLocale, strLocale,
                   IMServerTransport, "X/",
                   IMInputStyles, input_styles,
                   IMEncodingList, encodings,
                   IMProtocolHandler, MyProtoHandler,
                   IMFilterEventMask, KeyPressMask | KeyReleaseMask, IMOnKeysList, on_keys, NULL);

    free(input_styles);
    free(on_keys);
    free(encodings);

    if (ims == (XIMS) NULL) {
        FcitxLog(ERROR, _("Start FCITX error. Another XIM daemon named %s is running?"), imname);
        return False;
    }

    return True;
}

void SetIMState(Bool bState)
{
    IMChangeFocusStruct call_data;

    if (!CurrentIC)
        return;

    if (connect_id && CurrentIC->id) {
        call_data.connect_id = connect_id;
        call_data.icid = CurrentIC->id;

        if (bState) {
            IMPreeditStart(ims, (XPointer) & call_data);
            SetConnectID(connect_id, IS_CHN);
        } else {
            IMPreeditEnd(ims, (XPointer) & call_data);
            SetConnectID(connect_id, IS_CLOSED);
            bVK = False;

            SwitchIM(-2);
        }
    }
}

void CreateConnectID(IMOpenStruct * call_data)
{
    CONNECT_ID *connectIDNew;

    connectIDNew = (CONNECT_ID *) malloc(sizeof(CONNECT_ID));
    connectIDNew->next = connectIDsHead;
    connectIDNew->connect_id = call_data->connect_id;
    connectIDNew->imState = IS_CLOSED;
    connectIDNew->bTrackCursor = False;
    connectIDNew->pos.x = (DisplayWidth(dpy, iScreen) - inputWindow.iInputWindowWidth) / 2;
    connectIDNew->pos.y = DisplayHeight(dpy, iScreen) - inputWindow.iInputWindowHeight;

    connectIDsHead = connectIDNew;
}

void DestroyConnectID(CARD16 connect_id)
{
    CONNECT_ID *temp, *last;

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

    free(temp);
}

void SetConnectID(CARD16 connect_id, IME_STATE imState)
{
    CONNECT_ID *temp;

    temp = connectIDsHead;
    while (temp) {
        if (temp->connect_id == connect_id) {
            temp->imState = imState;
            return;
        }

        temp = temp->next;
    }
}

IME_STATE ConnectIDGetState(CARD16 connect_id)
{
    CONNECT_ID *temp;

    temp = connectIDsHead;
    g_last_connect_id = connect_id;

    while (temp) {
        if (temp->connect_id == connect_id)
            return temp->imState;

        temp = temp->next;
    }

    return IS_CLOSED;
}

void ConnectIDSetPos(CARD16 connect_id, int x, int y)
{
    CONNECT_ID *temp;

    temp = connectIDsHead;
    while (temp) {
        if (temp->connect_id == connect_id) {
            temp->pos.x = x;
            temp->pos.y = y;
            return;
        }

        temp = temp->next;
    }
}

position *ConnectIDGetPos(CARD16 connect_id)
{
    CONNECT_ID *temp;

    temp = connectIDsHead;
    while (temp) {
        if (temp->connect_id == connect_id)
            return &(temp->pos);

        temp = temp->next;
    }

    return (position *) NULL;
}

/*
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
Bool ConnectIDGetTrackCursor(CARD16 connect_id)
{
    CONNECT_ID *temp;

    temp = connectIDsHead;

    while (temp) {
        if (temp->connect_id == connect_id)
            return temp->bTrackCursor;

        temp = temp->next;
    }

    return False;
}

void ConnectIDSetTrackCursor(CARD16 connect_id, Bool bTrackCursor)
{
    CONNECT_ID *temp;

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

void CreateICID(IMChangeICStruct * call_data)
{
    ICID *icidNew;

    icidNew = (ICID *) malloc(sizeof(ICID));
    icidNew->next = icidsHead;
    icidNew->icid = call_data->icid;
    icidNew->connect_id = call_data->connect_id;
    icidNew->imState = IS_CLOSED;

    icidsHead = icidNew;
}

void DestroyICID(CARD16 icid)
{
    ICID *temp, *last;

    last = temp = icidsHead;
    while (temp) {
        if (temp->icid == icid)
            break;

        last = temp;;
        temp = temp->next;
    }

    if (!temp)
        return;

    if (temp == icidsHead)
        icidsHead = temp->next;
    else
        last->next = temp->next;

    free(temp);
}

void icidSetIMState(CARD16 icid, IME_STATE imState)
{
    ICID *temp;

    temp = icidsHead;

    while (temp) {
        if (temp->icid == icid) {
            temp->imState = imState;
#if _DEBUG
            FcitxLog(DEBUG, _("Set icid=%d(connect_id=%d) to %d"), icid, temp->connect_id, imState);
#endif
            return;
        }

        temp = temp->next;
    }
}

IME_STATE icidGetIMState(CARD16 icid)
{
    ICID *temp;

    temp = icidsHead;

    while (temp) {
        if (temp->icid == icid)
            return temp->imState;

        temp = temp->next;
    }

    return IS_CLOSED;
}

CARD16 ConnectIDGetICID(CARD16 connect_id)
{
    ICID *temp;

    temp = icidsHead;

    while (temp) {
        if (temp->connect_id == connect_id)
            return temp->icid;

        temp = temp->next;
    }

    return 0;
}

// vim: sw=4 sts=4 et tw=100
