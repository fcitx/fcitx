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
#include <time.h>
#include "ime.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/configfile.h"
#include "fcitx/keys.h"
#include "fcitx/profile.h"
#include "ime-internal.h"
#include "ui.h"
#include "fcitx-utils/utils.h"
#include "hook.h"
#include "frontend.h"
#include "hook-internal.h"
#include "instance.h"
#include "module.h"
#include "candidate.h"
#include "instance-internal.h"
#include "fcitx-internal.h"
#include "addon-internal.h"

static void UnloadIM(FcitxAddon* pim);
static const char* GetStateName(INPUT_RETURN_VALUE retVal);
static const UT_icd ime_icd = {sizeof(FcitxIM), NULL , NULL, NULL};
static const UT_icd imclass_icd = {sizeof(FcitxAddon*), NULL , NULL, NULL};
static int IMPriorityCmp(const void *a, const void *b);
static boolean IMMenuAction(FcitxUIMenu* menu, int index);
static void UpdateIMMenuShell(FcitxUIMenu *menu);
static void EnableIMInternal(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState);
static void CloseIMInternal(FcitxInstance* instance, FcitxInputContext* ic);
static void ChangeIMStateInternal(FcitxInstance* instance, FcitxInputContext* ic, IME_STATE objectState);

FCITX_GETTER_VALUE(FcitxInputState, IsInRemind, bIsInRemind, boolean)
FCITX_SETTER(FcitxInputState, IsInRemind, bIsInRemind, boolean)
FCITX_GETTER_VALUE(FcitxInputState, IsDoInputOnly, bIsDoInputOnly, boolean)
FCITX_SETTER(FcitxInputState, IsDoInputOnly, bIsDoInputOnly, boolean)
FCITX_GETTER_VALUE(FcitxInputState, RawInputBuffer, strCodeInput, char*)
FCITX_GETTER_VALUE(FcitxInputState, CursorPos, iCursorPos, int)
FCITX_SETTER(FcitxInputState, CursorPos, iCursorPos, int)
FCITX_GETTER_VALUE(FcitxInputState, ClientCursorPos, iClientCursorPos, int)
FCITX_SETTER(FcitxInputState, ClientCursorPos, iClientCursorPos, int)
FCITX_GETTER_VALUE(FcitxInputState, CandidateList, candList, struct _CandidateWordList*)
FCITX_GETTER_VALUE(FcitxInputState, AuxUp, msgAuxUp, Messages*)
FCITX_GETTER_VALUE(FcitxInputState, AuxDown, msgAuxDown, Messages*)
FCITX_GETTER_VALUE(FcitxInputState, Preedit, msgPreedit, Messages*)
FCITX_GETTER_VALUE(FcitxInputState, ClientPreedit, msgClientPreedit, Messages*)
FCITX_GETTER_VALUE(FcitxInputState, RawInputBufferSize, iCodeInputCount, int)
FCITX_SETTER(FcitxInputState, RawInputBufferSize, iCodeInputCount, int)
FCITX_GETTER_VALUE(FcitxInputState, ShowCursor, bShowCursor, boolean)
FCITX_SETTER(FcitxInputState, ShowCursor, bShowCursor, boolean)
FCITX_GETTER_VALUE(FcitxInputState, LastIsSingleChar, lastIsSingleHZ, boolean)
FCITX_SETTER(FcitxInputState, LastIsSingleChar, lastIsSingleHZ, boolean)
FCITX_SETTER(FcitxInputState, KeyReleased, keyReleased, KEY_RELEASED)

FcitxInputState* CreateFcitxInputState()
{
    FcitxInputState* input = fcitx_malloc0(sizeof(FcitxInputState));

    input->msgAuxUp = InitMessages();
    input->msgAuxDown = InitMessages();
    input->msgPreedit = InitMessages();
    input->msgClientPreedit = InitMessages();
    input->candList = CandidateWordInit();

    return input;
}

int IMPriorityCmp(const void *a, const void *b)
{
    FcitxIM *ta, *tb;
    ta = (FcitxIM*)a;
    tb = (FcitxIM*)b;
    int delta = ta->iPriority - tb->iPriority;
    if (delta != 0)
        return delta;
    else
        return strcmp(ta->uniqueName, tb->uniqueName);
}

