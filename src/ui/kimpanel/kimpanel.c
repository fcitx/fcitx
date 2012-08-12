/***************************************************************************
 *   Copyright (C) 2008~2010 by Zealot.Hoi                                 *
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
#include <libintl.h>
#include "config.h"
#include "fcitx/ui.h"
#include "fcitx-utils/log.h"
#include <dbus/dbus.h>
#include "module/dbus/dbusstuff.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx/hook.h"
#include "fcitx-utils/utils.h"
#include "fcitx/candidate.h"
#include <sys/socket.h>

#define FCITX_KIMPANEL_INTERFACE "org.kde.kimpanel.inputmethod"
#define FCITX_KIMPANEL_PATH "/kimpanel"

#define GetMenuItem(m, i) ((FcitxMenuItem*) utarray_eltptr(&(m)->shell, (i)))

const char * kimpanel_introspection_xml =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node name=\"" FCITX_KIMPANEL_PATH "\">\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"" FCITX_KIMPANEL_INTERFACE "\">\n"
    "    <signal name=\"ExecDialog\">\n"
    "      <arg name=\"prop\" direction=\"in\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"ExecMenu\">\n"
    "      <arg name=\"prop\" direction=\"in\" type=\"a\"/>\n"
    "    </signal>\n"
    "    <signal name=\"RegisterProperties\">\n"
    "      <arg name=\"prop\" direction=\"in\" type=\"a\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateProperty\">\n"
    "      <arg name=\"prop\" direction=\"in\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"RemoveProperty\">\n"
    "      <arg name=\"prop\" direction=\"in\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"ShowAux\">\n"
    "      <arg name=\"toshow\" direction=\"in\" type=\"b\"/>\n"
    "    </signal>\n"
    "    <signal name=\"ShowPreedit\">\n"
    "      <arg name=\"toshow\" direction=\"in\" type=\"b\"/>\n"
    "    </signal>\n"
    "    <signal name=\"ShowLookupTable\">\n"
    "      <arg name=\"toshow\" direction=\"in\" type=\"b\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateLookupTable\">\n"
    "      <arg name=\"label\" direction=\"in\" type=\"a\"/>\n"
    "      <arg name=\"text\" direction=\"in\" type=\"a\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateLookupTableCursor\">\n"
    "      <arg name=\"cursor\" direction=\"in\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdatePreeditCaret\">\n"
    "      <arg name=\"position\" direction=\"in\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdatePreeditText\">\n"
    "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"attr\" direction=\"in\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateAux\">\n"
    "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"attr\" direction=\"in\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateSpotLocation\">\n"
    "      <arg name=\"x\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"y\" direction=\"in\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateScreen\">\n"
    "      <arg name=\"screen\" direction=\"in\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"Enable\">\n"
    "      <arg name=\"toenable\" direction=\"in\" type=\"b\"/>\n"
    "    </signal>\n"
    "  </interface>\n"
    "</node>\n";

typedef struct _FcitxKimpanelUI {
    FcitxInstance* owner;
    DBusConnection* conn;
    int iOffsetY;
    int iOffsetX;
    FcitxMessages* messageUp;
    FcitxMessages* messageDown;
    int iCursorPos;
    int lastUpdateY;
    int lastUpdateX;
    int version;
    int lastUpdateW;
    int lastUpdateH;
    int lastCursor;
} FcitxKimpanelUI;

static void* KimpanelCreate(FcitxInstance* instance);
static void KimpanelCloseInputWindow(void* arg);
static void KimpanelShowInputWindow(void* arg);
static void KimpanelMoveInputWindow(void* arg);
static void KimpanelRegisterMenu(void *arg, FcitxUIMenu* menu);
static void KimpanelUpdateStatus(void *arg, FcitxUIStatus* status);
static void KimpanelRegisterStatus(void *arg, FcitxUIStatus* status);
static void KimpanelRegisterComplexStatus(void* arg, FcitxUIComplexStatus* status);
static void KimpanelUpdateComplexStatus(void* arg, FcitxUIComplexStatus* status);
static void KimpanelOnInputFocus(void *arg);
static void KimpanelOnInputUnFocus(void *arg);
static void KimpanelOnTriggerOn(void *arg);
static void KimpanelOnTriggerOff(void *arg);
static void KimpanelSuspend(void* arg);
static void KimpanelDestroy(void* arg);

static void KimUpdateLookupTableCursor(FcitxKimpanelUI* kimpanel, int cursor);
static void KimShowAux(FcitxKimpanelUI* kimpanel, boolean toShow);
static void KimShowPreedit(FcitxKimpanelUI* kimpanel, boolean toShow);
static void KimUpdateSpotLocation(FcitxKimpanelUI* kimpanel, int x, int y);
static void KimSetSpotRect(FcitxKimpanelUI* kimpanel, int x, int y, int w, int h);
static void KimShowLookupTable(FcitxKimpanelUI* kimpanel, boolean toShow);
static void KimUpdateLookupTable(FcitxKimpanelUI* kimpanel, char *labels[], int nLabel, char *texts[], int nText, boolean has_prev, boolean has_next);
static void KimUpdatePreeditText(FcitxKimpanelUI* kimpanel, char *text);
static void KimUpdateAux(FcitxKimpanelUI* kimpanel, char *text);
static void KimUpdatePreeditCaret(FcitxKimpanelUI* kimpanel, int position);
static void KimEnable(FcitxKimpanelUI* kimpanel, boolean toEnable);
static void KimRegisterProperties(FcitxKimpanelUI* kimpanel, char *props[], int n);
static void KimUpdateProperty(FcitxKimpanelUI* kimpanel, char *prop);
static DBusHandlerResult KimpanelDBusEventHandler(DBusConnection *connection, DBusMessage *message, void *user_data);
static DBusHandlerResult KimpanelDBusFilter(DBusConnection *connection, DBusMessage *message, void *user_data);
static int CalKimCursorPos(FcitxKimpanelUI *kimpanel);
static void KimpanelInputReset(void *arg);
static char* Status2String(FcitxUIStatus* status);
static char* ComplexStatus2String(FcitxUIComplexStatus* status);
static void KimpanelRegisterAllStatus(FcitxKimpanelUI* kimpanel);
static void KimpanelSetIMStatus(FcitxKimpanelUI* kimpanel);
static void KimExecMenu(FcitxKimpanelUI* kimpanel, char *props[], int n);
static void KimpanelServiceExistCallback(DBusPendingCall *call, void *data);
static void KimpanelIntrospect(FcitxKimpanelUI* kimpanel);
static void KimpanelIntrospectCallback(DBusPendingCall *call, void *data);

#define KIMPANEL_BUFFER_SIZE 4096

FCITX_DEFINE_PLUGIN(fcitx_kimpanel_ui, ui, FcitxUI) = {
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
    NULL,
    KimpanelSuspend,
    NULL,
    KimpanelDestroy,
    KimpanelRegisterComplexStatus,
    KimpanelUpdateComplexStatus,
    NULL
};

void* KimpanelCreate(FcitxInstance* instance)
{
    FcitxKimpanelUI *kimpanel = fcitx_utils_malloc0(sizeof(FcitxKimpanelUI));
    FcitxModuleFunctionArg arg;

    kimpanel->lastCursor = -2;
    kimpanel->version = 1;
    kimpanel->iCursorPos = 0;
    kimpanel->owner = instance;
    kimpanel->conn = InvokeFunction(instance, FCITX_DBUS, GETCONNECTION, arg);

    DBusError err;
    dbus_error_init(&err);
    do {
        if (kimpanel->conn == NULL) {
            FcitxLog(ERROR, "DBus Not initialized");
            break;
        }

        // add a rule to receive signals from kimpanel
        dbus_bus_add_match(kimpanel->conn,
                           "type='signal',interface='org.kde.impanel'",
                           &err);
        dbus_connection_flush(kimpanel->conn);
        if (dbus_error_is_set(&err)) {
            FcitxLog(ERROR, "Match Error (%s)", err.message);
            break;
        }
        dbus_bus_add_match(kimpanel->conn,
                           "type='signal',interface='org.kde.impanel2'",
                           &err);
        dbus_connection_flush(kimpanel->conn);
        if (dbus_error_is_set(&err)) {
            FcitxLog(ERROR, "Match Error (%s)", err.message);
            break;
        }

        dbus_bus_add_match(kimpanel->conn,
                           "type='signal',"
                           "interface='" DBUS_INTERFACE_DBUS "',"
                           "path='" DBUS_PATH_DBUS "',"
                           "member='NameOwnerChanged'",
                           &err);

        dbus_connection_flush(kimpanel->conn);
        if (dbus_error_is_set(&err)) {
            FcitxLog(ERROR, "Match Error (%s)", err.message);
            break;
        }

        if (!dbus_connection_add_filter(kimpanel->conn, KimpanelDBusFilter, kimpanel, NULL)) {
            FcitxLog(ERROR, "No memory");
            break;
        }

        DBusObjectPathVTable vtable = {NULL, &KimpanelDBusEventHandler, NULL, NULL, NULL, NULL };

        dbus_connection_register_object_path(kimpanel->conn, FCITX_KIMPANEL_PATH, &vtable, kimpanel);

        kimpanel->messageUp = FcitxMessagesNew();
        kimpanel->messageDown = FcitxMessagesNew();

        FcitxIMEventHook resethk;
        resethk.arg = kimpanel;
        resethk.func = KimpanelInputReset;
        FcitxInstanceRegisterResetInputHook(instance, resethk);

        const char* kimpanelServiceName = "org.kde.impanel";
        DBusMessage* message = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "NameHasOwner");
        dbus_message_append_args(message, DBUS_TYPE_STRING, &kimpanelServiceName, DBUS_TYPE_INVALID);

        DBusPendingCall* call = NULL;
        dbus_bool_t reply = dbus_connection_send_with_reply(kimpanel->conn, message,
                            &call,
                            0);
        if (reply == TRUE) {
            dbus_pending_call_set_notify(call,
                                         KimpanelServiceExistCallback,
                                         kimpanel,
                                         NULL);
        }
        dbus_connection_flush(kimpanel->conn);
        dbus_message_unref(message);

        KimpanelRegisterAllStatus(kimpanel);
        dbus_error_free(&err);
        return kimpanel;
    } while (0);

    dbus_error_free(&err);
    free(kimpanel);
    return NULL;
}

void KimpanelRegisterAllStatus(FcitxKimpanelUI* kimpanel)
{
    FcitxInstance* instance = kimpanel->owner;
    UT_array* uistats = FcitxInstanceGetUIStats(instance);
    UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
    char **prop = fcitx_utils_malloc0(sizeof(char*) * (2 + utarray_len(uistats) + utarray_len(uicompstats)));
    asprintf(&prop[0], "/Fcitx/logo:%s:%s:%s", _("Fcitx"), "fcitx", _("Fcitx"));

    char* icon;
    char* imname;
    char* description;
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE) {
        FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
        if (im) {
            icon = im->strIconName;
            imname = im->strName;
            description = im->strName;
        } else {
            icon = "kbd";
            imname = _("Disabled");
            description = _("Input Method Disabled");
        }
    } else {
        icon = "kbd";
        imname = _("Disabled");
        description = _("Input Method Disabled");
    }
    /* add fcitx- prefix, unless icon name is an absolute path */
    if (icon[0] == '/')
        asprintf(&prop[1], "/Fcitx/im:%s:%s:%s", imname, icon, description);
    else
        asprintf(&prop[1], "/Fcitx/im:%s:fcitx-%s:%s", imname, icon, description);

    int count = 2;

    FcitxUIComplexStatus *compstatus;
    for (compstatus = (FcitxUIComplexStatus *) utarray_front(uicompstats);
         compstatus != NULL;
         compstatus = (FcitxUIComplexStatus *) utarray_next(uicompstats, compstatus)) {
        if (!compstatus->visible)
            continue;
        prop[count] = ComplexStatus2String(compstatus);
        count ++;
    }

    FcitxUIStatus *status;
    for (status = (FcitxUIStatus *) utarray_front(uistats);
            status != NULL;
            status = (FcitxUIStatus *) utarray_next(uistats, status)) {
        if (!status->visible)
            continue;
        prop[count] = Status2String(status);
        count ++;
    }

    KimRegisterProperties(kimpanel, prop, count);

    while (count --)
        free(prop[count]);

    free(prop);
}

