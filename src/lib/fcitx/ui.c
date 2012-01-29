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

#include <dlfcn.h>
#include <libintl.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "ui.h"
#include "addon.h"
#include "fcitx-utils/utarray.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "instance.h"
#include "hook-internal.h"
#include "ime-internal.h"
#include "candidate.h"
#include "frontend.h"
#include "instance-internal.h"
#include "addon-internal.h"
#include "ui-internal.h"

/**
 * @file ui.c
 *
 * @brief user interface related function.
 */

/**
 * @brief a single string message
 **/

struct _FcitxMessage {
    /**
     * @brief The string of the message
     **/
    char            strMsg[MESSAGE_MAX_LENGTH + 1];
    /**
     * @brief the type of the message
     **/
    FcitxMessageType        type;
} ;

/**
 * @brief FcitxMessages to display on the input bar, this cannot be accessed directly
 **/

struct _FcitxMessages {
    /**
     * @brief array of message strings
     **/
    struct _FcitxMessage msg[MAX_MESSAGE_COUNT];
    /**
     * @brief number of message strings
     **/
    uint msgCount;
    /**
     * @brief the messages is updated or not
     **/
    boolean changed;
};

#define UI_FUNC_IS_VALID(funcname) (!(FcitxInstanceGetCurrentCapacity(instance) & CAPACITY_CLIENT_SIDE_UI) && instance->ui && instance->ui->ui->funcname)

static void FcitxUIShowInputWindow(FcitxInstance* instance);
static boolean FcitxUILoadInternal(FcitxInstance* instance, FcitxAddon* addon);
static void FcitxMenuItemFree(void* arg);

static const UT_icd menuICD = { sizeof(FcitxMenuItem), NULL, NULL, FcitxMenuItemFree };

FCITX_EXPORT_API
FcitxMessages* FcitxMessagesNew()
{
    return fcitx_utils_malloc0(sizeof(FcitxMessages));
}

FCITX_EXPORT_API
void FcitxMessagesSetMessageCount(FcitxMessages* m, int s)
{
    if ((s) <= MAX_MESSAGE_COUNT && s >= 0)
        ((m)->msgCount = (s));

    (m)->changed = true;
}

FCITX_EXPORT_API
int FcitxMessagesGetMessageCount(FcitxMessages* m)
{
    return m->msgCount;
}

FCITX_EXPORT_API
boolean FcitxMessagesIsMessageChanged(FcitxMessages* m)
{
    return m->changed;
}

FCITX_EXPORT_API
char* FcitxMessagesGetMessageString(FcitxMessages* m, int index)
{
    return m->msg[index].strMsg;
}

FCITX_EXPORT_API
FcitxMessageType FcitxMessagesGetMessageType(FcitxMessages* m, int index)
{
    if (m->msg[index].type <= MSG_TYPE_LAST)
        return m->msg[index].type;
    else
        return MSG_OTHER;
}

FCITX_EXPORT_API
void FcitxMessagesSetMessageChanged(FcitxMessages* m, boolean changed)
{
    m->changed = changed;
}

FCITX_EXPORT_API
void FcitxUILoad(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    FcitxAddon *addon;

    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->bEnabled && addon->category == AC_UI) {
            if (FcitxUILoadInternal(instance, addon))
                instance->uinormal = addon;

            if (instance->uinormal != NULL)
                break;
        }
    }

    instance->ui = instance->uinormal;

    if (instance->ui == NULL)
        FcitxLog(ERROR, "no usable user interface.");
    else {
        do {
            FcitxAddon* fallbackAddon = FcitxAddonsGetAddonByName(&instance->addons, addon->uifallback);
            if (fallbackAddon == NULL)
                break;

            if (!fallbackAddon->bEnabled)
                break;

            if (FcitxUILoadInternal(instance, fallbackAddon)) {
                instance->uifallback = fallbackAddon;
                if (instance->uifallback->ui->Suspend)
                    instance->uifallback->ui->Suspend(instance->uifallback->addonInstance);
            }
        } while (0);
    }
}