void InitBuiltInHotkey(FcitxInstance *instance)
{
    HotkeyHook hk;
    hk.hotkey = FCITX_CTRL_5;
    hk.hotkeyhandle = ImProcessReload;
    hk.arg = instance;
    RegisterHotkeyFilter(instance, hk);

    hk.hotkey = FCITX_ENTER;
    hk.hotkeyhandle = ImProcessEnter;
    hk.arg = instance;
    RegisterHotkeyFilter(instance, hk);

    hk.hotkey = FCITX_ESCAPE;
    hk.hotkeyhandle = ImProcessEscape;
    hk.arg = instance;
    RegisterHotkeyFilter(instance, hk);

    hk.hotkey = instance->config->hkRemind;
    hk.hotkeyhandle = ImProcessRemind;
    hk.arg = instance;
    RegisterHotkeyFilter(instance, hk);

    hk.hotkey = instance->config->hkSaveAll;
    hk.hotkeyhandle = ImProcessSaveAll;
    hk.arg = instance;
    RegisterHotkeyFilter(instance, hk);

    hk.hotkey = instance->config->hkSwitchEmbeddedPreedit;
    hk.hotkeyhandle = ImSwitchEmbeddedPreedit;
    hk.arg = instance;
    RegisterHotkeyFilter(instance, hk);
}

void InitFcitxIM(FcitxInstance* instance)
{
    utarray_init(&instance->imes, &ime_icd);
    utarray_init(&instance->availimes, &ime_icd);
    utarray_init(&instance->imeclasses, &imclass_icd);
}

FCITX_EXPORT_API
void SaveAllIM(FcitxInstance* instance)
{
    UT_array* imes = &instance->availimes;
    FcitxIM *pim;
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim)) {
        if ((pim)->Save)
            (pim)->Save(pim->klass);
    }
}

void UnloadAllIM(UT_array* ims)
{
    FcitxAddon **pim;
    for (pim = (FcitxAddon **) utarray_front(ims);
            pim != NULL;
            pim = (FcitxAddon **) utarray_next(ims, pim)) {
        FcitxAddon *im = *pim;
        UnloadIM(im);
    }
    utarray_clear(ims);
}

static const char* GetStateName(INPUT_RETURN_VALUE retVal)
{

    return "unknown";
}

void UnloadIM(FcitxAddon* pim)
{
    FcitxIMClass *im = pim->imclass;
    if (im->Destroy)
        im->Destroy(pim->addonInstance);
}

FCITX_EXPORT_API
void FcitxRegisterIM(FcitxInstance *instance,
                     void *addonInstance,
                     const char* name,
                     const char* iconName,
                     FcitxIMInit Init,
                     FcitxIMResetIM ResetIM,
                     FcitxIMDoInput DoInput,
                     FcitxIMGetCandWords GetCandWords,
                     FcitxIMPhraseTips PhraseTips,
                     FcitxIMSave Save,
                     FcitxIMReloadConfig ReloadConfig,
                     void *priv,
                     int priority
                    )
{
    FcitxRegisterIMv2(instance,
                      addonInstance,
                      iconName,
                      name,
                      iconName,
                      Init,
                      ResetIM,
                      DoInput,
                      GetCandWords,
                      PhraseTips,
                      Save,
                      ReloadConfig,
                      priv,
                      priority,
                      ""
                     );
}


FCITX_EXPORT_API
void FcitxRegisterIMv2(FcitxInstance *instance,
                       void *addonInstance,
                       const char* uniqueName,
                       const char* name,
                       const char* iconName,
                       FcitxIMInit Init,
                       FcitxIMResetIM ResetIM,
                       FcitxIMDoInput DoInput,
                       FcitxIMGetCandWords GetCandWords,
                       FcitxIMPhraseTips PhraseTips,
                       FcitxIMSave Save,
                       FcitxIMReloadConfig ReloadConfig,
                       void *priv,
                       int priority,
                       const char *langCode
                      )
{
    if (priority <= 0)
        return ;
    UT_array* imes = &instance->availimes ;
    FcitxIM newime;
    
    if (GetIMFromIMList(imes, uniqueName))
    {
        FcitxLog(ERROR, "%s already exists", uniqueName);
    }
    
    memset(&newime, 0, sizeof(FcitxIM));
    strncpy(newime.uniqueName, uniqueName, MAX_IM_NAME);
    strncpy(newime.strName, name, MAX_IM_NAME);
    strncpy(newime.strIconName, iconName, MAX_IM_NAME);
    newime.Init = Init;
    newime.ResetIM = ResetIM;
    newime.DoInput = DoInput;
    newime.GetCandWords = GetCandWords;
    newime.PhraseTips = PhraseTips;
    newime.Save = Save;
    newime.ReloadConfig = ReloadConfig;
    newime.klass = addonInstance;
    newime.iPriority = priority;
    newime.priv = priv;
    strncpy(newime.langCode, langCode, LANGCODE_LENGTH);
    newime.langCode[LANGCODE_LENGTH] = 0;

    utarray_push_back(imes, &newime);
}