void KimpanelSetIMStatus(FcitxKimpanelUI* kimpanel)
{
    FcitxInstance* instance = kimpanel->owner;
    char* status = NULL;
    char* icon;
    char* imname;
    char* description;
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(instance);
    if (ic == NULL) {
        icon = "kbd";
        imname = _("No input window");
        description = _("No input window");
    }
    else if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE) {
        FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
        if (im) {
            icon = im->strIconName;
            imname = im->strName;
            description = im->strName;
        } else {
            icon = "kbd";
            imname = _("Disabled");
            description = _("Input Method Disabled");
        }
    } else {
        icon = "kbd";
        imname = _("Disabled");
        description = _("Input Method Disabled");
    }
    /* add fcitx- prefix */
    if (icon[0] == '/')
        asprintf(&status, "/Fcitx/im:%s:%s:%s", imname, icon, description);
    else
        asprintf(&status, "/Fcitx/im:%s:fcitx-%s:%s", imname, icon, description);

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
    KimEnable(kimpanel, (FcitxInstanceGetCurrentStatev2(kimpanel->owner) == IS_ACTIVE));
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnInputUnFocus(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimEnable(kimpanel, (FcitxInstanceGetCurrentStatev2(kimpanel->owner) == IS_ACTIVE));
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnTriggerOff(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimEnable(kimpanel, false);
    KimpanelSetIMStatus(kimpanel);
}

void KimpanelOnTriggerOn(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimEnable(kimpanel, true);
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

    int x = 0, y = 0, w = 0, h = 0;

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(kimpanel->owner);
    FcitxInstanceGetWindowRect(kimpanel->owner, ic, &x, &y, &w, &h);

    if (kimpanel->version == 1)
        KimUpdateSpotLocation(kimpanel, x, y + h);
    else
        KimSetSpotRect(kimpanel, x, y, w, h);
}

void KimpanelRegisterMenu(void* arg, FcitxUIMenu* menu)
{
    return ;
}

void KimpanelRegisterStatus(void* arg, FcitxUIStatus* status)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelRegisterAllStatus(kimpanel);
    return ;
}

