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
 * @file   ime.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  按键和输入法通用功能处理
 *
 *
 */
#include <dlfcn.h>
#include <libintl.h>
#include "ime.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/hotkey.h"
#include "utils/configfile.h"
#include "keys.h"
#include "utils/profile.h"
#include "ime-internal.h"
#include "ui.h"
#include "utils/utils.h"
#include "hook.h"
#include "backend.h"

static void UnloadIM(FcitxIM* ime);
static const char* GetStateName(INPUT_RETURN_VALUE retVal);
static void UpdateInputWindow();
static const UT_icd im_icd = {sizeof(FcitxIM*), NULL ,NULL, NULL};

FcitxInputState input;

void InitBuiltInHotkey()
{
    HotkeyHook hk;
    hk.hotkey = FCITX_CTRL_5;
    hk.hotkeyhandle = ImProcessReload;
    RegisterHotkeyFilter(hk);
    
    hk.hotkey = FCITX_ENTER;
    hk.hotkeyhandle = ImProcessEnter;
    RegisterHotkeyFilter(hk);
    
    hk.hotkey = FCITX_ESCAPE;
    hk.hotkeyhandle = ImProcessEscape;
    RegisterHotkeyFilter(hk);
}

UT_array* GetFcitxIMS()
{
    static UT_array* ims = NULL;
    if (ims == NULL)
        utarray_new(ims, &im_icd);
    return ims;
}

void SaveAllIM(void)
{
    UT_array* ims = GetFcitxIMS();
    FcitxIM **pim;
    for ( pim = (FcitxIM **) utarray_front(ims);
          pim != NULL;
          pim = (FcitxIM **) utarray_next(ims, pim))
    {
        FcitxIM *im = *pim;
        if (im->Save)
            im->Save();
    }
}

void UnloadAllIM()
{
    UT_array* ims = GetFcitxIMS();
    FcitxIM **pim;
    for ( pim = (FcitxIM **) utarray_front(ims);
          pim != NULL;
          pim = (FcitxIM **) utarray_next(ims, pim))
    {
        FcitxIM *im = *pim;
        UnloadIM(im);
    }
    utarray_free(ims);
}

static const char* GetStateName(INPUT_RETURN_VALUE retVal)
{
    switch (retVal)
    {
        case IRV_DO_NOTHING:
            return "IRV_DO_NOTHING";
        case IRV_DONOT_PROCESS:
            return "IRV_DONOT_PROCESS";
        case IRV_DONOT_PROCESS_CLEAN:
            return "IRV_DONOT_PROCESS_CLEAN";
        case IRV_CLEAN:
            return "IRV_CLEAN";
        case IRV_TO_PROCESS:
            return "IRV_TO_PROCESS";
        case IRV_DISPLAY_MESSAGE:
            return "IRV_DISPLAY_MESSAGE";
        case IRV_DISPLAY_CANDWORDS:
            return "IRV_DISPLAY_CANDWORDS";
        case IRV_DISPLAY_LAST:
            return "IRV_DISPLAY_LAST";
        case IRV_PUNC:
            return "IRV_PUNC";
        case IRV_ENG:
            return "IRV_ENG";
        case IRV_GET_LEGEND:
            return "IRV_GET_LEGEND";
        case IRV_GET_CANDWORDS:
            return "IRV_GET_CANDWORDS";
        case IRV_GET_CANDWORDS_NEXT:
            return "IRV_GET_CANDWORDS_NEXT";
    }
    return "unknown";
}

void UnloadIM(FcitxIM* im)
{
    //TODO: DestroyImage(&ime->icon);
    if (im->Destroy)
        im->Destroy();
}

