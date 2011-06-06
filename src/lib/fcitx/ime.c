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
 * @brief  Process Keyboard Event and Input Method
 *
 */
#include <dlfcn.h>
#include <libintl.h>
#include "ime.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/cutils.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/configfile.h"
#include "fcitx-utils/keys.h"
#include "fcitx/profile.h"
#include "ime-internal.h"
#include "ui.h"
#include "fcitx-utils/utils.h"
#include "hook.h"
#include "backend.h"
#include "hook-internal.h"
#include "instance.h"

static void UnloadIM(FcitxAddon* pim);
static const char* GetStateName(INPUT_RETURN_VALUE retVal);
static void UpdateInputWindow(FcitxInstance* instance);
static const UT_icd ime_icd = {sizeof(FcitxIM), NULL ,NULL, NULL};
static const UT_icd imclass_icd = {sizeof(FcitxIMClass*), NULL ,NULL, NULL};
static int IMPriorityCmp(const void *a, const void *b);

int IMPriorityCmp(const void *a, const void *b)
{
    FcitxIM *ta, *tb;
    ta = (FcitxIM*)a;
    tb = (FcitxIM*)b;
    return ta->iPriority - tb->iPriority;
}

void InitBuiltInHotkey(FcitxInstance *instance)
{
    HotkeyHook hk;
    hk.hotkey = FCITX_CTRL_5;
    hk.hotkeyhandle = ImProcessReload;
    hk.arg = instance;
    RegisterHotkeyFilter(hk);
    
    hk.hotkey = FCITX_ENTER;
    hk.hotkeyhandle = ImProcessEnter;
    hk.arg = instance;
    RegisterHotkeyFilter(hk);
    
    hk.hotkey = FCITX_ESCAPE;
    hk.hotkeyhandle = ImProcessEscape;
    hk.arg = instance;
    RegisterHotkeyFilter(hk);
}

void InitFcitxIM(FcitxInstance* instance)
{
    utarray_init(&instance->imes, &ime_icd);
    utarray_init(&instance->imeclasses, &imclass_icd);
}

void SaveAllIM(FcitxInstance* instance)
{
    UT_array* imes = &instance->imes;
    FcitxIM *pim;
    for ( pim = (FcitxIM *) utarray_front(imes);
          pim != NULL;
          pim = (FcitxIM *) utarray_next(imes, pim))
    {
        if ((pim)->Save)
            (pim)->Save(pim->klass);
    }
}