void KimpanelRegisterComplexStatus(void* arg, FcitxUIComplexStatus* status)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelRegisterAllStatus(kimpanel);
    return ;
}

char* Status2String(FcitxUIStatus* status)
{
    char *result = NULL;
    asprintf(&result, "/Fcitx/%s:%s:fcitx-%s-%s:%s",
             status->name,
             status->shortDescription,
             status->name,
             ((status->getCurrentStatus(status->arg)) ? "active" : "inactive"),
             status->longDescription
            );

    return result;
}


char* ComplexStatus2String(FcitxUIComplexStatus* status)
{
    const char* iconName = status->getIconName(status->arg);

    const char* templ;
    switch(iconName[0]) {
        case '/':
            templ = "/Fcitx/%s:%s:%s:%s";
            break;
        case '\0':
            templ = "/Fcitx/%s:%s:%s:%s";
            break;
        default:
            templ = "/Fcitx/%s:%s:fcitx-%s:%s";
            break;
    }

    char *result = NULL;
    asprintf(&result, templ,
             status->name,
             status->shortDescription,
             iconName,
             status->longDescription
            );

    return result;
}

void KimpanelShowInputWindow(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    FcitxInstance* instance = kimpanel->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    kimpanel->iCursorPos = FcitxUINewMessageToOldStyleMessage(instance, kimpanel->messageUp, kimpanel->messageDown);
    FcitxMessages* messageDown = kimpanel->messageDown;
    FcitxMessages* messageUp = kimpanel->messageUp;
    FcitxLog(DEBUG, "KimpanelShowInputWindow");
    KimpanelMoveInputWindow(kimpanel);

    int n = FcitxMessagesGetMessageCount(messageDown);
    int nLabels = 0;
    int nTexts = 0;
    char *label[33];
    char *text[33];
    char cmb[KIMPANEL_BUFFER_SIZE] = "";
    int i;
    int pos = -1;

    if (n) {
        for (i = 0; i < n; i++) {
            FcitxLog(DEBUG, "Type: %d Text: %s" , FcitxMessagesGetMessageType(messageDown, i), FcitxMessagesGetMessageString(messageDown, i));

            if (FcitxMessagesGetMessageType(messageDown, i) == MSG_INDEX) {
                if (nLabels) {
                    text[nTexts++] = strdup(cmb);
                }
                char *needfree = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(messageDown, i));
                char *msgstr;
                if (needfree)
                    msgstr = needfree;
                else
                    msgstr = strdup(FcitxMessagesGetMessageString(messageDown, i));

                label[nLabels++] = msgstr;
                strcpy(cmb, "");
            } else {
                char *needfree = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(messageDown, i));
                char *msgstr;
                if (needfree)
                    msgstr = needfree;
                else
                    msgstr = FcitxMessagesGetMessageString(messageDown, i);

                if (strlen(cmb) + strlen(msgstr) + 1 < KIMPANEL_BUFFER_SIZE)
                    strcat(cmb, msgstr);
                if (needfree)
                    free(needfree);
                if (FcitxMessagesGetMessageType(messageDown, i) == MSG_FIRSTCAND)
                    pos = nTexts;
            }
        }
        text[nTexts++] = strdup(cmb);
        if (nLabels < nTexts) {
            for (; nLabels < nTexts; nLabels++) {
                label[nLabels] = strdup("");
            }
        } else if (nTexts < nLabels) {
            for (; nTexts < nLabels; nTexts++) {
                text[nTexts] = strdup("");
            }
        }
        FcitxLog(DEBUG, "Labels %d, Texts %d, CMB:%s", nLabels, nTexts, cmb);
        if (nTexts == 0) {
            KimShowLookupTable(kimpanel, false);
        } else {
            KimUpdateLookupTable(kimpanel,
                                 label,
                                 nLabels,
                                 text,
                                 nTexts,
                                 FcitxCandidateWordHasPrev(FcitxInputStateGetCandidateList(input)),
                                 FcitxCandidateWordHasNext(FcitxInputStateGetCandidateList(input)));
            KimShowLookupTable(kimpanel, true);
        }
        for (i = 0; i < nTexts; i++)
            free(text[i]);
        for (i = 0; i < nLabels; i++)
            free(label[i]);
    } else {
        KimUpdateLookupTable(kimpanel,
                             NULL,
                             0,
                             NULL,
                             0,
                             FcitxCandidateWordHasPrev(FcitxInputStateGetCandidateList(input)),
                             FcitxCandidateWordHasNext(FcitxInputStateGetCandidateList(input)));
        KimShowLookupTable(kimpanel, false);
    }
    KimUpdateLookupTableCursor(kimpanel, pos);

    n = FcitxMessagesGetMessageCount(messageUp);
    char aux[MESSAGE_MAX_LENGTH] = "";
    char empty[MESSAGE_MAX_LENGTH] = "";
    if (n) {
        for (i = 0; i < n; i++) {

            char *needfree = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(messageUp, i));
            char *msgstr;
            if (needfree)
                msgstr = needfree;
            else
                msgstr = FcitxMessagesGetMessageString(messageUp, i);

            strcat(aux, msgstr);
            if (needfree)
                free(needfree);
            FcitxLog(DEBUG, "updateMesssages Up:%s", aux);
        }
        if (FcitxInputStateGetShowCursor(input)) {
            KimUpdatePreeditText(kimpanel, aux);
            KimUpdateAux(kimpanel, empty);
            KimShowPreedit(kimpanel, true);
            KimUpdatePreeditCaret(kimpanel, CalKimCursorPos(kimpanel));
            KimShowAux(kimpanel, false);
        } else {
            KimUpdatePreeditText(kimpanel, empty);
            KimUpdateAux(kimpanel, aux);
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
    KimpanelRegisterAllStatus(kimpanel);
    return ;
}