boolean LoadAllIM(FcitxInstance* instance)
{
    UT_array* addons = &instance->addons;
    UT_array* ims = &instance->imeclasses;
    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->bEnabled && addon->category == AC_INPUTMETHOD) {
            char *modulePath;
            switch (addon->type) {
            case AT_SHAREDLIBRARY: {
                FILE *fp = GetLibFile(addon->library, "r", &modulePath);
                void *handle;
                FcitxIMClass * imclass;
                if (!fp)
                    break;
                fclose(fp);
                handle = dlopen(modulePath, RTLD_LAZY | RTLD_GLOBAL);
                if (!handle) {
                    FcitxLog(ERROR, _("IM: open %s fail %s") , modulePath , dlerror());
                    break;
                }

                if (!CheckABIVersion(handle)) {
                    FcitxLog(ERROR, "%s ABI Version Error", addon->name);
                    dlclose(handle);
                    break;
                }

                imclass = dlsym(handle, "ime");
                if (!imclass || !imclass->Create) {
                    FcitxLog(ERROR, _("IM: bad im %s"), addon->name);
                    dlclose(handle);
                    break;
                }
                if ((addon->addonInstance = imclass->Create(instance)) == NULL) {
                    dlclose(handle);
                    break;
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

    if (utarray_len(&instance->availimes) <= 0) {
        FcitxLog(ERROR, _("No available Input Method"));
        return false;
    }

    instance->imLoaded = true;

    UpdateIMList(instance);

    return true;
}

FCITX_EXPORT_API
boolean IsHotKey(FcitxKeySym sym, int state, HOTKEYS * hotkey)
{
    state &= KEY_CTRL_ALT_SHIFT_COMP;
    if (hotkey[0].sym && sym == hotkey[0].sym && (hotkey[0].state == state))
        return true;
    if (hotkey[1].sym && sym == hotkey[1].sym && (hotkey[1].state == state))
        return true;
    return false;
}

FCITX_EXPORT_API
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
    FcitxIM* currentIM = GetCurrentIM(instance);
    FcitxInputState *input = instance->input;

    FcitxConfig *fc = instance->config;

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */
    if (GetCurrentIC(instance) == NULL)
        return IRV_TO_PROCESS;

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (event == FCITX_RELEASE_KEY) {
        if (GetCurrentState(instance) != IS_CLOSED) {
            if ((timestamp - input->lastKeyPressedTime) < 500 && (!input->bIsDoInputOnly)) {
                if (fc->bIMSwitchKey && (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT) || IsHotKey(sym, state, FCITX_LCTRL_LSHIFT2))) {
                    if (GetCurrentState(instance) == IS_ACTIVE) {
                        if (input->keyReleased == KR_CTRL_SHIFT)
                            SwitchIM(instance, -1);
                    } else if (IsHotKey(sym, state, fc->hkTrigger))
                        CloseIM(instance, GetCurrentIC(instance));
                } else if (IsHotKey(sym, state, fc->switchKey) && input->keyReleased == KR_CTRL && !fc->bDoubleSwitchKey) {
                    retVal = IRV_DONOT_PROCESS;
                    if (fc->bSendTextWhenSwitchEng) {
                        if (input->iCodeInputCount != 0) {
                            strcpy(GetOutputString(input), FcitxInputStateGetRawInputBuffer(input));
                            retVal = IRV_ENG;
                        }
                    }
                    input->keyReleased = KR_OTHER;
                    if (GetCurrentState(instance) == IS_ENG)
                        ShowInputSpeed(instance);
                    ChangeIMState(instance, GetCurrentIC(instance));
                } else if (IsHotKey(sym, state, fc->i2ndSelectKey) && input->keyReleased == KR_2ND_SELECTKEY) {
                    if (!input->bIsInRemind) {
                        retVal = CandidateWordChooseByIndex(input->candList, 1);
                    } else {
                        strcpy(GetOutputString(input), " ");
                        retVal = IRV_COMMIT_STRING;
                    }
                    input->keyReleased = KR_OTHER;
                } else if (IsHotKey(sym, state, fc->i3rdSelectKey) && input->keyReleased == KR_3RD_SELECTKEY) {
                    if (!input->bIsInRemind) {
                        retVal = CandidateWordChooseByIndex(input->candList, 2);
                    } else {
                        strcpy(GetOutputString(input), "　");
                        retVal = IRV_COMMIT_STRING;
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
        return IRV_DO_NOTHING;

    if (retVal == IRV_TO_PROCESS) {
        /* process key event for switch key */
        if (event == FCITX_PRESS_KEY) {
            if (!IsHotKey(sym, state, fc->switchKey))
                input->keyReleased = KR_OTHER;
            else {
                if ((input->keyReleased == KR_CTRL)
                        && (timestamp - input->lastKeyPressedTime < fc->iTimeInterval)
                        && fc->bDoubleSwitchKey) {
                    CommitString(instance, GetCurrentIC(instance), FcitxInputStateGetRawInputBuffer(input));
                    ChangeIMState(instance, GetCurrentIC(instance));
                }
            }

            input->lastKeyPressedTime = timestamp;
            if (IsHotKey(sym, state, fc->switchKey)) {
                input->keyReleased = KR_CTRL;
                retVal = IRV_DO_NOTHING;
            } else if (fc->bIMSwitchKey && (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT) || IsHotKey(sym, state, FCITX_LCTRL_LSHIFT2))) {
                input->keyReleased = KR_CTRL_SHIFT;
                retVal = IRV_DO_NOTHING;
            } else if (IsHotKey(sym, state, fc->hkTrigger)) {
                /* trigger key has the highest priority, so we check it first */
                if (GetCurrentState(instance) == IS_ENG) {
                    ChangeIMState(instance, GetCurrentIC(instance));
                    ShowInputSpeed(instance);
                } else
                    CloseIM(instance, GetCurrentIC(instance));

                retVal = IRV_DO_NOTHING;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS && event != FCITX_PRESS_KEY)
        retVal = IRV_DONOT_PROCESS;

    /* the key processed before this phase is very important, we don't let any interrupt */
    if (GetCurrentState(instance) == IS_ACTIVE
            && retVal == IRV_TO_PROCESS
       ) {
        if (!input->bIsDoInputOnly) {
            ProcessPreInputFilter(instance, sym, state, &retVal);
        }

        if (retVal == IRV_TO_PROCESS) {
            if (IsHotKey(sym, state, fc->i2ndSelectKey)) {
                if (CandidateWordGetByIndex(input->candList, 1) != NULL) {
                    input->keyReleased = KR_2ND_SELECTKEY;
                    return IRV_DO_NOTHING;
                }
            } else if (IsHotKey(sym, state, fc->i3rdSelectKey)) {
                if (CandidateWordGetByIndex(input->candList, 2) != NULL) {
                    input->keyReleased = KR_3RD_SELECTKEY;
                    return IRV_DO_NOTHING;
                }
            }

            if (!IsHotKey(sym, state, FCITX_LCTRL_LSHIFT) && currentIM) {
                retVal = currentIM->DoInput(currentIM->klass, sym, state);
            }
        }

        /* check choose key first, because it might trigger update candidates */
        if (!input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
            int index = CheckChooseKey(sym, state, CandidateWordGetChoose(input->candList));
            if (index >= 0)
                retVal = CandidateWordChooseByIndex(input->candList, index);
        }
    }

    if (GetCurrentState(instance) == IS_ACTIVE && currentIM && (retVal & IRV_FLAG_UPDATE_CANDIDATE_WORDS)) {
        CleanInputWindow(instance);
        retVal = currentIM->GetCandWords(currentIM->klass);
        ProcessUpdateCandidates(instance);
    }

    /*
        * since all candidate word are cached in candList, so we don't need to trigger
        * GetCandWords after go for another page, simply update input window is ok.
        */
    if (GetCurrentState(instance) == IS_ACTIVE && !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        if (IsHotKey(sym, state, fc->hkPrevPage)) {
            if (CandidateWordGoPrevPage(input->candList))
                retVal = IRV_DISPLAY_CANDWORDS;
        } else if (IsHotKey(sym, state, fc->hkNextPage)) {
            if (CandidateWordGoNextPage(input->candList))
                retVal = IRV_DISPLAY_CANDWORDS;
        }
    }

    if (GetCurrentState(instance) == IS_ACTIVE && !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        ProcessPostInputFilter(instance, sym, state, &retVal);
    }

    if (retVal == IRV_TO_PROCESS) {
        retVal = CheckHotkey(instance, sym, state);
    }

    FcitxLog(DEBUG, "ProcessKey Return State: %s", GetStateName(retVal));

    ProcessInputReturnValue(instance, retVal);

    return retVal;
}

FCITX_EXPORT_API
void ProcessInputReturnValue(
    FcitxInstance* instance,
    INPUT_RETURN_VALUE retVal
)
{
    FcitxIM* currentIM = GetCurrentIM(instance);
    FcitxInputState *input = instance->input;
    FcitxConfig *fc = instance->config;

    if (retVal & IRV_FLAG_PENDING_COMMIT_STRING) {
        CommitString(instance, GetCurrentIC(instance), GetOutputString(input));
        instance->iHZInputed += (int)(utf8_strlen(GetOutputString(input)));
    }

    if (retVal & IRV_FLAG_DO_PHRASE_TIPS) {
        CleanInputWindow(instance);
        if (fc->bPhraseTips && currentIM && currentIM->PhraseTips)
            DoPhraseTips(instance);
        UpdateInputWindow(instance);

        ResetInput(instance);
        input->lastIsSingleHZ = 0;
    }

    if (retVal & IRV_FLAG_RESET_INPUT) {
        ResetInput(instance);
        CloseInputWindow(instance);
    }

    if (retVal & IRV_FLAG_DISPLAY_LAST) {
        CleanInputWindow(instance);
        AddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%c", FcitxInputStateGetRawInputBuffer(input)[0]);
        AddMessageAtLast(input->msgAuxDown, MSG_TIPS, "%s", GetOutputString(input));
    }

    if (retVal & IRV_FLAG_UPDATE_INPUT_WINDOW)
        UpdateInputWindow(instance);
}

FCITX_EXPORT_API
void ForwardKey(FcitxInstance* instance, FcitxInputContext *ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    if (ic == NULL)
        return;
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    frontend->ForwardKey((*pfrontend)->addonInstance, ic, event, sym, state);
}

FCITX_EXPORT_API
void SwitchIM(FcitxInstance* instance, int index)
{
    UT_array* imes = &instance->imes;
    int iIMCount = utarray_len(imes);

    CleanInputWindow(instance);
    ResetInput(instance);
    UpdateInputWindow(instance);

    FcitxIM* lastIM, *newIM;

    if (instance->iIMIndex >= iIMCount || instance->iIMIndex < 0)
        lastIM = NULL;
    else {
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
    } else if (index >= 0)
        instance->iIMIndex = index;

    if (instance->iIMIndex >= iIMCount || instance->iIMIndex < 0)
        newIM = NULL;
    else {
        newIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    }

    if (lastIM && lastIM->Save)
        lastIM->Save(lastIM->klass);
    if (newIM && newIM->Init)
        newIM->Init(newIM->klass);

    ResetInput(instance);
    SaveProfile(instance->profile);
}

/**
 * @brief 重置输入状态
 */
FCITX_EXPORT_API
void ResetInput(FcitxInstance* instance)
{
    FcitxInputState *input = instance->input;
    CandidateWordReset(input->candList);
    input->iCursorPos = 0;
    input->iClientCursorPos = 0;

    FcitxInputStateGetRawInputBuffer(input)[0] = '\0';
    input->iCodeInputCount = 0;

    input->bIsDoInputOnly = false;
    input->bIsInRemind = false;

    UT_array* ims = &instance->imes;

    FcitxIM* currentIM = (FcitxIM*) utarray_eltptr(ims, instance->iIMIndex);

    if (currentIM && currentIM->ResetIM)
        currentIM->ResetIM(currentIM->klass);

    ResetInputHook(instance);
}

void DoPhraseTips(FcitxInstance* instance)
{
    UT_array* ims = &instance->imes;
    FcitxIM* currentIM = (FcitxIM*) utarray_eltptr(ims, instance->iIMIndex);
    FcitxInputState *input = instance->input;

    if (currentIM->PhraseTips && currentIM->PhraseTips(currentIM->klass))
        input->lastIsSingleHZ = -1;
    else
        input->lastIsSingleHZ = 0;
}

INPUT_RETURN_VALUE ImProcessEnter(void *arg)
{
    FcitxInstance *instance = (FcitxInstance *)arg;
    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    FcitxInputState *input = instance->input;
    FcitxConfig *fc = instance->config;

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
            CleanInputWindow(instance);
            strcpy(GetOutputString(input), FcitxInputStateGetRawInputBuffer(input));
            retVal = IRV_ENG;
            break;
        }
    }
    return retVal;
}

INPUT_RETURN_VALUE ImProcessEscape(void* arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    FcitxInputState *input = instance->input;
    if (input->iCodeInputCount || input->bIsInRemind)
        return IRV_CLEAN;
    else
        return IRV_DONOT_PROCESS;
}

INPUT_RETURN_VALUE ImProcessRemind(void* arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    UpdateStatus(instance, "remind");
    return IRV_DONOT_PROCESS;
}

INPUT_RETURN_VALUE ImProcessReload(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    ReloadConfig(instance);
    return IRV_DO_NOTHING;
}

FCITX_EXPORT_API
void ReloadConfig(FcitxInstance *instance)
{
    if (!LoadConfig(instance->config))
        EndInstance(instance);

    if (!LoadProfile(instance->profile, instance))
        EndInstance(instance);

    CandidateWordSetPageSize(instance->input->candList, instance->config->iMaxCandWord);

    /* Reload All IM, Module, and UI Config */
    UT_array* addons = &instance->addons;

    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->category == AC_MODULE &&
                addon->bEnabled &&
                addon->addonInstance) {
            if (addon->module->ReloadConfig)
                addon->module->ReloadConfig(addon->addonInstance);
        }
    }

    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->category == AC_FRONTEND &&
                addon->bEnabled &&
                addon->addonInstance) {
            if (addon->frontend->ReloadConfig)
                addon->frontend->ReloadConfig(addon->addonInstance);
        }
    }


    UT_array* imes = &instance->imes;
    FcitxIM* pim;
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim)) {
        if (pim->ReloadConfig)
            pim->ReloadConfig(pim->klass);
    }

    if (instance->ui && instance->ui->ui->ReloadConfig)
        instance->ui->ui->ReloadConfig(instance->ui->addonInstance);
}

