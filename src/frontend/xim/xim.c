/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <libintl.h>
#include <ctype.h>
#include <errno.h>

#include "fcitx/fcitx.h"
#include "fcitx/addon.h"
#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/fcitx-config.h"
#include "IMdkit.h"
#include "Xi18n.h"
#include "IC.h"
#include "xim.h"
#include "ximhandler.h"
#include "module/x11/x11stuff.h"
#include "fcitx-config/xdg.h"
#include "fcitx/hook.h"

static void* XimCreate(FcitxInstance* instance, int frontendid);
static boolean XimDestroy(void* arg);
static void XimEnableIM(void* arg, FcitxInputContext* ic);
static void XimCloseIM(void* arg, FcitxInputContext* ic);
static void XimCommitString(void* arg, FcitxInputContext* ic, char* str);
static void XimForwardKey(void* arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
static void XimSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y);
static void XimGetWindowPosition(void* arg, FcitxInputContext* ic, int* x, int* y);
static void XimUpdatePreedit(void* arg, FcitxInputContext* ic);

static Bool XimProtocolHandler(XIMS _ims, IMProtocol * call_data);
static inline Bool MyStrcmp(char *str1, char *str2);

static XIMStyle OverTheSpot_Styles[] = {
    XIMPreeditPosition | XIMStatusArea, //OverTheSpot
    XIMPreeditPosition | XIMStatusNothing,      //OverTheSpot
    XIMPreeditPosition | XIMStatusNone, //OverTheSpot
    XIMPreeditNothing | XIMStatusNothing,       //Root
    XIMPreeditNothing | XIMStatusNone,  //Root
    /*
    XIMPreeditArea | XIMStatusArea,         //OffTheSpot
    XIMPreeditArea | XIMStatusNothing,      //OffTheSpot
    XIMPreeditArea | XIMStatusNone,         //OffTheSpot */
    0
};

static XIMStyle OnTheSpot_Styles [] = {
    XIMPreeditPosition | XIMStatusNothing,
    XIMPreeditCallbacks | XIMStatusNothing,
    XIMPreeditNothing | XIMStatusNothing,
    XIMPreeditPosition | XIMStatusCallbacks,
    XIMPreeditCallbacks | XIMStatusCallbacks,
    XIMPreeditNothing | XIMStatusCallbacks,
    0
};

