/***************************************************************************
 *   Copyright (C) 2008~2010 by Zealot.Hoi                                 *
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
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include "config.h"
#include <fcitx/ui.h>
#include <fcitx-utils/cutils.h>
#include <dbus/dbus.h>
#include <fcitx/instance.h>

static void* KimpanelCreate(FcitxInstance* instance);
static void KimpanelCloseInputWindow(void* arg);
static void KimpanelShowInputWindow(void* arg);
static void KimpanelMoveInputWindow(void* arg);
static void KimpanelRegisterMenu(void *arg, FcitxUIMenu* menu);
static void KimpanelUpdateStatus(void *arg, FcitxUIStatus* status);
static void KimpanelRegisterStatus(void *arg, FcitxUIStatus* status);
static void KimpanelOnInputFocus(void *arg);
static void KimpanelOnInputUnFocus(void *arg);
static void KimpanelOnTriggerOn(void *arg);
static void KimpanelOnTriggerOff(void *arg);
static void KimpanelDisplayMessage(void *arg, char *title, char **msg, int length);
static void KimpanelInputReset(void *arg);

typedef struct FcitxKimpanelUI
{
    FcitxInstance* owner;
} FcitxKimpanelUI;

FCITX_EXPORT_API
FcitxUI ui = {
    KimpanelCreate,
    KimpanelCloseInputWindow,
    KimpanelShowInputWindow,
    KimpanelMoveInputWindow,
    KimpanelUpdateStatus,
    KimpanelRegisterStatus,
    KimpanelRegisterMenu,
    KimpanelOnInputFocus,
    KimpanelOnInputUnFocus,
    KimpanelOnTriggerOn,
    KimpanelOnTriggerOff,
    KimpanelDisplayMessage,
    NULL
};

void* KimpanelCreate(FcitxInstance* instance)
{
    FcitxKimpanelUI *kimpanel = fcitx_malloc0(sizeof(FcitxKimpanelUI));
    kimpanel->owner = instance;
    return kimpanel;
}


void KimpanelDBusEventHandler(void* arg, DBusMessage* msg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    FcitxIM* im = GetCurrentIM(instance);
    DBusMessageIter args;
    int int0;

    if (dbus_message_is_signal(msg, "org.kde.impanel", "MovePreeditCaret")) {
        FcitxLog(DEBUG, "MovePreeditCaret");
        // read the parameters
        if (!dbus_message_iter_init(msg, &args))
            FcitxLog(DEBUG, "Message has no arguments!"); 
        else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
            FcitxLog(DEBUG, "Argument is not INT32!"); 
        else {
            dbus_message_iter_get_basic(&args, &int0);
            FcitxLog(DEBUG, "Got Signal with value %d", int0);
        }
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "SelectCandidate")) {
        FcitxLog(DEBUG, "SelectCandidate: ");
        // read the parameters
        if (!dbus_message_iter_init(msg, &args))
            FcitxLog(DEBUG, "Message has no arguments!"); 
        else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
            FcitxLog(DEBUG, "Argument is not INT32!"); 
        else {
            dbus_message_iter_get_basic(&args, &int0);

            char *pstr;

            if (!bIsInLegend) {
                if (im->GetCandWord)
                {
                    pstr = im->GetCandWord(im->klass, 1);
                    if (pstr) {
                        SendHZtoClient(0, pstr);
                        if (!fcitxProfile.bUseLegend) {
                            ResetInput();
                        }
                    }
                }
            } else {
                pstr = im->GetLegendCandWord(int0);
                if (pstr) {
                    SendHZtoClient(0, pstr);
                    if (!iLegendCandWordCount) {
                        ResetInput ();
                    }
                }
            }
            FcitxLog(DEBUG, "%d:%s"), int0,pstr);
        }
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageUp")) {
        FcitxLog(DEBUG, "LookupTablePageUp"));
        im[gs.iIMIndex].GetCandWords (SM_PREV);
        updateMessages();
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageDown")) {
        FcitxLog(DEBUG, "LookupTablePageDown"));
        im[gs.iIMIndex].GetCandWords (SM_NEXT);
        updateMessages();
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "TriggerProperty")) {
        FcitxLog(DEBUG, "TriggerProperty: "));
        // read the parameters
        if (!dbus_message_iter_init(msg, &args))
            FcitxLog(DEBUG, "Message has no arguments!")); 
        else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
            FcitxLog(DEBUG, "Argument is not STRING!")); 
        else {
            dbus_message_iter_get_basic(&args, &s0);
            FcitxLog(DEBUG, "%s"), s0);

            triggerProperty(s0);
        }
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "PanelCreated")) {
        FcitxLog(DEBUG, "PanelCreated"));
        registerProperties();
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "Exit")) {
        FcitxLog(DEBUG, "Exit"));
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "ReloadConfig")) {
        FcitxLog(DEBUG, "ReloadConfig"));
    }
}


void KIMExecDialog(char *prop)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "ExecDialog"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null"));
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMExecMenu(char *props[],int n)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "ExecMenu"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    if (n == -1) {
        n = 0;
        while (*(props[n])!= 0) {
            n++;
        }
    }

    int i;
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&sub);
    for (i = 0; i < n; i++) {
        if (!dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &props[i])) { 
            FcitxLog(DEBUG, "Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMRegisterProperties(char *props[], int n)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "RegisterProperties"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    if (n == -1) {
        n = 0;
        while (*(props[n])!= 0) {
            n++;
        }
    }

    int i;
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&sub);
    for (i = 0; i < n; i++) {
        if (!dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &props[i])) { 
            FcitxLog(DEBUG, "Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdateProperty(char *prop)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdateProperty"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMRemoveProperty(char *prop)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "RemoveProperty"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMEnable(Bool toEnable)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "Enable"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toEnable)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMShowAux(Bool toShow)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "ShowAux"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMShowPreedit(Bool toShow)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "ShowPreedit"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMShowLookupTable(Bool toShow)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "ShowLookupTable"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdateLookupTable(char *labels[], int nLabel, char *texts[], int nText, Bool has_prev, Bool has_next)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdateLookupTable"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    int i;
    DBusMessageIter subLabel;
    DBusMessageIter subText;
    DBusMessageIter subAttrs;
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subLabel);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subLabel, DBUS_TYPE_STRING, &labels[i])) { 
            FcitxLog(DEBUG, "Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&subLabel);

    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subText);
    for (i = 0; i < nText; i++) {
        if (!dbus_message_iter_append_basic(&subText, DBUS_TYPE_STRING, &texts[i])) { 
            FcitxLog(DEBUG, "Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&subText);

    char *attr = "";
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subAttrs);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subAttrs, DBUS_TYPE_STRING, &attr)) { 
            FcitxLog(DEBUG, "Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&subAttrs);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_prev);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_next);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdatePreeditCaret(int position)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdatePreeditCaret"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &position)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdatePreeditText(char *text)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdatePreeditText"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    char *attr = "";
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdateAux(char *text)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdateAux"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    char *attr = "";

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdateSpotLocation(int x,int y)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdateSpotLocation"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &x)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &y)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void KIMUpdateScreen(int id)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors 
    msg = dbus_message_new_signal("/fcitx", // object name of the signal
            "org.kde.fcitx", // interface name of the signal
            "UpdateScreen"); // name of the signal
    if (NULL == msg) 
    { 
        FcitxLog(DEBUG, "Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &id)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!")); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void updateMessages()
{
    inputWindow.bShowPrev = inputWindow.bShowNext = False;

    int n = messageDown.msgCount;
    int nLabels = 0;
    int nTexts = 0;
    char *label[33];
    char *text[33];
    char cmb[100] = "";
    int i;

    if (n) {
        for (i=0;i<n;i++) {
            FcitxLog(DEBUG, "Type: %d Text: %s"),messageDown.msg[i].type,messageDown.msg[i].strMsg);

            if (messageDown.msg[i].type == MSG_INDEX) {
                if (nLabels) {
                    text[nTexts++] = fcitxProfile.bUseGBKT ? ConvertGBKSimple2Tradition(cmb) : strdup(cmb);
                }
                label[nLabels++] = strdup(messageDown.msg[i].strMsg);
                strcpy(cmb,"");
            } else {
                strcat(cmb,(messageDown.msg[i].strMsg));
            }
        }
        text[nTexts++] = fcitxProfile.bUseGBKT ? ConvertGBKSimple2Tradition(cmb) : strdup(cmb);
/*        if (nLabels < nTexts) {
            nTexts = nLabels;
        }*/
        if (nLabels < nTexts ) {
            for (; nLabels < nTexts; nLabels++) {
                label[nLabels] = strdup("");
            }
        }
        else if (nTexts < nLabels) {
            for (; nTexts < nLabels; nTexts++) {
                text[nTexts] = strdup("");
            }
        }
        FcitxLog(DEBUG, "Labels %d, Texts %d, CMB:%s"),nLabels,nTexts, cmb); 
        if (nTexts == 0) {
            KIMShowLookupTable(False);
        }
        else {
            if (bIsInLegend) {
            if (iCurrentLegendCandPage > 0)
                inputWindow.bShowPrev = True;
            if (iCurrentLegendCandPage < iLegendCandPageCount)
                inputWindow.bShowNext = True;
            } else {
            if (iCurrentCandPage > 0)
            inputWindow.bShowPrev = True;
            if (iCurrentCandPage < iCandPageCount)
                inputWindow.bShowNext = True;
            }
            KIMUpdateLookupTable(label,nLabels,text,nTexts,inputWindow.bShowPrev,inputWindow.bShowNext);
            KIMShowLookupTable(True);
        }
        for (i = 0; i < nTexts; i++)
            free(text[i]);
        for (i = 0; i < nLabels; i++)
            free(label[i]);
    } else {
        KIMUpdateLookupTable(NULL,0,NULL,0,inputWindow.bShowPrev,inputWindow.bShowNext);
        KIMShowLookupTable(False);
    }
    
    n = messageUp.msgCount;
    char aux[MESSAGE_MAX_LENGTH] = "";
    char empty[MESSAGE_MAX_LENGTH] = "";
    if (n) {
        char* strGBKT;
        // FIXME: buffer overflow
        for (i=0;i<n;i++) {
            strcat(aux,messageUp.msg[i].strMsg);
            FcitxLog(DEBUG, "updateMesssages Up:%s"), aux);
        }
        if (inputWindow.bShowCursor)
        {
            strGBKT = fcitxProfile.bUseGBKT ? ConvertGBKSimple2Tradition(aux) : aux;
            KIMUpdatePreeditText(strGBKT);
            KIMUpdateAux(empty);
            KIMShowPreedit(True);
            KIMUpdatePreeditCaret(calKIMCursorPos());
            KIMShowAux(False);
        }
        else {
            strGBKT = fcitxProfile.bUseGBKT ? ConvertGBKSimple2Tradition(aux) : aux;
            KIMUpdatePreeditText(empty);
            KIMUpdateAux(strGBKT);
            KIMShowPreedit(False);
            KIMShowAux(True);
        }
        if (fcitxProfile.bUseGBKT)
            free(strGBKT);
    } else {
        KIMShowPreedit(False);
        KIMShowAux(False);
    }
}

