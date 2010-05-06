#include "main.h"
#include "InputWindow.h"
#include "ime.h"
#include "tools.h"
#include "xim.h"
#include "DBus.h"

#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <strings.h> // bzero()
#include <ctype.h>

#ifndef _ENABLE_DBUS
Bool bUseDBus = False;
#else

Bool bUseDBus = False;
DBusConnection *conn;
int ret;

Property logo_prop;
Property state_prop;
Property punc_prop;
Property corner_prop;
Property gbk_prop;
Property gbkt_prop;
Property legend_prop;

#define TOOLTIP_STATE_ENG "英语"
#define TOOLTIP_STATE_CHN "中文"
#define TOOLTIP_PUNK_CHN "UP"
#define TOOLTIP_PUNK_ENG "NP"
#define TOOLTIP_CORNER_ENABLE "全角"
#define TOOLTIP_CORNER_DISABLE "半角"
#define TOOLTIP_GBK_ENABLE "GBK"
#define TOOLTIP_GBK_DISABLE "GB"
#define TOOLTIP_GBKT_ENABLE "繁体"
#define TOOLTIP_GBKT_DISABLE "简体"
#define TOOLTIP_LEGEND_ENABLE "联想"
#define TOOLTIP_LEGEND_DISABLE "无联"

//extern IMProtocol * current_call_data;
extern INT8 iIMCount;
extern INT8 iIMIndex;
extern CARD16 connect_id;
extern CARD16 icid;
extern Bool bIsInLegend;
extern Bool bShowPrev;
extern Bool bShowNext;
extern int iLegendCandWordCount;
extern int iCurrentCandPage;
extern int iCurrentLegendCandPage;
extern int iCandPageCount;
extern int iLegendCandPageCount;
extern int iCursorPos;
extern Bool bShowCursor;

void fixProperty(Property *prop);
char* convertMessage(char* in);
static void MyDBusEventHandler();
int calKIMCursorPos();

char sLogoLabel[MAX_IM_NAME * 2 + 1];

//#define DEBUG_DBUS

