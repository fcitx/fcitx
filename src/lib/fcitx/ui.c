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
#include "fcitx-utils/cutils.h"
#include <sys/stat.h>
#include "fcitx-utils/utils.h"
#include "instance.h"
#include "hook-internal.h"
#include "ime-internal.h"

FcitxUI dummyUI;

struct MESSAGE{
    char            strMsg[MESSAGE_MAX_LENGTH + 1];
    MSG_TYPE        type;
} ;

struct Messages {
    MESSAGE msg[MAX_MESSAGE_COUNT];
    uint msgCount;
    boolean changed;
};

Messages* InitMessages()
{
    return fcitx_malloc0(sizeof(Messages));
}

void SetMessageCount(Messages* m, int s)
{
    if ((s) <= MAX_MESSAGE_COUNT && s >= 0)
        ((m)->msgCount = (s));
    (m)->changed = true;
}

int GetMessageCount(Messages* m)
{
    return m->msgCount;
}

boolean IsMessageChanged(Messages* m)
{
    return m->changed;
}

char* GetMessageString(Messages* m, int index)
{
    return m->msg[index].strMsg;
}

MSG_TYPE GetMessageType(Messages* m, int index)
{
    return m->msg[index].type;
}

void SetMessageChanged(Messages* m, boolean changed)
{
    m->changed = changed;
}

void LoadUserInterface(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    FcitxAddon *addon;
    for ( addon = (FcitxAddon *) utarray_front(addons);
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
                        handle = dlopen(modulePath, RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("UI: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        addon->ui=dlsym(handle,"ui");
                        if(!addon->ui || !addon->ui->Create)
                        {
                            FcitxLog(ERROR, _("UI: bad ui"));
                            dlclose(handle);
                            break;
                        }
                        if((addon->addonInstance = addon->ui->Create(instance)) == NULL)
                        {
                            dlclose(handle);
                            return;
                        }
                        
                        /* some may register before ui load, so load it here */
                        if (addon->ui->RegisterStatus)
                        {
                            UT_array* uistats = &instance->uistats;
                            FcitxUIStatus *status;
                            for ( status = (FcitxUIStatus *) utarray_front(uistats);
                                status != NULL;
                                status = (FcitxUIStatus *) utarray_next(uistats, status))
                                addon->ui->RegisterStatus(addon->addonInstance, status);
                        }
                        
                        if (addon->ui->RegisterMenu)
                        {
                            UT_array* uimenus = &instance->uimenus;
                            FcitxUIMenu **menupp;
                            for ( menupp = (FcitxUIMenu **) utarray_front(uimenus);
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

void SetMessage(Messages* message, int position, MSG_TYPE type, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SetMessageV(message, position, type, fmt, ap);
    va_end(ap);
}

void SetMessageText(Messages* message, int position, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SetMessageV(message, position, message->msg[position].type, fmt, ap);
    va_end(ap);
}

void SetMessageV(Messages* message, int position, MSG_TYPE type, const char* fmt, va_list ap)
{
    if (position < MAX_MESSAGE_COUNT)
    {
        vsnprintf(message->msg[position].strMsg, MESSAGE_MAX_LENGTH, fmt, ap);
        message->msg[position].type = type;
        message->changed = true;
    }
}

void MessageConcatLast(Messages* message, const char* text)
{
    strncat(message->msg[message->msgCount - 1].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

void MessageConcat(Messages* message, int position, const char* text)
{
    strncat(message->msg[position].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

void CloseInputWindow(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->CloseInputWindow)
        instance->ui->ui->CloseInputWindow(instance->ui->addonInstance);
}

void ShowInputWindow(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->ShowInputWindow)
        instance->ui->ui->ShowInputWindow(instance->ui->addonInstance);
}

void MoveInputWindow(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->MoveInputWindow)
        instance->ui->ui->MoveInputWindow(instance->ui->addonInstance);
}

FcitxUIStatus *GetUIStatus(FcitxInstance* instance, const char* name)
{
    UT_array* uistats = &instance->uistats;
    FcitxUIStatus *status;
    for ( status = (FcitxUIStatus *) utarray_front(uistats);
          status != NULL;
          status = (FcitxUIStatus *) utarray_next(uistats, status))
         if (strcmp(status->name, name) == 0)
             break;
   return status;
}

void UpdateStatus(FcitxInstance* instance, const char* name)
{
    FcitxLog(DEBUG, "Update Status for %s", name);
    
    FcitxUIStatus *status = GetUIStatus(instance, name);
    
    if (status != NULL)
    {
        status->toggleStatus(status->arg);
        if (instance->ui && instance->ui->ui->UpdateStatus)
            instance->ui->ui->UpdateStatus(instance->ui->addonInstance ,status);
    }
}

void RegisterStatus(struct FcitxInstance* instance, void* arg, const char* name, const char* shortDesc, const char* longDesc, void (*toggleStatus)(void *arg), boolean (*getStatus)(void *arg))
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

void RegisterMenu(FcitxInstance* instance, FcitxUIMenu* menu)
{
    UT_array* uimenus = &instance->uimenus;
    if (!menu)
        return ;
    menu->mark = -1;
    utarray_push_back(uimenus, &menu);    
}

void AddMenuShell(FcitxUIMenu* menu, char* string, MenuShellType type, FcitxUIMenu* subMenu)
{
    MenuShell shell;
    memset(&shell, 0, sizeof(MenuShell));
    if (string) {
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

void ClearMenuShell(FcitxUIMenu* menu)
{
    utarray_clear(&menu->shell);
}

void OnInputFocus(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->OnInputFocus)
        instance->ui->ui->OnInputFocus(instance->ui->addonInstance);
    
    InputFocusHook(instance);
    ResetInput(instance);
    CloseInputWindow(instance);
}

void OnInputUnFocus(struct FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->OnInputUnFocus)
        instance->ui->ui->OnInputUnFocus(instance->ui->addonInstance);
    
    InputUnFocusHook(instance);
}

void OnTriggerOn(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->OnTriggerOn)
        instance->ui->ui->OnTriggerOn(instance->ui->addonInstance);
    
    TriggerOnHook(instance);
    ShowInputSpeed(instance);
}

void DisplayMessage(FcitxInstance *instance, char *title, char **msg, int length)
{
    if (instance->ui && instance->ui->ui->DisplayMessage)
        instance->ui->ui->DisplayMessage(instance->ui->addonInstance, title, msg, length);
}

void OnTriggerOff(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->OnTriggerOff)
        instance->ui->ui->OnTriggerOff(instance->ui->addonInstance);
    
    TriggerOffHook(instance);
}

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
boolean
IsInBox(int x0, int y0, int x1, int y1, int w, int h)
{
    if (x0 >= x1 && x0 <= x1 + w && y0 >= y1 && y0 <= y1 + h)
        return true;

    return false;
}

boolean UISupportMainWindow(FcitxInstance* instance)
{
    if (instance->ui && instance->ui->ui->MainWindowSizeHint)
        return true;
    else
        return false;
}

void GetMainWindowSize(FcitxInstance* instance, int* x, int* y, int* w, int* h)
{
    if (instance->ui && instance->ui->ui->MainWindowSizeHint)
        instance->ui->ui->MainWindowSizeHint(instance->ui->addonInstance, x, y, w, h);
}