FCITX_EXPORT_API
char* GetOutputString(FcitxInputState* input)
{
    return input->strStringGet;
}

FCITX_EXPORT_API
FcitxIM* GetCurrentIM(FcitxInstance* instance)
{
    UT_array* imes = &instance->imes;
    FcitxIM* pcurrentIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    return pcurrentIM;
}

FCITX_EXPORT_API
void EnableIM(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState)
{
    if (ic == NULL)
        return;
    instance->globalState = IS_ACTIVE;
    switch (instance->config->shareState) {
    case ShareState_All:
    case ShareState_PerProgram: {
        FcitxInputContext *rec = instance->ic_list;
        while (rec != NULL) {
            boolean flag = false;
            if (instance->config->shareState == ShareState_All)
                flag = true;
            else {
                UT_array* frontends = &instance->frontends;
                FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
                if (pfrontend) {
                    FcitxFrontend* frontend = (*pfrontend)->frontend;
                    if (frontend->CheckICFromSameApplication &&
                            frontend->CheckICFromSameApplication((*pfrontend)->addonInstance, rec, ic))
                        flag = true;
                }
            }

            if (flag)
                EnableIMInternal(instance, rec, keepState);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        EnableIMInternal(instance, ic, keepState);
        break;
    }
}


void EnableIMInternal(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState)
{
    if (ic == NULL)
        return ;
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    IME_STATE oldstate = ic->state;
    ic->state = IS_ACTIVE;
    if (oldstate == IS_CLOSED)
        frontend->EnableIM((*pfrontend)->addonInstance, ic);

    if (ic == GetCurrentIC(instance)) {
        if (oldstate == IS_CLOSED)
            OnTriggerOn(instance);
        if (!keepState)
            ResetInput(instance);
    }
}

FCITX_EXPORT_API
void CloseIM(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;
    instance->globalState = IS_CLOSED;
    switch (instance->config->shareState) {
    case ShareState_All:
    case ShareState_PerProgram: {
        FcitxInputContext *rec = instance->ic_list;
        while (rec != NULL) {
            boolean flag = false;
            if (instance->config->shareState == ShareState_All)
                flag = true;
            else {
                UT_array* frontends = &instance->frontends;
                FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
                if (pfrontend) {
                    FcitxFrontend* frontend = (*pfrontend)->frontend;
                    if (frontend->CheckICFromSameApplication &&
                            frontend->CheckICFromSameApplication((*pfrontend)->addonInstance, rec, ic))
                        flag = true;
                }
            }

            if (flag)
                CloseIMInternal(instance, rec);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        CloseIMInternal(instance, ic);
        break;
    }
}

void CloseIMInternal(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return ;
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    ic->state = IS_CLOSED;
    frontend->CloseIM((*pfrontend)->addonInstance, ic);

    if (ic == GetCurrentIC(instance)) {
        OnTriggerOff(instance);
        CloseInputWindow(instance);
    }
}

/**
 * @brief 更改输入法状态
 *
 * @param _connect_id
 */
FCITX_EXPORT_API
void ChangeIMState(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;
    IME_STATE objectState;
    if (ic->state == IS_ENG)
        objectState = IS_ACTIVE;
    else
        objectState = IS_ENG;

    instance->globalState = objectState;
    switch (instance->config->shareState) {
    case ShareState_All:
    case ShareState_PerProgram: {
        FcitxInputContext *rec = instance->ic_list;
        while (rec != NULL) {
            boolean flag = false;
            if (instance->config->shareState == ShareState_All)
                flag = true;
            else {
                UT_array* frontends = &instance->frontends;
                FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
                if (pfrontend) {
                    FcitxFrontend* frontend = (*pfrontend)->frontend;
                    if (frontend->CheckICFromSameApplication &&
                            frontend->CheckICFromSameApplication((*pfrontend)->addonInstance, rec, ic))
                        flag = true;
                }
            }

            if (flag)
                ChangeIMStateInternal(instance, rec, objectState);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        ChangeIMStateInternal(instance, ic, objectState);
        break;
    }
}

void ChangeIMStateInternal(FcitxInstance* instance, FcitxInputContext* ic, IME_STATE objectState)
{
    if (!ic)
        return;
    if (ic->state == objectState)
        return;
    ic->state = objectState;
    if (ic == GetCurrentIC(instance)) {
        if (objectState == IS_ACTIVE) {
            ResetInput(instance);
        } else {
            ResetInput(instance);
            CloseInputWindow(instance);
        }
    }
}

void InitIMMenu(FcitxInstance* instance)
{
    strcpy(instance->imMenu.candStatusBind, "im");
    strcpy(instance->imMenu.name, _("Input Method"));

    instance->imMenu.UpdateMenuShell = UpdateIMMenuShell;
    instance->imMenu.MenuAction = IMMenuAction;
    instance->imMenu.priv = instance;
    instance->imMenu.isSubMenu = false;
}

boolean IMMenuAction(FcitxUIMenu *menu, int index)
{
    FcitxInstance* instance = (FcitxInstance*) menu->priv;
    SwitchIM(instance, index);
    return true;
}

void UpdateIMMenuShell(FcitxUIMenu *menu)
{
    FcitxInstance* instance = (FcitxInstance*) menu->priv;
    ClearMenuShell(menu);
    
    FcitxIM* pim;
    UT_array* imes = &instance->imes;
    utarray_init(&instance->imMenu.shell, &menuICD);
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim))
        AddMenuShell(&instance->imMenu, pim->strName, MENUTYPE_SIMPLE, NULL);

    menu->mark = instance->iIMIndex;
}

void ShowInputSpeed(FcitxInstance* instance)
{
    FcitxInputState* input = instance->input;

    if (!instance->config->bShowInputWindowTriggering)
        return;

    input->bShowCursor = false;

    CleanInputWindow(instance);
    if (instance->config->bShowVersion) {
        AddMessageAtLast(input->msgAuxUp, MSG_TIPS, "FCITX " VERSION);
    }

    //显示打字速度
    if (instance->config->bShowUserSpeed) {
        double          timePassed;

        timePassed = instance->totaltime + difftime(time(NULL), instance->timeStart);
        if (((int) timePassed) == 0)
            timePassed = 1.0;

        SetMessageCount(input->msgAuxDown, 0);
        AddMessageAtLast(input->msgAuxDown, MSG_OTHER, _("Input Speed: "));
        AddMessageAtLast(input->msgAuxDown, MSG_CODE, "%d", (int)(instance->iHZInputed * 60 / timePassed));
        AddMessageAtLast(input->msgAuxDown, MSG_OTHER, _("/min  Time Used: "));
        AddMessageAtLast(input->msgAuxDown, MSG_CODE, "%d", (int) timePassed / 60);
        AddMessageAtLast(input->msgAuxDown, MSG_OTHER, _("min Num of Characters: "));
        AddMessageAtLast(input->msgAuxDown, MSG_CODE, "%u", instance->iHZInputed);

    }

    UpdateInputWindow(instance);
}

INPUT_RETURN_VALUE ImProcessSaveAll(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    SaveAllIM(instance);
    return IRV_DO_NOTHING;
}


INPUT_RETURN_VALUE ImSwitchEmbeddedPreedit(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    instance->profile->bUsePreedit = !instance->profile->bUsePreedit;
    SaveProfile(instance->profile);
    UpdateInputWindow(instance);
    return IRV_DO_NOTHING;
}

FCITX_EXPORT_API
void CleanInputWindow(FcitxInstance *instance)
{
    CleanInputWindowUp(instance);
    CleanInputWindowDown(instance);
}

FCITX_EXPORT_API
void CleanInputWindowUp(FcitxInstance *instance)
{
    FcitxInputState* input = instance->input;
    SetMessageCount(input->msgAuxUp, 0);
    SetMessageCount(input->msgPreedit, 0);
    SetMessageCount(input->msgClientPreedit, 0);
}

FCITX_EXPORT_API
void CleanInputWindowDown(FcitxInstance* instance)
{
    FcitxInputState* input = instance->input;
    CandidateWordReset(input->candList);
    SetMessageCount(input->msgAuxDown, 0);
}

FCITX_EXPORT_API
int CheckChooseKey(FcitxKeySym sym, int state, const char* strChoose)
{
    if (state != 0)
        return -1;

    sym = KeyPadToMain(sym);

    int i = 0;

    while (strChoose[i]) {
        if (sym == strChoose[i])
            return i;
        i++;
    }

    return -1;
}

int GetIMIndexByName(FcitxInstance* instance, char* imName)
{
    UT_array* imes = &instance->imes;
    FcitxIM *pim;
    int index = 0, i = 0;
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim)) {
        if (strcmp(imName, pim->uniqueName) == 0) {
            index = i;
            break;
        }
        i ++;
    }
    return index;
}

FCITX_EXPORT_API
void UpdateIMList(FcitxInstance* instance)
{
    if (!instance->imLoaded)
        return;

    UT_array* imList = SplitString(instance->profile->imList, ',');
    utarray_sort(&instance->availimes, IMPriorityCmp);
    utarray_clear(&instance->imes);

    char** pstr;
    FcitxIM* ime;
    for (pstr = (char**) utarray_front(imList);
            pstr != NULL;
            pstr = (char**) utarray_next(imList, pstr)) {
        char* str = *pstr;
        char* pos = strchr(str, ':');
        if (pos) {
            ime = NULL;
            *pos = '\0';
            pos ++;
            if (strcmp(pos, "True") == 0)
                ime = GetIMFromIMList(&instance->availimes, str);

            if (ime)
                utarray_push_back(&instance->imes, ime);
        }
    }

    for (ime = (FcitxIM*) utarray_front(&instance->availimes);
            ime != NULL;
            ime = (FcitxIM*) utarray_next(&instance->availimes, ime)) {
        if (!IMIsInIMNameList(imList, ime)) {
            /* ok, make all im priority larger than 100 disable by default */
            if (ime->iPriority < PRIORITY_DISABLE)
                utarray_push_back(&instance->imes, ime);
        }
    }

    utarray_free(imList);

    instance->iIMIndex = GetIMIndexByName(instance, instance->profile->imName);

    SwitchIM(instance, instance->iIMIndex);
    UpdateIMListHook(instance);
}

FCITX_EXPORT_API
FcitxIM* GetIMFromIMList(UT_array* imes, const char* name)
{
    FcitxIM* ime = NULL;
    for (ime = (FcitxIM*) utarray_front(imes);
            ime !=  NULL;
            ime = (FcitxIM*) utarray_next(imes, ime)) {
        if (strcmp(ime->uniqueName, name) == 0)
            break;
    }
    return ime;
}

boolean IMIsInIMNameList(UT_array* imList, FcitxIM* ime)
{
    char** pstr;
    for (pstr = (char**) utarray_front(imList);
            pstr != NULL;
            pstr = (char**) utarray_next(imList, pstr)) {
        if (strncmp(ime->uniqueName, *pstr, strlen(ime->uniqueName)) == 0)
            return true;
    }
    return false;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
