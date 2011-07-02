/***************************************************************************
 *   Copyright (C) 2008~2010 by Zealot.Hoi                                 *
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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
#include <libintl.h>
#include "config.h"
#include <fcitx/ui.h>
#include <fcitx-utils/cutils.h>
#include <dbus/dbus.h>
#include "module/dbus/dbusstuff.h"
#include <fcitx/instance.h>
#include <fcitx/module.h>
#include <fcitx/backend.h>
#include <fcitx/hook.h>
#include <fcitx/ime-internal.h>

typedef struct FcitxKimpanelUI
{
    FcitxInstance* owner;
    DBusConnection* conn;
    int iOffsetY;
    int iOffsetX;
} FcitxKimpanelUI;

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

static void KimShowAux(FcitxKimpanelUI* kimpanel, boolean toShow);
static void KimShowPreedit(FcitxKimpanelUI* kimpanel, boolean toShow);
static void KimUpdateSpotLocation(FcitxKimpanelUI* kimpanel, int x, int y);
static void KimShowLookupTable(FcitxKimpanelUI* kimpanel, boolean toShow);
static void KimUpdateLookupTable(FcitxKimpanelUI* kimpanel, char *labels[], int nLabel, char *texts[], int nText, boolean has_prev, boolean has_next);
static void KimUpdatePreeditText(FcitxKimpanelUI* kimpanel, char *text);
static void KimUpdateAux(FcitxKimpanelUI* kimpanel, char *text);
static void KimUpdatePreeditCaret(FcitxKimpanelUI* kimpanel, int position);
static void KimRegisterProperties(FcitxKimpanelUI* kimpanel, char *props[], int n);
static void KimUpdateProperty(FcitxKimpanelUI* kimpanel, char *prop);
static boolean KimpanelDBusEventHandler(void* arg, DBusMessage* msg);
static int CalKimCursorPos(FcitxKimpanelUI *kimpanel);
static void KimpanelInputReset(void *arg);
static char* Status2String(FcitxUIStatus* status);
static void KimpanelRegisterBuiltinStatus(FcitxKimpanelUI* kimpanel);
static void KimpanelSetIMStatus(FcitxKimpanelUI* kimpanel);
static void KimExecMenu(FcitxKimpanelUI* kimpanel, char *props[],int n);

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
    NULL,
    NULL,
    NULL
};

void* KimpanelCreate(FcitxInstance* instance)
{
    FcitxKimpanelUI *kimpanel = fcitx_malloc0(sizeof(FcitxKimpanelUI));
    FcitxModuleFunctionArg arg;

    kimpanel->owner = instance;
    kimpanel->conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);
        
    // add a rule to receive signals from kimpanel
    DBusError err;
    dbus_error_init(&err);
    dbus_bus_add_match(kimpanel->conn,
            "type='signal',interface='org.kde.impanel'", 
            &err);
    dbus_connection_flush(kimpanel->conn);
    if (dbus_error_is_set(&err)) { 
        FcitxLog(ERROR, "Match Error (%s)", err.message);
        return NULL;
    }

    arg.args[0] = KimpanelDBusEventHandler;
    arg.args[1] = kimpanel;
    InvokeFunction(instance, FCITX_DBUS, ADDEVENTHANDLER, arg);
    
    FcitxIMEventHook resethk;
    resethk.arg = kimpanel;
    resethk.func = KimpanelInputReset;
    RegisterResetInputHook(resethk);
    
    KimpanelRegisterBuiltinStatus(kimpanel);
    return kimpanel;
}

void KimpanelRegisterBuiltinStatus(FcitxKimpanelUI* kimpanel)
{
    char* prop[2] = { NULL, NULL };
    asprintf(&prop[0], "/Fcitx/logo:%s:%s:%s", _("Fcitx"), "fcitx", _("Fcitx"));
    asprintf(&prop[1], "/Fcitx/im:%s:%s:%s", _("Disabled"), "fcitx-eng", _("Input Method Disabled"));
    
    KimRegisterProperties(kimpanel, prop, 2);
    free(prop[0]);
    free(prop[1]);
}

void KimpanelSetIMStatus(FcitxKimpanelUI* kimpanel)
{    
    FcitxInstance* instance = kimpanel->owner;
    char* status = NULL;
    char* icon;
    char* imname;
    char* description;
    if (GetCurrentState(instance) != IS_ACTIVE )
    {
        icon = "eng";
        imname = _("Disabled");
        description = _("Input Method Disabled");
    }
    else
    {
        FcitxIM* im = GetCurrentIM(instance);
        icon = im->strIconName;
        imname = _(im->strName);
        description = _(im->strName);
    }
    /* add fcitx- prefix */
    asprintf(&status, "/Fcitx/im:%s:fcitx-%s:%s", imname, icon, description );
    
    KimUpdateProperty(kimpanel, status);
    free(status);
}

