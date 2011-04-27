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

#include "core/fcitx.h"
#include "core/module.h"
#include "core/hook.h"
#include "utils/utils.h"
#include "fcitx-config/xdg.h"

#include "AutoEng.h"
#include <core/keys.h>

struct {
    UT_array* autoEng;
    char buf[MAX_USER_INPUT + 1];
    int index;
    boolean active;
} autoEngState;

static const UT_icd autoeng_icd = { sizeof(AUTO_ENG), 0, 0, 0 };


/**
 * @brief Initialize for Auto English
 *
 * @return boolean
 **/
static boolean AutoEngInit();

/**
 * @brief Load AutoEng File
 *
 * @return void
 **/
static void            LoadAutoEng (void);

/**
 * @brief Cache the input key for autoeng, only simple key without combine key will be record
 *
 * @param sym keysym
 * @param state key state
 * @param retval Return state value
 * @return boolean
 **/
static boolean ProcessAutoEng(long unsigned int sym,
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
static void FreeAutoEng (void);

/**
 * @brief Check whether need to switch to English
 *
 * @param  str string
 * @return boolean
 **/
boolean            SwitchToEng (char *);


/**
 * @brief Update message for Auto eng
 *
 * @return void
 **/
void ShowAutoEngMessage();

FCITX_EXPORT_API
FcitxModule module = {
    AutoEngInit,
    NULL,
    FreeAutoEng
};

boolean AutoEngInit()
{
    LoadAutoEng();
    RegisterPreInputFilter(ProcessAutoEng);
    RegisterResetInputHook(ResetAutoEng);

    ResetAutoEng();
    return true;
}

static boolean ProcessAutoEng(long unsigned int sym,
                              unsigned int state,
                              INPUT_RETURN_VALUE *retval
                             )
{
    if (autoEngState.active)
    {
        if (IsHotKeySimple(sym,state))
        {
            autoEngState.buf[autoEngState.index] = sym;
            autoEngState.index++;
            autoEngState.buf[autoEngState.index] = '\0';
            *retval = IRV_DISPLAY_MESSAGE;
        }
        else if (IsHotKey(sym, state, FCITX_BACKSPACE))
        {
            autoEngState.index -- ;
            autoEngState.buf[autoEngState.index] = '\0';
            if (autoEngState.index == 0)
            {
                ResetAutoEng();
                *retval = IRV_CLEAN;
            }
            else
                *retval = IRV_DISPLAY_MESSAGE;
        }
        else if (IsHotKey(sym, state, FCITX_ENTER))
        {
            strcpy(GetOutputString(), autoEngState.buf);
            ResetAutoEng();
            *retval = IRV_GET_CANDWORDS;
        }
        ShowAutoEngMessage();
        return true;
    }
    if (IsHotKeySimple(sym, state))
    {
        autoEngState.buf[autoEngState.index] = sym;
        autoEngState.index++;
        autoEngState.buf[autoEngState.index] = '\0';

        if (SwitchToEng(autoEngState.buf))
        {
            *retval = IRV_DISPLAY_MESSAGE;
            autoEngState.active = true;
            ShowAutoEngMessage();
            return true;
        }
    }
    else if (IsHotKey(sym, state, FCITX_BACKSPACE))
    {
        if (autoEngState.index > 0)
        {
            autoEngState.index -- ;
            autoEngState.buf[autoEngState.index] = '\0';
        }
    }
    return false;
}

void ResetAutoEng()
{
    autoEngState.index = 0;
    autoEngState.buf[autoEngState.index] = '\0';
    autoEngState.active = false;
}

void LoadAutoEng (void)
{
    FILE    *fp;
    char    *buf;
    size_t   length = 0;

    fp = GetXDGFileData("AutoEng.dat", "rt", NULL);
    if (!fp)
        return;

    utarray_new(autoEngState.autoEng, &autoeng_icd);
    AUTO_ENG autoeng;

    while  (getline(&buf, &length, fp) != -1) {
        char* line = trim(buf);
        if (strlen(line) > MAX_AUTO_TO_ENG)
            FcitxLog(WARNING, _("Too long item for AutoEng"));
        strncpy(autoeng.str, line, MAX_AUTO_TO_ENG);
        free(line);
        autoeng.str[MAX_AUTO_TO_ENG] = '\0';
        utarray_push_back(autoEngState.autoEng, &autoeng);
    }

    free(buf);

    fclose (fp);
}

void FreeAutoEng (void)
{
    if (autoEngState.autoEng)
    {
        utarray_free(autoEngState.autoEng);
        autoEngState.autoEng = NULL;
    }
}

boolean SwitchToEng (char *str)
{
    AUTO_ENG*       autoeng;
    for (autoeng = (AUTO_ENG *) utarray_front(autoEngState.autoEng);
            autoeng != NULL;
            autoeng = (AUTO_ENG *) utarray_next(autoEngState.autoEng, autoeng))
        if (!strcmp (str, autoeng->str))
            return true;

    return false;
}

void ShowAutoEngMessage()
{
    Messages *msgUp = GetMessageUp();
    Messages *msgDown = GetMessageDown();

    SetMessageCount(msgUp, 0);
    SetMessageCount(msgDown, 0);
    
    if (autoEngState.buf[0] == '\0')
        return;
    
    AddMessageAtLast(msgUp, MSG_INPUT, autoEngState.buf);
    AddMessageAtLast(msgDown, MSG_OTHER, _("Press enter to input text"));
}