void LoadAllIM(void)
{
    UT_array* ims = GetFcitxIMS();
    UT_array* addons = GetFcitxAddons();
    FcitxState* gs = GetFcitxGlobalState();
    FcitxAddon *addon;
    for ( addon = (FcitxAddon *) utarray_front(addons);
          addon != NULL;
          addon = (FcitxAddon *) utarray_next(addons, addon))
    {
        if (addon->bEnabled && addon->category == AC_INPUTMETHOD)
        {
            char *modulePath;
            switch (addon->type)
            {
                case AT_SHAREDLIBRARY:
                    {
                        FILE *fp = GetLibFile(addon->library, "r", &modulePath);
                        void *handle;
                        FcitxIM* im;
                        if (!fp)
                            break;
                        fclose(fp);
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("IM: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        im=dlsym(handle,"ime");
                        if(!im || !im->Init)
                        {
                            FcitxLog(ERROR, _("IM: bad im"));
                            dlclose(handle);
                            break;
                        }
                        if(!im->Init())
                        {
                            dlclose(handle);
                            return;
                        }
                        addon->im = im;
                        utarray_push_back(ims, &im);
                    }
                default:
                    break;
            }
            free(modulePath);
        }
    }
    if (gs->iIMIndex < 0)
        gs->iIMIndex = 0;
    if (gs->iIMIndex > utarray_len(ims))
        gs->iIMIndex = utarray_len(ims) - 1;
    if (utarray_len(ims) <= 0)
    {
        FcitxLog(ERROR, _("No available Input Method"));
        exit(1);
    }
}

boolean IsHotKey(FcitxKeySym sym, int state, HOTKEYS * hotkey)
{
    state &= KEY_CTRL_ALT_SHIFT_COMP;
    if (sym == hotkey[0].sym && (hotkey[0].state == state) )
        return True;
    if (sym == hotkey[1].sym && (hotkey[1].state == state) )
        return True;
    return False;
}

INPUT_RETURN_VALUE ProcessKey(FcitxKeyEventType event,
                              long unsigned int timestamp,
                              FcitxKeySym sym,
                              unsigned int state)
{
    FcitxState* gs = GetFcitxGlobalState();
    if (sym == 0) {
        return IRV_DONOT_PROCESS;
    }

    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    char *pstr;
    FcitxIM* currentIM = GetCurrentIM();
    
    FcitxConfig *fc = (FcitxConfig*) GetConfig();

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (event == FCITX_RELEASE_KEY ) {
        if (GetCurrentIC()->state != IS_CLOSED) {
            if ((timestamp - input.lastKeyPressedTime) < 500 && (!input.bIsDoInputOnly)) {
                if (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                    if (GetCurrentIC()->state == IS_ACTIVE)
                        SwitchIM(-1);
                    else if (IsHotKey(sym, state, fc->hkTrigger))
                        CloseIM(GetCurrentIC());
                    /* else if (bVK)
                        ChangVK(); */
                } else if (IsHotKey(sym, state, fc->switchKey) && input.keyReleased == KR_CTRL && !fc->bDoubleSwitchKey) {
                    retVal = IRV_DONOT_PROCESS;
                    if (fc->bSendTextWhenSwitchEng) {
                        if (input.iCodeInputCount != 0) {
                            strcpy(input.strStringGet, input.strCodeInput);
                            retVal = IRV_ENG;
                        }
                    }
                    input.keyReleased = KR_OTHER;
                    ChangeIMState(GetCurrentIC());
                } else if (IsHotKey(sym, state, fc->i2ndSelectKey) && input.keyReleased == KR_2ND_SELECTKEY) {
                    if (!input.bIsInLegend) {
                        pstr = currentIM->GetCandWord(1);
                        if (pstr) {
                            strcpy(input.strStringGet, pstr);
                            if (input.bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (input.iCandWordCount != 0)
                            retVal = IRV_DISPLAY_CANDWORDS;
                        else
                            retVal = IRV_TO_PROCESS;
                    } else {
                        strcpy(input.strStringGet, " ");
                        SetMessageCount(gs->messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }
                    input.keyReleased = KR_OTHER;
                } else if (IsHotKey(sym, state, fc->i3rdSelectKey) && input.keyReleased == KR_3RD_SELECTKEY) {
                    if (!input.bIsInLegend) {
                        pstr = currentIM->GetCandWord(2);
                        if (pstr) {
                            strcpy(input.strStringGet, pstr);
                            if (input.bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (input.iCandWordCount)
                            retVal = IRV_DISPLAY_CANDWORDS;
                    } else {
                        strcpy(input.strStringGet, "　");
                        SetMessageCount(gs->messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }

                    input.keyReleased = KR_OTHER;
                }
            }
        }
    }

    /* Added by hubert_star AT forum.ubuntu.com.cn */
    if (event == FCITX_RELEASE_KEY
        && IsHotKeySimple(sym, state)
        && retVal == IRV_TO_PROCESS)
        return IRV_DONOT_PROCESS;
 
    if (retVal == IRV_TO_PROCESS) {
        /* process key event for switch key */
        if (event == FCITX_PRESS_KEY) {
            if (!IsHotKey(sym, state, fc->switchKey))
                input.keyReleased = KR_OTHER;
            else {
                if ((input.keyReleased == KR_CTRL)
                    && (timestamp - input.lastKeyPressedTime < fc->iTimeInterval)
                    && fc->bDoubleSwitchKey) {
                    CommitString(GetCurrentIC(), input.strCodeInput);
                    ChangeIMState(GetCurrentIC());
                }
            }

            input.lastKeyPressedTime = timestamp;
            if (IsHotKey(sym, state, fc->switchKey)) {
                input.keyReleased = KR_CTRL;
                retVal = IRV_DO_NOTHING;
            } else if (IsHotKey(sym, state, fc->hkTrigger)) {
                /* trigger key has the highest priority, so we check it first */
                if (GetCurrentIC()->state == IS_ENG) {
                    ChangeIMState(GetCurrentIC());
                } else
                    CloseIM(GetCurrentIC());

                retVal = IRV_DO_NOTHING;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS && event != FCITX_PRESS_KEY)
        retVal = IRV_DONOT_PROCESS;

    if (GetCurrentIC()->state == IS_ACTIVE) {
        if (!input.bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
            ProcessPreInputFilter(sym, state, &retVal);
        }
        
        if (retVal == IRV_TO_PROCESS)
        {
            if (IsHotKey(sym, state, fc->i2ndSelectKey)) {
                if (input.iCandWordCount >= 2)
                {
                    input.keyReleased = KR_2ND_SELECTKEY;
                    return IRV_DONOT_PROCESS;
                }
            } else if (IsHotKey(sym, state, fc->i3rdSelectKey)) {
                if (input.iCandWordCount >= 3)
                {
                    input.keyReleased = KR_3RD_SELECTKEY;
                    return IRV_DONOT_PROCESS;
                }
            }

            if (!IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                //调用输入法模块
                /* TODO: use module
                if (fcitxProfile.bCorner && (IsHotKeySimple(sym, state))) {
                    //有人报 空格 的全角不对，正确的是0xa1 0xa1
                    //但查资料却说全角符号总是以0xa3开始。
                    //由于0xa3 0xa0可能会显示乱码，因此采用0xa1 0xa1的方式
                    sprintf(strStringGet, "%s", sCornerTrans[sym - 32]);
                    retVal = IRV_GET_CANDWORDS;
                } else */

                retVal = currentIM->DoInput(sym, state);
                if (!input.bCursorAuto)
                    input.iCursorPos = input.iCodeInputCount;
            }
        }
        
        if (!input.bIsDoInputOnly && retVal == IRV_TO_PROCESS)
            if (!input.iInCap) {
                if (IsHotKey(sym, state, fc->hkPrevPage))
                    retVal = currentIM->GetCandWords(SM_PREV);
                else if (IsHotKey(sym, state, fc->hkNextPage))
                    retVal = currentIM->GetCandWords(SM_NEXT);
            }


        if (!input.bIsDoInputOnly && retVal == IRV_TO_PROCESS)
            ProcessPostInputFilter(sym, state, &retVal);
    }
    
    if (retVal == IRV_TO_PROCESS) {
        retVal = CheckHotkey(sym, state);
    }
    
    FcitxLog(DEBUG, "ProcessKey Return State: %s", GetStateName(retVal));
    
    switch (retVal)
    {
        case IRV_DO_NOTHING:
            break;
        case IRV_TO_PROCESS:
        case IRV_DONOT_PROCESS:
        case IRV_DONOT_PROCESS_CLEAN:
            ForwardKey(GetCurrentIC(), event, sym, state);
            
            if (retVal != IRV_DONOT_PROCESS_CLEAN)
                break;
        case IRV_CLEAN:
            ResetInput();
            CloseInputWindow();
            break;
            
        case IRV_DISPLAY_CANDWORDS:
            input.bShowPrev = input.bShowNext = false;
            if (input.bIsInLegend) {
                if (input.iCurrentLegendCandPage > 0)
                    input.bShowPrev = true;
                if (input.iCurrentLegendCandPage < input.iLegendCandPageCount)
                    input.bShowNext = true;
            } else {
                if (input.iCurrentCandPage > 0)
                    input.bShowPrev = true;
                if (input.iCurrentCandPage < input.iCandPageCount)
                    input.bShowNext = true;
            }

            ShowInputWindow();
            break;
            
        case IRV_DISPLAY_LAST:
            input.bShowNext = input.bShowPrev = False;
            SetMessageCount(gs->messageUp, 0);
            AddMessageAtLast(gs->messageUp, MSG_INPUT, "%c", input.strCodeInput[0]);
            SetMessageCount(gs->messageDown, 0);
            AddMessageAtLast(gs->messageDown, MSG_TIPS, "%s", input.strStringGet);
            UpdateInputWindow();
            
            break;
        case IRV_DISPLAY_MESSAGE:
            input.bShowNext = False;
            input.bShowPrev = False;
            UpdateInputWindow();
            break;
        case IRV_GET_LEGEND:
            CommitString(GetCurrentIC(), input.strStringGet);
            input.iHZInputed += (int) (utf8_strlen(input.strStringGet));        //粗略统计字数
            if (input.iLegendCandWordCount) {
                input.bShowNext = input.bShowPrev = false;
                if (input.iCurrentLegendCandPage > 0)
                    input.bShowPrev = true;
                if (input.iCurrentLegendCandPage < input.iLegendCandPageCount)
                    input.bShowNext = true;
                input.iCodeInputCount = 0;
                ShowInputWindow();
            } else {
                ResetInput();
                CloseInputWindow();
            }

            break;
        case IRV_GET_CANDWORDS:
            CommitString(GetCurrentIC(), input.strStringGet);
            if (fc->bPhraseTips && currentIM->PhraseTips)
                DoPhraseTips();
            input.iHZInputed += (int) (utf8_strlen(input.strStringGet));
            UpdateInputWindow();

            ResetInput();
            input.lastIsSingleHZ = 0;
            break;
        case IRV_ENG:
        case IRV_PUNC:
            input.iHZInputed += (int) (utf8_strlen(input.strStringGet));        //粗略统计字数
            ResetInput();
            UpdateInputWindow();
        case IRV_GET_CANDWORDS_NEXT:
            CommitString(GetCurrentIC(), input.strStringGet);
            input.bLastIsNumber = false;
            input.lastIsSingleHZ = 0;

            if (retVal == IRV_GET_CANDWORDS_NEXT || input.lastIsSingleHZ == -1) {
                input.iHZInputed += (int) (utf8_strlen(input.strStringGet));    //粗略统计字数
                ShowInputWindow();
            }

            break;
        default:
            ;
    }
    
    return retVal;
}

void ForwardKey(FcitxInputContext *ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    UT_array* backends = GetFcitxBackends();
    FcitxBackend** pbackend = (FcitxBackend**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = *pbackend;
    backend->ForwardKey(ic, event, sym, state);
}

void SwitchIM(int index)
{
    FcitxState* gs = GetFcitxGlobalState();
    UT_array* ims = GetFcitxIMS();
    int iIMCount = utarray_len(ims);
    
    FcitxIM* lastIM, *newIM;

    if (gs->iIMIndex >= iIMCount || gs->iIMIndex < 0)
        lastIM = NULL;
    else
    {
        FcitxIM** plastIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
        lastIM = *plastIM;
    }

    if (index == -1) {
        if (gs->iIMIndex >= (iIMCount - 1))
            gs->iIMIndex = 0;
        else
            gs->iIMIndex++;
    } 
    
    if (index >= iIMCount)
        gs->iIMIndex = iIMCount - 1;
    else if (index < 0)
        gs->iIMIndex = 0;
    else
        gs->iIMIndex = index;

    if (gs->iIMIndex >= iIMCount || gs->iIMIndex < 0)
        newIM = NULL;
    else
    {
        FcitxIM** pnewIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
        newIM = *pnewIM;
    }

    if (lastIM && lastIM->Save)
        lastIM->Save();
    if (newIM && newIM->Init)
        newIM->Init();

    ResetInput();
    SaveProfile();
}

/** 
 * @brief 重置输入状态
 */
void ResetInput(void)
{
    FcitxState* gs = GetFcitxGlobalState();
    input.iCandPageCount = 0;
    input.iCurrentCandPage = 0;
    input.iCandWordCount = 0;
    input.iLegendCandWordCount = 0;
    input.iCurrentLegendCandPage = 0;
    input.iLegendCandPageCount = 0;
    input.iCursorPos = 0;

    input.strCodeInput[0] = '\0';
    input.iCodeInputCount = 0;  

    input.bIsDoInputOnly = false;

    input.bShowPrev = false;
    input.bShowNext = false;

    input.bIsInLegend = false;
    
    input.iInCap = 0;

    UT_array* ims = GetFcitxIMS();
    FcitxIM** pcurentIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
    FcitxIM* curentIM = *pcurentIM;

    if (curentIM && curentIM->ResetIM)
        curentIM->ResetIM();
    
    CloseInputWindow();
}

void DoPhraseTips(void)
{
    FcitxState* gs = GetFcitxGlobalState();
    UT_array* ims = GetFcitxIMS();
    FcitxIM** pcurrentIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
    FcitxIM* currentIM = *pcurrentIM;

    if (currentIM->PhraseTips && currentIM->PhraseTips())
        input.lastIsSingleHZ = -1;
    else
        input.lastIsSingleHZ = 0;
}

INPUT_RETURN_VALUE ImProcessEnter()
{
    INPUT_RETURN_VALUE retVal;    
    FcitxState* gs = GetFcitxGlobalState();
    FcitxConfig *fc = (FcitxConfig*) GetConfig();
    
    if (input.iInCap) {
        if (!input.iCodeInputCount)
            strcpy(input.strStringGet, ";");
        else
            strcpy(input.strStringGet, input.strCodeInput);
        retVal = IRV_PUNC;
        SetMessageCount(gs->messageDown, 0);
        SetMessageCount(gs->messageUp, 0);
        input.iInCap = 0;
    } else if (!input.iCodeInputCount)
        retVal = IRV_DONOT_PROCESS;
    else {
        switch (fc->enterToDo) {
        case K_ENTER_NOTHING:
            retVal = IRV_DO_NOTHING;
            break;
        case K_ENTER_CLEAN:
            retVal = IRV_CLEAN;
            break;
        case K_ENTER_SEND:
            SetMessageCount(gs->messageDown, 0);
            SetMessageCount(gs->messageUp, 0);
            strcpy(input.strStringGet, input.strCodeInput);
            retVal = IRV_ENG;
            break;
        }
    }
    return retVal;
}

INPUT_RETURN_VALUE ImProcessEscape()
{
    if (input.iCodeInputCount || input.iInCap || input.bIsInLegend)
        return IRV_CLEAN;
    else
        return IRV_DONOT_PROCESS;
}

INPUT_RETURN_VALUE ImProcessReload()
{
    ReloadConfig();
    return IRV_DO_NOTHING;
}

void ReloadConfig()
{
}

void UpdateInputWindow()
{
    FcitxState* gs = GetFcitxGlobalState();
    if (GetMessageCount(gs->messageDown) == 0)
        CloseInputWindow();
    else
        ShowInputWindow();
}

boolean IsInLegend()
{
    return input.bIsInLegend;
}

char* GetOutputString()
{
    return input.strStringGet;
}

FcitxIM* GetCurrentIM()
{
    UT_array* ims = GetFcitxIMS();
    FcitxState* gs = GetFcitxGlobalState();
    FcitxIM** pcurrentIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
    FcitxIM* currentIM = *pcurrentIM;
    return currentIM;
}