#ifdef DEBUG_DBUS
#define debug_dbus(fmt,arg...) \
    fprintf(stderr,fmt,##arg)
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
        debug_dbus("Connection Error (%s)\n", err.message); 
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
        debug_dbus( "Name Error (%s)\n", err.message); 
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
        debug_dbus("Match Error (%s)\n", err.message);
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

static void MyDBusEventHandler()
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
        if (dbus_message_is_signal(msg, "org.kde.impanel", "MovePreeditCaret")) {
            debug_dbus("MovePreeditCaret\n");
            // read the parameters
            if (!dbus_message_iter_init(msg, &args))
                debug_dbus("Message has no arguments!\n"); 
            else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
                debug_dbus("Argument is not INT32!\n"); 
            else {
                dbus_message_iter_get_basic(&args, &int0);
                printf("Got Signal with value %d\n", int0);
            }
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "SelectCandidate")) {
            debug_dbus("SelectCandidate: ");
            // read the parameters
            if (!dbus_message_iter_init(msg, &args))
                debug_dbus( "Message has no arguments!\n"); 
            else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) 
                debug_dbus( "Argument is not INT32!\n"); 
            else {
                dbus_message_iter_get_basic(&args, &int0);

                char *pstr;

                if (!bIsInLegend) {
                    pstr = im[iIMIndex].GetCandWord(int0);
                    if (pstr) {
                        SendHZtoClient(0,pstr);
                        if (!bUseLegend) {
                            ResetInput();
                        }
                    }
                    updateMessages();
                } else {
                    pstr = im[iIMIndex].GetLegendCandWord(int0);
                    if (pstr) {
                        SendHZtoClient(0,pstr);
                        if (!iLegendCandWordCount) {
                            ResetInput ();
                        }
                    }
                    updateMessages();
                }
                debug_dbus("%d:%s\n", int0,pstr);
            }
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageUp")) {
            debug_dbus("LookupTablePageUp\n");
            im[iIMIndex].GetCandWords (SM_PREV);
            updateMessages();
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageDown")) {
            debug_dbus("LookupTablePageDown\n");
            im[iIMIndex].GetCandWords (SM_NEXT);
            updateMessages();
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "TriggerProperty")) {
            debug_dbus("TriggerProperty: ");
            // read the parameters
            if (!dbus_message_iter_init(msg, &args))
                debug_dbus( "Message has no arguments!\n"); 
            else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) 
                debug_dbus( "Argument is not STRING!\n"); 
            else {
                dbus_message_iter_get_basic(&args, &s0);
                debug_dbus("%s\n", s0);

                triggerProperty(s0);
            }
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "PanelCreated")) {
            debug_dbus("PanelCreated\n");
            registerProperties();
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "Exit")) {
            debug_dbus("Exit\n");
        }
        if (dbus_message_is_signal(msg, "org.kde.impanel", "ReloadConfig")) {
            debug_dbus("ReloadConfig\n");
        }
        // free the message
        dbus_message_unref(msg);
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
        debug_dbus( "Message Null\n");
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
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
            debug_dbus( "Out Of Memory!\n"); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
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
            debug_dbus( "Out Of Memory!\n"); 
        }
    }
    dbus_message_iter_close_container(&args,&sub);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toEnable)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &toShow)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
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
            debug_dbus( "Out Of Memory!\n"); 
        }
    }
    dbus_message_iter_close_container(&args,&subLabel);

    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subText);
    for (i = 0; i < nText; i++) {
        if (!dbus_message_iter_append_basic(&subText, DBUS_TYPE_STRING, &texts[i])) { 
            debug_dbus( "Out Of Memory!\n"); 
        }
    }
    dbus_message_iter_close_container(&args,&subText);

    char *attr = "";
    dbus_message_iter_open_container(&args,DBUS_TYPE_ARRAY,"s",&subAttrs);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subAttrs, DBUS_TYPE_STRING, &attr)) { 
            debug_dbus( "Out Of Memory!\n"); 
        }
    }
    dbus_message_iter_close_container(&args,&subAttrs);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_prev);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_next);

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &position)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    char *attr = "";
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    char *attr = "";

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &text)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &attr)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &x)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &y)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
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
        debug_dbus( "Message Null\n"); 
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &id)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }

    // send the message and flush the connection
    if (!dbus_connection_send(conn, msg, &serial)) { 
        debug_dbus( "Out Of Memory!\n"); 
    }
    dbus_connection_flush(conn);

    // free the message 
    dbus_message_unref(msg);

}

void updateMessages()
{
    bShowPrev = bShowNext = False;

    int n = uMessageDown;
    int nLabels = 0;
    int nTexts = 0;
    char *label[33];
    char *text[33];
    char cmb[100] = "";
    int i;
    char* utfmsg = NULL;

    if (n) {
        for (i=0;i<n;i++) {
            debug_dbus("Type: %d Text: %s\n",messageDown[i].type,(utfmsg=convertMessage(messageDown[i].strMsg)));
            free(utfmsg);
            if (messageDown[i].type == MSG_INDEX) {
                if (nLabels) {
                    text[nTexts++] = strdup(cmb);
                }
                label[nLabels++] = convertMessage(messageDown[i].strMsg);
                strcpy(cmb,"");
            } else {
                strcat(cmb,(utfmsg=convertMessage(messageDown[i].strMsg)));
                free(utfmsg);
            }
        }
        text[nTexts++] = strdup(cmb);
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
        debug_dbus("Labels %d, Texts %d\n, CMB:%s",nLabels,nTexts, cmb); 
        if (nTexts == 0) {
            KIMShowLookupTable(False);
        }
        else {
            if (bIsInLegend) {
            if (iCurrentLegendCandPage > 0)
                bShowPrev = True;
            if (iCurrentLegendCandPage < iLegendCandPageCount)
                bShowNext = True;
            } else {
            if (iCurrentCandPage > 0)
            bShowPrev = True;
            if (iCurrentCandPage < iCandPageCount)
                bShowNext = True;
            }
            KIMUpdateLookupTable(label,nLabels,text,nTexts,bShowPrev,bShowNext);
            KIMShowLookupTable(True);
        }
        for (i = 0; i < nTexts; i++)
            free(text[i]);
        for (i = 0; i < nLabels; i++)
            free(label[i]);
    } else {
        KIMUpdateLookupTable(NULL,0,NULL,0,bShowPrev,bShowNext);
        KIMShowLookupTable(False);
    }
    
    n = uMessageUp;
    char aux[MESSAGE_MAX_LENGTH] = "";
    char empty[MESSAGE_MAX_LENGTH] = "";
    if (n) {
        // FIXME: buffer overflow
        for (i=0;i<n;i++) {
            strcat(aux,(utfmsg=convertMessage(messageUp[i].strMsg)));
            free(utfmsg);
            debug_dbus("updateMesssages Up:%s\n", aux);
        }
        if (bShowCursor)
        {
            KIMUpdatePreeditText(aux);
            KIMUpdateAux(empty);
            KIMShowPreedit(True);
            KIMUpdatePreeditCaret(calKIMCursorPos());
            KIMShowAux(False);
        }
        else {
            KIMUpdatePreeditText(empty);
            KIMUpdateAux(aux);
            KIMShowPreedit(False);
            KIMShowAux(True);
        }
    } else {
        KIMShowPreedit(False);
        KIMShowAux(False);
    }
}