void KimpanelUpdateComplexStatus(void* arg, FcitxUIComplexStatus* status)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelRegisterAllStatus(kimpanel);
    return ;
}

static DBusHandlerResult KimpanelDBusEventHandler(DBusConnection *connection, DBusMessage *msg, void *arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;

    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
        DBusMessage *reply = dbus_message_new_method_return(msg);

        dbus_message_append_args(reply, DBUS_TYPE_STRING, &kimpanel_introspection_xml, DBUS_TYPE_INVALID);
        dbus_connection_send(kimpanel->conn, reply, NULL);
        dbus_message_unref(reply);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void KimpanelReset(FcitxKimpanelUI* kimpanel)
{
    kimpanel->lastCursor = -2;
    kimpanel->lastUpdateH = -2;
    kimpanel->lastUpdateW = -2;
    kimpanel->lastUpdateX = -2;
    kimpanel->lastUpdateY = -2;
}

DBusHandlerResult KimpanelDBusFilter(DBusConnection* connection, DBusMessage* msg, void* user_data)
{
    FCITX_UNUSED(connection);
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) user_data;
    FcitxInstance* instance = kimpanel->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(kimpanel->owner);
    int int0;
    const char* s0 = NULL;
    if (dbus_message_is_signal(msg, "org.kde.impanel", "MovePreeditCaret")) {
        FcitxLog(DEBUG, "MovePreeditCaret");
        DBusError error;
        dbus_error_init(&error);
        dbus_message_get_args(msg, &error, DBUS_TYPE_INT32, &int0 , DBUS_TYPE_INVALID);
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "SelectCandidate")) {
        FcitxLog(DEBUG, "SelectCandidate: ");
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_INT32, &int0 , DBUS_TYPE_INVALID)) {
            FcitxInstanceChooseCandidateByIndex(instance, int0);
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageUp")) {
        FcitxLog(DEBUG, "LookupTablePageUp");
        if (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0) {
            FcitxCandidateWordGoPrevPage(FcitxInputStateGetCandidateList(input));
            FcitxInstanceProcessInputReturnValue(instance, IRV_DISPLAY_CANDWORDS);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "LookupTablePageDown")) {
        FcitxLog(DEBUG, "LookupTablePageDown");
        if (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0) {
            FcitxCandidateWordGoNextPage(FcitxInputStateGetCandidateList(input));
            FcitxInstanceProcessInputReturnValue(instance, IRV_DISPLAY_CANDWORDS);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "TriggerProperty")) {
        FcitxLog(DEBUG, "TriggerProperty: ");
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &s0 , DBUS_TYPE_INVALID)) {
            size_t len = strlen("/Fcitx/");
            if (strlen(s0) > len) {
                s0 += len;
                if (strcmp("logo", s0) == 0) {
                    size_t len = 3;
                    char **prop = fcitx_utils_malloc0(len * sizeof(char*));
                    asprintf(&prop[0], "/Fcitx/logo/toggle:%s::%s", _("Toggle Input Method"), _("Toggle Input Method"));
                    asprintf(&prop[1], "/Fcitx/logo/configureim:%s:configure:%s", _("Configure Current Input Method"), _("Configure Current Input Method"));
                    asprintf(&prop[2], "/Fcitx/logo/restart:%s:view-refresh:%s",_("Restart"), _("Restart"));
                    KimExecMenu(kimpanel, prop, len );
                    while (len --)
                        free(prop[len]);
                    free(prop);
                } else if (strncmp("logo/", s0, strlen("logo/")) == 0) {
                    s0 += strlen("logo/");
                    if (strcmp(s0, "toggle") == 0)
                        FcitxInstanceChangeIMState(instance, FcitxInstanceGetCurrentIC(instance));
                    else if (strcmp(s0, "configureim") == 0) {
                        FcitxIM* im = FcitxInstanceGetCurrentIM(kimpanel->owner);
                        if (im && im->owner) {
                            fcitx_utils_launch_configure_tool_for_addon(im->uniqueName);
                        }
                        else
                            fcitx_utils_launch_configure_tool();
                    }
                    else if (strcmp(s0, "restart") == 0) {
                        fcitx_utils_launch_restart();
                    }
                } else if (strcmp("keyboard", s0) == 0) {
                    FcitxInstanceCloseIM(instance,
                                         FcitxInstanceGetCurrentIC(instance));
                } else if (strncmp("im/", s0, strlen("im/")) == 0) {
                    s0 += strlen("im/");
                    FcitxInstanceSwitchIMByName(instance, s0);
                } else if (strncmp("im", s0, strlen("im")) == 0) {
                    UT_array* imes = FcitxInstanceGetIMEs(instance);
                    FcitxIM* pim;
                    int index = 0;
                    size_t len = utarray_len(imes);
                    char **prop = fcitx_utils_malloc0(len * sizeof(char*));
                    for (pim = (FcitxIM *) utarray_front(imes);
                            pim != NULL;
                            pim = (FcitxIM *) utarray_next(imes, pim)) {

                        if (pim->strIconName[0] == '/')
                            asprintf(&prop[index], "/Fcitx/im/%s:%s:%s:%s", pim->uniqueName, pim->strName, pim->strIconName, pim->strName);
                        else
                            asprintf(&prop[index], "/Fcitx/im/%s:%s:fcitx-%s:%s", pim->uniqueName, pim->strName, pim->strIconName, pim->strName);
                        index ++;
                    }
                    KimExecMenu(kimpanel, prop, len);
                    while (len --)
                        free(prop[len]);
                    free(prop);
                } else {
                    /* menu */
                    const char* pos;
                    if ((pos = strchr(s0, '/'))) {
                        char* statusName = strndup(s0, pos - s0);
                        FcitxUIMenu* menup = FcitxUIGetMenuByStatusName(instance, statusName);
                        free(statusName);
                        pos ++;
                        int index = 0;
                        sscanf(pos, "%d", &index);
                        if (menup)
                            menup->MenuAction(menup, index);
                    }
                    else {
                        FcitxUIMenu* menu = FcitxUIGetMenuByStatusName(instance, s0);
                        if (menu) {
                            menu->UpdateMenu(menu);

                            int i = 0, index = 0;
                            char **prop = fcitx_utils_malloc0(utarray_len(&menu->shell) * sizeof(char*));
                            for (i = 0; i < utarray_len(&menu->shell); i++) {
                                if (GetMenuItem(menu, i)->type == MENUTYPE_SIMPLE) {
                                    asprintf(&prop[index], "/Fcitx/%s/%d:%s::%s", s0, index, GetMenuItem(menu, i)->tipstr, GetMenuItem(menu, i)->tipstr);
                                    index ++;
                                }
                            }
                            KimExecMenu(kimpanel, prop, index);
                            while (index --)
                                free(prop[index]);
                            free(prop);
                        }
                        else
                            FcitxUIUpdateStatus(instance, s0);
                    }
                }
            }
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "PanelCreated")) {
        FcitxLog(DEBUG, "PanelCreated");
        FcitxUIResumeFromFallback(instance);
        KimpanelReset(kimpanel);
        KimpanelRegisterAllStatus(kimpanel);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel2", "PanelCreated2")) {
        FcitxLog(DEBUG, "PanelCreated2");
        FcitxUIResumeFromFallback(instance);
        kimpanel->version = 2;
        KimpanelReset(kimpanel);
        KimpanelRegisterAllStatus(kimpanel);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "Exit")) {
        FcitxLog(DEBUG, "Exit");
        FcitxInstanceEnd(instance);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "ReloadConfig")) {
        FcitxLog(DEBUG, "ReloadConfig");
        FcitxInstanceReloadConfig(instance);
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "Restart")) {
        FcitxLog(DEBUG, "Restart");
        fcitx_utils_launch_restart();
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, "org.kde.impanel", "Configure")) {
        FcitxLog(DEBUG, "Configure");
        fcitx_utils_launch_configure_tool();
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
        const char* service, *oldowner, *newowner;
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_get_args(msg, &error,
                                  DBUS_TYPE_STRING, &service ,
                                  DBUS_TYPE_STRING, &oldowner ,
                                  DBUS_TYPE_STRING, &newowner ,
                                  DBUS_TYPE_INVALID)) {
            /* old die and no new one */
            if (strcmp(service, "org.kde.impanel") == 0
                    && strlen(oldowner) > 0
                    && strlen(newowner) == 0)
                FcitxUISwitchToFallback(instance);
            /*
             * since if new rise, it will send PanelCreated,
             * So don't need to re-register here
             */
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}



