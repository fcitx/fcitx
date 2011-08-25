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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <dlfcn.h>
#include <libintl.h>
#include <stdarg.h>

#include "ui.h"
#include "addon.h"
#include "fcitx-utils/utarray.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include <sys/stat.h>
#include "fcitx-utils/utils.h"
#include "instance.h"
#include "hook-internal.h"
#include "ime-internal.h"
#include "candidate.h"
#include "frontend.h"

/**
 * @file ui.c
 *
 * @brief user interface related function.
 */

/**
 * @brief a single string message
 **/

struct _MESSAGE
{
    /**
     * @brief The string of the message
     **/
    char            strMsg[MESSAGE_MAX_LENGTH + 1];
    /**
     * @brief the type of the message
     **/
    MSG_TYPE        type;
} ;

/**
 * @brief Messages to display on the input bar, this cannot be accessed directly
 **/

struct _Messages
{
    /**
     * @brief array of message strings
     **/
    MESSAGE msg[MAX_MESSAGE_COUNT];
    /**
     * @brief number of message strings
     **/
    uint msgCount;
    /**
     * @brief the messages is updated or not
     **/
    boolean changed;
};

#define UI_FUNC_IS_VALID(funcname) (!(GetCurrentCapacity(instance) & CAPACITY_CLIENT_SIDE_UI) && instance->ui && instance->ui->ui->funcname)

static void ShowInputWindow(FcitxInstance* instance);

FCITX_EXPORT_API
Messages* InitMessages()
{
    return fcitx_malloc0(sizeof(Messages));
}

FCITX_EXPORT_API
void SetMessageCount(Messages* m, int s)
{
    if ((s) <= MAX_MESSAGE_COUNT && s >= 0)
        ((m)->msgCount = (s));

    (m)->changed = true;
}

FCITX_EXPORT_API
int GetMessageCount(Messages* m)
{
    return m->msgCount;
}

FCITX_EXPORT_API
boolean IsMessageChanged(Messages* m)
{
    return m->changed;
}

FCITX_EXPORT_API
char* GetMessageString(Messages* m, int index)
{
    return m->msg[index].strMsg;
}

FCITX_EXPORT_API
MSG_TYPE GetMessageType(Messages* m, int index)
{
    return m->msg[index].type;
}

FCITX_EXPORT_API
void SetMessageChanged(Messages* m, boolean changed)
{
    m->changed = changed;
}

FCITX_EXPORT_API
void LoadUserInterface(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    FcitxAddon *addon;

    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon))
    {
        if (addon->bEnabled && addon->category == AC_UI)
        {
            char *modulePath;

            switch (addon->type)
            {

            case AT_SHAREDLIBRARY:
                {
                    FILE *fp = GetLibFile(addon->library, "r", &modulePath);
                    void *handle;

                    if (!fp)
                        break;

                    fclose(fp);

                    handle = dlopen(modulePath, RTLD_LAZY | RTLD_GLOBAL);

                    if (!handle)
                    {
                        FcitxLog(ERROR, _("UI: open %s fail %s") , modulePath , dlerror());
                        break;
                    }

                    addon->ui = dlsym(handle, "ui");

                    if (!addon->ui || !addon->ui->Create)
                    {
                        FcitxLog(ERROR, _("UI: bad ui"));
                        dlclose(handle);
                        break;
                    }

                    if ((addon->addonInstance = addon->ui->Create(instance)) == NULL)
                    {
                        dlclose(handle);
                        return;
                    }

                    /* some may register before ui load, so load it here */
                    if (addon->ui->RegisterStatus)
                    {
                        UT_array* uistats = &instance->uistats;
                        FcitxUIStatus *status;

                        for (status = (FcitxUIStatus *) utarray_front(uistats);
                                status != NULL;
                                status = (FcitxUIStatus *) utarray_next(uistats, status))
                            addon->ui->RegisterStatus(addon->addonInstance, status);
                    }

                    if (addon->ui->RegisterMenu)
                    {
                        UT_array* uimenus = &instance->uimenus;
                        FcitxUIMenu **menupp;

                        for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
                                menupp != NULL;
                                menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp))
                            addon->ui->RegisterMenu(addon->addonInstance, *menupp);
                    }

                    instance->ui = addon;
                }

                break;

            default:
                break;
            }

            free(modulePath);

            if (instance->ui != NULL)
                break;
        }
    }

    if (instance->ui == NULL)
        FcitxLog(ERROR, "no usable user interface.");
}