boolean FcitxUILoadInternal(FcitxInstance* instance, FcitxAddon* addon)
{
    boolean success = false;
    char *modulePath;

    switch (addon->type) {

    case AT_SHAREDLIBRARY: {
        FILE *fp = FcitxXDGGetLibFile(addon->library, "r", &modulePath);
        void *handle;

        if (!fp)
            break;

        fclose(fp);

        handle = dlopen(modulePath, RTLD_LAZY | RTLD_GLOBAL);

        if (!handle) {
            FcitxLog(ERROR, _("UI: open %s fail %s") , modulePath , dlerror());
            break;
        }

        if (!CheckABIVersion(handle)) {
            FcitxLog(ERROR, "%s ABI Version Error", addon->name);
            dlclose(handle);
            break;
        }

        addon->ui = dlsym(handle, "ui");

        if (!addon->ui || !addon->ui->Create) {
            FcitxLog(ERROR, _("UI: bad ui"));
            dlclose(handle);
            break;
        }

        if ((addon->addonInstance = addon->ui->Create(instance)) == NULL) {
            dlclose(handle);
            break;
        }

        /* some may register before ui load, so load it here */
        if (addon->ui->RegisterStatus) {
            UT_array* uistats = &instance->uistats;
            FcitxUIStatus *status;

            for (status = (FcitxUIStatus *) utarray_front(uistats);
                    status != NULL;
                    status = (FcitxUIStatus *) utarray_next(uistats, status))
                addon->ui->RegisterStatus(addon->addonInstance, status);
        }

        if (addon->ui->RegisterMenu) {
            UT_array* uimenus = &instance->uimenus;
            FcitxUIMenu **menupp;

            for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
                    menupp != NULL;
                    menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp))
                addon->ui->RegisterMenu(addon->addonInstance, *menupp);
        }

        success = true;
    }

    break;

    default:
        break;
    }

    free(modulePath);
    return success;
}

FCITX_EXPORT_API
void FcitxMessagesAddMessageAtLast(FcitxMessages* message, FcitxMessageType type, const char *fmt, ...)
{

    if (message->msgCount < MAX_MESSAGE_COUNT) {
        va_list ap;
        va_start(ap, fmt);
        FcitxMessagesSetMessageV(message, message->msgCount, type, fmt, ap);
        va_end(ap);
        message->msgCount ++;
        message->changed = true;
    }
}

FCITX_EXPORT_API
void FcitxMessagesSetMessage(FcitxMessages* message, int position, FcitxMessageType type, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FcitxMessagesSetMessageV(message, position, type, fmt, ap);
    va_end(ap);
}

FCITX_EXPORT_API
void FcitxMessagesSetMessageText(FcitxMessages* message, int position, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    FcitxMessagesSetMessageV(message, position, message->msg[position].type, fmt, ap);
    va_end(ap);
}

FCITX_EXPORT_API
void FcitxMessagesSetMessageV(FcitxMessages* message, int position, FcitxMessageType type, const char* fmt, va_list ap)
{
    if (position < MAX_MESSAGE_COUNT) {
        vsnprintf(message->msg[position].strMsg, MESSAGE_MAX_LENGTH, fmt, ap);
        message->msg[position].type = type;
        message->changed = true;
    }
}

