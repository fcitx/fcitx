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
#include "utils/utarray.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"
#include <sys/stat.h>
#include <utils/utils.h>

FcitxUI* ui = NULL;
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
    return malloc0(sizeof(Messages));
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

const char* GetMessageString(Messages* m, int index)
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

void LoadUserInterface()
{
    UT_array* addons = GetFcitxAddons();
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
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("UI: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        ui=dlsym(handle,"ui");
                        if(!ui || !ui->Init)
                        {
                            FcitxLog(ERROR, _("Backend: bad im"));
                            dlclose(handle);
                            break;
                        }
                        if(!ui->Init())
                        {
                            dlclose(handle);
                            return;
                        }
                    }
                    break;
                default:
                    break;
            }
            
            free(modulePath);
            if (ui != NULL)
                break;
        }
    }
    
    if (ui == NULL)
        ui = &dummyUI;
}

void AddMessageAtLast(Messages* message, MSG_TYPE type, char *fmt, ...)
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

void SetMessage(Messages* message, int position, MSG_TYPE type, char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    SetMessageV(message, position, type, fmt, ap);
    va_end(ap);
}

void SetMessageV(Messages* message, int position, MSG_TYPE type,char* fmt, va_list ap)
{
    if (position < MAX_MESSAGE_COUNT)
    {
        vsnprintf(message->msg[position].strMsg, MESSAGE_MAX_LENGTH, fmt, ap);
        message->msg[position].type = type;
        message->changed = true;
    }
}

void MessageConcatLast(Messages* message, char* text)
{
    strncat(message->msg[message->msgCount - 1].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

void MessageConcat(Messages* message, int position, char* text)
{
    strncat(message->msg[position].strMsg, text, MESSAGE_MAX_LENGTH);
    message->changed = true;
}

void CloseInputWindow()
{
    if (ui && ui->CloseInputWindow)
        ui->CloseInputWindow();
}

void ShowInputWindow()
{
    if (ui && ui->ShowInputWindow)
        ui->ShowInputWindow();
}

void MoveInputWindow()
{
    if (ui && ui->MoveInputWindow)
        ui->MoveInputWindow();
}

void UpdateStatus(const char* name)
{
    FcitxLog(DEBUG, "Update Status for %s", name);
    
    UT_array* uistats = GetUIStatus();
    FcitxUIStatus *status;
    for ( status = (FcitxUIStatus *) utarray_front(uistats);
          status != NULL;
          status = (FcitxUIStatus *) utarray_next(uistats, status))
         if (strcmp(status->name, name) == 0)
             break;
    
    if (status != NULL)
    {
        status->toggleStatus();
        if (ui && ui->UpdateStatus)
            ui->UpdateStatus(status);
    }
}

void RegisterStatus(const char* name, void (*toggleStatus)(), boolean (*getStatus)())
{
    FcitxUIStatus status;
    status.name = strdup(name);
    status.getCurrentStatus = getStatus;
    status.toggleStatus = toggleStatus;
    UT_array* uistats = GetUIStatus();
    
    utarray_push_back(uistats, &status);
    FcitxUIStatus* newstat = (FcitxUIStatus*) utarray_back(uistats);
    
    if (ui && ui->RegisterStatus)
        ui->RegisterStatus(newstat);
}

void OnInputFocus()
{
    if (ui && ui->OnInputFocus)
        ui->OnInputFocus();
}