void KimExecDialog(FcitxKimpanelUI* kimpanel, char *prop)
{
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "ExecDialog"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    if (dbus_message_append_args(msg, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID)) {
        dbus_connection_send(kimpanel->conn, msg, &serial);
    }

    dbus_connection_flush(kimpanel->conn);

    // free the message
    dbus_message_unref(msg);

}

void KimExecMenu(FcitxKimpanelUI* kimpanel, char *props[], int n)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "ExecMenu"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    if (n == -1) {
        n = 0;
        while (*(props[n]) != 0) {
            if (!fcitx_utf8_check_string(props[n]))
                return;
            n++;
        }

    }

    int i;
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &sub);
    for (i = 0; i < n; i++) {
        if (!dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &props[i])) {
            FcitxLog(DEBUG, "Out Of Memory!");
        }
    }
    dbus_message_iter_close_container(&args, &sub);

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
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "RegisterProperties"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    if (n == -1) {
        n = 0;
        while (*(props[n]) != 0) {
            if (!fcitx_utf8_check_string(props[n]))
                return;
            n++;
        }
    }

    int i;
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    DBusMessageIter sub;
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &sub);
    for (i = 0; i < n; i++) {
        if (!dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &props[i])) {
            FcitxLog(DEBUG, "Out Of Memory!");
        }
    }
    dbus_message_iter_close_container(&args, &sub);

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // free the message
    dbus_message_unref(msg);

}