FCITX_EXPORT_API
void FcitxMessagesMessageConcatLast(FcitxMessages* message, const char* text)
{
    strncat(message->msg[message->msgCount - 1].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

FCITX_EXPORT_API
void FcitxMessagesMessageConcat(FcitxMessages* message, int position, const char* text)
{
    strncat(message->msg[position].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

FCITX_EXPORT_API
void FcitxUICloseInputWindow(FcitxInstance* instance)
{
    FcitxInstanceCleanInputWindow(instance);
    instance->uiflag |= UI_UPDATE;
}

FCITX_EXPORT_API
void FcitxUIUpdateInputWindow(FcitxInstance *instance)
{
    instance->uiflag |= UI_UPDATE;

    if (FcitxMessagesIsMessageChanged(instance->input->msgClientPreedit))
        FcitxInstanceUpdatePreedit(instance, FcitxInstanceGetCurrentIC(instance));
}

void FcitxUIShowInputWindow(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(ShowInputWindow))
        instance->ui->ui->ShowInputWindow(instance->ui->addonInstance);
}

FCITX_EXPORT_API
void FcitxUIMoveInputWindow(FcitxInstance* instance)
{
    instance->uiflag |= UI_MOVE;
}

FCITX_EXPORT_API
FcitxUIStatus *FcitxUIGetStatusByName(FcitxInstance* instance, const char* name)
{
    UT_array* uistats = &instance->uistats;
    FcitxUIStatus *status;

    for (status = (FcitxUIStatus *) utarray_front(uistats);
            status != NULL;
            status = (FcitxUIStatus *) utarray_next(uistats, status))
        if (strcmp(status->name, name) == 0)
            break;

    return status;
}

FCITX_EXPORT_API
void FcitxUIUpdateStatus(FcitxInstance* instance, const char* name)
{
    FcitxLog(DEBUG, "Update Status for %s", name);

    FcitxUIStatus *status = FcitxUIGetStatusByName(instance, name);

    if (status != NULL) {
        status->toggleStatus(status->arg);

        if (UI_FUNC_IS_VALID(UpdateStatus))
            instance->ui->ui->UpdateStatus(instance->ui->addonInstance , status);
    }
}

FCITX_EXPORT_API
void FcitxUISetStatusVisable(FcitxInstance* instance, const char* name, boolean visible)
{
    FcitxUIStatus *status = FcitxUIGetStatusByName(instance, name);
    if (!status)
        return;
    
    if (status->visible != visible) {
        status->visible = visible;

        if (UI_FUNC_IS_VALID(UpdateStatus))
            instance->ui->ui->UpdateStatus(instance->ui->addonInstance , status);
    }
}

FCITX_EXPORT_API
void FcitxUIRegisterStatus(
    struct _FcitxInstance* instance,
    void* arg,
    const char* name,
    const char* shortDesc,
    const char* longDesc,
    void (*toggleStatus)(void *arg),
    boolean(*getStatus)(void *arg)
)
{
    FcitxUIStatus status;

    memset(&status, 0 , sizeof(FcitxUIStatus));

    status.name = strdup(name);

    status.shortDescription = strdup(shortDesc);

    status.longDescription = strdup(longDesc);

    status.getCurrentStatus = getStatus;

    status.toggleStatus = toggleStatus;

    status.arg = arg;
    
    status.visible = true;

    UT_array* uistats = &instance->uistats;

    utarray_push_back(uistats, &status);
}

FCITX_EXPORT_API
void FcitxUIRegisterMenu(FcitxInstance* instance, FcitxUIMenu* menu)
{
    UT_array* uimenus = &instance->uimenus;

    if (!menu)
        return ;

    menu->mark = -1;

    utarray_push_back(uimenus, &menu);
}

FCITX_EXPORT_API
void FcitxMenuAddMenuItem(FcitxUIMenu* menu, char* string, FcitxMenuItemType type, FcitxUIMenu* subMenu)
{
    FcitxMenuItem shell;
    memset(&shell, 0, sizeof(FcitxMenuItem));

    if (string == NULL && type != MENUTYPE_DIVLINE) {
        return;
    }
    if (string)
        shell.tipstr = strdup(string);
    else
        shell.tipstr = NULL;

    shell.type = type;

    shell.isselect = false;

    if (type == MENUTYPE_SUBMENU)
        shell.subMenu = subMenu;

    utarray_push_back(&menu->shell, &shell);
}

FCITX_EXPORT_API
void FcitxMenuClear(FcitxUIMenu* menu)
{
    utarray_clear(&menu->shell);
}

FCITX_EXPORT_API
void FcitxUIOnInputFocus(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnInputFocus))
        instance->ui->ui->OnInputFocus(instance->ui->addonInstance);

    FcitxInstanceProcessInputFocusHook(instance);

    FcitxInstanceResetInput(instance);
    if (instance->config->firstAsInactive) {
        if (FcitxInstanceGetCurrentState(instance) == IS_ACTIVE)
            FcitxInstanceSwitchIM(instance, instance->lastIMIndex);
        else if (FcitxInstanceGetCurrentState(instance) == IS_ENG) {
            if (instance->iIMIndex != 0)
                instance->lastIMIndex = instance->iIMIndex;
            FcitxInstanceSwitchIMInternal(instance, 0, false);
        }
    }

    FcitxUICloseInputWindow(instance);
}

FCITX_EXPORT_API
void FcitxUIOnInputUnFocus(struct _FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnInputUnFocus))
        instance->ui->ui->OnInputUnFocus(instance->ui->addonInstance);

    FcitxInstanceProcessInputUnFocusHook(instance);
}

FCITX_EXPORT_API
void FcitxUIOnTriggerOn(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnTriggerOn))
        instance->ui->ui->OnTriggerOn(instance->ui->addonInstance);

    FcitxInstanceProcessTriggerOnHook(instance);

    instance->timeStart = time(NULL);

    FcitxInstanceShowInputSpeed(instance);
}

FCITX_EXPORT_API
void FcitxUIDisplayMessage(FcitxInstance *instance, char *title, char **msg, int length)
{
    if (UI_FUNC_IS_VALID(DisplayMessage))
        instance->ui->ui->DisplayMessage(instance->ui->addonInstance, title, msg, length);
}

FCITX_EXPORT_API
void FcitxUIOnTriggerOff(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnTriggerOff))
        instance->ui->ui->OnTriggerOff(instance->ui->addonInstance);

    FcitxInstanceProcessTriggerOffHook(instance);

    instance->totaltime += difftime(time(NULL), instance->timeStart);
}