void UnloadAllIM(UT_array* ims)
{
    FcitxAddon **pim;
    for ( pim = (FcitxAddon **) utarray_front(ims);
          pim != NULL;
          pim = (FcitxAddon **) utarray_next(ims, pim))
    {
        FcitxAddon *im = *pim;
        UnloadIM(im);
    }
    utarray_clear(ims);
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

void UnloadIM(FcitxAddon* pim)
{
    //TODO: DestroyImage(&ime->icon);
    FcitxIMClass *im = pim->imclass;
    if (im->Destroy)
        im->Destroy(pim->addonInstance);
}

void FcitxRegsiterIM(FcitxInstance *instance,
                     void *addonInstance,
                     const char* name,
                     const char* iconName,
                     FcitxIMInit Init,
                     FcitxIMResetIM ResetIM, 
                     FcitxIMDoInput DoInput, 
                     FcitxIMGetCandWords GetCandWords, 
                     FcitxIMGetCandWord GetCandWord, 
                     FcitxIMGetLegendCandWord GetLegendCandWord, 
                     FcitxIMPhraseTips PhraseTips, 
                     FcitxIMSave Save,
                     void *priv,
                     int priority
)
{
    if (priority <= 0)
        return ;
    UT_array* imes = &instance->imes ;
    FcitxIM newime;
    strncpy(newime.strName, name, MAX_IM_NAME);
    strncpy(newime.strIconName, iconName, MAX_IM_NAME);
    newime.Init = Init;
    newime.ResetIM = ResetIM;
    newime.DoInput = DoInput;
    newime.GetCandWord =GetCandWord;
    newime.GetCandWords = GetCandWords;
    newime.GetLegendCandWord = GetLegendCandWord;
    newime.PhraseTips = PhraseTips;
    newime.Save = Save;
    newime.klass = addonInstance;
    newime.iPriority = priority;
    newime.priv = priv;
    
    utarray_push_back(imes, &newime);
}

void LoadAllIM(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    UT_array* ims = &instance->imeclasses;
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
                        FcitxIMClass * imclass;
                        if (!fp)
                            break;
                        fclose(fp);
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("IM: open %s fail %s") , modulePath ,dlerror());
                            break;
                        }
                        imclass=dlsym(handle,"ime");
                        if(!imclass || !imclass->Create)
                        {
                            FcitxLog(ERROR, _("IM: bad im %s"), addon->name);
                            dlclose(handle);
                            break;
                        }
                        if((addon->addonInstance = imclass->Create(instance)) == NULL)
                        {
                            dlclose(handle);
                            return;
                        }
                        addon->imclass = imclass;
                        utarray_push_back(ims, &addon);
                    }
                default:
                    break;
            }
            free(modulePath);
        }
    }
    if (instance->iIMIndex < 0)
        instance->iIMIndex = 0;
    if (instance->iIMIndex > utarray_len(&instance->imes))
        instance->iIMIndex = utarray_len(&instance->imes) - 1;
    if (utarray_len(&instance->imes) <= 0)
    {
        FcitxLog(ERROR, _("No available Input Method"));
        exit(1);
    }
    utarray_sort(&instance->imes, IMPriorityCmp);
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