void KimUpdateProperty(FcitxKimpanelUI* kimpanel, char *prop)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdateProperty"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    if (!fcitx_utf8_check_string(prop))
        return;

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // free the message
    dbus_message_unref(msg);

}

void KimRemoveProperty(FcitxKimpanelUI* kimpanel, char *prop)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "RemoveProperty"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    if (!fcitx_utf8_check_string(prop))
        return;

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &prop)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // free the message
    dbus_message_unref(msg);

}

void KimEnable(FcitxKimpanelUI* kimpanel, boolean toEnable)
{
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "Enable"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimShowAux(FcitxKimpanelUI* kimpanel, boolean toShow)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "ShowAux"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimShowPreedit(FcitxKimpanelUI* kimpanel, boolean toShow)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "ShowPreedit"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimUpdateLookupTableCursor(FcitxKimpanelUI* kimpanel, int cursor)
{
    if (kimpanel->lastCursor != cursor)
        kimpanel->lastCursor = cursor;
    else
        return;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdateLookupTableCursor"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &cursor)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // free the message
    dbus_message_unref(msg);
}

void KimShowLookupTable(FcitxKimpanelUI* kimpanel, boolean toShow)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "ShowLookupTable"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimUpdateLookupTable(FcitxKimpanelUI* kimpanel, char *labels[], int nLabel, char *texts[], int nText, boolean has_prev, boolean has_next)
{
    int i;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdateLookupTable"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }
    for (i = 0; i < nLabel; i++) {
        if (!fcitx_utf8_check_string(labels[i]))
            return;
    }
    for (i = 0; i < nText; i++) {
        if (!fcitx_utf8_check_string(texts[i]))
            return;
    }
    DBusMessageIter subLabel;
    DBusMessageIter subText;
    DBusMessageIter subAttrs;
    // append arguments onto signal
    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &subLabel);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subLabel, DBUS_TYPE_STRING, &labels[i])) {
            FcitxLog(DEBUG, "Out Of Memory!");
        }
    }
    dbus_message_iter_close_container(&args, &subLabel);

    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &subText);
    for (i = 0; i < nText; i++) {
        if (!dbus_message_iter_append_basic(&subText, DBUS_TYPE_STRING, &texts[i])) {
            FcitxLog(DEBUG, "Out Of Memory!");
        }
    }
    dbus_message_iter_close_container(&args, &subText);

    char *attr = "";
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &subAttrs);
    for (i = 0; i < nLabel; i++) {
        if (!dbus_message_iter_append_basic(&subAttrs, DBUS_TYPE_STRING, &attr)) {
            FcitxLog(DEBUG, "Out Of Memory!");
        }
    }
    dbus_message_iter_close_container(&args, &subAttrs);

    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_prev);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &has_next);

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // free the message
    dbus_message_unref(msg);

}

