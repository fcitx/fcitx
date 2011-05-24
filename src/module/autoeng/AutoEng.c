/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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

/**
 * @file AutoEng.c
 * @brief Auto Switch to English
 */
#include <limits.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"
#include "fcitx-utils/utils.h"
#include "fcitx-config/xdg.h"

#include "AutoEng.h"
#include <fcitx-utils/keys.h>
#include <fcitx/ui.h>
#include <fcitx/instance.h>

typedef struct FcitxAutoEngState {
    UT_array* autoEng;
    char buf[MAX_USER_INPUT + 1];
    int index;
    boolean active;
} FcitxAutoEngState;

static const UT_icd autoeng_icd = { sizeof(AUTO_ENG), 0, 0, 0 };


/**
 * @brief Initialize for Auto English
 *
 * @return boolean
 **/
static void* AutoEngCreate(FcitxInstance *instance);

/**
 * @brief Load AutoEng File
 *
 * @return void
 **/
static void            LoadAutoEng (FcitxAutoEngState* autoEngState);

/**
 * @brief Cache the input key for autoeng, only simple key without combine key will be record
 *
 * @param sym keysym
 * @param state key state
 * @param retval Return state value
 * @return boolean
 **/
static boolean ProcessAutoEng(void* arg,
                              long unsigned int sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             );

/**
 * @brief clean the cache while reset input
 *
 * @return void
 **/
static void ResetAutoEng();

/**
 * @brief Free Auto Eng Data
 *
 * @return void
 **/
static void FreeAutoEng (void* arg);

/**
 * @brief Check whether need to switch to English
 *
 * @param  str string
 * @return boolean
 **/
boolean            SwitchToEng (FcitxAutoEngState* autoEngState, char* str);


/**
 * @brief Update message for Auto eng
 *
 * @return void
 **/
void ShowAutoEngMessage(FcitxAutoEngState* autoEngState);

FCITX_EXPORT_API
FcitxModule module = {
    AutoEngCreate,
    NULL,
    FreeAutoEng
};

void* AutoEngCreate(FcitxInstance *instance)
{
    FcitxAutoEngState* autoEngState = fcitx_malloc0(sizeof(FcitxAutoEngState));
    LoadAutoEng(autoEngState);
    
    KeyFilterHook khk;
    khk.arg = autoEngState;
    khk.func = ProcessAutoEng;
    
    FcitxResetInputHook rhk;
    rhk.arg = autoEngState;
    rhk.func = ResetAutoEng;
    
    RegisterPreInputFilter(khk);
    RegisterResetInputHook(rhk);

    ResetAutoEng();
    return autoEngState;
}

static boolean ProcessAutoEng(void* arg,long unsigned int sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             )
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    if (autoEngState->active)
    {
        if (IsHotKeySimple(sym,state))
        {
            autoEngState->buf[autoEngState->index] = sym;
            autoEngState->index++;
            autoEngState->buf[autoEngState->index] = '\0';
            *retval = IRV_DISPLAY_MESSAGE;
        }
        else if (IsHotKey(sym, state, FCITX_BACKSPACE))
        {
            autoEngState->index -- ;
            autoEngState->buf[autoEngState->index] = '\0';
            if (autoEngState->index == 0)
            {
                ResetAutoEng();
                *retval = IRV_CLEAN;
            }
            else
                *retval = IRV_DISPLAY_MESSAGE;
        }
        else if (IsHotKey(sym, state, FCITX_ENTER))
        {
            strcpy(GetOutputString(), autoEngState->buf);
            ResetAutoEng();
            *retval = IRV_GET_CANDWORDS;
        }
        ShowAutoEngMessage(autoEngState);
        return true;
    }
    if (IsHotKeySimple(sym, state))
    {
        autoEngState->buf[autoEngState->index] = sym;
        autoEngState->index++;
        autoEngState->buf[autoEngState->index] = '\0';

        if (SwitchToEng(autoEngState, autoEngState->buf))
        {
            *retval = IRV_DISPLAY_MESSAGE;
            autoEngState->active = true;
            ShowAutoEngMessage(autoEngState);
            return true;
        }
    }
    else if (IsHotKey(sym, state, FCITX_BACKSPACE))
    {
        if (autoEngState->index > 0)
        {
            autoEngState->index -- ;
            autoEngState->buf[autoEngState->index] = '\0';
        }
    }
    return false;
}

void ResetAutoEng(void* arg)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    autoEngState->index = 0;
    autoEngState->buf[autoEngState->index] = '\0';
    autoEngState->active = false;
}

void LoadAutoEng (FcitxAutoEngState* autoEngState)
{
    FILE    *fp;
    char    *buf;
    size_t   length = 0;

    fp = GetXDGFileData("AutoEng.dat", "rt", NULL);
    if (!fp)
        return;

    utarray_new(autoEngState->autoEng, &autoeng_icd);
    AUTO_ENG autoeng;

    while  (getline(&buf, &length, fp) != -1) {
        char* line = trim(buf);
        if (strlen(line) > MAX_AUTO_TO_ENG)
            FcitxLog(WARNING, _("Too long item for AutoEng"));
        strncpy(autoeng.str, line, MAX_AUTO_TO_ENG);
        free(line);
        autoeng.str[MAX_AUTO_TO_ENG] = '\0';
        utarray_push_back(autoEngState->autoEng, &autoeng);
    }

    free(buf);

    fclose (fp);
}

void FreeAutoEng (void* arg)
{
    FcitxAutoEngState* autoEngState = (FcitxAutoEngState*) arg;
    if (autoEngState->autoEng)
    {
        utarray_free(autoEngState->autoEng);
        autoEngState->autoEng = NULL;
    }
}

boolean SwitchToEng (FcitxAutoEngState* autoEngState, char *str)
{
    AUTO_ENG*       autoeng;
    for (autoeng = (AUTO_ENG *) utarray_front(autoEngState->autoEng);
            autoeng != NULL;
            autoeng = (AUTO_ENG *) utarray_next(autoEngState->autoEng, autoeng))
        if (!strcmp (str, autoeng->str))
            return true;

    return false;
}

void ShowAutoEngMessage(FcitxAutoEngState* autoEngState)
{
    Messages *msgUp = GetMessageUp();
    Messages *msgDown = GetMessageDown();

    SetMessageCount(msgUp, 0);
    SetMessageCount(msgDown, 0);
    
    if (autoEngState->buf[0] == '\0')
        return;
    
    AddMessageAtLast(msgUp, MSG_INPUT, autoEngState->buf);
    AddMessageAtLast(msgDown, MSG_OTHER, _("Press enter to input text"));
}