char* convertMessage(char* in)
{
    char* strTemp = NULL;
    char* result = NULL;
    if (in) {
        if (bUseGBKT) {
            strTemp = ConvertGBKSimple2Tradition(in);
        }
        else
            strTemp = in;
        result = g2u(strTemp);
        if (bUseGBKT && strTemp)
            free(strTemp);

        if (result == NULL)
            result = strdup("");
        return result;
    }
    else
        return NULL;
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
    logo_prop.icon = PKGDATADIR "/xpm/" "Logo.png";
    logo_prop.tip = "小企鹅输入法";
    props[0] = property2string(&logo_prop);

    state_prop.key = "/Fcitx/State";
    state_prop.label = TOOLTIP_STATE_ENG;
    state_prop.icon = PKGDATADIR "/xpm/" "Ying.png";
    state_prop.tip = "小企鹅输入法";
    props[1] = property2string(&state_prop);

    punc_prop.key = "/Fcitx/Punc";
    punc_prop.label = bChnPunc ? TOOLTIP_PUNK_CHN : TOOLTIP_PUNK_ENG;

    if (bChnPunc) {
        punc_prop.icon = PKGDATADIR "/xpm/" "full-punct.png";
    } else {
        punc_prop.icon = PKGDATADIR "/xpm/" "half-punct.png";
    }
    punc_prop.tip = "中英文标点切换";
    props[2] = property2string(&punc_prop);

    corner_prop.key = "/Fcitx/Corner";
    corner_prop.label = bCorner ? TOOLTIP_CORNER_ENABLE : TOOLTIP_CORNER_DISABLE;
    if (bCorner) {
        corner_prop.icon = PKGDATADIR "/xpm/" "full-letter.png";
    } else {
        corner_prop.icon = PKGDATADIR "/xpm/" "half-letter.png";
    }
    corner_prop.tip = "全半角切换";
    props[3] = property2string(&corner_prop);

    gbk_prop.key = "/Fcitx/GBK";
    gbk_prop.label = bUseGBK ? TOOLTIP_GBK_ENABLE : TOOLTIP_GBK_DISABLE;
    if (bUseGBK) {
        gbk_prop.icon = PKGDATADIR "/xpm/" "gbk.png";
    } else {
        gbk_prop.icon = PKGDATADIR "/xpm/" "gb.png";
    }
    gbk_prop.tip = bUseGBK ? "标准GB字符集" : "扩展GBK字符集";
    props[4] = property2string(&gbk_prop);

    gbkt_prop.key = "/Fcitx/GBKT";
    gbkt_prop.label = bUseGBKT ? TOOLTIP_GBKT_ENABLE : TOOLTIP_GBKT_DISABLE;
    if (bUseGBKT) {
        gbkt_prop.icon = PKGDATADIR "/xpm/" "Fan.png";
    } else {
        gbkt_prop.icon = PKGDATADIR "/xpm/" "Jian.png";
    }
    gbkt_prop.tip = "简繁体转换";
    props[5] = property2string(&gbkt_prop);

    legend_prop.key = "/Fcitx/Legend";
    legend_prop.label = bUseLegend ? TOOLTIP_LEGEND_ENABLE : TOOLTIP_LEGEND_DISABLE;
    if (bUseLegend) {
        legend_prop.icon = PKGDATADIR "/xpm/" "Lian.png";
    } else {
        legend_prop.icon = PKGDATADIR "/xpm/" "WuLian.png";
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
            prop->icon = PKGDATADIR "/xpm/" "Zhong.png";
        } else {
            prop->icon = PKGDATADIR "/xpm/" "Ying.png";
        }
    }
    if (prop == &punc_prop) {
        prop->label = bChnPunc ? TOOLTIP_PUNK_CHN : TOOLTIP_PUNK_ENG;
        if (bChnPunc) {
            prop->icon = PKGDATADIR "/xpm/" "full-punct.png";
        } else {
            prop->icon = PKGDATADIR "/xpm/" "half-punct.png";
        }
    }
    if (prop == &corner_prop) {
        prop->label = bCorner ? TOOLTIP_CORNER_ENABLE : TOOLTIP_CORNER_DISABLE;
        if (bCorner) {
            prop->icon = PKGDATADIR "/xpm/" "full-letter.png";
        } else {
            prop->icon = PKGDATADIR "/xpm/" "half-letter.png";
        }
    }
    if (prop == &gbk_prop) {
        prop->label = bUseGBK ? TOOLTIP_GBK_ENABLE : TOOLTIP_GBK_DISABLE;
        if (bUseGBK) {
            prop->icon = PKGDATADIR "/xpm/" "gbk.png";
        } else {
            prop->icon = PKGDATADIR "/xpm/" "gb.png";
        }
    }
    if (prop == &gbkt_prop) {
        prop->label = bUseGBKT ? TOOLTIP_GBKT_ENABLE : TOOLTIP_GBKT_DISABLE;
        if (bUseGBKT) {
            prop->icon = PKGDATADIR "/xpm/" "Fan.png";
        } else {
            prop->icon = PKGDATADIR "/xpm/" "Jian.png";
        }
    }
    if (prop == &legend_prop) {
        prop->label = bUseLegend ? TOOLTIP_LEGEND_ENABLE : TOOLTIP_LEGEND_DISABLE;
        if (bUseLegend) {
            prop->icon = PKGDATADIR "/xpm/" "Lian.png";
        } else {
            prop->icon = PKGDATADIR "/xpm/" "WuLian.png";
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
            name = g2u(im[i].strName);
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
            free(name);
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
    if (strcmp(propKey,gbk_prop.key) == 0) {
        ChangeGBK();
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

char* g2u(char *instr)
{
    iconv_t cd;
    char *inbuf;
    char *outbuf;
    char *outptr;
    size_t insize=strlen(instr);
    size_t outputbufsize=MESSAGE_MAX_LENGTH;
    size_t avail=outputbufsize;
    unsigned int nconv;
    inbuf=instr;
    outbuf=(char *)malloc(outputbufsize * sizeof(char));
    outptr=outbuf;    //使用outptr作为空闲空间指针以避免outbuf被改变
    memset(outbuf,'\0',outputbufsize);
    cd=iconv_open("utf-8","gb18030");    //将字符串编码由gtk转换为utf-8
    if(cd==(iconv_t)-1)
    {
        return NULL;
    }
    nconv=iconv(cd,&inbuf,&insize,&outptr,&avail);
    iconv_close(cd);
    return outbuf;
}

char* u2g(char *instr)
{
    iconv_t cd;
    char *inbuf;
    char *outbuf;
    char *outptr;
    size_t insize=strlen(instr);
    size_t outputbufsize=MESSAGE_MAX_LENGTH;
    size_t avail=outputbufsize;
    unsigned int nconv;
    inbuf=instr;
    outbuf=(char *)malloc(outputbufsize * sizeof(char));
    outptr=outbuf;    //使用outptr作为空闲空间指针以避免outbuf被改变
    memset(outbuf,'\0',outputbufsize);
    cd=iconv_open("gb18030","utf-8");    //将字符串编码由utf-8转换为gbk
    if(cd==(iconv_t)-1)
    {
        return NULL;
    }
    nconv=iconv(cd,&inbuf,&insize,&outptr,&avail);
    iconv_close(cd);
    return outbuf;

}

void fixProperty(Property *prop)
{
    if (strcmp(prop->label,"Fcitx") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "Logo.png";
    }
    if (strcmp(prop->label,"智能拼音") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "PinYin.png";
    }
    if (strcmp(prop->label,"智能双拼") == 0) {
        //        prop->label = "双拼";
        prop->icon = "";
    }
    if (strcmp(prop->label,"区位") == 0) {
        //        prop->label = "区位";
        prop->icon = "";
    }
    if (strcmp(prop->label,"五笔字型") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "Wubi.png";
    }
    if (strcmp(prop->label,"五笔拼音") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "Wubi.png";
    }
    if (strcmp(prop->label,"二笔") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "Erbi.png";
    }
    if (strcmp(prop->label,"仓颉") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "CangJie.png";
    }
    if (strcmp(prop->label,"晚风") == 0) {
        prop->icon = "";
    }
    if (strcmp(prop->label,"冰蟾全息") == 0) {
        //        prop->label = "冰";
        prop->icon = "";
    }
    if (strcmp(prop->label,"自然码") == 0) {
        prop->icon = PKGDATADIR "/xpm/" "Ziranma.png";
    }
    if (strcmp(prop->label,"电报码") == 0) {
        prop->icon = "";
        //        prop->label = "电";
    }
}

void updatePropertyByConnectID(CARD16 connect_id) {
	int iIndex = ConnectIDGetState(connect_id);
    char *need_free = NULL;
	switch (iIndex) {
	case IS_CLOSED:
        iState = IS_ENG;
        strcpy(logo_prop.label, "Fcitx");
		updateProperty(&logo_prop);
		updateProperty(&state_prop);
		break;
	case IS_CHN:
        iState = IS_CHN;
		strcpy(logo_prop.label, (need_free = g2u(im[iIMIndex].strName)));
		updateProperty(&logo_prop);
		updateProperty(&state_prop);
		break;
	case IS_ENG:
        iState = IS_ENG;
		strcpy(logo_prop.label, (need_free = g2u(im[iIMIndex].strName)));
		updateProperty(&logo_prop);
		updateProperty(&state_prop);
		break;
	}
    if (need_free)
        free(need_free);
}

int calKIMCursorPos()
{
    int             i = 0;
    int             iChar;
    int             iCount = 0;

    const char      *p1;
    const char      *pivot;
    Bool            bEn;

    iChar = iCursorPos;

    for (i = 0; i < uMessageUp ; i++) {
        if (bShowCursor && iChar) {
            p1 = pivot = messageUp[i].strMsg;
            while (*p1 && p1 < pivot + iChar) {
                if (isprint (*p1))	//使用中文字体
                    bEn = True;
                else {
                    bEn = False;
                    p1 += 2;
                    iCount ++;
                }
                while (*p1 && p1 < pivot + iChar) {
                    if (isprint (*p1)) {
                        if (!bEn)
                            break;
                        p1 ++;
                        iCount ++;
                    }
                    else {
                        if (bEn)
                            break;
                        p1 += 2;
                        iCount ++;
                    }
                }
            }
            if (strlen (messageUp[i].strMsg) > iChar) {
            iChar = 0;
            }
            else {
                iChar -= strlen (messageUp[i].strMsg);
            }
        }
    }

    return iCount;
}
// vim: sw=4 sts=4 et tw=100
#endif // DBUS_C