void KimUpdatePreeditCaret(FcitxKimpanelUI* kimpanel, int position)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdatePreeditCaret"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimUpdatePreeditText(FcitxKimpanelUI* kimpanel, char *text)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdatePreeditText"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimUpdateAux(FcitxKimpanelUI* kimpanel, char *text)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdateAux"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }
    if (!fcitx_utf8_check_string(text))
        return;

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

    // free the message
    dbus_message_unref(msg);

}

void KimUpdateSpotLocation(FcitxKimpanelUI* kimpanel, int x, int y)
{
    if (kimpanel->lastUpdateX == x && kimpanel->lastUpdateY == y)
        return;
    kimpanel->lastUpdateX = x;
    kimpanel->lastUpdateY = y;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdateSpotLocation"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

void KimSetSpotRect(FcitxKimpanelUI* kimpanel, int x, int y, int w, int h)
{
    if (kimpanel->lastUpdateX == x && kimpanel->lastUpdateY == y && kimpanel->lastUpdateW == w && kimpanel->lastUpdateH == h)
        return;
    kimpanel->lastUpdateX = x;
    kimpanel->lastUpdateY = y;
    kimpanel->lastUpdateW = w;
    kimpanel->lastUpdateH = h;
    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;

    // create a signal and check for errors
    msg = dbus_message_new_method_call("org.kde.impanel",
                                  "/org/kde/impanel",
                                  "org.kde.impanel2",
                                  "SetSpotRect"); // name of the signal
    if (NULL == msg) {
        FcitxLog(DEBUG, "Message Null");
        return;
    }

    if (!dbus_message_append_args(
        msg,
        DBUS_TYPE_INT32, &x,
        DBUS_TYPE_INT32, &y,
        DBUS_TYPE_INT32, &w,
        DBUS_TYPE_INT32, &h,
        DBUS_TYPE_INVALID
    )) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // send the message and flush the connection
    if (!dbus_connection_send(kimpanel->conn, msg, &serial)) {
        FcitxLog(DEBUG, "Out Of Memory!");
    }

    // free the message
    dbus_message_unref(msg);
}


void KimUpdateScreen(FcitxKimpanelUI* kimpanel, int id)
{

    dbus_uint32_t serial = 0; // unique number to associate replies with requests
    DBusMessage* msg;
    DBusMessageIter args;

    // create a signal and check for errors
    msg = dbus_message_new_signal(FCITX_KIMPANEL_PATH, // object name of the signal
                                  FCITX_KIMPANEL_INTERFACE, // interface name of the signal
                                  "UpdateScreen"); // name of the signal
    if (NULL == msg) {
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

    // free the message
    dbus_message_unref(msg);

}

int CalKimCursorPos(FcitxKimpanelUI *kimpanel)
{
    size_t             i = 0;
    int             iChar;
    int             iCount = 0;
    FcitxMessages* messageUp = kimpanel->messageUp;

    const char      *p1;
    const char      *pivot;

    iChar = kimpanel->iCursorPos;


    for (i = 0; i < FcitxMessagesGetMessageCount(messageUp) ; i++) {
        if (kimpanel->iCursorPos && iChar) {
            p1 = pivot = FcitxMessagesGetMessageString(messageUp, i);
            while (*p1 && p1 < pivot + iChar) {
                p1 = p1 + fcitx_utf8_char_len(p1);
                iCount ++;
            }
            if (strlen(FcitxMessagesGetMessageString(messageUp, i)) > iChar) {
                iChar = 0;
            } else {
                iChar -= strlen(FcitxMessagesGetMessageString(messageUp, i));
            }
        }
    }

    return iCount;
}

void KimpanelServiceExistCallback(DBusPendingCall *call, void *data)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) data;

    DBusMessage* msg = dbus_pending_call_steal_reply(call);

    if (msg) {
        dbus_bool_t has;
        DBusError error;
        dbus_error_init(&error);
        dbus_message_get_args(msg, &error, DBUS_TYPE_BOOLEAN, &has , DBUS_TYPE_INVALID);
        if (!has) {
            FcitxUISwitchToFallback(kimpanel->owner);
        }
        else {
            KimpanelIntrospect(kimpanel);
        }
        dbus_message_unref(msg);
        dbus_error_free(&error);
    }

    dbus_pending_call_cancel(call);
    dbus_pending_call_unref(call);
}