void KimpanelInputReset(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnInputFocus(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnInputUnFocus(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnTriggerOff(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnTriggerOn(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelCloseInputWindow(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    FcitxLog(DEBUG, "KimpanelCloseInputWindow");
    /* why kimpanel sucks, there is not obvious method to close it */
    KimShowAux(kimpanel, false);
    KimShowPreedit(kimpanel, false);
    KimShowLookupTable(kimpanel, false);
}

void KimpanelMoveInputWindow(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    FcitxLog(DEBUG, "KimpanelMoveInputWindow");
    kimpanel->iOffsetX = 12;
    kimpanel->iOffsetY = 0;
    
    int x, y;
    
    FcitxInputContext* ic = GetCurrentIC(kimpanel->owner);
    GetWindowPosition(kimpanel->owner, ic, &x, &y);

    int iTempInputWindowX, iTempInputWindowY;

    if (x < 0)
        iTempInputWindowX = 0;
    else
        iTempInputWindowX = x + kimpanel->iOffsetX;

    if (y < 0)
        iTempInputWindowY = 0;
    else
        iTempInputWindowY = y + kimpanel->iOffsetY;

    KimUpdateSpotLocation(kimpanel, x, y);
}

void KimpanelRegisterMenu(void* arg, FcitxUIMenu* menu)
{
    return ;
}

void KimpanelRegisterStatus(void* arg, FcitxUIStatus* status)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    char *prop[1];
    prop[0] = Status2String(status);
    KimRegisterProperties(kimpanel, prop, 1);
    free(prop[0]);
    return ;
}

char* Status2String(FcitxUIStatus* status)
{
    char *result = NULL;
    asprintf(&result, "/Fcitx/%s:%s:%s_%s:%s",
             status->name,
             status->shortDescription,
             status->name,
             ((status->getCurrentStatus(status->arg)) ? "active": "inactive"),
             status->longDescription
             );

    return result;
}

void KimpanelShowInputWindow(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    FcitxInstance* instance = kimpanel->owner;
    FcitxInputState* input = &instance->input;
    Messages* messageDown = GetMessageDown(instance);
    Messages* messageUp = GetMessageUp(instance);
    FcitxLog(DEBUG, "KimpanelShowInputWindow");

    int n = GetMessageCount(messageDown);
    int nLabels = 0;
    int nTexts = 0;
    char *label[33];
    char *text[33];
    char cmb[100] = "";
    int i;

    if (n) {
        for (i=0;i<n;i++) {
            FcitxLog(DEBUG, "Type: %d Text: %s" , GetMessageType(messageDown, i), GetMessageString(messageDown, i));

            if (GetMessageType(messageDown, i) == MSG_INDEX) {
                if (nLabels) {
                    text[nTexts++] = strdup(cmb);
                }
                label[nLabels++] = strdup( GetMessageString(messageDown, i) );
                strcpy(cmb,"");
            } else {
                strcat(cmb, GetMessageString(messageDown, i));
            }
        }
        text[nTexts++] = strdup(cmb);
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
        FcitxLog(DEBUG, "Labels %d, Texts %d, CMB:%s", nLabels, nTexts, cmb); 
        if (nTexts == 0) {
            KimShowLookupTable(kimpanel, false);
        }
        else {
            KimUpdateLookupTable(kimpanel, label,nLabels,text,nTexts,input->bShowPrev,input->bShowNext);
            KimShowLookupTable(kimpanel, true);
        }
        for (i = 0; i < nTexts; i++)
            free(text[i]);
        for (i = 0; i < nLabels; i++)
            free(label[i]);
    } else {
        KimUpdateLookupTable(kimpanel, NULL,0,NULL,0,input->bShowPrev,input->bShowNext);
        KimShowLookupTable(kimpanel, false);
    }
    
    n = GetMessageCount(messageUp);
    char aux[MESSAGE_MAX_LENGTH] = "";
    char empty[MESSAGE_MAX_LENGTH] = "";
    if (n) {
        char* strGBKT;
        for (i=0;i<n;i++) {
            strcat(aux, GetMessageString(messageUp, i));
            FcitxLog(DEBUG, "updateMesssages Up:%s", aux);
        }
        if (instance->bShowCursor)
        {
            strGBKT = aux;
            KimUpdatePreeditText(kimpanel, strGBKT);
            KimUpdateAux(kimpanel, empty);
            KimShowPreedit(kimpanel, true);
            KimUpdatePreeditCaret(kimpanel, CalKimCursorPos(kimpanel));
            KimShowAux(kimpanel, false);
        }
        else {
            strGBKT = aux;
            KimUpdatePreeditText(kimpanel, empty);
            KimUpdateAux(kimpanel, strGBKT);
            KimShowPreedit(kimpanel, false);
            KimShowAux(kimpanel, true);
        }
    } else {
        KimShowPreedit(kimpanel, false);
        KimShowAux(kimpanel, false);
    }

}

void KimpanelUpdateStatus(void* arg, FcitxUIStatus* status)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    char *prop = NULL;
    prop = Status2String(status);
    KimUpdateProperty(kimpanel, prop);
    return ;
}

boolean KimpanelDBusEventHandler(void* arg, DBusMessage* msg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    FcitxInstance* instance = kimpanel->owner;
    FcitxInputState* input = &kimpanel->owner->input;
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
        return true;
    }
    else if (dbus_message_is_signal(msg, "org.kde.impanel", "SelectCandidate")) {
        FcitxLog(DEBUG, "SelectCandidate: ");
        // read the parameters
        if (!dbus_message_iter_init(msg, &args))
            FcitxLog(DEBUG, "Message has no arguments!"); 
        else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
            FcitxLog(DEBUG, "Argument is not INT32!"); 
        else {
            dbus_message_iter_get_basic(&args, &int0);

            char *pstr;

            if (im->GetCandWord)
            {
                pstr = im->GetCandWord(im->klass, int0);
                INPUT_RETURN_VALUE retVal;
                if (pstr) {
                    strcpy(input->strStringGet, pstr);
                    if (input->bIsInLegend)
                        retVal = IRV_GET_LEGEND;
                    else
                        retVal = IRV_GET_CANDWORDS;
                } else if (input->iCandWordCount != 0)
                    retVal = IRV_DISPLAY_CANDWORDS;
                else
                    retVal = IRV_TO_PROCESS;
                ProcessInputReturnValue(instance, retVal);
            }
            FcitxLog(DEBUG, "%d:%s", int0, pstr);
        }
        return true;
    }
    else if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageUp")) {
        FcitxLog(DEBUG, "LookupTablePageUp");
        INPUT_RETURN_VALUE retVal = im->GetCandWords (im->klass, SM_PREV);
        ProcessInputReturnValue(instance, retVal);
        return true;
    }
    else if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageDown")) {
        FcitxLog(DEBUG, "LookupTablePageDown");
        INPUT_RETURN_VALUE retVal = im->GetCandWords (im->klass,SM_NEXT);
        ProcessInputReturnValue(instance, retVal);
        return true;
    }
    else if (dbus_message_is_signal(msg, "org.kde.impanel", "TriggerProperty")) {
        FcitxLog(DEBUG, "TriggerProperty: ");
        // read the parameters
        if (!dbus_message_iter_init(msg, &args))
            FcitxLog(DEBUG, "Message has no arguments!"); 
        else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
            FcitxLog(DEBUG, "Argument is not STRING!"); 
        else {
            const char* s0;
            dbus_message_iter_get_basic(&args, &s0);

            size_t len = strlen("/Fcitx/");
            if (strlen(s0) > len)
            {
                s0 += len;
                if (strcmp("logo", s0) == 0)
                {
                    if (GetCurrentState(instance) == IS_CLOSED) {
                        EnableIM(instance, GetCurrentIC(instance), false);
                    }
                    else {
                        CloseIM(instance, GetCurrentIC(instance));
                    }
                }
                else if (strncmp("im/", s0, strlen("im/")) == 0)
                {
                    s0 += strlen("im/");
                    int index = atoi(s0);
                    SwitchIM(instance, index);
                }
                else if (strncmp("im", s0, strlen("im")) == 0)
                {
                    UT_array* imes = &instance->imes;
                    FcitxIM* pim;
                    int index;
                    size_t len = utarray_len(imes);
                    char **prop = fcitx_malloc0(len * sizeof(char*));
                    for (pim = (FcitxIM *) utarray_front(imes);
                         pim != NULL;
                         pim = (FcitxIM *) utarray_next(imes, pim))
                    {
                        asprintf(&prop[index], "/Fcitx/im/%d:%s:fcitx-%s:%s", index, _(pim->strName), pim->strIconName, _(pim->strName));
                        index ++;
                    }
                    KimExecMenu(kimpanel, prop , len);
                    while(len -- )
                        free(prop[len]);
                }
                else
                    UpdateStatus(instance, s0);
            }            
        }
        return true;
    }
    else if (dbus_message_is_signal(msg, "org.kde.impanel", "PanelCreated")) {
        FcitxLog(DEBUG, "PanelCreated");
        
        KimpanelRegisterBuiltinStatus(kimpanel);
        
        UT_array* uistats = &instance->uistats;
        FcitxUIStatus *status;
        for ( status = (FcitxUIStatus *) utarray_front(uistats);
            status != NULL;
            status = (FcitxUIStatus *) utarray_next(uistats, status))
            KimpanelRegisterStatus(kimpanel, status);
        return true;
    }
    else if (dbus_message_is_signal(msg, "org.kde.impanel", "Exit")) {
        FcitxLog(DEBUG, "Exit");
        EndInstance(instance);
        return true;
    }
    if (dbus_message_is_signal(msg, "org.kde.impanel", "ReloadConfig")) {
        FcitxLog(DEBUG, "ReloadConfig");
        return true;
    }
    
    return false;
}


