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
#include <strings.h> // bzero()
#include <ctype.h>
#include <limits.h>

#include "tools/tools.h"
#include "ui/InputWindow.h"
#include "im/special/vk.h"
#include "core/ime.h"
#include "core/xim.h"
#include "interface/DBus.h"
#include "fcitx-config/profile.h"

#ifdef _ENABLE_DBUS
DBusConnection *conn;
int ret;

Property logo_prop;
Property state_prop;
Property punc_prop;
Property corner_prop;
Property vk_prop;
Property gbkt_prop;
Property legend_prop;

char sVKmsg[PATH_MAX] = "";

#define TOOLTIP_STATE_ENG "英语"
#define TOOLTIP_STATE_CHN "中文"
#define TOOLTIP_PUNK_CHN "UP"
#define TOOLTIP_PUNK_ENG "NP"
#define TOOLTIP_CORNER_ENABLE "全角"
#define TOOLTIP_CORNER_DISABLE "半角"
#define TOOLTIP_VK_ENABLE "VK-ON"
#define TOOLTIP_VK_DISABLE "VK-OFF"
#define TOOLTIP_GBKT_ENABLE "繁体"
#define TOOLTIP_GBKT_DISABLE "简体"
#define TOOLTIP_LEGEND_ENABLE "联想"
#define TOOLTIP_LEGEND_DISABLE "无联"

//extern IMProtocol * current_call_data;
extern INT8 iIMCount;
extern CARD16 connect_id;
extern CARD16 icid;
extern Bool bIsInLegend;
extern int iLegendCandWordCount;
extern int iCurrentCandPage;
extern int iCurrentLegendCandPage;
extern int iCandPageCount;
extern int iLegendCandPageCount;
extern char *sVKHotkey;

void fixProperty(Property *prop);
char* convertMessage(char* in);
int calKIMCursorPos();

char sLogoLabel[MAX_IM_NAME * 2 + 1];

//#define DEBUG_DBUS

#ifdef DEBUG_DBUS
#define debug_dbus(fmt,arg...) \
    FcitxLog(DEBUG,fmt,##arg)
#else
static int /* inline */ debug_dbus(const char* fmt, ...)
__attribute__((format(printf, 1, 2)));

static int /* inline */ debug_dbus(const char* fmt, ...)
{
     return 0;
}
#endif

Bool InitDBus()
{
    DBusError err;
    // first init dbus
    dbus_error_init(&err);
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        debug_dbus(_("Connection Error (%s)"), err.message); 
        dbus_error_free(&err); 
    }
    if (NULL == conn) { 
        return False;
    }

    // request a name on the bus
    ret = dbus_bus_request_name(conn, "org.kde.fcitx", 
            DBUS_NAME_FLAG_REPLACE_EXISTING,
            &err);
    if (dbus_error_is_set(&err)) { 
        debug_dbus( _("Name Error (%s)"), err.message); 
        dbus_error_free(&err); 
    }
    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
        return False;
    }

    dbus_connection_flush(conn);
    // add a rule to receive signals from kimpanel
    dbus_bus_add_match(conn, 
            "type='signal',interface='org.kde.impanel'", 
            &err);
    dbus_connection_flush(conn);
    if (dbus_error_is_set(&err)) { 
        debug_dbus(_("Match Error (%s)"), err.message);
        return False;
    }

    logo_prop.label = sLogoLabel;

    return True;
}

void TestSendSignal()
{
}

void DBusLoop(void *val)
{
    MyDBusEventHandler();
}