FCITX_EXPORT_API
void AddMessageAtLast(Messages* message, MSG_TYPE type, const char *fmt, ...)
{

    if (message->msgCount < MAX_MESSAGE_COUNT)
    {
        va_list ap;
        va_start(ap, fmt);
        SetMessageV(message, message->msgCount, type, fmt, ap);
        va_end(ap);
        message->msgCount ++;
        message->changed = true;
    }
}

FCITX_EXPORT_API
void SetMessage(Messages* message, int position, MSG_TYPE type, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SetMessageV(message, position, type, fmt, ap);
    va_end(ap);
}

FCITX_EXPORT_API
void SetMessageText(Messages* message, int position, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SetMessageV(message, position, message->msg[position].type, fmt, ap);
    va_end(ap);
}

FCITX_EXPORT_API
void SetMessageV(Messages* message, int position, MSG_TYPE type, const char* fmt, va_list ap)
{
    if (position < MAX_MESSAGE_COUNT)
    {
        vsnprintf(message->msg[position].strMsg, MESSAGE_MAX_LENGTH, fmt, ap);
        message->msg[position].type = type;
        message->changed = true;
    }
}

FCITX_EXPORT_API
void MessageConcatLast(Messages* message, const char* text)
{
    strncat(message->msg[message->msgCount - 1].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

FCITX_EXPORT_API
void MessageConcat(Messages* message, int position, const char* text)
{
    strncat(message->msg[position].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

FCITX_EXPORT_API
void CloseInputWindow(FcitxInstance* instance)
{
    CleanInputWindow(instance);
    FcitxInputContext* ic = GetCurrentIC(instance);
    if (ic)
    {
        if (ic->contextCaps & CAPACITY_CLIENT_SIDE_UI)
            UpdateClientSideUI(instance, ic);

        if (ic->contextCaps & CAPACITY_PREEDIT)
            UpdatePreedit(instance, GetCurrentIC(instance));
    }

    if (UI_FUNC_IS_VALID(CloseInputWindow ))
        instance->ui->ui->CloseInputWindow(instance->ui->addonInstance);
}

FCITX_EXPORT_API
void UpdateInputWindow(FcitxInstance *instance)
{
    instance->uiflag |= UI_UPDATE;

    if (IsMessageChanged(instance->input.msgPreedit))
        UpdatePreedit(instance, GetCurrentIC(instance));
}

void ShowInputWindow(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(ShowInputWindow))
        instance->ui->ui->ShowInputWindow(instance->ui->addonInstance);
}

FCITX_EXPORT_API
void MoveInputWindow(FcitxInstance* instance)
{
    instance->uiflag |= UI_MOVE;
}

FCITX_EXPORT_API
FcitxUIStatus *GetUIStatus(FcitxInstance* instance, const char* name)
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
void UpdateStatus(FcitxInstance* instance, const char* name)
{
    FcitxLog(DEBUG, "Update Status for %s", name);

    FcitxUIStatus *status = GetUIStatus(instance, name);

    if (status != NULL)
    {
        status->toggleStatus(status->arg);

        if (UI_FUNC_IS_VALID(UpdateStatus))
            instance->ui->ui->UpdateStatus(instance->ui->addonInstance , status);
    }
}

FCITX_EXPORT_API
void RegisterStatus(struct _FcitxInstance* instance, void* arg, const char* name, const char* shortDesc, const char* longDesc, void (*toggleStatus)(void *arg), boolean(*getStatus)(void *arg))
{
    FcitxUIStatus status;

    if (strlen(name) > MAX_STATUS_NAME)
        return;

    memset(&status, 0 , sizeof(FcitxUIStatus));

    strncpy(status.name, name, MAX_STATUS_NAME);

    strncpy(status.shortDescription, shortDesc, MAX_STATUS_NAME);

    strncpy(status.longDescription, longDesc, MAX_STATUS_NAME);

    status.getCurrentStatus = getStatus;

    status.toggleStatus = toggleStatus;

    status.arg = arg;

    UT_array* uistats = &instance->uistats;

    utarray_push_back(uistats, &status);
}

FCITX_EXPORT_API
void RegisterMenu(FcitxInstance* instance, FcitxUIMenu* menu)
{
    UT_array* uimenus = &instance->uimenus;

    if (!menu)
        return ;

    menu->mark = -1;

    utarray_push_back(uimenus, &menu);
}

FCITX_EXPORT_API
void AddMenuShell(FcitxUIMenu* menu, char* string, MenuShellType type, FcitxUIMenu* subMenu)
{
    MenuShell shell;
    memset(&shell, 0, sizeof(MenuShell));

    if (string)
    {
        if (strlen(string) > MAX_MENU_STRING_LENGTH)
            return;
        else
            strncpy(shell.tipstr, string, MAX_MENU_STRING_LENGTH);
    }

    shell.type = type;

    shell.isselect = false;

    if (type == MENUTYPE_SUBMENU)
        shell.subMenu = subMenu;

    utarray_push_back(&menu->shell, &shell);
}

FCITX_EXPORT_API
void ClearMenuShell(FcitxUIMenu* menu)
{
    utarray_clear(&menu->shell);
}

FCITX_EXPORT_API
void OnInputFocus(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnInputFocus))
        instance->ui->ui->OnInputFocus(instance->ui->addonInstance);

    InputFocusHook(instance);

    ResetInput(instance);

    CloseInputWindow(instance);
}

FCITX_EXPORT_API
void OnInputUnFocus(struct _FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnInputUnFocus))
        instance->ui->ui->OnInputUnFocus(instance->ui->addonInstance);

    InputUnFocusHook(instance);
}