void KimExecDialog(FcitxKimpanelUI* kimpanel, char *prop)
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
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimExecMenu(FcitxKimpanelUI* kimpanel, char *props[],int n)
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
        FcitxLog(DEBUG, "Message Null"); 
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
            FcitxLog(DEBUG, "Out Of Memory!"); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimRegisterProperties(FcitxKimpanelUI* kimpanel, char *props[], int n)
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
        FcitxLog(DEBUG, "Message Null"); 
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
            FcitxLog(DEBUG, "Out Of Memory!"); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdateProperty(FcitxKimpanelUI* kimpanel, char *prop)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimRemoveProperty(FcitxKimpanelUI* kimpanel, char *prop)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

#ifdef UNUSED
void KimEnable(FcitxKimpanelUI* kimpanel, boolean toEnable)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toEnable)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}
#endif

void KimShowAux(FcitxKimpanelUI* kimpanel, boolean toShow)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimShowPreedit(FcitxKimpanelUI* kimpanel, boolean toShow)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimShowLookupTable(FcitxKimpanelUI* kimpanel, boolean toShow)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdateLookupTable(FcitxKimpanelUI* kimpanel, char *labels[], int nLabel, char *texts[], int nText, boolean has_prev, boolean has_next)
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
        FcitxLog(DEBUG, "Message Null"); 
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
            FcitxLog(DEBUG, "Out Of Memory!"); 
        }
    }
    dbus_message_iter_close_container(&args,&subLabel);

    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subText);
    for (i = 0; i < nText; i++) {
        if (!dbus_message_iter_append_basic(&subText, DBUS_TYPE_STRING, &texts[i])) { 
            FcitxLog(DEBUG, "Out Of Memory!"); 
        }
    }
    dbus_message_iter_close_container(&args,&subText);

    char *attr = "";
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subAttrs);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subAttrs, DBUS_TYPE_STRING, &attr)) { 
            FcitxLog(DEBUG, "Out Of Memory!"); 
        }
    }
    dbus_message_iter_close_container(&args,&subAttrs);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_prev);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_next);

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdatePreeditCaret(FcitxKimpanelUI* kimpanel, int position)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &position)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdatePreeditText(FcitxKimpanelUI* kimpanel, char *text)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    char *attr = "";
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdateAux(FcitxKimpanelUI* kimpanel, char *text)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    char *attr = "";

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdateSpotLocation(FcitxKimpanelUI* kimpanel, int x, int y)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &x)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &y)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