void MyDBusEventHandler()
{
    DBusMessage *msg;
    DBusMessageIter args;

    int int0;
    char *s0;
   
    for (;;) {
        dbus_connection_read_write(conn, 0);
        msg = dbus_connection_pop_message(conn);
        if ( NULL == msg) {
            usleep(300);
            continue;
        }
        FcitxLock();
        if (dbus_message_is_signal(msg, "org.kde.impanel", "MovePreeditCaret")) {
            debug_dbus(_("MovePreeditCaret"));
            // read the parameters
            if (!dbus_message_iter_init(msg, &args))
                debug_dbus(_("Message has no arguments!")); 
            else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
                debug_dbus(_("Argument is not INT32!")); 
            else {
                dbus_message_iter_get_basic(&args, &int0);
                debug_dbus(_("Got Signal with value %d"), int0);
            }
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "SelectCandidate")) {
            debug_dbus(_("SelectCandidate: "));
            // read the parameters
            if (!dbus_message_iter_init(msg, &args))
                debug_dbus( _("Message has no arguments!")); 
            else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
                debug_dbus(_("Argument is not INT32!")); 
            else {
                dbus_message_iter_get_basic(&args, &int0);

                char *pstr;

                if (!bIsInLegend) {
                    pstr = im[gs.iIMIndex].GetCandWord(int0);
                    if (pstr) {
                        SendHZtoClient(0,pstr);
                        if (!fcitxProfile.bUseLegend) {
                            ResetInput();
                        }
                    }
                    updateMessages();
                } else {
                    pstr = im[gs.iIMIndex].GetLegendCandWord(int0);
                    if (pstr) {
                        SendHZtoClient(0,pstr);
                        if (!iLegendCandWordCount) {
                            ResetInput ();
                        }
                    }
                    updateMessages();
                }
                debug_dbus(_("%d:%s"), int0,pstr);
            }
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageUp")) {
            debug_dbus(_("LookupTablePageUp"));
            im[gs.iIMIndex].GetCandWords (SM_PREV);
            updateMessages();
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageDown")) {
            debug_dbus(_("LookupTablePageDown"));
            im[gs.iIMIndex].GetCandWords (SM_NEXT);
            updateMessages();
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "TriggerProperty")) {
            debug_dbus(_("TriggerProperty: "));
            // read the parameters
            if (!dbus_message_iter_init(msg, &args))
                debug_dbus(_("Message has no arguments!")); 
            else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
                debug_dbus(_("Argument is not STRING!")); 
            else {
                dbus_message_iter_get_basic(&args, &s0);
                debug_dbus(_("%s"), s0);

                triggerProperty(s0);
            }
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "PanelCreated")) {
            debug_dbus(_("PanelCreated"));
            registerProperties();
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "Exit")) {
            debug_dbus(_("Exit"));
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "ReloadConfig")) {
            debug_dbus(_("ReloadConfig"));
        }
        // free the message
        dbus_message_unref(msg);
        FcitxUnlock();
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
        debug_dbus(_("Message Null"));
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
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
            debug_dbus(_("Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
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
            debug_dbus(_("Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toEnable)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
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
            debug_dbus(_("Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&subLabel);

    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subText);
    for (i = 0; i < nText; i++) {
        if (!dbus_message_iter_append_basic(&subText, DBUS_TYPE_STRING, &texts[i])) { 
            debug_dbus(_("Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&subText);

    char *attr = "";
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subAttrs);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subAttrs, DBUS_TYPE_STRING, &attr)) { 
            debug_dbus(_("Out Of Memory!")); 
        }
    }
    dbus_message_iter_close_container(&args,&subAttrs);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_prev);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_next);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &position)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    char *attr = "";
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        debug_dbus(_("Out Of Memory!")); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    char *attr = "";

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        debug_dbus(_("Out Of Memory!")); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &x)) { 
        debug_dbus(_("Out Of Memory!")); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &y)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
        debug_dbus(_("Message Null")); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &id)) { 
        debug_dbus(_("Out Of Memory!")); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus(_("Out Of Memory!")); 
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
            debug_dbus(_("Type: %d Text: %s"),messageDown.msg[i].type,messageDown.msg[i].strMsg);

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
        debug_dbus(_("Labels %d, Texts %d, CMB:%s"),nLabels,nTexts, cmb); 
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
            debug_dbus(_("updateMesssages Up:%s"), aux);
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


    logo_prop.key = "/Fcitx/Logo";
    strcpy(logo_prop.label, "Fcitx");
    logo_prop.icon = "fcitx";
    logo_prop.tip = "小企鹅输入法";
    props[0] = property2string(&logo_prop);

    state_prop.key = "/Fcitx/State";
    state_prop.label = TOOLTIP_STATE_ENG;
    state_prop.icon = "fcitx-eng";
    state_prop.tip = "小企鹅输入法";
    props[1] = property2string(&state_prop);

    punc_prop.key = "/Fcitx/Punc";
    punc_prop.label = fcitxProfile.bChnPunc ? TOOLTIP_PUNK_CHN : TOOLTIP_PUNK_ENG;

    if (fcitxProfile.bChnPunc) {
        punc_prop.icon = "fcitx-full-punct";
    } else {
        punc_prop.icon = "fcitx-half-punct";
    }
    punc_prop.tip = "中英文标点切换";
    props[2] = property2string(&punc_prop);

    corner_prop.key = "/Fcitx/Corner";
    corner_prop.label = fcitxProfile.bCorner ? TOOLTIP_CORNER_ENABLE : TOOLTIP_CORNER_DISABLE;
    if (fcitxProfile.bCorner) {
        corner_prop.icon = "fcitx-full-letter";
    } else {
        corner_prop.icon = "fcitx-half-letter";
    }
    corner_prop.tip = "全半角切换";
    props[3] = property2string(&corner_prop);

    vk_prop.key = "/Fcitx/VK";
    vk_prop.label = bVK ? TOOLTIP_VK_ENABLE : TOOLTIP_VK_DISABLE;
    if (bVK) {
        vk_prop.icon = "fcitx-vkon";
    } else {
        vk_prop.icon = "fcitx-vkoff";
    }
    sprintf(sVKmsg,  "虚拟键盘切换：%s", sVKHotkey);
    vk_prop.tip = sVKmsg;
    props[4] = property2string(&vk_prop);

    gbkt_prop.key = "/Fcitx/GBKT";
    gbkt_prop.label = fcitxProfile.bUseGBKT ? TOOLTIP_GBKT_ENABLE : TOOLTIP_GBKT_DISABLE;
    if (fcitxProfile.bUseGBKT) {
        gbkt_prop.icon = "fcitx-trad";
    } else {
        gbkt_prop.icon = "fcitx-simp";
    }
    gbkt_prop.tip = "简繁体转换";
    props[5] = property2string(&gbkt_prop);

    legend_prop.key = "/Fcitx/Legend";
    legend_prop.label = fcitxProfile.bUseLegend ? TOOLTIP_LEGEND_ENABLE : TOOLTIP_LEGEND_DISABLE;
    if (fcitxProfile.bUseLegend) {
        legend_prop.icon = "fcitx-legend";
    } else {
        legend_prop.icon = "fcitx-nolegend";
    }
    legend_prop.tip = "启用联想支持";
    props[6] = property2string(&legend_prop);

    KIMRegisterProperties(props,7);
    for(i=0;i<7;i++) {
        free(props[i]);
    }
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
        prop->label = (ConnectIDGetState(connect_id) == IS_CHN) ? TOOLTIP_STATE_CHN: TOOLTIP_STATE_ENG;
        if (ConnectIDGetState(connect_id) == IS_CHN) {
            prop->icon = "fcitx-chn";
        } else {
            prop->icon = "fcitx-eng";
        }
    }
    if (prop == &punc_prop) {
        prop->label = fcitxProfile.bChnPunc ? TOOLTIP_PUNK_CHN : TOOLTIP_PUNK_ENG;
        if (fcitxProfile.bChnPunc) {
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

void triggerProperty(char *propKey)
{
    char *menu[32];
    char *name;
    int i;
    bzero(menu,32);
    if (strcmp(propKey,logo_prop.key) == 0) {
        for (i = 0; i < iIMCount; i++) {
            name = im[i].strName;
            //menu[i] = (char *)malloc(100*sizeof(char));
            Property prop;
            prop.key = (char *)malloc(100*sizeof(char));
            prop.icon = "";
            sprintf(prop.key,"/Fcitx/Logo/%d",i);
            prop.label = name;
            prop.tip = name;
            //            strcat(prop.tip,"输入法");
            fixProperty(&prop);
            menu[i] = property2string(&prop);
            free(prop.key);
        }
        KIMExecMenu(menu,iIMCount);
        for (i = 0;i < iIMCount; i++) {
            free(menu[i]);
        }
    } else {
        if (strstr(propKey,logo_prop.key) == propKey) {
            i = atoi(strrchr(propKey,'/') + 1);
            /*
            if (ConnectIDGetState (connect_id) == IS_CLOSED) {
                SetConnectID (connect_id, IS_CHN);
	            icidSetIMState(ConnectIDGetICID(connect_id), IS_CHN);
                EnterChineseMode (False);
            } else if (ConnectIDGetState (connect_id) == IS_ENG) {
                ChangeIMState(connect_id);
                EnterChineseMode (False);
            }*/
            SwitchIM(i);
        }
    }

    if (strcmp(propKey,state_prop.key) == 0) {
        ChangeIMState(connect_id);
    }
    if (strcmp(propKey,punc_prop.key) == 0) {
        ChangePunc();
    }
    if (strcmp(propKey,corner_prop.key) == 0) {
        ChangeCorner();
    }
    if (strcmp(propKey,vk_prop.key) == 0) {
//        SwitchVK();
    }
    if (strcmp(propKey,gbkt_prop.key) == 0) {
        ChangeGBKT();
    }
    if (strcmp(propKey,legend_prop.key) == 0) {
        ChangeLegend();
    }
}

char* property2string(Property *prop)
{
    int len;
    char *result = NULL;

    len = strlen(prop->key)
        + strlen(prop->label)
        + strlen(prop->icon)
        + strlen(prop->tip)
        + 3 + 1;
    result = malloc(len * sizeof(char));

    bzero(result,len*sizeof(char));
    strcat(result,prop->key);
    strcat(result,":");
    strcat(result,prop->label);
    strcat(result,":");
    strcat(result,prop->icon);
    strcat(result,":");
    strcat(result,prop->tip);
    return result;
}

void fixProperty(Property *prop)
{
    if (strcmp(prop->label,"Fcitx") == 0) {
        prop->icon = "fcitx";
    }
    if (strcmp(prop->label,"智能拼音") == 0) {
        prop->icon = "fcitx-pinyin";
    }
    if (strcmp(prop->label,"智能双拼") == 0) {
        prop->icon = "";
    }
    if (strcmp(prop->label,"区位") == 0) {
        prop->icon = "";
    }
    if (strcmp(prop->label,"五笔字型") == 0) {
        prop->icon = "fcitx-wubi";
    }
    if (strcmp(prop->label,"五笔拼音") == 0) {
        prop->icon = "fcitx-wubi";
    }
    if (strcmp(prop->label,"二笔") == 0) {
        prop->icon = "fcitx-erbi";
    }
    if (strcmp(prop->label,"仓颉") == 0) {
        prop->icon = "fcitx-cangjie";
    }
    if (strcmp(prop->label,"晚风") == 0) {
        prop->icon = "";
    }
    if (strcmp(prop->label,"冰蟾全息") == 0) {
        prop->icon = "";
    }
    if (strcmp(prop->label,"自然码") == 0) {
        prop->icon = "fcitx-ziranma";
    }
    if (strcmp(prop->label,"电报码") == 0) {
        prop->icon = "";
    }
}

void updatePropertyByConnectID(CARD16 connect_id) {
	int iIndex = ConnectIDGetState(connect_id);
	switch (iIndex) {
	case IS_CLOSED:
        iState = IS_ENG;
        strcpy(logo_prop.label, "Fcitx");
		updateProperty(&logo_prop);
		updateProperty(&state_prop);
		break;
	case IS_CHN:
        iState = IS_CHN;
		strcpy(logo_prop.label, im[gs.iIMIndex].strName);
		updateProperty(&logo_prop);
		updateProperty(&state_prop);
		break;
	case IS_ENG:
        iState = IS_ENG;
		strcpy(logo_prop.label, im[gs.iIMIndex].strName);
		updateProperty(&logo_prop);
		updateProperty(&state_prop);
		break;
	}
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
#endif // DBUS_C