FCITX_EXPORT_API
void FcitxMenuUpdate(FcitxUIMenu* menu)
{
    if (menu && menu->UpdateMenu) {
        menu->UpdateMenu(menu);
    }
}

/*
 * 判断鼠标点击处是否处于指定的区域内
 */
FCITX_EXPORT_API
boolean
FcitxUIIsInBox(int x0, int y0, int x1, int y1, int w, int h)
{
    if (x0 >= x1 && x0 <= x1 + w && y0 >= y1 && y0 <= y1 + h)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxUISupportMainWindow(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(MainWindowSizeHint))
        return true;
    else
        return false;
}

FCITX_EXPORT_API
void FcitxUIGetMainWindowSize(FcitxInstance* instance, int* x, int* y, int* w, int* h)
{
    if (UI_FUNC_IS_VALID(MainWindowSizeHint))
        instance->ui->ui->MainWindowSizeHint(instance->ui->addonInstance, x, y, w, h);
}

FCITX_EXPORT_API
int FcitxUINewMessageToOldStyleMessage(FcitxInstance* instance, FcitxMessages* msgUp, FcitxMessages* msgDown)
{
    int i = 0;
    FcitxInputState* input = instance->input;
    int extraLength = input->iCursorPos;
    FcitxMessagesSetMessageCount(msgUp, 0);
    FcitxMessagesSetMessageCount(msgDown, 0);

    for (i = 0; i < FcitxMessagesGetMessageCount(input->msgAuxUp) ; i ++) {
        FcitxMessagesAddMessageAtLast(msgUp, FcitxMessagesGetMessageType(input->msgAuxUp, i), "%s", FcitxMessagesGetMessageString(input->msgAuxUp, i));
        extraLength += strlen(FcitxMessagesGetMessageString(input->msgAuxUp, i));
    }

    for (i = 0; i < FcitxMessagesGetMessageCount(input->msgPreedit) ; i ++)
        FcitxMessagesAddMessageAtLast(msgUp, FcitxMessagesGetMessageType(input->msgPreedit, i), "%s", FcitxMessagesGetMessageString(input->msgPreedit, i));

    for (i = 0; i < FcitxMessagesGetMessageCount(input->msgAuxDown) ; i ++)
        FcitxMessagesAddMessageAtLast(msgDown, FcitxMessagesGetMessageType(input->msgAuxDown, i), "%s", FcitxMessagesGetMessageString(input->msgAuxDown, i));

    FcitxCandidateWord* candWord = NULL;

    for (candWord = FcitxCandidateWordGetCurrentWindow(input->candList), i = 0;
            candWord != NULL;
            candWord = FcitxCandidateWordGetCurrentWindowNext(input->candList, candWord), i ++) {
        char strTemp[3] = { '\0', '\0', '\0' };
        strTemp[0] = FcitxCandidateWordGetChoose(input->candList)[i];

        if (instance->config->bPointAfterNumber)
            strTemp[1] = '.';

        FcitxMessagesAddMessageAtLast(msgDown, MSG_INDEX, "%s", strTemp);

        FcitxMessageType type = candWord->wordType;

        if (i == 0
            && FcitxCandidateWordGetCurrentPage(input->candList) == 0
            && type == MSG_OTHER)
            type = MSG_FIRSTCAND;

        FcitxMessagesAddMessageAtLast(msgDown, type, "%s", candWord->strWord);

        if (candWord->strExtra && strlen(candWord->strExtra) != 0)
            FcitxMessagesAddMessageAtLast(msgDown, candWord->extraType, "%s", candWord->strExtra);

        FcitxMessagesAddMessageAtLast(msgDown, MSG_OTHER, " ");
    }

    return extraLength;
}

FCITX_EXPORT_API
char* FcitxUIMessagesToCString(FcitxMessages* messages)
{
    int length = 0;
    int i = 0;

    for (i = 0; i < FcitxMessagesGetMessageCount(messages) ; i ++)
        length += strlen(FcitxMessagesGetMessageString(messages, i));

    char* str = fcitx_utils_malloc0(sizeof(char) * (length + 1));

    for (i = 0; i < FcitxMessagesGetMessageCount(messages) ; i ++)
        strcat(str, FcitxMessagesGetMessageString(messages, i));

    return str;
}