void KimUpdateScreen(FcitxKimpanelUI* kimpanel, int id)
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
        FcitxLog(DEBUG, "Message Null"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &id)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) { 
        FcitxLog(DEBUG, "Out Of Memory!"); 
    }
    dbus_connection_flush(kimpanel->conn);

    // free the message 
    dbus_message_unref(msg);

}

int CalKimCursorPos(FcitxKimpanelUI *kimpanel)
{
    int             i = 0;
    int             iChar;
    int             iCount = 0;
    FcitxInstance* instance = kimpanel->owner;
    FcitxInputState* input = &instance->input;
    Messages* messageUp = GetMessageUp(instance);

    const char      *p1;
    const char      *pivot;

    iChar = input->iCursorPos;
    

    for (i = 0; i < GetMessageCount(messageUp) ; i++) {
        if (instance->bShowCursor && iChar) {
            p1 = pivot = GetMessageString(messageUp, i);
            while (*p1 && p1 < pivot + iChar) {
                p1 = p1 + utf8_char_len(p1);
                iCount ++;
            }
            if (strlen (GetMessageString(messageUp, i)) > iChar) {
            iChar = 0;
            }
            else {
                iChar -= strlen (GetMessageString(messageUp, i));
            }
        }
    }

    return iCount;
}
// vim: sw=4 sts=4 et tw=100
