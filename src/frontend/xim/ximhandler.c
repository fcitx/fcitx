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

#include <X11/Xutil.h>

#include "fcitx/ui.h"
#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx-config/hotkey.h"
#include "fcitx-utils/log.h"
#include "xim.h"
#include "ximhandler.h"
#include "Xi18n.h"
#include "IC.h"

Bool XIMOpenHandler(FcitxXimFrontend* xim, IMOpenStruct * call_data)
{
    return True;
}


Bool XIMGetICValuesHandler(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    XimGetIC(xim, call_data);

    return True;
}

Bool XIMSetICValuesHandler(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    XimSetIC(xim, call_data);
    FcitxInputContext* ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
    SetTrackPos(xim, ic, call_data);

    return True;
}

Bool XIMSetFocusHandler(FcitxXimFrontend* xim, IMChangeFocusStruct * call_data)
{
    FcitxInputContext* ic =  FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
    if (ic == NULL)
        return True;

    if (!FcitxInstanceSetCurrentIC(xim->owner, ic))
        return True;

    SetTrackPos(xim, ic, NULL);

    if (ic) {
        FcitxUIOnInputFocus(xim->owner);
    } else {
        FcitxUICloseInputWindow(xim->owner);
        FcitxUIMoveInputWindow(xim->owner);
    }

    return True;
}

Bool XIMUnsetFocusHandler(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(xim->owner);
    if (ic && GetXimIC(ic)->id == call_data->icid) {
        FcitxInstanceSetCurrentIC(xim->owner, NULL);
        FcitxUICloseInputWindow(xim->owner);
        FcitxUIOnInputUnFocus(xim->owner);
    }

    return True;
}

Bool XIMCloseHandler(FcitxXimFrontend* xim, IMOpenStruct * call_data)
{
    FcitxUICloseInputWindow(xim->owner);
    FcitxInstanceSaveAllIM(xim->owner);
    return True;
}

Bool XIMCreateICHandler(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    FcitxInstanceCreateIC(xim->owner, xim->frontendid, call_data);
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(xim->owner);

    if (ic == NULL) {
        ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
        FcitxInstanceSetCurrentIC(xim->owner, ic);
    }

    return True;
}

Bool XIMDestroyICHandler(FcitxXimFrontend* xim, IMChangeICStruct * call_data)
{
    FcitxInstanceDestroyIC(xim->owner, xim->frontendid, &call_data->icid);

    return True;
}

Bool XIMTriggerNotifyHandler(FcitxXimFrontend* xim, IMTriggerNotifyStruct * call_data)
{
    FcitxInputContext* ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
    if (ic == NULL)
        return True;

    FcitxInstanceSetCurrentIC(xim->owner, ic);
    FcitxInstanceEnableIM(xim->owner, ic, false);
    FcitxUIOnTriggerOn(xim->owner);
    return True;
}


void SetTrackPos(FcitxXimFrontend* xim, FcitxInputContext* ic, IMChangeICStruct * call_data)
{
    if (ic == NULL)
        return;

    int i;
    FcitxXimIC* ximic = GetXimIC(ic);
    if (call_data) {
        XICAttribute *pre_attr = ((IMChangeICStruct *) call_data)->preedit_attr;

        for (i = 0; i < (int)((IMChangeICStruct *) call_data)->preedit_attr_num; i++, pre_attr++) {
            if (!strcmp(XNSpotLocation, pre_attr->name)) {
                ximic->bHasCursorLocation = true;
                ximic->offset_x = (*(XPoint *) pre_attr->value).x;
                ximic->offset_y = (*(XPoint *) pre_attr->value).y;
            }
        }
    }

    Window window = None;
    if (ximic->focus_win)
        window = ximic->focus_win;
    else if (ximic->client_win)
        window = ximic->client_win;


    if (window != None) {
        Window dst;
        XWindowAttributes attr;
        XGetWindowAttributes(xim->display, window, &attr);

        if (ximic->offset_x < 0 && ximic->offset_y < 0) {

            XTranslateCoordinates(xim->display, window, RootWindow(xim->display, xim->iScreen),
                                  0, attr.height,
                                  &ic->offset_x, &ic->offset_y,
                                  &dst
                                 );
        } else {
            XTranslateCoordinates(xim->display, window, RootWindow(xim->display, xim->iScreen),
                                  ximic->offset_x, ximic->offset_y,
                                  &ic->offset_x, &ic->offset_y,
                                  &dst);
        }
    }

    if (ic == FcitxInstanceGetCurrentIC(xim->owner))
        FcitxUIMoveInputWindow(xim->owner);
}