void showMessageDown(MESSAGE *msgs, int n)
{
}

void registerProperties()
{
    char *props[10];

    int i = 0;

    KIMRegisterProperties(props,7);
}

void updateProperty(Property *prop)
{
    if (!prop) {
        return;
    }
    // logo_prop need to be handled specially (enable/disable)
    if (prop == &logo_prop) {
        fixProperty(prop);
    }
    if (prop == &state_prop) {
        prop->label = (GetCurrentState() == IS_ACTIVE) ? TOOLTIP_STATE_CHN: TOOLTIP_STATE_ENG;
        if (GetCurrentState() == IS_ACTIVE) {
            prop->icon = "fcitx-chn";
        } else {
            prop->icon = "fcitx-eng";
        }
    }
    if (prop == &punc_prop) {
        prop->label = fcitxProfile.bWidePunc ? TOOLTIP_PUNK_CHN : TOOLTIP_PUNK_ENG;
        if (fcitxProfile.bWidePunc) {
            prop->icon = "fcitx-full-punct";
        } else {
            prop->icon = "fcitx-half-punct";
        }
    }
    if (prop == &corner_prop) {
        prop->label = fcitxProfile.bCorner ? TOOLTIP_CORNER_ENABLE : TOOLTIP_CORNER_DISABLE;
        if (fcitxProfile.bCorner) {
            prop->icon = "fcitx-full-letter";
        } else {
            prop->icon = "fcitx-half-letter";
        }
    }
    if (prop == &vk_prop) {
        prop->label = bVK ? TOOLTIP_VK_ENABLE : TOOLTIP_VK_DISABLE;
        if (bVK) {
            prop->icon = "fcitx-vkon";
        } else {
            prop->icon = "fcitx-vkoff";
        }
    }
    if (prop == &gbkt_prop) {
        prop->label = fcitxProfile.bUseGBKT ? TOOLTIP_GBKT_ENABLE : TOOLTIP_GBKT_DISABLE;
        if (fcitxProfile.bUseGBKT) {
            prop->icon = "fcitx-trad";
        } else {
            prop->icon = "fcitx-simp";
        }
    }
    if (prop == &legend_prop) {
        prop->label = fcitxProfile.bUseLegend ? TOOLTIP_LEGEND_ENABLE : TOOLTIP_LEGEND_DISABLE;
        if (fcitxProfile.bUseLegend) {
            prop->icon = "fcitx-legend";
        } else {
            prop->icon = "fcitx-nolegend";
        }
    }
    char* need_free;
    KIMUpdateProperty((need_free = property2string(prop)));
    free(need_free);
}

int calKIMCursorPos()
{
    int             i = 0;
    int             iChar;
    int             iCount = 0;

    const char      *p1;
    const char      *pivot;

    iChar = iCursorPos;

    for (i = 0; i < messageUp.msgCount ; i++) {
        if (inputWindow.bShowCursor && iChar) {
            p1 = pivot = messageUp.msg[i].strMsg;
            while (*p1 && p1 < pivot + iChar) {
                p1 = p1 + utf8_char_len(p1);
                iCount ++;
            }
            if (strlen (messageUp.msg[i].strMsg) > iChar) {
            iChar = 0;
            }
            else {
                iChar -= strlen (messageUp.msg[i].strMsg);
            }
        }
    }

    return iCount;
}
// vim: sw=4 sts=4 et tw=100