INPUT_RETURN_VALUE ProcessKey(
    FcitxInstance* instance,
    FcitxKeyEventType event,
                              long unsigned int timestamp,
                              FcitxKeySym sym,
                              unsigned int state)
{
    if (sym == 0) {
        return IRV_DONOT_PROCESS;
    }

    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    char *pstr;
    FcitxIM* currentIM = GetCurrentIM(instance);
    FcitxInputState *input = &instance->input;
    
    FcitxConfig *fc = &instance->config;

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (event == FCITX_RELEASE_KEY ) {
        if (GetCurrentIC(instance)->state != IS_CLOSED) {
            if ((timestamp - input->lastKeyPressedTime) < 500 && (!input->bIsDoInputOnly)) {
                if (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                    if (GetCurrentIC(instance)->state == IS_ACTIVE)
                        SwitchIM(instance, -1);
                    else if (IsHotKey(sym, state, fc->hkTrigger))
                        CloseIM(instance, GetCurrentIC(instance));
                    /* else if (bVK)
                        ChangVK(); */
                } else if (IsHotKey(sym, state, fc->switchKey) && input->keyReleased == KR_CTRL && !fc->bDoubleSwitchKey) {
                    retVal = IRV_DONOT_PROCESS;
                    if (fc->bSendTextWhenSwitchEng) {
                        if (input->iCodeInputCount != 0) {
                            strcpy(input->strStringGet, input->strCodeInput);
                            retVal = IRV_ENG;
                        }
                    }
                    input->keyReleased = KR_OTHER;
                    ChangeIMState(instance, GetCurrentIC(instance));
                } else if (IsHotKey(sym, state, fc->i2ndSelectKey) && input->keyReleased == KR_2ND_SELECTKEY) {
                    if (!input->bIsInLegend) {
                        pstr = currentIM->GetCandWord(currentIM->klass, 1);
                        if (pstr) {
                            strcpy(input->strStringGet, pstr);
                            if (input->bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (input->iCandWordCount != 0)
                            retVal = IRV_DISPLAY_CANDWORDS;
                        else
                            retVal = IRV_TO_PROCESS;
                    } else {
                        strcpy(input->strStringGet, " ");
                        SetMessageCount(instance->messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }
                    input->keyReleased = KR_OTHER;
                } else if (IsHotKey(sym, state, fc->i3rdSelectKey) && input->keyReleased == KR_3RD_SELECTKEY) {
                    if (!input->bIsInLegend) {
                        pstr = currentIM->GetCandWord(currentIM->klass, 2);
                        if (pstr) {
                            strcpy(input->strStringGet, pstr);
                            if (input->bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (input->iCandWordCount)
                            retVal = IRV_DISPLAY_CANDWORDS;
                    } else {
                        strcpy(input->strStringGet, "　");
                        SetMessageCount(instance->messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }

                    input->keyReleased = KR_OTHER;
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
                input->keyReleased = KR_OTHER;
            else {
                if ((input->keyReleased == KR_CTRL)
                    && (timestamp - input->lastKeyPressedTime < fc->iTimeInterval)
                    && fc->bDoubleSwitchKey) {
                    CommitString(instance, GetCurrentIC(instance), input->strCodeInput);
                    ChangeIMState(instance, GetCurrentIC(instance));
                }
            }

            input->lastKeyPressedTime = timestamp;
            if (IsHotKey(sym, state, fc->switchKey)) {
                input->keyReleased = KR_CTRL;
                retVal = IRV_DO_NOTHING;
            } else if (IsHotKey(sym, state, fc->hkTrigger)) {
                /* trigger key has the highest priority, so we check it first */
                if (GetCurrentIC(instance)->state == IS_ENG) {
                    ChangeIMState(instance, GetCurrentIC(instance));
                } else
                    CloseIM(instance, GetCurrentIC(instance));

                retVal = IRV_DO_NOTHING;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS && event != FCITX_PRESS_KEY)
        retVal = IRV_DONOT_PROCESS;

    if (GetCurrentIC(instance)->state == IS_ACTIVE) {
        if (!input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
            ProcessPreInputFilter(sym, state, &retVal);
        }
        
        if (retVal == IRV_TO_PROCESS)
        {
            if (IsHotKey(sym, state, fc->i2ndSelectKey)) {
                if (input->iCandWordCount >= 2)
                {
                    input->keyReleased = KR_2ND_SELECTKEY;
                    return IRV_DONOT_PROCESS;
                }
            } else if (IsHotKey(sym, state, fc->i3rdSelectKey)) {
                if (input->iCandWordCount >= 3)
                {
                    input->keyReleased = KR_3RD_SELECTKEY;
                    return IRV_DONOT_PROCESS;
                }
            }

            if (!IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                retVal = currentIM->DoInput(currentIM->klass, sym, state);
                if (!input->bCursorAuto)
                    input->iCursorPos = input->iCodeInputCount;
            }
        }
        
        if (!input->bIsDoInputOnly && retVal == IRV_TO_PROCESS)
        {
            if (IsHotKey(sym, state, fc->hkPrevPage))
                retVal = currentIM->GetCandWords(currentIM->klass, SM_PREV);
            else if (IsHotKey(sym, state, fc->hkNextPage))
                retVal = currentIM->GetCandWords(currentIM->klass, SM_NEXT);
        }


        if (!input->bIsDoInputOnly && retVal == IRV_TO_PROCESS)
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
            ForwardKey(instance, GetCurrentIC(instance), event, sym, state);
            
            if (retVal != IRV_DONOT_PROCESS_CLEAN)
                break;
        case IRV_CLEAN:
            ResetInput(instance);
            CloseInputWindow(instance);
            break;
            
        case IRV_DISPLAY_CANDWORDS:
            input->bShowPrev = input->bShowNext = false;
            if (input->bIsInLegend) {
                if (input->iCurrentLegendCandPage > 0)
                    input->bShowPrev = true;
                if (input->iCurrentLegendCandPage < input->iLegendCandPageCount)
                    input->bShowNext = true;
            } else {
                if (input->iCurrentCandPage > 0)
                    input->bShowPrev = true;
                if (input->iCurrentCandPage < input->iCandPageCount)
                    input->bShowNext = true;
            }

            ShowInputWindow(instance);
            break;
            
        case IRV_DISPLAY_LAST:
            input->bShowNext = input->bShowPrev = False;
            SetMessageCount(instance->messageUp, 0);
            AddMessageAtLast(instance->messageUp, MSG_INPUT, "%c", input->strCodeInput[0]);
            SetMessageCount(instance->messageDown, 0);
            AddMessageAtLast(instance->messageDown, MSG_TIPS, "%s", input->strStringGet);
            UpdateInputWindow(instance);
            
            break;
        case IRV_DISPLAY_MESSAGE:
            input->bShowNext = False;
            input->bShowPrev = False;
            UpdateInputWindow(instance);
            break;
        case IRV_GET_LEGEND:
            CommitString(instance, GetCurrentIC(instance), input->strStringGet);
            input->iHZInputed += (int) (utf8_strlen(input->strStringGet));        //粗略统计字数
            if (input->iLegendCandWordCount) {
                input->bShowNext = input->bShowPrev = false;
                if (input->iCurrentLegendCandPage > 0)
                    input->bShowPrev = true;
                if (input->iCurrentLegendCandPage < input->iLegendCandPageCount)
                    input->bShowNext = true;
                input->iCodeInputCount = 0;
                ShowInputWindow(instance);
            } else {
                ResetInput(instance);
                CloseInputWindow(instance);
            }

            break;
        case IRV_GET_CANDWORDS:
            CommitString(instance, GetCurrentIC(instance), input->strStringGet);
            SetMessageCount(GetMessageUp(instance), 0);
            SetMessageCount(GetMessageDown(instance), 0);
            if (fc->bPhraseTips && currentIM->PhraseTips)
                DoPhraseTips(instance);
            input->iHZInputed += (int) (utf8_strlen(input->strStringGet));
            UpdateInputWindow(instance);

            ResetInput(instance);
            input->lastIsSingleHZ = 0;
            break;
        case IRV_ENG:
        case IRV_PUNC:
            input->iHZInputed += (int) (utf8_strlen(input->strStringGet));        //粗略统计字数
            ResetInput(instance);
            UpdateInputWindow(instance);
        case IRV_GET_CANDWORDS_NEXT:
            CommitString(instance, GetCurrentIC(instance), input->strStringGet);
            input->bLastIsNumber = false;
            input->lastIsSingleHZ = 0;

            if (retVal == IRV_GET_CANDWORDS_NEXT || input->lastIsSingleHZ == -1) {
                input->iHZInputed += (int) (utf8_strlen(input->strStringGet));    //粗略统计字数
                ShowInputWindow(instance);
            }

            break;
        default:
            ;
    }
    
    return retVal;
}

void ForwardKey(FcitxInstance* instance, FcitxInputContext *ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    UT_array* backends = &instance->backends;
    FcitxAddon** pbackend = (FcitxAddon**) utarray_eltptr(backends, ic->backendid);
    if (pbackend == NULL)
        return;
    FcitxBackend* backend = (*pbackend)->backend;
    backend->ForwardKey((*pbackend)->addonInstance, ic, event, sym, state);
}

void SwitchIM(FcitxInstance* instance, int index)
{
    UT_array* imes = &instance->imes;
    int iIMCount = utarray_len(imes);
    
    FcitxIM* lastIM, *newIM;

    if (instance->iIMIndex >= iIMCount || instance->iIMIndex < 0)
        lastIM = NULL;
    else
    {
        lastIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    }
    
    if (index >= iIMCount)
        instance->iIMIndex = iIMCount - 1;
    else if (index < -1)
        instance->iIMIndex = 0;
    else if (index == -1) {
        if (instance->iIMIndex >= (iIMCount - 1))
            instance->iIMIndex = 0;
        else
            instance->iIMIndex++;
    } 
    else if (index >= 0)
        instance->iIMIndex = index;

    if (instance->iIMIndex >= iIMCount || instance->iIMIndex < 0)
        newIM = NULL;
    else
    {
        newIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    }

    if (lastIM && lastIM->Save)
        lastIM->Save(lastIM->klass);
    if (newIM && newIM->Init)
        newIM->Init(newIM->klass);

    ResetInput(instance);
    SaveProfile(&instance->profile);
}

/** 
 * @brief 重置输入状态
 */
void ResetInput(FcitxInstance* instance)
{
    FcitxInputState *input = &instance->input;
    input->iCandPageCount = 0;
    input->iCurrentCandPage = 0;
    input->iCandWordCount = 0;
    input->iLegendCandWordCount = 0;
    input->iCurrentLegendCandPage = 0;
    input->iLegendCandPageCount = 0;
    input->iCursorPos = 0;

    input->strCodeInput[0] = '\0';
    input->iCodeInputCount = 0;  

    input->bIsDoInputOnly = false;

    input->bShowPrev = false;
    input->bShowNext = false;

    input->bIsInLegend = false;
    
    UT_array* ims = &instance->imes;

    FcitxIM* currentIM = (FcitxIM*) utarray_eltptr(ims, instance->iIMIndex);

    if (currentIM && currentIM->ResetIM)
        currentIM->ResetIM(currentIM->klass);
    
    ResetInputHook();
}

void DoPhraseTips(FcitxInstance* instance)
{
    UT_array* ims = &instance->imes;
    FcitxIM* currentIM = (FcitxIM*) utarray_eltptr(ims, instance->iIMIndex);
    FcitxInputState *input = &instance->input;

    if (currentIM->PhraseTips && currentIM->PhraseTips(currentIM->klass))
        input->lastIsSingleHZ = -1;
    else
        input->lastIsSingleHZ = 0;
}

INPUT_RETURN_VALUE ImProcessEnter(void *arg)
{
    FcitxInstance *instance = (FcitxInstance *)arg;
    INPUT_RETURN_VALUE retVal; 
    FcitxInputState *input = &instance->input;
    FcitxConfig *fc = &instance->config;
    
    if (!input->iCodeInputCount)
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
            SetMessageCount(instance->messageDown, 0);
            SetMessageCount(instance->messageUp, 0);
            strcpy(input->strStringGet, input->strCodeInput);
            retVal = IRV_ENG;
            break;
        }
    }
    return retVal;
}

INPUT_RETURN_VALUE ImProcessEscape(void* arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    FcitxInputState *input = &instance->input;
    if (input->iCodeInputCount || input->bIsInLegend)
        return IRV_CLEAN;
    else
        return IRV_DONOT_PROCESS;
}

INPUT_RETURN_VALUE ImProcessReload(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    ReloadConfig(instance);
    return IRV_DO_NOTHING;
}

void ReloadConfig(FcitxInstance *instance)
{
}

void UpdateInputWindow(FcitxInstance *instance)
{
    if (GetMessageCount(instance->messageUp) == 0 && GetMessageCount(instance->messageDown) == 0)
        CloseInputWindow(instance);
    else
        ShowInputWindow(instance);
}

boolean IsInLegend(FcitxInputState* input)
{
    return input->bIsInLegend;
}

char* GetOutputString(FcitxInputState* input)
{
    return input->strStringGet;
}

FcitxIM* GetCurrentIM(FcitxInstance* instance)
{
    UT_array* imes = &instance->imes;
    FcitxIM* pcurrentIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    return pcurrentIM;
}

void EnableIM(FcitxInstance* instance, boolean keepState)
{
    if (!keepState)
    {
        ResetInput(instance);
    }
    FcitxInputContext* ic = GetCurrentIC(instance);
    ic->state = IS_ACTIVE;
}