FCITX_EXPORT_API
char* FcitxUICandidateWordToCString(FcitxInstance* instance)
{
    size_t len = 0;
    int i;
    FcitxInputState* input = instance->input;
    FcitxCandidateWord* candWord;
    for (candWord = FcitxCandidateWordGetCurrentWindow(input->candList), i = 0;
            candWord != NULL;
            candWord = FcitxCandidateWordGetCurrentWindowNext(input->candList, candWord), i ++) {
        char strTemp[3] = { '\0', '\0', '\0' };
        strTemp[0] = FcitxCandidateWordGetChoose(input->candList)[i];

        if (instance->config->bPointAfterNumber)
            strTemp[1] = '.';

        len += strlen(strTemp);
        len += strlen(candWord->strWord);

        if (candWord->strExtra && strlen(candWord->strExtra) != 0)
            len += strlen(candWord->strExtra);

        len ++;
    }

    char *result = fcitx_utils_malloc0(sizeof(char) * (len + 1));

    for (candWord = FcitxCandidateWordGetCurrentWindow(input->candList), i = 0;
            candWord != NULL;
            candWord = FcitxCandidateWordGetCurrentWindowNext(input->candList, candWord), i ++) {
        char strTemp[3] = { '\0', '\0', '\0' };
        strTemp[0] = FcitxCandidateWordGetChoose(input->candList)[i];

        if (instance->config->bPointAfterNumber)
            strTemp[1] = '.';

        strcat(result, strTemp);
        strcat(result, candWord->strWord);

        if (candWord->strExtra && strlen(candWord->strExtra) != 0)
            strcat(result, candWord->strExtra);

        strcat(result, " ");
    }

    return result;
}

void FcitxUIUpdateInputWindowReal(FcitxInstance *instance)
{
    FcitxInputState* input = instance->input;
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(instance);
    FcitxCapacityFlags flags = CAPACITY_NONE;
    if (ic != NULL)
        flags = ic->contextCaps;

    if (flags & CAPACITY_CLIENT_SIDE_UI) {
        FcitxInstanceUpdateClientSideUI(instance, ic);
        return;
    }

    boolean toshow = false;

    if (FcitxMessagesGetMessageCount(input->msgAuxUp) != 0
            || FcitxMessagesGetMessageCount(input->msgAuxDown) != 0)
        toshow = true;

    if (FcitxCandidateWordGetListSize(input->candList) > 1)
        toshow = true;

    if (FcitxCandidateWordGetListSize(input->candList) == 1
            && (!instance->config->bHideInputWindowWhenOnlyPreeditString
                || !instance->config->bHideInputWindowWhenOnlyOneCandidate))
        toshow = true;

    if (FcitxMessagesGetMessageCount(input->msgPreedit) != 0
            && !((flags & CAPACITY_PREEDIT) && instance->config->bHideInputWindowWhenOnlyPreeditString && instance->profile->bUsePreedit))
        toshow = true;

    if (!toshow) {
        FcitxInstanceUpdatePreedit(instance, ic);
        if (UI_FUNC_IS_VALID(CloseInputWindow))
            instance->ui->ui->CloseInputWindow(instance->ui->addonInstance);
    } else
        FcitxUIShowInputWindow(instance);
}

void FcitxUIMoveInputWindowReal(FcitxInstance *instance)
{
    if (UI_FUNC_IS_VALID(MoveInputWindow))
        instance->ui->ui->MoveInputWindow(instance->ui->addonInstance);
}

FCITX_EXPORT_API
void FcitxUISwitchToFallback(struct _FcitxInstance* instance)
{
    if (!instance->uifallback || instance->ui != instance->uinormal)
        return;

    if (instance->uinormal->ui->Suspend)
        instance->uinormal->ui->Suspend(instance->uinormal->addonInstance);

    if (instance->uifallback->ui->Resume)
        instance->uifallback->ui->Resume(instance->uifallback->addonInstance);

    instance->ui = instance->uifallback;
}

FCITX_EXPORT_API
void FcitxUIResumeFromFallback(struct _FcitxInstance* instance)
{
    if (!instance->uifallback || instance->ui != instance->uifallback)
        return;
    if (instance->uifallback->ui->Suspend)
        instance->uifallback->ui->Suspend(instance->uifallback->addonInstance);

    if (instance->uinormal->ui->Resume)
        instance->uinormal->ui->Resume(instance->uinormal->addonInstance);

    instance->ui = instance->uinormal;

}

FCITX_EXPORT_API
boolean FcitxUIIsFallback(struct _FcitxInstance* instance, struct _FcitxAddon* addon)
{
    return instance->uifallback == addon;
}

FCITX_EXPORT_API
void FcitxMenuInit(FcitxUIMenu* menu)
{
    utarray_init(&menu->shell, &menuICD);
}

void FcitxMenuItemFree(void* arg)
{
    FcitxMenuItem* item = arg;
    if (item->tipstr)
        free(item->tipstr);
}


// kate: indent-mode cstyle; space-indent on; indent-width 4;