FCITX_EXPORT_API
FcitxFrontend frontend = {
    XimCreate,
    XimDestroy,
    XimCreateIC,
    XimCheckIC,
    XimDestroyIC,
    XimEnableIM,
    XimCloseIM,
    XimCommitString,
    XimForwardKey,
    XimSetWindowOffset,
    XimGetWindowPosition,
    XimUpdatePreedit,
    NULL,
    NULL,
    XimCheckICFromSameApplication,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

FcitxXimFrontend *ximfrontend;

CONFIG_DESC_DEFINE(GetXimConfigDesc, "fcitx-xim.desc")

/* Supported Chinese Encodings */
static XIMEncoding zhEncodings[] = {
    "COMPOUND_TEXT",
    NULL
};

char strLocale[201] = "zh_CN.GB18030,zh_CN.GB2312,zh_CN,zh_CN.GBK,zh_CN.UTF-8,zh_CN.UTF8,en_US.UTF-8,en_US.UTF8";

Bool MyStrcmp(char *str1, char *str2)
{
    return !strncmp(str1, str2, strlen(str2));
}

void* XimCreate(FcitxInstance* instance, int frontendid)
{
    if (ximfrontend != NULL)
        return NULL;
    FcitxXimFrontend* xim = fcitx_utils_malloc0(sizeof(FcitxXimFrontend));
    if (xim == NULL)
        return NULL;

    ximfrontend = xim;

    XIMStyles *input_styles;
    XIMEncodings *encodings;
    char *imname = NULL;
    char *p;
    FcitxModuleFunctionArg arg;

    xim->display = InvokeFunction(instance, FCITX_X11, GETDISPLAY, arg);

    if (xim->display == NULL) {
        FcitxLog(FATAL, _("X11 not initialized"));
        free(xim);
        return NULL;
    }

    xim->iScreen = DefaultScreen(xim->display);
    xim->owner = instance;
    xim->frontendid = frontendid;

    xim->ximWindow = XCreateSimpleWindow(xim->display, DefaultRootWindow(xim->display), 0, 0, 1, 1, 1, 0, 0);
    if (xim->ximWindow == (Window) NULL) {
        FcitxLog(FATAL, _("Can't Create imWindow"));
        free(xim);
        return NULL;
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

    if (GetXimConfigDesc() == NULL)
        xim->bUseOnTheSpotStyle = false;
    else {
        FcitxConfigFileDesc* configDesc = GetXimConfigDesc();

        FILE *fp;
        char *file;
        fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-xim.config", "rt", &file);
        FcitxLog(DEBUG, "Load Config File %s", file);
        free(file);
        if (!fp) {
            if (errno == ENOENT) {
                char *file;
                FILE *fp2 = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-xim.config", "wt", &file);
                FcitxLog(DEBUG, "Save Config to %s", file);
                FcitxConfigSaveConfigFileFp(fp2, &xim->gconfig, configDesc);
                free(file);
                if (fp2)
                    fclose(fp2);
            }
        }

        FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

        FcitxXimFrontendConfigBind(xim, cfile, configDesc);
        FcitxConfigBindSync((FcitxGenericConfig*)xim);

        if (fp)
            fclose(fp);
    }

    input_styles = (XIMStyles *) malloc(sizeof(XIMStyles));
    if (xim->bUseOnTheSpotStyle) {
        input_styles->count_styles = sizeof(OnTheSpot_Styles) / sizeof(XIMStyle) - 1;
        input_styles->supported_styles = OnTheSpot_Styles;
    } else {
        input_styles->count_styles = sizeof(OverTheSpot_Styles) / sizeof(XIMStyle) - 1;
        input_styles->supported_styles = OverTheSpot_Styles;
    }

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

    xim->ims = IMOpenIM(xim->display,
                        IMModifiers, "Xi18n",
                        IMServerWindow, xim->ximWindow,
                        IMServerName, imname,
                        IMLocale, strLocale,
                        IMServerTransport, "X/",
                        IMInputStyles, input_styles,
                        IMEncodingList, encodings,
                        IMProtocolHandler, XimProtocolHandler,
                        IMFilterEventMask, KeyPressMask | KeyReleaseMask,
                        NULL);

    free(input_styles);
    free(encodings);

    if (xim->ims == (XIMS) NULL) {
        FcitxLog(ERROR, _("Start XIM error. Another XIM daemon named %s is running?"), imname);
        free(xim);
        return NULL;
    }
    return xim;
}

Bool XimProtocolHandler(XIMS _ims, IMProtocol * call_data)
{
    switch (call_data->major_code) {
    case XIM_OPEN:
        FcitxLog(DEBUG, "XIM_OPEN:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_CLOSE:
        FcitxLog(DEBUG, "XIM_CLOSE:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_CREATE_IC:
        FcitxLog(DEBUG, "XIM_CREATE_IC:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_DESTROY_IC:
        FcitxLog(DEBUG, "XIM_DESTROY_IC:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_SET_IC_VALUES:
        FcitxLog(DEBUG, "XIM_SET_IC_VALUES:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_GET_IC_VALUES:
        FcitxLog(DEBUG, "XIM_GET_IC_VALUES:\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_FORWARD_EVENT:
        FcitxLog(DEBUG, "XIM_FORWARD_EVENT:\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_SET_IC_FOCUS:
        FcitxLog(DEBUG, "XIM_SET_IC_FOCUS:\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_UNSET_IC_FOCUS:
        FcitxLog(DEBUG, "XIM_UNSET_IC_FOCUS:\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_RESET_IC:
        FcitxLog(DEBUG, "XIM_RESET_IC:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    case XIM_TRIGGER_NOTIFY:
        FcitxLog(DEBUG, "XIM_TRIGGER_NOTIFY:\t\ticid=%d\tconnect_id=%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id);
        break;
    default:
        FcitxLog(DEBUG, "XIM_DEFAULT:\t\ticid=%d\tconnect_id=%d\t%d", ((IMForwardEventStruct *) call_data)->icid,
                 ((IMForwardEventStruct *) call_data)->connect_id, call_data->major_code);
        break;
    }

    switch (call_data->major_code) {
    case XIM_OPEN:
        return XIMOpenHandler(ximfrontend, (IMOpenStruct *) call_data);
    case XIM_CLOSE:
        return XIMCloseHandler(ximfrontend, (IMOpenStruct *) call_data);
    case XIM_CREATE_IC:
        return XIMCreateICHandler(ximfrontend, (IMChangeICStruct *) call_data);
    case XIM_DESTROY_IC:
        return XIMDestroyICHandler(ximfrontend, (IMChangeICStruct *) call_data);
    case XIM_SET_IC_VALUES:
        return XIMSetICValuesHandler(ximfrontend, (IMChangeICStruct *) call_data);
    case XIM_GET_IC_VALUES:
        return XIMGetICValuesHandler(ximfrontend, (IMChangeICStruct *) call_data);
    case XIM_FORWARD_EVENT:
        XIMProcessKey(ximfrontend, (IMForwardEventStruct *) call_data);
        return True;
    case XIM_SET_IC_FOCUS:
        return XIMSetFocusHandler(ximfrontend, (IMChangeFocusStruct *) call_data);
    case XIM_UNSET_IC_FOCUS:
        return XIMUnsetFocusHandler(ximfrontend, (IMChangeICStruct *) call_data);;
    case XIM_RESET_IC:
        return True;
    case XIM_TRIGGER_NOTIFY:
        return XIMTriggerNotifyHandler(ximfrontend, (IMTriggerNotifyStruct *) call_data);
    case XIM_PREEDIT_START_REPLY:
        return 0;
    case XIM_PREEDIT_CARET_REPLY:
        return 0;
    case XIM_SYNC_REPLY:
        return 0;
    default:
        return True;
    }
}

boolean XimDestroy(void* arg)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;

    if (xim->ims)
        IMCloseIM(xim->ims);

    return true;
}

void XimEnableIM(void* arg, FcitxInputContext* ic)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    IMChangeFocusStruct call_data;
    FcitxXimIC* ximic = (FcitxXimIC*) ic->privateic;
    call_data.connect_id = ximic->connect_id;
    call_data.icid = ximic->id;
    IMPreeditStart(xim->ims, (XPointer) &call_data);
}

void XimCloseIM(void* arg, FcitxInputContext* ic)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    IMChangeFocusStruct call_data;
    FcitxXimIC* ximic = (FcitxXimIC*) ic->privateic;
    call_data.connect_id = ximic->connect_id;
    call_data.icid = ximic->id;
    IMPreeditEnd(xim->ims, (XPointer) &call_data);
}

void XimCommitString(void* arg, FcitxInputContext* ic, char* str)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    XTextProperty tp;
    IMCommitStruct cms;
    FcitxXimIC* ximic = (FcitxXimIC*) ic->privateic;

    /* avoid Seg fault */
    if (!ic)
        return;

    /*
     * I'm not sure whether xim should commit string before preedit done
     * but this can fix opera's crash in specific input box
     * quite strange.
     */
    if (GetXimIC(ic)->bPreeditStarted == true) {
        XimPreeditCallbackDraw(xim, GetXimIC(ic), "", 0);
        XimPreeditCallbackDone(xim, GetXimIC(ic));
        GetXimIC(ic)->bPreeditStarted = false;
    }

    Xutf8TextListToTextProperty(xim->display, (char **) &str, 1, XCompoundTextStyle, &tp);

    memset(&cms, 0, sizeof(IMCommitStruct));
    cms.major_code = XIM_COMMIT;
    cms.icid = ximic->id;
    cms.connect_id = ximic->connect_id;
    cms.flag = XimLookupChars;
    cms.commit_string = (char *) tp.value;
    IMCommitString(xim->ims, (XPointer) & cms);
    XFree(tp.value);
}

void XimForwardKey(void *arg, FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    XEvent xEvent;

    xEvent.xkey.type = (event == FCITX_PRESS_KEY) ? KeyPress : KeyRelease;
    xEvent.xkey.display = xim->display;
    xEvent.xkey.serial = xim->currentSerialNumberKey;
    xEvent.xkey.send_event = False;
    xEvent.xkey.x = xEvent.xkey.y = xEvent.xkey.x_root = xEvent.xkey.y_root = 0;
    xEvent.xkey.same_screen = False;
    xEvent.xkey.subwindow = None;
    xEvent.xkey.window = None;
    xEvent.xkey.root = DefaultRootWindow(xim->display);
    xEvent.xkey.state = state;
    if (GetXimIC(ic)->focus_win)
        xEvent.xkey.window = GetXimIC(ic)->focus_win;
    else if (GetXimIC(ic)->client_win)
        xEvent.xkey.window = GetXimIC(ic)->client_win;

    xEvent.xkey.keycode = XKeysymToKeycode(xim->display, sym);
    XimForwardKeyInternal(xim, GetXimIC(ic), &xEvent);
}

void XimSetWindowOffset(void* arg, FcitxInputContext* ic, int x, int y)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    FcitxXimIC* ximic = GetXimIC(ic);
    Window window = None, dst;
    if (ximic->focus_win)
        window = ximic->focus_win;
    else if (ximic->client_win)
        window = ximic->client_win;

    if (window != None) {
        XTranslateCoordinates(xim->display, RootWindow(xim->display, xim->iScreen), window,
                              x, y,
                              &ic->offset_x, &ic->offset_y,
                              &dst
                             );
    }
}

void XimGetWindowPosition(void* arg, FcitxInputContext* ic, int* x, int* y)
{
    *x = ic->offset_x;
    *y = ic->offset_y;
}

void XimUpdatePreedit(void* arg, FcitxInputContext* ic)
{
    FcitxXimFrontend* xim = (FcitxXimFrontend*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(xim->owner);
    char* strPreedit = FcitxUIMessagesToCString(FcitxInputStateGetClientPreedit(input));
    char* str = FcitxInstanceProcessOutputFilter(xim->owner, strPreedit);
    if (str) {
        free(strPreedit);
        strPreedit = str;
    }

    if (strlen(strPreedit) == 0 && GetXimIC(ic)->bPreeditStarted == true) {
        XimPreeditCallbackDraw(xim, GetXimIC(ic), strPreedit, 0);
        XimPreeditCallbackDone(xim, GetXimIC(ic));
        GetXimIC(ic)->bPreeditStarted = false;
    }

    if (strlen(strPreedit) != 0 && GetXimIC(ic)->bPreeditStarted == false) {
        XimPreeditCallbackStart(xim, GetXimIC(ic));
        GetXimIC(ic)->bPreeditStarted = true;
    }
    if (strlen(strPreedit) != 0) {
        XimPreeditCallbackDraw(xim, GetXimIC(ic), strPreedit, FcitxInputStateGetCursorPos(input));
    }

    free(strPreedit);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