void KimpanelIntrospect(FcitxKimpanelUI* kimpanel)
{
    const char* kimpanelServiceName = "org.kde.impanel";
    DBusMessage* message = dbus_message_new_method_call(kimpanelServiceName, "/org/kde/impanel", DBUS_INTERFACE_INTROSPECTABLE, "Introspect");

    DBusPendingCall* call = NULL;
    dbus_bool_t reply = dbus_connection_send_with_reply(kimpanel->conn, message,
                        &call,
                        0);
    if (reply == TRUE) {
        dbus_pending_call_set_notify(call,
                                    KimpanelIntrospectCallback,
                                    kimpanel,
                                    NULL);
    }
    dbus_connection_flush(kimpanel->conn);
    dbus_message_unref(message);
}

void KimpanelIntrospectCallback(DBusPendingCall* call, void* data)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) data;

    DBusMessage* msg = dbus_pending_call_steal_reply(call);

    if (msg) {
        const char* s;
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_get_args(msg, &error, DBUS_TYPE_STRING, &s , DBUS_TYPE_INVALID)) {
            if (strstr(s, "org.kde.impanel2"))
                kimpanel->version = 2;
        }
        dbus_message_unref(msg);
        dbus_error_free(&error);
    }

    dbus_pending_call_cancel(call);
    dbus_pending_call_unref(call);
}


void KimpanelSuspend(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    kimpanel->version = 1;
}


void KimpanelDestroy(void* arg)
{
    FcitxKimpanelUI* kimpanel = (FcitxKimpanelUI*) arg;
    KimpanelCloseInputWindow(kimpanel);
    KimRegisterProperties(kimpanel, NULL, 0);
    dbus_connection_unregister_object_path(kimpanel->conn, FCITX_KIMPANEL_PATH);
    dbus_connection_remove_filter(kimpanel->conn, KimpanelDBusFilter, kimpanel);

    DBusError err;
    dbus_error_init(&err);
    dbus_bus_remove_match(kimpanel->conn,
                          "type='signal',interface='org.kde.impanel'",
                          &err);
    dbus_connection_flush(kimpanel->conn);

    dbus_bus_remove_match(kimpanel->conn,
                        "type='signal',"
                        "interface='" DBUS_INTERFACE_DBUS "',"
                        "path='" DBUS_PATH_DBUS "',"
                        "member='NameOwnerChanged'",
                        &err);

    dbus_connection_flush(kimpanel->conn);
    dbus_error_free(&err);

    free(kimpanel->messageUp);
    free(kimpanel->messageDown);
    free(kimpanel);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
