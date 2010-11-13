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
#include "fcitx-config/xdg.h"

#define CHECK_ENV(env, value, icase) (!getenv(env) \
        || (icase ? \
            (0 != strcmp(getenv(env), (value))) \
            : (0 != strcasecmp(getenv(env), (value)))))

XIMS ims;
Window ximWindow;

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
extern IC *ic_list;

extern uint iInputWindowHeight;
extern uint iInputWindowWidth;
extern int iCodeInputCount;
extern uint uMessageDown;
extern uint uMessageUp;
extern Bool bVK;

//计算打字速度
extern Bool bStartRecordType;
extern uint iHZInputed;

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
    return True;
}

void SetTrackPos(IMChangeICStruct * call_data)
{
    Bool flag = False;
    if (CurrentIC == NULL)
        return;
    if (CurrentIC != (IC *) FindIC(call_data->icid))
        return;

    if (fcitxProfile.bTrackCursor) {
        int i;
        XICAttribute *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

        for (i = 0; i < (int) ((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
            if (!strcmp(XNSpotLocation, pre_attr->name)) {
                flag = True;
                
                CurrentIC->offset_x = (*(XPoint *) pre_attr->value).x;
                CurrentIC->offset_y = (*(XPoint *) pre_attr->value).y;
            }
        }
    }

    MoveInputWindow();
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

    if (CurrentIC && CurrentIC->state != IS_CLOSED) {
        EnterChineseMode(True);

        if (!fc.bUseDBus)
            DrawMainWindow();

        if (CurrentIC->state == IS_CHN) {
            if (bVK)
                DisplayVKWindow();
        } else {
            CloseInputWindow();
            XUnmapWindow(dpy, vkWindow.window);
        }
    } else {
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
            updatePropertyByState();
#endif

        //When application gets the focus, re-record the time.
        bStartRecordType = False;
        iHZInputed = 0;

        if (fcitxProfile.bTrackCursor) {
            MoveInputWindow();
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
    if (CurrentIC && CurrentIC->id == call_data->icid)
    {
        CloseInputWindow();
        XUnmapWindow(dpy, vkWindow.window);
    }

    return True;
}

Bool MyCloseHandler(IMOpenStruct * call_data)
{
    CloseInputWindow();

    XUnmapWindow(dpy, vkWindow.window);

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
    }
#ifdef _ENABLE_DBUS
    if (fc.bUseDBus)
        updatePropertyByState();
#endif
    return True;
}

Bool MyDestroyICHandler(IMChangeICStruct * call_data)
{
    if (CurrentIC == (IC *) FindIC(call_data->icid)) {
        CloseInputWindow();
        XUnmapWindow(dpy, vkWindow.window);
    }

    DestroyIC(call_data);

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
        updatePropertyByState();
#endif
}

Bool MyTriggerNotifyHandler(IMTriggerNotifyStruct * call_data)
{
    IC* ic = (IC *) FindIC(call_data->icid);
    if (ic == NULL)
        return True;

    CurrentIC = ic;
    ic->state = IS_CHN;

    EnterChineseMode(False);
    if (!fc.bUseDBus)
        DrawMainWindow();

    SetTrackPos((IMChangeICStruct *) call_data);
    if (fc.bShowInputWindowTriggering && !fcitxProfile.bCorner) {
        DisplayInputWindow();
    }

#ifdef _ENABLE_TRAY
        if (!fc.bUseDBus)
            DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size, tray.size);
#endif
    return True;
}

Bool MyProtoHandler(XIMS _ims, IMProtocol * call_data)
{
#ifdef _DEBUG
    switch (call_data->major_code) {
    case XIM_OPEN:
        FcitxLog(DEBUG, "XIM_OPEN:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_CLOSE:
        FcitxLog(DEBUG, "XIM_CLOSE:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_CREATE_IC:
        FcitxLog(DEBUG, "XIM_CREATE_IC:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_DESTROY_IC:
        FcitxLog(DEBUG, "XIM_DESTROY_IC:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_SET_IC_VALUES:
        FcitxLog(DEBUG, "XIM_SET_IC_VALUES:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_GET_IC_VALUES:
        FcitxLog(DEBUG, "XIM_GET_IC_VALUES:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_FORWARD_EVENT:
        FcitxLog(DEBUG, "XIM_FORWARD_EVENT:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_SET_IC_FOCUS:
        FcitxLog(DEBUG, "XIM_SET_IC_FOCUS:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_UNSET_IC_FOCUS:
        FcitxLog(DEBUG, "XIM_UNSET_IC_FOCUS:\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_RESET_IC:
        FcitxLog(DEBUG, "XIM_RESET_IC:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    case XIM_TRIGGER_NOTIFY:
        FcitxLog(DEBUG, "XIM_TRIGGER_NOTIFY:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    default:
        FcitxLog(DEBUG, "XIM_DEFAULT:\t\ticid=%d\tconnect_id=%d\n", ((IMForwardEventStruct *) call_data)->icid,
               ((IMForwardEventStruct *) call_data)->connect_id);
    }
#endif

    switch (call_data->major_code) {
    case XIM_OPEN:
        return MyOpenHandler((IMOpenStruct *) call_data);
    case XIM_CLOSE:
        return MyCloseHandler((IMOpenStruct *) call_data);
    case XIM_CREATE_IC:
        return MyCreateICHandler((IMChangeICStruct *) call_data);
    case XIM_DESTROY_IC:
        return MyDestroyICHandler((IMChangeICStruct *) call_data);
    case XIM_SET_IC_VALUES:
        return MySetICValuesHandler((IMChangeICStruct *) call_data);
    case XIM_GET_IC_VALUES:
        return MyGetICValuesHandler((IMChangeICStruct *) call_data);
    case XIM_FORWARD_EVENT:
        ProcessKey((IMForwardEventStruct *) call_data);
        return True;
    case XIM_SET_IC_FOCUS:
        return MySetFocusHandler((IMChangeFocusStruct *) call_data);
    case XIM_UNSET_IC_FOCUS:
        return MyUnsetFocusHandler((IMChangeICStruct *) call_data);;
    case XIM_RESET_IC:
        return True;
    case XIM_TRIGGER_NOTIFY:
        return MyTriggerNotifyHandler((IMTriggerNotifyStruct *) call_data);
    default:
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
    FcitxLog(DEBUG, _("Sending %s  icid=%d connectid=%d\n"), strHZ, CurrentIC->id, CurrentIC->connect_id);
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
        cms.connect_id = CurrentIC->connect_id;
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
            msg[5] = _("You can set those variables in ~/.bashrc or ~/.xprofile .");
            DrawMessageWindow(_("Setting Hint"), msg, 6);
            DisplayMessageWindow();
        }
        fc.bHintWindow = False;
        SaveConfig();
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

    if (CurrentIC->id) {
        call_data.connect_id = CurrentIC->connect_id;
        call_data.icid = CurrentIC->id;
        
        if (bState) {
            IMPreeditStart(ims, (XPointer) & call_data);
            CurrentIC->state = IS_CHN;
        } else {
            IMPreeditEnd(ims, (XPointer) & call_data);
            CurrentIC->state = IS_CLOSED;
            bVK = False;
            
            SwitchIM(-2);
        }
        
    }
}

void CloseAllIM()
{
    IC             *rec;
    IMChangeFocusStruct call_data;

    for (rec = ic_list; rec != NULL; rec = rec->next)
    {
        call_data.connect_id = rec->connect_id;
        call_data.icid = rec->id;
        
        IMPreeditEnd(ims, (XPointer) & call_data);
        rec->state = IS_CLOSED;
    }

    IMCloseIM(ims);
     
}

// vim: sw=4 sts=4 et tw=100