FCITX_EXPORT_API
void OnTriggerOn(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnTriggerOn))
        instance->ui->ui->OnTriggerOn(instance->ui->addonInstance);

    TriggerOnHook(instance);

    ShowInputSpeed(instance);
}

FCITX_EXPORT_API
void DisplayMessage(FcitxInstance *instance, char *title, char **msg, int length)
{
    if (UI_FUNC_IS_VALID(DisplayMessage))
        instance->ui->ui->DisplayMessage(instance->ui->addonInstance, title, msg, length);
}

FCITX_EXPORT_API
void OnTriggerOff(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(OnTriggerOff))
        instance->ui->ui->OnTriggerOff(instance->ui->addonInstance);

    TriggerOffHook(instance);
}

FCITX_EXPORT_API
void UpdateMenuShell(FcitxUIMenu* menu)
{
    if (menu && menu->UpdateMenuShell)
    {
        menu->UpdateMenuShell(menu);
    }
}

/*
 * 判断鼠标点击处是否处于指定的区域内
 */
FCITX_EXPORT_API
boolean
IsInBox(int x0, int y0, int x1, int y1, int w, int h)
{
    if (x0 >= x1 && x0 <= x1 + w && y0 >= y1 && y0 <= y1 + h)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean UISupportMainWindow(FcitxInstance* instance)
{
    if (UI_FUNC_IS_VALID(MainWindowSizeHint))
        return true;
    else
        return false;
}

FCITX_EXPORT_API
void GetMainWindowSize(FcitxInstance* instance, int* x, int* y, int* w, int* h)
{
    if (UI_FUNC_IS_VALID(MainWindowSizeHint))
        instance->ui->ui->MainWindowSizeHint(instance->ui->addonInstance, x, y, w, h);
}

FCITX_EXPORT_API
int NewMessageToOldStyleMessage(FcitxInstance* instance, Messages* msgUp, Messages* msgDown)
{
    int i = 0;
    FcitxInputState* input = &instance->input;
    int extraLength = input->iCursorPos;
    SetMessageCount(msgUp, 0);
    SetMessageCount(msgDown, 0);

    for (i = 0; i < GetMessageCount(input->msgAuxUp) ; i ++)
    {
        AddMessageAtLast(msgUp, GetMessageType(input->msgAuxUp, i), GetMessageString(input->msgAuxUp, i));
        extraLength += strlen(GetMessageString(input->msgAuxUp, i));
    }

    for (i = 0; i < GetMessageCount(input->msgPreedit) ; i ++)
        AddMessageAtLast(msgUp, GetMessageType(input->msgPreedit, i), GetMessageString(input->msgPreedit, i));

    for (i = 0; i < GetMessageCount(input->msgAuxDown) ; i ++)
        AddMessageAtLast(msgDown, GetMessageType(input->msgAuxDown, i), GetMessageString(input->msgAuxDown, i));

    CandidateWord* candWord = NULL;

    for (candWord = CandidateWordGetCurrentWindow(input->candList), i = 0;
            candWord != NULL;
            candWord = CandidateWordGetCurrentWindowNext(input->candList, candWord), i ++)
    {
        char strTemp[3] = { '\0', '\0', '\0' };
        strTemp[0] = CandidateWordGetChoose(input->candList)[i];

        if (ConfigGetPointAfterNumber(instance->config))
            strTemp[1] = '.';

        AddMessageAtLast(msgDown, MSG_INDEX, strTemp);

        MSG_TYPE type = MSG_OTHER;

        if (i == 0 && CandidateWordGetCurrentPage(input->candList) == 0)
            type = MSG_FIRSTCAND;

        AddMessageAtLast(msgDown, type, candWord->strWord);

        if (candWord->strExtra && strlen(candWord->strExtra) != 0)
            AddMessageAtLast(msgDown, MSG_CODE, candWord->strExtra);

        AddMessageAtLast(msgDown, MSG_OTHER, " ");
    }

    return extraLength;
}

FCITX_EXPORT_API
char* MessagesToCString(Messages* messages)
{
    int length = 0;
    int i = 0;

    for (i = 0; i < GetMessageCount(messages) ; i ++)
        length += strlen(GetMessageString(messages, i));

    char* str = fcitx_malloc0(sizeof(char) * (length + 1));

    for (i = 0; i < GetMessageCount(messages) ; i ++)
        strcat(str, GetMessageString(messages, i));

    return str;
}

FCITX_EXPORT_API
char* CandidateWordToCString(FcitxInstance* instance)
{
    size_t len = 0;
    int i;
    FcitxInputState* input = &instance->input;
    CandidateWord* candWord;
    for (candWord = CandidateWordGetCurrentWindow(input->candList), i = 0;
         candWord != NULL;
         candWord = CandidateWordGetCurrentWindowNext(input->candList, candWord), i ++)
    {
        char strTemp[3] = { '\0', '\0', '\0' };
        strTemp[0] = CandidateWordGetChoose(input->candList)[i];

        if (ConfigGetPointAfterNumber(instance->config))
            strTemp[1] = '.';

        len += strlen(strTemp);
        len += strlen(candWord->strWord);

        if (candWord->strExtra && strlen(candWord->strExtra) != 0)
            len += strlen(candWord->strExtra);

        len ++;
    }

    char *result = fcitx_malloc0(sizeof(char) * (len + 1));

    for (candWord = CandidateWordGetCurrentWindow(input->candList), i = 0;
         candWord != NULL;
         candWord = CandidateWordGetCurrentWindowNext(input->candList, candWord), i ++)
    {
        char strTemp[3] = { '\0', '\0', '\0' };
        strTemp[0] = CandidateWordGetChoose(input->candList)[i];

        if (ConfigGetPointAfterNumber(instance->config))
            strTemp[1] = '.';

        strcat(result, strTemp);
        strcat(result, candWord->strWord);

        if (candWord->strExtra && strlen(candWord->strExtra) != 0)
            strcat(result, candWord->strExtra);

        strcat(result, " ");
    }

    return result;
}

void UpdateInputWindowReal(FcitxInstance *instance)
{
    FcitxInputState* input = &instance->input;
    FcitxInputContext* ic = GetCurrentIC(instance);
    CapacityFlags flags = CAPACITY_NONE;
    if (ic != NULL)
        flags = ic->contextCaps;

    if (flags & CAPACITY_CLIENT_SIDE_UI)
    {
        UpdateClientSideUI(instance, ic);
        return;
    }

    boolean toshow = false;

    if (GetMessageCount(input->msgAuxUp) != 0
       || GetMessageCount(input->msgAuxDown) != 0)
        toshow = true;

    if (CandidateWordGetListSize(input->candList) > 1)
        toshow = true;

    if (CandidateWordGetListSize(input->candList) == 1
        && (!instance->config->bHideInputWindowWhenOnlyPreeditString
        || !instance->config->bHideInputWindowWhenOnlyOneCandidate))
        toshow = true;

    if (GetMessageCount(input->msgPreedit) != 0
        && !((flags & CAPACITY_PREEDIT ) && instance->config->bHideInputWindowWhenOnlyPreeditString && instance->profile->bUsePreedit))
        toshow = true;

    if (!toshow)
    {
        UpdatePreedit(instance, ic);
        if (UI_FUNC_IS_VALID(CloseInputWindow))
            instance->ui->ui->CloseInputWindow(instance->ui->addonInstance);
    }
    else
        ShowInputWindow(instance);
}

void MoveInputWindowReal(FcitxInstance *instance)
{
    if (UI_FUNC_IS_VALID(MoveInputWindow))
        instance->ui->ui->MoveInputWindow(instance->ui->addonInstance);
}

// kate: indent-mode cstyle; space-indent on; indent-width 4;