void XIMProcessKey(FcitxXimFrontend* xim, IMForwardEventStruct * call_data)
{
    KeySym originsym;
    FcitxKeySym sym;
    XKeyEvent *kev;
    int keyCount;
    unsigned int state, originstate;
    char strbuf[STRBUFLEN];
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(xim->owner);
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(xim->owner);
    FcitxInputState* input = FcitxInstanceGetInputState(xim->owner);

    if (ic == NULL) {
        ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
        FcitxInstanceSetCurrentIC(xim->owner, ic);
    }

    if (ic == NULL)
        return;

    if (GetXimIC(ic)->id != call_data->icid) {
        ic = FcitxInstanceFindIC(xim->owner, xim->frontendid, &call_data->icid);
        if (ic == NULL)
            return;
    }

    kev = (XKeyEvent *) & call_data->event;
    memset(strbuf, 0, STRBUFLEN);
    keyCount = XLookupString(kev, strbuf, STRBUFLEN, &originsym, NULL);

    originstate = kev->state - (kev->state & FcitxKeyState_NumLock) - (kev->state & FcitxKeyState_CapsLock) - (kev->state & FcitxKeyState_ScrollLock);
    originstate &= FcitxKeyState_UsedMask;
    FcitxHotkeyGetKey((FcitxKeySym) originsym, originstate, &sym, &state);
    FcitxLog(DEBUG,
             "KeyRelease=%d  state=%d  KEYCODE=%d  KEYSYM=%d  keyCount=%d",
             (call_data->event.type == KeyRelease), state, kev->keycode, (int) sym, keyCount);

    xim->currentSerialNumberCallData = call_data->serial_number;
    xim->currentSerialNumberKey = kev->serial;

    FcitxKeyEventType type = (call_data->event.type == KeyRelease) ? (FCITX_RELEASE_KEY) : (FCITX_PRESS_KEY);

    if (ic->state == IS_CLOSED) {
        if (type == FCITX_PRESS_KEY && FcitxHotkeyIsHotKey(sym, state, config->hkTrigger)) {
            FcitxInstanceEnableIM(xim->owner, ic, false);
            FcitxInputStateSetKeyReleased(input, KR_OTHER);
            return;
        } else {
            XimForwardKeyInternal(xim,
                                  GetXimIC(ic),
                                  &call_data->event
                                 );
            return;
        }
    }

    INPUT_RETURN_VALUE retVal = FcitxInstanceProcessKey(xim->owner, type,
                                           kev->time,
                                           sym, state);

    if ((retVal & IRV_FLAG_FORWARD_KEY) || retVal == IRV_TO_PROCESS) {
        XimForwardKeyInternal(xim,
                              GetXimIC(ic),
                              &call_data->event
                             );
    } else {
        if (!GetXimIC(ic)->bHasCursorLocation)
            SetTrackPos(xim, ic, NULL);
    }
    xim->currentSerialNumberCallData = xim->currentSerialNumberKey = 0L;
}


void XimForwardKeyInternal(FcitxXimFrontend *xim,
                           FcitxXimIC* ic,
                           XEvent* xEvent
                          )
{
    IMForwardEventStruct forwardEvent;

    memset(&forwardEvent, 0, sizeof(IMForwardEventStruct));
    forwardEvent.connect_id = ic->connect_id;
    forwardEvent.icid = ic->id;
    forwardEvent.major_code = XIM_FORWARD_EVENT;
    forwardEvent.sync_bit = 0;
    forwardEvent.serial_number = xim->currentSerialNumberCallData;

    memcpy(&(forwardEvent.event), xEvent, sizeof(XEvent));
    IMForwardEvent(xim->ims, (XPointer)(&forwardEvent));
}

void XIMClose(FcitxXimFrontend* xim, FcitxInputContext* ic, FcitxKeySym sym, unsigned int state, int count)
{
    if (ic == NULL)
        return;
    IMChangeFocusStruct call_data;

    call_data.connect_id = GetXimIC(ic)->connect_id;
    call_data.icid = GetXimIC(ic)->id;

    IMPreeditEnd(xim->ims, (XPointer) &call_data);
}


void
XimPreeditCallbackStart(FcitxXimFrontend *xim, const FcitxXimIC* ic)
{
    IMPreeditCBStruct pcb;

    pcb.major_code = XIM_PREEDIT_START;
    pcb.minor_code = 0;
    pcb.connect_id = ic->connect_id;
    pcb.icid = ic->id;
    pcb.todo.return_value = 0;
    IMCallCallback(xim->ims, (XPointer) & pcb);
}


void
XimPreeditCallbackDone(FcitxXimFrontend *xim, const FcitxXimIC* ic)
{
    IMPreeditCBStruct pcb;

    pcb.major_code = XIM_PREEDIT_DONE;
    pcb.minor_code = 0;
    pcb.connect_id = ic->connect_id;
    pcb.icid = ic->id;
    pcb.todo.return_value = 0;
    IMCallCallback(xim->ims, (XPointer) & pcb);
}

void
XimPreeditCallbackDraw(FcitxXimFrontend* xim, FcitxXimIC* ic, const char* preedit_string, int cursorPos)
{
    IMPreeditCBStruct pcb;
    XIMText text;
    XTextProperty tp;

    uint i, len;

    if (preedit_string == NULL)
        return;

    len = fcitx_utf8_strlen(preedit_string);

    if (len + 1 > xim->feedback_len) {
        xim->feedback_len = len + 1;
        if (xim->feedback) {
            xim->feedback = realloc(xim->feedback, sizeof(XIMFeedback) * xim->feedback_len);
        } else {
            xim->feedback = fcitx_utils_malloc0(sizeof(XIMFeedback) * xim->feedback_len);
        }
    }

    for (i = 0; i < len; i++) {
        xim->feedback[i] = XIMUnderline;
    }
    xim->feedback[len] = 0;

    pcb.major_code = XIM_PREEDIT_DRAW;
    pcb.connect_id = ic->connect_id;
    pcb.icid = ic->id;

    pcb.todo.draw.caret = cursorPos;
    pcb.todo.draw.chg_first = 0;
    pcb.todo.draw.chg_length = ic->onspot_preedit_length;
    pcb.todo.draw.text = &text;

    text.feedback = xim->feedback;

    if (len > 0) {
        Xutf8TextListToTextProperty(xim->display,
                                    (char **)&preedit_string,
                                    1, XCompoundTextStyle, &tp);
        text.encoding_is_wchar = 0;
        text.length = strlen((char*)tp.value);
        text.string.multi_byte = (char*)tp.value;
        IMCallCallback(xim->ims, (XPointer) & pcb);
        XFree(tp.value);
    } else {
        text.encoding_is_wchar = 0;
        text.length = 0;
        text.string.multi_byte = "";
        IMCallCallback(xim->ims, (XPointer) & pcb);
        len = 0;
    }
    ic->onspot_preedit_length = len;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
