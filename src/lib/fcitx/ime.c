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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

/**
 * @file   ime.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  Process Keyboard Event and Input Method
 *
 */
#include <dlfcn.h>
#include <libintl.h>
#include <time.h>
#include "config.h"
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
#include "context-internal.h"


static const FcitxHotkey* switchKey1[] = {
    FCITX_RCTRL,
    FCITX_RSHIFT,
    FCITX_LSHIFT,
    FCITX_LCTRL,
    FCITX_ALT_LSHIFT,
    FCITX_ALT_RSHIFT,

    FCITX_RCTRL,
    FCITX_RSHIFT,

    FCITX_LALT,
    FCITX_RALT,
    FCITX_LALT,

    FCITX_LSUPER,
    FCITX_RSUPER,
    FCITX_LSUPER,

    FCITX_CTRL_LSUPER,
    FCITX_CTRL_RSUPER,
    FCITX_SUPER_LCTRL,
    FCITX_SUPER_RCTRL,
    FCITX_NONE_KEY,
};

static const FcitxHotkey* switchKey2[] = {
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,

    FCITX_LCTRL,
    FCITX_LSHIFT,

    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_RALT,

    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_RSUPER,

    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY,
    FCITX_NONE_KEY
};

static const FcitxHotkey* imSWNextKey1[] = {
    FCITX_LCTRL_LSHIFT,
    FCITX_LALT_LSHIFT,
    FCITX_LCTRL_LSUPER,
    FCITX_LALT_LSUPER,
};

static const FcitxHotkey* imSWNextKey2[] = {
    FCITX_LCTRL_LSHIFT2,
    FCITX_LALT_LSHIFT2,
    FCITX_LCTRL_LSUPER2,
    FCITX_LALT_LSUPER2,
};

static const FcitxHotkey* imSWPrevKey1[] = {
    FCITX_RCTRL_RSHIFT,
    FCITX_RALT_RSHIFT,
    FCITX_RCTRL_RSUPER,
    FCITX_RALT_RSUPER,
};

static const FcitxHotkey* imSWPrevKey2[] = {
    FCITX_RCTRL_RSHIFT2,
    FCITX_RALT_RSHIFT2,
    FCITX_RCTRL_RSUPER2,
    FCITX_RALT_RSUPER2,
};


static const UT_icd ime_icd = { sizeof(FcitxIM), NULL , NULL, NULL };
static boolean IMMenuAction(FcitxUIMenu* menu, int index);
static void UpdateIMMenuItem(FcitxUIMenu *menu);
static void FcitxInstanceEnableIMInternal(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState);
static void FcitxInstanceCloseIMInternal(FcitxInstance* instance, FcitxInputContext* ic);
static void FcitxInstanceChangeIMStateWithKey(FcitxInstance* instance, FcitxInputContext* ic, boolean withSwitchKey);
static void FcitxInstanceChangeIMStateInternal(FcitxInstance* instance, FcitxInputContext* ic, FcitxContextState objectState, boolean withSwitchKey);
static void FreeIMEntry(FcitxIMEntry* entry);

FCITX_GETTER_VALUE(FcitxInputState, IsInRemind, bIsInRemind, boolean)
FCITX_SETTER(FcitxInputState, IsInRemind, bIsInRemind, boolean)
FCITX_GETTER_VALUE(FcitxInputState, IsDoInputOnly, bIsDoInputOnly, boolean)
FCITX_SETTER(FcitxInputState, IsDoInputOnly, bIsDoInputOnly, boolean)
FCITX_GETTER_VALUE(FcitxInputState, OutputString, strStringGet, char*)
FCITX_GETTER_VALUE(FcitxInputState, LastCommitString, strLastCommit, const char*)
FCITX_GETTER_VALUE(FcitxInputState, RawInputBuffer, strCodeInput, char*)
FCITX_GETTER_VALUE(FcitxInputState, CursorPos, iCursorPos, int)
FCITX_SETTER(FcitxInputState, CursorPos, iCursorPos, int)
FCITX_GETTER_VALUE(FcitxInputState, ClientCursorPos, iClientCursorPos, int)
FCITX_SETTER(FcitxInputState, ClientCursorPos, iClientCursorPos, int)
FCITX_GETTER_VALUE(FcitxInputState, CandidateList, candList, struct _FcitxCandidateWordList*)
FCITX_GETTER_VALUE(FcitxInputState, AuxUp, msgAuxUp, FcitxMessages*)
FCITX_GETTER_VALUE(FcitxInputState, AuxDown, msgAuxDown, FcitxMessages*)
FCITX_GETTER_VALUE(FcitxInputState, Preedit, msgPreedit, FcitxMessages*)
FCITX_GETTER_VALUE(FcitxInputState, ClientPreedit, msgClientPreedit, FcitxMessages*)
FCITX_GETTER_VALUE(FcitxInputState, RawInputBufferSize, iCodeInputCount, int)
FCITX_SETTER(FcitxInputState, RawInputBufferSize, iCodeInputCount, int)
FCITX_GETTER_VALUE(FcitxInputState, ShowCursor, bShowCursor, boolean)
FCITX_SETTER(FcitxInputState, ShowCursor, bShowCursor, boolean)
FCITX_GETTER_VALUE(FcitxInputState, LastIsSingleChar, lastIsSingleHZ, boolean)
FCITX_SETTER(FcitxInputState, KeyCode, keycode, uint32_t)
FCITX_SETTER(FcitxInputState, KeySym, keysym, uint32_t)
FCITX_SETTER(FcitxInputState, KeyState, keystate, uint32_t)
FCITX_GETTER_VALUE(FcitxInputState, KeyCode, keycode, uint32_t)
FCITX_GETTER_VALUE(FcitxInputState, KeySym, keysym, uint32_t)
FCITX_GETTER_VALUE(FcitxInputState, KeyState, keystate, uint32_t)
FCITX_SETTER(FcitxInputState, LastIsSingleChar, lastIsSingleHZ, boolean)

CONFIG_BINDING_BEGIN(FcitxIMEntry)
CONFIG_BINDING_REGISTER("InputMethod", "UniqueName", uniqueName)
CONFIG_BINDING_REGISTER("InputMethod", "Name", name)
CONFIG_BINDING_REGISTER("InputMethod", "IconName", iconName)
CONFIG_BINDING_REGISTER("InputMethod", "Parent", parent)
CONFIG_BINDING_REGISTER("InputMethod", "LangCode", langCode)
CONFIG_BINDING_REGISTER("InputMethod", "Priority", priority)
CONFIG_BINDING_END()

FcitxInputState* FcitxInputStateCreate()
{
    FcitxInputState* input = fcitx_utils_malloc0(sizeof(FcitxInputState));

    input->msgAuxUp = FcitxMessagesNew();
    input->msgAuxDown = FcitxMessagesNew();
    input->msgPreedit = FcitxMessagesNew();
    input->msgClientPreedit = FcitxMessagesNew();
    input->candList = FcitxCandidateWordNewList();

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

void FcitxInstanceInitBuiltInHotkey(FcitxInstance *instance)
{
    FcitxHotkeyHook hk;
    hk.hotkey = instance->config->hkReloadConfig;
    hk.hotkeyhandle = ImProcessReload;
    hk.arg = instance;
    FcitxInstanceRegisterHotkeyFilter(instance, hk);

    hk.hotkey = FCITX_ENTER;
    hk.hotkeyhandle = ImProcessEnter;
    hk.arg = instance;
    FcitxInstanceRegisterHotkeyFilter(instance, hk);

    hk.hotkey = FCITX_ESCAPE;
    hk.hotkeyhandle = ImProcessEscape;
    hk.arg = instance;
    FcitxInstanceRegisterHotkeyFilter(instance, hk);

    hk.hotkey = instance->config->hkRemind;
    hk.hotkeyhandle = ImProcessRemind;
    hk.arg = instance;
    FcitxInstanceRegisterHotkeyFilter(instance, hk);

    hk.hotkey = instance->config->hkSaveAll;
    hk.hotkeyhandle = ImProcessSaveAll;
    hk.arg = instance;
    FcitxInstanceRegisterHotkeyFilter(instance, hk);

    hk.hotkey = instance->config->hkSwitchEmbeddedPreedit;
    hk.hotkeyhandle = ImSwitchEmbeddedPreedit;
    hk.arg = instance;
    FcitxInstanceRegisterHotkeyFilter(instance, hk);
}

void FcitxInstanceInitIM(FcitxInstance* instance)
{
    utarray_init(&instance->imes, &ime_icd);
    utarray_init(&instance->availimes, &ime_icd);
    utarray_init(&instance->imeclasses, fcitx_ptr_icd);
}

FCITX_EXPORT_API
void FcitxInstanceSaveAllIM(FcitxInstance* instance)
{
    UT_array* imes = &instance->availimes;
    FcitxIM *pim;
    for (pim = (FcitxIM*)utarray_front(imes);
         pim != NULL;
         pim = (FcitxIM*)utarray_next(imes, pim)) {
        if (pim->Save) {
            pim->Save(pim->klass);
        }
    }
}

boolean FcitxInstanceCheckICFromSameApplication (FcitxInstance* instance, FcitxInputContext* rec, FcitxInputContext* ic) {
    if (rec->frontendid != ic->frontendid)
        return false;
    if (rec == ic)
        return true;
    FcitxInputContext2* rec2 = (FcitxInputContext2*) rec;
    FcitxInputContext2* ic2 = (FcitxInputContext2*) ic;
    if (rec2->imname || ic2->imname)
        return false;
    FcitxAddon **pfrontend = FcitxInstanceGetPFrontend(instance, ic->frontendid);
    if (pfrontend) {
        FcitxFrontend *frontend = (*pfrontend)->frontend;
        if (frontend->CheckICFromSameApplication &&
            frontend->CheckICFromSameApplication((*pfrontend)->addonInstance,
                                                 rec, ic)) {
            return true;
        }
    }

    return false;
}

void FcitxInstanceLoadIM(FcitxInstance* instance, FcitxAddon* addon)
{
    if (!addon)
        return;

    if (addon->type == AT_SHAREDLIBRARY) {
        char* modulePath;
        FILE *fp = FcitxXDGGetLibFile(addon->library, "r", &modulePath);
        void *handle;
        FcitxIMClass2 * imclass2;
        if (!fp) {
            free(modulePath);
            return;
        }
        fclose(fp);
        handle = dlopen(modulePath, RTLD_NOW | RTLD_NODELETE | (addon->loadLocal ? RTLD_LOCAL : RTLD_GLOBAL));
        if (!handle) {
            FcitxLog(ERROR, _("IM: open %s fail %s") , modulePath , dlerror());
            free(modulePath);
            return;
        }
        free(modulePath);

        if (!FcitxCheckABIVersion(handle, addon->name)) {
            FcitxLog(ERROR, "%s ABI Version Error", addon->name);
            dlclose(handle);
            return;
        }

        boolean isIMClass2 = false;
        imclass2 = FcitxGetSymbol(handle, addon->name, "ime2");
        if (!imclass2)
            imclass2 = FcitxGetSymbol(handle, addon->name, "ime");
        else
            isIMClass2 = true;
        if (!imclass2 || !imclass2->Create) {
            FcitxLog(ERROR, _("IM: bad im %s"), addon->name);
            dlclose(handle);
            return;
        }

        addon->isIMClass2 = isIMClass2;
        instance->currentIMAddon = addon;
        if ((addon->addonInstance = imclass2->Create(instance)) == NULL) {
            dlclose(handle);
            return;
        }
        instance->currentIMAddon = NULL;
        addon->imclass2 = imclass2;
        utarray_push_back(&instance->imeclasses, &addon);
    }
}

void FcitxInstanceRegisterEmptyIMEntry(FcitxInstance *instance,
                                       const char* name,
                                       const char* uniqueName,
                                       const char* iconName,
                                       int priority,
                                       const char *langCode,
                                       FcitxAddon* addon
                                      )
{
    UT_array* imes = &instance->availimes ;
    FcitxIM* entry = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, uniqueName);
    if (entry) {
        return;
    } else {
        utarray_extend_back(imes);
        entry = (FcitxIM*) utarray_back(imes);
    }

    if (entry == NULL)
        return;

    entry->uniqueName = strdup(uniqueName);
    entry->strName = strdup(name);
    entry->strIconName = strdup(iconName);
    entry->iPriority = priority;
    strncpy(entry->langCode, langCode, LANGCODE_LENGTH);
    entry->langCode[LANGCODE_LENGTH] = 0;
    entry->initialized = false;
    entry->owner = addon;
}

FCITX_EXPORT_API
void FcitxInstanceRegisterIM(FcitxInstance *instance,
                       void *imclass,
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
                       FcitxIMKeyBlocker KeyBlocker,
                       int priority,
                       const char *langCode
                      )
{
    FcitxIMIFace iface;
    memset(&iface, 0, sizeof(FcitxIMIFace));
    iface.Init = Init;
    iface.ResetIM = ResetIM;
    iface.DoInput = DoInput;
    iface.GetCandWords = GetCandWords;
    iface.PhraseTips = PhraseTips;
    iface.Save = Save;
    iface.ReloadConfig = ReloadConfig;
    iface.KeyBlocker = KeyBlocker;
    FcitxInstanceRegisterIMv2(instance,
                       imclass,
                       uniqueName,
                       name,
                       iconName,
                       iface,
                       priority,
                       langCode
                      );
}

FCITX_EXPORT_API
void FcitxInstanceRegisterIMv2(FcitxInstance *instance,
                       void *imclass,
                       const char* uniqueName,
                       const char* name,
                       const char* iconName,
                       FcitxIMIFace iface,
                       int priority,
                       const char *langCode
                      )
{
    if (priority <= 0)
        return ;

    if (priority == PRIORITY_MAGIC_FIRST)
        priority = 0;

    UT_array* imes = &instance->availimes ;
    FcitxIM* entry;

    entry = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, uniqueName);
    if (entry) {
        if (entry->initialized) {
            FcitxLog(ERROR, "%s already exists", uniqueName);
            return ;
        }
    } else {
        utarray_extend_back(imes);
        entry = (FcitxIM*) utarray_back(imes);
    }

    if (entry == NULL)
        return;

    if (!entry->uniqueName)
        entry->uniqueName = strdup(uniqueName);
    if (!entry->strName)
        entry->strName = strdup(name);
    if (!entry->strIconName)
        entry->strIconName = strdup(iconName);
    entry->Init = iface.Init;
    entry->ResetIM = iface.ResetIM;
    entry->DoInput = iface.DoInput;
    entry->GetCandWords = iface.GetCandWords;
    entry->PhraseTips = iface.PhraseTips;
    entry->Save = iface.Save;
    entry->ReloadConfig = iface.ReloadConfig;
    entry->KeyBlocker = iface.KeyBlocker;
    entry->UpdateSurroundingText = iface.UpdateSurroundingText;
    entry->DoReleaseInput = iface.DoReleaseInput;
    entry->OnClose = iface.OnClose;
    entry->GetSubModeName = iface.GetSubModeName;
    entry->klass = imclass;
    entry->iPriority = priority;
    if (langCode)
        strncpy(entry->langCode, langCode, LANGCODE_LENGTH);
    entry->langCode[LANGCODE_LENGTH] = 0;
    entry->initialized = true;
    entry->owner = instance->currentIMAddon;
}

CONFIG_DESC_DEFINE(GetIMConfigDesc, "inputmethod.desc")

boolean FcitxInstanceLoadAllIM(FcitxInstance* instance)
{
    FcitxStringHashSet* sset = FcitxXDGGetFiles("inputmethod", NULL, ".conf");
    FcitxStringHashSet* curs = sset;
    UT_array* addons = &instance->addons;

    while (curs) {
        FILE* fp = FcitxXDGGetFileWithPrefix("inputmethod", curs->name, "r", NULL);
        FcitxConfigFile* cfile = NULL;
        if (fp) {
            cfile = FcitxConfigParseConfigFileFp(fp, GetIMConfigDesc());
            fclose(fp);
        }

        if (cfile) {
            FcitxIMEntry* entry = fcitx_utils_malloc0(sizeof(FcitxIMEntry));
            FcitxIMEntryConfigBind(entry, cfile, GetIMConfigDesc());
            FcitxConfigBindSync(&entry->config);
            FcitxAddon *addon = FcitxAddonsGetAddonByName(&instance->addons, entry->parent);

            if (addon
                && addon->bEnabled
                && addon->category == AC_INPUTMETHOD
                && addon->registerMethod == IMRM_CONFIGFILE
               ) {
                FcitxInstanceRegisterEmptyIMEntry(instance,
                                        entry->name,
                                        entry->uniqueName,
                                        entry->iconName,
                                        entry->priority,
                                        entry->langCode,
                                        addon
                                       );
            }
            FreeIMEntry(entry);
        }
        curs = curs->hh.next;
    }

    fcitx_utils_free_string_hash_set(sset);

    /* only load those input methods that can only register by themselves */
    FcitxAddon *addon;
    for (addon = (FcitxAddon *) utarray_front(addons);
            addon != NULL;
            addon = (FcitxAddon *) utarray_next(addons, addon)) {
        /*
         * since we want to use this function to do some "reload"
         * we need to check addon->addonInstance
         */
        if (addon->bEnabled && addon->category == AC_INPUTMETHOD && !addon->addonInstance) {
            switch (addon->type) {
            case AT_SHAREDLIBRARY: {
                if (addon->registerMethod == IMRM_SELF)
                    FcitxInstanceLoadIM(instance, addon);
            }
            break;
            default:
                break;
            }
        }
    }

    if (utarray_len(&instance->availimes) <= 0) {
        FcitxLog(ERROR, _("No available Input Method"));
        return false;
    }

    instance->imLoaded = true;

    FcitxInstanceUpdateIMList(instance);

    return true;
}

static void
_NormalizeHotkeyForModifier(const FcitxHotkey* origin, FcitxHotkey* group1, FcitxHotkey* group2)
{
    int i;
    memcpy(group1, origin, sizeof(FcitxHotkey[2]));
    for (i = 0; i < 2; i ++) {
        if (FcitxHotkeyIsHotKeyModifierCombine(group1[i].sym, group1[i].state)) {
            group1[i].state &= ~FcitxHotkeyModifierToState(group1[i].sym);
        }
    }
    memcpy(group2, origin, sizeof(FcitxHotkey[2]));
    for (i = 0; i < 2; i ++) {
        if (FcitxHotkeyIsHotKeyModifierCombine(group2[i].sym, group2[i].state)) {
            group2[i].state |= FcitxHotkeyModifierToState(group2[i].sym);
        }
    }
}

/**
 * consider different situation
 * ctrl+space -> on press (reason space is not modifier
 * space -> on press
 * a -> on press
 * tab -> on press
 * ctrl -> on release
 *
 * once here's a wrong implementation that using all key without unicode as trigger on release
 * which passes press to im. (to mozc)
 */
static inline boolean IsTriggerOnRelease(FcitxKeySym sym, unsigned int state) {
    return (FcitxHotkeyIsHotKeyModifierCombine(sym, state));
}

INPUT_RETURN_VALUE _DoTrigger(FcitxInstance* instance)
{
    if (FcitxInstanceGetCurrentState(instance) == IS_INACTIVE) {
        FcitxInstanceChangeIMState(instance, instance->CurrentIC);
        FcitxInstanceShowInputSpeed(instance, false);
    } else {
        FcitxInstanceCloseIM(instance, instance->CurrentIC);
    }
    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE _DoActivate(FcitxInstance* instance)
{
    if (FcitxInstanceGetCurrentState(instance) != IS_ACTIVE) {
        FcitxInstanceEnableIM(instance, instance->CurrentIC, false);
        return IRV_DO_NOTHING;
    }
    return IRV_TO_PROCESS;
}

INPUT_RETURN_VALUE _DoDeactivate(FcitxInstance* instance)
{
    if (FcitxInstanceGetCurrentState(instance) == IS_ACTIVE) {
        FcitxInstanceCloseIM(instance, instance->CurrentIC);
        return IRV_DO_NOTHING;
    }
    return IRV_TO_PROCESS;
}

INPUT_RETURN_VALUE _DoSwitchIM(FcitxInstance* instance)
{
    if (instance->config->bIMSwitchKey
        && (instance->config->bIMSwitchIncludeInactive || FcitxInstanceGetCurrentState(instance) == IS_ACTIVE)) {
        FcitxInstanceSwitchIMByIndex(instance, instance->config->bIMSwitchIncludeInactive ? -1 : -3);
        return IRV_DO_NOTHING;
    }
    return IRV_TO_PROCESS;
}

INPUT_RETURN_VALUE _DoSwitchIMReverse(FcitxInstance* instance)
{
    if (instance->config->bIMSwitchKey
        && (instance->config->bIMSwitchIncludeInactive || FcitxInstanceGetCurrentState(instance) == IS_ACTIVE)) {
        FcitxInstanceSwitchIMByIndex(instance, instance->config->bIMSwitchIncludeInactive ? -2 : -4);
        return IRV_DO_NOTHING;
    }
    return IRV_TO_PROCESS;
}

INPUT_RETURN_VALUE _DoSwitch(FcitxInstance* instance)
{
    INPUT_RETURN_VALUE retVal = IRV_DONOT_PROCESS;
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    if (currentIM && currentIM->OnClose) {
        currentIM->OnClose(currentIM->klass, CET_ChangeByInactivate);
    }
    else {
        if (instance->config->bSendTextWhenSwitchEng) {
            if (instance->input->iCodeInputCount != 0) {
                strcpy(FcitxInputStateGetOutputString(instance->input), FcitxInputStateGetRawInputBuffer(instance->input));
                retVal = IRV_ENG;
            }
        }
    }
    FcitxInstanceChangeIMStateWithKey(instance, instance->CurrentIC, true);
    FcitxInstanceShowInputSpeed(instance, false);

    return retVal;
}

INPUT_RETURN_VALUE _Do2ndSelect(FcitxInstance* instance)
{
    FcitxInputState* input = instance->input;
    if (input->bIsInRemind || FcitxCandidateWordGetPageSize(input->candList) != 0) {
        if (!input->bIsInRemind) {
            return FcitxCandidateWordChooseByIndex(input->candList, 1);
        } else {
            strcpy(FcitxInputStateGetOutputString(input), " ");
            return IRV_COMMIT_STRING;
        }
    }
    return IRV_TO_PROCESS;
}

INPUT_RETURN_VALUE _Do3ndSelect(FcitxInstance* instance)
{
    FcitxInputState* input = instance->input;
    if (input->bIsInRemind || FcitxCandidateWordGetPageSize(input->candList) != 0) {
        if (!input->bIsInRemind) {
            return FcitxCandidateWordChooseByIndex(input->candList, 2);
        } else {
            strcpy(FcitxInputStateGetOutputString(input), "\xe3\x80\x80");
            return IRV_COMMIT_STRING;
        }
    }
    return IRV_TO_PROCESS;
}

boolean _Check2ndSelect(FcitxInstance* instance) {
    return FcitxCandidateWordGetByIndex(instance->input->candList, 1) != NULL;
}

boolean _Check3ndSelect(FcitxInstance* instance) {
    return FcitxCandidateWordGetByIndex(instance->input->candList, 2) != NULL;
}

boolean _CheckActivate(FcitxInstance* instance)
{
    return FcitxInstanceGetCurrentState(instance) != IS_ACTIVE;
}

boolean _CheckDeactivate(FcitxInstance* instance)
{
    return FcitxInstanceGetCurrentState(instance) == IS_ACTIVE;
}

boolean _CheckSwitch(FcitxInstance* instance) {
    return instance->CurrentIC->state == IS_ACTIVE
           || !instance->config->bUseExtraTriggerKeyOnlyWhenUseItToInactivate
           || ((FcitxInputContext2*)(instance->CurrentIC))->switchBySwitchKey;
}

boolean _CheckSwitchIM(FcitxInstance* instance) {
    return instance->config->bIMSwitchKey;
}

FCITX_EXPORT_API
INPUT_RETURN_VALUE FcitxInstanceProcessKey(
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
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    FcitxInputState *input = instance->input;

    FcitxGlobalConfig *fc = instance->config;

    const FcitxHotkey* hkSwitchNext1 = imSWNextKey1[fc->iIMSwitchKey];
    const FcitxHotkey* hkSwitchNext2 = imSWNextKey2[fc->iIMSwitchKey];

    const FcitxHotkey* hkSwitchPrev1 = imSWPrevKey1[fc->iIMSwitchKey];
    const FcitxHotkey* hkSwitchPrev2 = imSWPrevKey2[fc->iIMSwitchKey];

    const FcitxHotkey* hkSwitchKey1;
    const FcitxHotkey* hkSwitchKey2;

    FcitxHotkey hkCustomSwitchKey1[2];
    FcitxHotkey hkCustomSwitchKey2[2];
    FcitxHotkey hkTrigger1[2];
    FcitxHotkey hkTrigger2[2];
    FcitxHotkey hkActivate1[2];
    FcitxHotkey hkActivate2[2];
    FcitxHotkey hkInactivate1[2];
    FcitxHotkey hkInactivate2[2];
    // check config.desc
#define CUSTOM_SWITCH_KEY 19
    if ((int) fc->iSwitchKey < CUSTOM_SWITCH_KEY) {
        hkSwitchKey1 = switchKey1[fc->iSwitchKey];
        hkSwitchKey2 = switchKey2[fc->iSwitchKey];
    } else {
        _NormalizeHotkeyForModifier(fc->hkCustomSwitchKey, hkCustomSwitchKey1, hkCustomSwitchKey2);
        hkSwitchKey1 = hkCustomSwitchKey1;
        hkSwitchKey2 = hkCustomSwitchKey2;
    }
    _NormalizeHotkeyForModifier(fc->hkTrigger, hkTrigger1, hkTrigger2);
    _NormalizeHotkeyForModifier(fc->hkActivate, hkActivate1, hkActivate2);
    _NormalizeHotkeyForModifier(fc->hkInactivate, hkInactivate1, hkInactivate2);

    struct {
        KEY_RELEASED kr;
        const FcitxHotkey* hk1;
        const FcitxHotkey* hk2;
        INPUT_RETURN_VALUE (*callback)(FcitxInstance*);
        boolean (*check)(FcitxInstance*);
    } keyHandle[] = {
        {KR_2ND_SELECTKEY, fc->i2ndSelectKey, NULL, _Do2ndSelect, _Check2ndSelect}, // KR_2ND_SELECTKEY,
        {KR_3RD_SELECTKEY, fc->i3rdSelectKey, NULL, _Do3ndSelect, _Check3ndSelect}, // KR_3RD_SELECTKEY,
        {KR_SWITCH, hkSwitchKey1, hkSwitchKey2, _DoSwitch, _CheckSwitch}, // KR_SWITCH
        {KR_SWITCH_IM, hkSwitchNext1, hkSwitchNext2, _DoSwitchIM, _CheckSwitchIM}, //  KR_SWITCH_IM,
        {KR_SWITCH_IM_REVERSE, hkSwitchPrev1, hkSwitchPrev2, _DoSwitchIMReverse, _CheckSwitchIM}, // KR_SWITCH_IM_REVERSE,
        {KR_TRIGGER, hkTrigger1, hkTrigger2, _DoTrigger, NULL},
        {KR_ACTIVATE, hkActivate1, hkActivate2, _DoActivate, _CheckActivate},
        {KR_DEACTIVATE, hkInactivate1, hkInactivate2, _DoDeactivate, _CheckDeactivate}, //  KR_DEACTIVATE
    };

    if (instance->CurrentIC == NULL)
        return IRV_TO_PROCESS;

    if (event == FCITX_PRESS_KEY
        && !FcitxHotkeyIsHotKeyModifierCombine(sym, state))
    {
        if (FcitxInstanceRemoveTimeoutByFunc(instance, HideInputSpeed))
            HideInputSpeed(instance);
    }

    if (instance->CurrentIC->contextCaps & CAPACITY_PASSWORD)
        return IRV_TO_PROCESS;

    if (currentIM == NULL)
        return IRV_TO_PROCESS;

    boolean triggerOnRelease = IsTriggerOnRelease(sym, state);

#define HAVE_IM (utarray_len(&instance->imes) > 1)
#define CHECK_HOTKEY(name) ((name##1 ? FcitxHotkeyIsHotKey(sym, state, name##1) : false) || (name##2 ? FcitxHotkeyIsHotKey(sym, state, name##2) : false))

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (event == FCITX_RELEASE_KEY
        && FcitxInstanceGetCurrentState(instance) != IS_CLOSED
        && (timestamp - input->lastKeyPressedTime) < 500
        && (!input->bIsDoInputOnly)
        && HAVE_IM) {
        int i;
        for (i = 0; i < FCITX_ARRAY_SIZE(keyHandle); i ++) {
            if (retVal == IRV_TO_PROCESS
                && input->keyReleased == keyHandle[i].kr
                && CHECK_HOTKEY(keyHandle[i].hk)) {
                if (triggerOnRelease) {
                    retVal = keyHandle[i].callback(instance);
                } else {
                    retVal = IRV_DO_NOTHING;
                }
            }

            if (retVal != IRV_TO_PROCESS) {
                input->keyReleased = KR_OTHER;
                break;
            }
        }
        if (i == FCITX_ARRAY_SIZE(keyHandle)) {
            input->keyReleased = KR_OTHER;
        }
    }

#if 0
    /* Added by hubert_star AT forum.ubuntu.com.cn */
    if (event == FCITX_RELEASE_KEY && IsHotKeySimple(sym, state) &&
        retVal == IRV_TO_PROCESS)
        return IRV_DO_NOTHING;
#endif

    if (retVal == IRV_TO_PROCESS) {
        /* process key event for switch key */
        if (event == FCITX_PRESS_KEY) {
            input->lastKeyPressedTime = timestamp;
            do {
                if (!HAVE_IM) {
                    break;
                }
                int i;
                for (i = 0; i < FCITX_ARRAY_SIZE(keyHandle); i ++) {
                    if (CHECK_HOTKEY(keyHandle[i].hk) && (!keyHandle[i].check || keyHandle[i].check(instance))) {
                        if (!triggerOnRelease) {
                            retVal = keyHandle[i].callback(instance);
                        }
                        input->keyReleased = keyHandle[i].kr;
                        break;
                    }
                }
                if (i == FCITX_ARRAY_SIZE(keyHandle)) {
                    input->keyReleased = KR_OTHER;
                }
            } while(0);
        }
    }

    if (retVal == IRV_TO_PROCESS && event == FCITX_RELEASE_KEY) {
        FcitxInstanceProcessPreReleaseInputFilter(instance, sym, state, &retVal);

         if (retVal == IRV_TO_PROCESS && currentIM && currentIM->DoReleaseInput) {
            retVal = currentIM->DoReleaseInput(currentIM->klass, sym, state);
         }

        if (retVal == IRV_TO_PROCESS) {
            FcitxInstanceProcessPostReleaseInputFilter(instance, sym, state, &retVal);
        }

        if (retVal == IRV_TO_PROCESS) {
            retVal = IRV_DONOT_PROCESS;
        }
    }

    /* the key processed before this phase is very important,
     * we don't let any interrupt */
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE &&
        retVal == IRV_TO_PROCESS) {
        if (!input->bIsDoInputOnly) {
            FcitxInstanceProcessPreInputFilter(instance, sym, state, &retVal);
        }

        if (retVal == IRV_TO_PROCESS) {
            if (!FcitxHotkeyIsHotKey(sym, state, imSWNextKey1[fc->iIMSwitchKey]) && currentIM) {
                retVal = currentIM->DoInput(currentIM->klass, sym, state);
            }
        }

        /* check choose key first, because it might trigger update candidates */
        if (!input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
            int index = FcitxCandidateWordCheckChooseKey(input->candList,
                                                         sym, state);
            if (index >= 0)
                retVal = FcitxCandidateWordChooseByIndex(input->candList,
                                                         index);
        }
    }

    return FcitxInstanceDoInputCallback(instance, retVal, event, timestamp,
                                        sym, state);
}


FCITX_EXPORT_API
void FcitxInstanceChooseCandidateByIndex(
    FcitxInstance* instance,
    int index)
{
    if (!(FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE && index < 10)) {
        return;
    }
    FcitxInputState *input = instance->input;
    INPUT_RETURN_VALUE retVal = FcitxCandidateWordChooseByIndex(input->candList, index);
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE && currentIM && (retVal & IRV_FLAG_UPDATE_CANDIDATE_WORDS)) {
        if (currentIM->GetCandWords) {
            FcitxInstanceCleanInputWindow(instance);
            retVal = currentIM->GetCandWords(currentIM->klass);
            FcitxInstanceProcessUpdateCandidates(instance);
        }
    }
    FcitxInstanceProcessInputReturnValue(instance, retVal);
}


FCITX_EXPORT_API
INPUT_RETURN_VALUE FcitxInstanceDoInputCallback(
    FcitxInstance* instance,
    INPUT_RETURN_VALUE retVal,
    FcitxKeyEventType event,
    long unsigned int timestamp,
    FcitxKeySym sym,
    unsigned int state)
{
    FCITX_UNUSED(event);
    FCITX_UNUSED(timestamp);
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    FcitxInputState *input = instance->input;

    FcitxGlobalConfig *fc = instance->config;

    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE && currentIM &&
        (retVal & IRV_FLAG_UPDATE_CANDIDATE_WORDS)) {
        if (currentIM->GetCandWords) {
            FcitxInstanceCleanInputWindow(instance);
            retVal = currentIM->GetCandWords(currentIM->klass);
            FcitxInstanceProcessUpdateCandidates(instance);
        }
    }

    /*
     * since all candidate word are cached in candList, so we don't need to trigger
     * GetCandWords after go for another page, simply update input window is ok.
     */
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE &&
        !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        if (FcitxHotkeyIsHotKey(sym, state,
                                FcitxConfigPrevPageKey(instance, fc))) {
            if (FcitxCandidateWordGoPrevPage(input->candList))
                retVal = IRV_DISPLAY_CANDWORDS;
        } else if (FcitxHotkeyIsHotKey(sym, state,
                                       FcitxConfigNextPageKey(instance, fc))) {
            if (FcitxCandidateWordGoNextPage(input->candList))
                retVal = IRV_DISPLAY_CANDWORDS;
        }
    }

    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE &&
        !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        FcitxInstanceProcessPostInputFilter(instance, sym, state, &retVal);
    }

    if (retVal == IRV_TO_PROCESS) {
        retVal = FcitxInstanceProcessHotkey(instance, sym, state);
    }

    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE &&
        !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        if (currentIM && currentIM->KeyBlocker)
            retVal = currentIM->KeyBlocker(currentIM->klass, sym, state);
        else
            retVal = FcitxStandardKeyBlocker(input, sym, state);
    }

    FcitxInstanceProcessInputReturnValue(instance, retVal);

    return retVal;
}

FCITX_EXPORT_API
void FcitxInstanceProcessInputReturnValue(
    FcitxInstance* instance,
    INPUT_RETURN_VALUE retVal
)
{
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    FcitxInputState *input = instance->input;
    FcitxGlobalConfig *fc = instance->config;

    if (retVal & IRV_FLAG_PENDING_COMMIT_STRING) {
        FcitxInstanceCommitString(instance, instance->CurrentIC, FcitxInputStateGetOutputString(input));
    }

    if (retVal & IRV_FLAG_DO_PHRASE_TIPS) {
        FcitxInstanceCleanInputWindow(instance);
        if (fc->bPhraseTips && currentIM && currentIM->PhraseTips)
            FcitxInstanceDoPhraseTips(instance);
        FcitxUIUpdateInputWindow(instance);

        FcitxInstanceResetInput(instance);
        input->lastIsSingleHZ = 0;
    }

    if (retVal & IRV_FLAG_RESET_INPUT) {
        FcitxInstanceResetInput(instance);
        FcitxUICloseInputWindow(instance);
    }

    if (retVal & IRV_FLAG_DISPLAY_LAST) {
        FcitxInstanceCleanInputWindow(instance);
        char str[2] = {FcitxInputStateGetRawInputBuffer(input)[0], '\0'};
        FcitxMessagesAddMessageStringsAtLast(input->msgAuxUp, MSG_INPUT, str);
        FcitxMessagesAddMessageStringsAtLast(
            input->msgAuxDown, MSG_TIPS,
            FcitxInputStateGetLastCommitString(input));
    }

    if (retVal & IRV_FLAG_UPDATE_INPUT_WINDOW)
        FcitxUIUpdateInputWindow(instance);
}

FCITX_EXPORT_API
void FcitxInstanceForwardKey(FcitxInstance* instance, FcitxInputContext *ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
{
    if (ic == NULL)
        return;
    FcitxAddon **pfrontend = FcitxInstanceGetPFrontend(instance, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend *frontend = (*pfrontend)->frontend;
    frontend->ForwardKey((*pfrontend)->addonInstance, ic, event, sym, state);
}

FCITX_EXPORT_API
void FcitxInstanceSwitchIM(FcitxInstance* instance, int index)
{
    FcitxInstanceSwitchIMByIndex(instance, index);
}

FCITX_EXPORT_API
void FcitxInstanceSwitchIMByName(FcitxInstance* instance, const char* name)
{
    FcitxIM* im = FcitxInstanceGetIMFromIMList(instance, IMAS_Enable, name);
    if (im) {
        if (FcitxInstanceGetCurrentIC(instance)) {
            // Ignore the request if current IM is already same with the name
            // Add such check here is mainly for GitHub Issue #203.
            // Another possibility is to add it to FcitxInstanceSwitchIMInternal,
            // but that part of code is too hack and some initialization may
            // relies on it.
            // This function is only used in UI and DBus remote control,
            // Thus it is safe to change it here.
            FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
            if (currentIM && strcmp(currentIM->strName, name) == 0) {
                return;
            }

            int idx = FcitxInstanceGetIMIndexByName(instance, name);
            if (idx < 0)
                return;
            FcitxInstanceSwitchIMByIndex(instance, idx);
        } else {
            FcitxInstanceSetDelayedIM(instance, name);
        }
    }
}

FCITX_EXPORT_API
void FcitxInstanceSwitchIMByIndex(FcitxInstance* instance, int index)
{
    UT_array* imes = &instance->imes;
    const int iIMCount = utarray_len(imes);
    /* less than -2, invalid, set to zero
     * -2 scroll back
     * -1 scroll forward
     * 0~positive select
     */
    if (index < -4 || index >= iIMCount)
        return;

    boolean skipZero = (index == -3 || index == -4);
    if (index == -2 || index == -4) {
        if (instance->iIMIndex > 0) {
            index = instance->iIMIndex -1;
            if (index == 0 && skipZero)
                index = iIMCount - 1;
        }
        else
            index = iIMCount - 1;
    } else if (index == -1 || index == -3) {
        if (instance->iIMIndex >= (iIMCount - 1))
            index = skipZero ? 1 : 0;
        else
            index = instance->iIMIndex + 1;
    }
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(instance);
    if (index == 0)
        FcitxInstanceCloseIM(instance, ic);
    else {
        if (ic)
            FcitxInstanceSetLocalIMName(instance, ic, NULL);
        FcitxInstanceSwitchIMInternal(instance, index, true, true, true);
        FcitxInstanceShowInputSpeed(instance, false);

        if (FcitxInstanceGetCurrentState(instance) != IS_ACTIVE) {
            FcitxInstanceEnableIM(instance, FcitxInstanceGetCurrentIC(instance), false);
        }
    }
}

FCITX_EXPORT_API
void FcitxInstanceUnregisterIM(FcitxInstance* instance, const char* name)
{
    FcitxIM* im = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, name);
    if (!im)
        return;

    unsigned int index = utarray_eltidx(&instance->availimes, im);
    utarray_erase(&instance->availimes, index, 1);
}

void
FcitxInstanceSwitchIMInternal(FcitxInstance* instance, int index,
                              boolean skipZero, boolean updateGlobal, boolean userSwitchIM)
{
    UT_array* imes = &instance->imes;
    const int iIMCount = utarray_len(imes);

    FcitxIM* lastIM, *newIM;

    /* set lastIM */
    lastIM = fcitx_array_eltptr(imes, instance->iIMIndex);

    if (lastIM) {

        if (userSwitchIM) {
            if (lastIM->OnClose) {
                lastIM->OnClose(lastIM->klass, CET_SwitchIM);
            }
        }
        if (lastIM->Save) {
            lastIM->Save(lastIM->klass);
        }
    }

    FcitxInstanceCleanInputWindow(instance);
    FcitxInstanceResetInput(instance);
    FcitxUIUpdateInputWindow(instance);

    /* update instance->iIMIndex start */
    if (index >= iIMCount)
        instance->iIMIndex = iIMCount - 1;
    else if (index < 0)
        instance->iIMIndex = 0;
    else if (index >= 0)
        instance->iIMIndex = index;

    if (skipZero && instance->iIMIndex == 0) {
        instance->iIMIndex = 1;
    }
    if (iIMCount <= 1) {
        instance->iIMIndex = 0;
    }
    /* update instance->iIMIndex end */

    /* set newIM */
    newIM = fcitx_array_eltptr(imes, instance->iIMIndex);

    /* lazy load */
    if (newIM && !newIM->initialized) {
        char* name = strdup(newIM->uniqueName);
        FcitxInstanceLoadIM(instance, newIM->owner);
        FcitxIM* im = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, name);
        if (!im->initialized) {
            im->initialized = true;
            unsigned int index = utarray_eltidx(&instance->availimes, im);
            utarray_erase(&instance->availimes, index, 1);
        }

        FcitxInstanceUpdateIMList(instance);
        instance->iIMIndex = FcitxInstanceGetIMIndexByName(instance, name);
        newIM = fcitx_array_eltptr(imes, instance->iIMIndex);
        free(name);
    }

    if (newIM && newIM->Init) {
        FcitxInstanceResetContext(instance, FCF_ResetOnInputMethodChange);
        FcitxInstanceSetContext(instance, CONTEXT_IM_LANGUAGE, newIM->langCode);
        newIM->Init(newIM->klass);
    }

    if (newIM && updateGlobal) {
        if (fcitx_utils_strcmp0(instance->globalIMName, newIM->uniqueName) != 0) {
            fcitx_utils_free(instance->globalIMName);
            instance->globalIMName = strdup(newIM->uniqueName);
            FcitxProfileSave(instance->profile);
        }
    }

    FcitxInstanceResetInput(instance);
    FcitxInstanceProcessIMChangedHook(instance);
}

/**
 * 重置输入状态
 */
FCITX_EXPORT_API
void FcitxInstanceResetInput(FcitxInstance* instance)
{
    FcitxInputState *input = instance->input;
    FcitxCandidateWordReset(input->candList);
    input->iCursorPos = 0;
    input->iClientCursorPos = 0;

    FcitxInputStateGetRawInputBuffer(input)[0] = '\0';
    input->iCodeInputCount = 0;

    input->bIsDoInputOnly = false;
    input->bIsInRemind = false;

    UT_array* ims = &instance->imes;

    FcitxIM *currentIM = fcitx_array_eltptr(ims, instance->iIMIndex);

    if (currentIM && currentIM->ResetIM)
        currentIM->ResetIM(currentIM->klass);

    FcitxInstanceProcessResetInputHook(instance);
}

FCITX_EXPORT_API
void FcitxInstanceSendCloseEvent(struct _FcitxInstance* instance, FcitxIMCloseEventType closeEvent)
{
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    if (currentIM && currentIM->OnClose) {
        currentIM->OnClose(currentIM->klass, closeEvent);
    }
}

void FcitxInstanceDoPhraseTips(FcitxInstance* instance)
{
    UT_array *ims = &instance->imes;
    FcitxIM *currentIM = fcitx_array_eltptr(ims, instance->iIMIndex);
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

    if (!input->iCodeInputCount)
        retVal = IRV_DONOT_PROCESS;
    else {
        FcitxInstanceCleanInputWindow(instance);
        strcpy(FcitxInputStateGetOutputString(input), FcitxInputStateGetRawInputBuffer(input));
        retVal = IRV_ENG;
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
    FcitxUIUpdateStatus(instance, "remind");
    return IRV_DONOT_PROCESS;
}

INPUT_RETURN_VALUE ImProcessReload(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    FcitxInstanceReloadConfig(instance);
    return IRV_DO_NOTHING;
}

void FcitxInstanceReloadAddon(FcitxInstance* instance)
{
    FcitxAddon* addonHead = FcitxAddonsLoadInternal(&instance->addons, true);
    FcitxInstanceFillAddonOwner(instance, addonHead);
    FcitxInstanceResolveAddonDependencyInternal(instance, addonHead);
    FcitxInstanceLoadAllIM(instance);
}

FCITX_EXPORT_API
void FcitxInstanceReloadAddonConfig(FcitxInstance *instance, const char* addonname)
{
    if (!addonname)
        return;

    if (strcmp(addonname, "global") == 0) {
        if (!FcitxGlobalConfigLoad(instance->config))
            FcitxInstanceEnd(instance);

        FcitxCandidateWordSetPageSize(instance->input->candList, instance->config->iMaxCandWord);
    } else if (strcmp(addonname, "profile") == 0) {
        if (!FcitxProfileLoad(instance->profile, instance))
            FcitxInstanceEnd(instance);
    } else if (strcmp(addonname, "ui") == 0) {
        if (instance->ui && instance->ui->ui->ReloadConfig)
            instance->ui->ui->ReloadConfig(instance->ui->addonInstance);
    } else if (strcmp(addonname, "addon") == 0) {
        instance->eventflag |= FEF_RELOAD_ADDON;
    } else {
        do {
            FcitxIM* im;
            im = FcitxInstanceGetIMByName(instance, addonname);
            if (im && im->ReloadConfig) {
                im->ReloadConfig(im->klass);
                break;
            }

            FcitxAddon *addon = FcitxAddonsGetAddonByName(&instance->addons, addonname);
            if (!addon || !addon->bEnabled || !addon->addonInstance)
                break;
            switch (addon->category) {
                case AC_MODULE:
                    if (addon->module->ReloadConfig)
                        addon->module->ReloadConfig(addon->addonInstance);
                    break;
                case AC_UI:
                    if (addon->ui->ReloadConfig)
                        addon->ui->ReloadConfig(addon->addonInstance);
                    break;
                case AC_FRONTEND:
                    if (addon->frontend->ReloadConfig)
                        addon->frontend->ReloadConfig(addon->addonInstance);
                    break;
                case AC_INPUTMETHOD:
                    /* imclass and imclass2 are in same union, only check one of them */
                    if (addon->imclass) {
                        for (im = (FcitxIM*) utarray_front(&instance->availimes);
                             im != NULL;
                             im = (FcitxIM*) utarray_next(&instance->availimes, im)) {
                            if (im->owner == addon && im->ReloadConfig) {
                                im->ReloadConfig(im->klass);
                            }
                        }

                        if (addon->isIMClass2 && addon->imclass2->ReloadConfig) {
                            addon->imclass2->ReloadConfig(addon->addonInstance);
                        }
                    }
                    break;
                default:
                    break;
            }
        } while(0);
    }
}

FCITX_EXPORT_API
void FcitxInstanceReloadConfig(FcitxInstance *instance)
{
    if (!FcitxGlobalConfigLoad(instance->config))
        FcitxInstanceEnd(instance);

    if (!FcitxProfileLoad(instance->profile, instance))
        FcitxInstanceEnd(instance);

    FcitxCandidateWordSetPageSize(instance->input->candList, instance->config->iMaxCandWord);

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

    for (addon = (FcitxAddon *) utarray_front(addons);
         addon != NULL;
         addon = (FcitxAddon *) utarray_next(addons, addon)) {
        if (addon->category == AC_INPUTMETHOD &&
                addon->bEnabled &&
                addon->addonInstance &&
                addon->isIMClass2) {
            if (addon->imclass2->ReloadConfig)
                addon->imclass2->ReloadConfig(addon->addonInstance);
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
    
    instance->eventflag |= FEF_RELOAD_ADDON;
}

static inline void FcitxInstanceSetICStatus(FcitxInstance* instance, FcitxInputContext* ic, FcitxContextState state)
{
    if (ic->state != state) {
        ic->state = state;
        FcitxInstanceProcessICStateChangedHook(instance, ic);
    }
}

FCITX_EXPORT_API
void FcitxInstanceSetLocalIMName(FcitxInstance* instance, FcitxInputContext* ic, const char* imname)
{
    FcitxInputContext2* ic2 = (FcitxInputContext2*) ic;
    if (ic2->imname) {
        free(ic2->imname);
        ic2->imname = NULL;
    }

    if (imname)
        ic2->imname = strdup(imname);

    if (ic == FcitxInstanceGetCurrentIC(instance)) {
        FcitxInstanceUpdateCurrentIM(instance, false, true);
    }
}

/* the "force" to this function is used when the list changed and index is not valid */
boolean FcitxInstanceUpdateCurrentIM(FcitxInstance* instance, boolean force, boolean userSwitchIM) {
    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(instance);
    if (!ic && !force)
        return false;
    FcitxInputContext2* ic2 = (FcitxInputContext2*) ic;
    int globalIndex = FcitxInstanceGetIMIndexByName(instance, instance->globalIMName);
    boolean forceSwtich = force;
    boolean updateGlobal = false;
    /* global index is not valid, that's why we need to fix it. */
    if (globalIndex <= 0) {
        UT_array *ime = &instance->imes;
        FcitxIM *im = (FcitxIM*)utarray_eltptr(ime, 1);
        if (im) {
            fcitx_utils_string_swap(&instance->globalIMName, im->uniqueName);
            globalIndex = 1;
            forceSwtich = true;
            updateGlobal = true;
        }
    }
    int targetIMIndex = 0;
    boolean skipZero = false;

    if (ic2 && ic2->imname) {
        FcitxIM* im = FcitxInstanceGetIMFromIMList(instance, IMAS_Enable, ic2->imname);
        if (!im) {
            free(ic2->imname);
            ic2->imname = NULL;
        }
    }

    if (ic && ic->state != IS_ACTIVE) {
        targetIMIndex = 0;
    }
    else {
        if (ic2 && ic2->imname)
            targetIMIndex = FcitxInstanceGetIMIndexByName(instance, ic2->imname);
        else
            targetIMIndex = globalIndex;
        skipZero = true;
    }

    if (forceSwtich || targetIMIndex != instance->iIMIndex) {
        FcitxInstanceSwitchIMInternal(instance, targetIMIndex, skipZero, updateGlobal, userSwitchIM);
        return true;
    }
    else
        return false;
}

FCITX_EXPORT_API
void FcitxInstanceEnableIM(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState)
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
                flag = FcitxInstanceCheckICFromSameApplication(instance, rec, ic);
            }

            if (flag && (rec == ic || !(rec->contextCaps & CAPACITY_CLIENT_SIDE_CONTROL_STATE)))
                FcitxInstanceEnableIMInternal(instance, rec, keepState);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        FcitxInstanceEnableIMInternal(instance, ic, keepState);
        break;
    }

    FcitxInstanceUpdateCurrentIM(instance, false, false);
    instance->input->keyReleased = KR_OTHER;
}


void FcitxInstanceEnableIMInternal(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState)
{
    if (ic == NULL)
        return ;
    FcitxAddon **pfrontend = FcitxInstanceGetPFrontend(instance, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    FcitxContextState oldstate = ic->state;
    FcitxInstanceSetICStatus(instance, ic, IS_ACTIVE);
    if (oldstate == IS_CLOSED)
        frontend->EnableIM((*pfrontend)->addonInstance, ic);

    if (ic == instance->CurrentIC) {
        if (oldstate == IS_CLOSED)
            FcitxUIOnTriggerOn(instance);
        if (!keepState)
            FcitxInstanceResetInput(instance);
    }
}

FCITX_EXPORT_API
void FcitxInstanceCloseIM(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;

    if (!(ic->contextCaps & CAPACITY_CLIENT_SIDE_CONTROL_STATE)) {
        if (ic->state == IS_ACTIVE)
            FcitxInstanceChangeIMState(instance, ic);
        return;
    }

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
                flag = FcitxInstanceCheckICFromSameApplication(instance, rec, ic);
            }

            if (flag && (rec == ic || !(rec->contextCaps & CAPACITY_CLIENT_SIDE_CONTROL_STATE)))
                FcitxInstanceCloseIMInternal(instance, rec);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        FcitxInstanceCloseIMInternal(instance, ic);
        break;
    }
}

void FcitxInstanceCloseIMInternal(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return ;
    FcitxAddon **pfrontend = FcitxInstanceGetPFrontend(instance, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    FcitxInstanceSetICStatus(instance, ic, IS_CLOSED);
    frontend->CloseIM((*pfrontend)->addonInstance, ic);

    if (ic == instance->CurrentIC) {
        FcitxUIOnTriggerOff(instance);
        FcitxUICloseInputWindow(instance);
        FcitxInstanceResetInput(instance);
    }
}

void FcitxInstanceChangeIMStateWithKey(FcitxInstance* instance, FcitxInputContext* ic, boolean withSwitchKey)
{
    if (ic == NULL)
        return;
    FcitxContextState objectState;
    if (ic->state == IS_INACTIVE)
        objectState = IS_ACTIVE;
    else
        objectState = IS_INACTIVE;

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
                flag = FcitxInstanceCheckICFromSameApplication(instance, rec, ic);
            }

            if (flag && (rec == ic || !(rec->contextCaps & CAPACITY_CLIENT_SIDE_CONTROL_STATE)))
                FcitxInstanceChangeIMStateInternal(instance, rec, objectState, withSwitchKey);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        FcitxInstanceChangeIMStateInternal(instance, ic, objectState, withSwitchKey);
        break;
    }

    FcitxInstanceUpdateCurrentIM(instance, false, !withSwitchKey);
}

/**
 * 更改输入法状态
 *
 * @param _connect_id
 */
FCITX_EXPORT_API
void FcitxInstanceChangeIMState(FcitxInstance* instance, FcitxInputContext* ic)
{
    FcitxInstanceChangeIMStateWithKey(instance, ic, false);
}

void FcitxInstanceChangeIMStateInternal(FcitxInstance* instance, FcitxInputContext* ic, FcitxContextState objectState, boolean withSwitchKey)
{
    if (!ic)
        return;
    if (ic->state == objectState)
        return;
    FcitxInstanceSetICStatus(instance, ic, objectState);
    FcitxInputContext2* ic2 = (FcitxInputContext2*) ic;
    ic2->switchBySwitchKey = withSwitchKey;
    if (ic == instance->CurrentIC) {
        if (objectState != IS_ACTIVE) {
            FcitxUICloseInputWindow(instance);
        }
    }
}

void FcitxInstanceInitIMMenu(FcitxInstance* instance)
{
    FcitxMenuInit(&instance->imMenu);
    instance->imMenu.candStatusBind = NULL;
    instance->imMenu.name = strdup(_("Input Method"));

    instance->imMenu.UpdateMenu = UpdateIMMenuItem;
    instance->imMenu.MenuAction = IMMenuAction;
    instance->imMenu.priv = instance;
    instance->imMenu.isSubMenu = false;
}

boolean IMMenuAction(FcitxUIMenu *menu, int index)
{
    FcitxInstance* instance = (FcitxInstance*) menu->priv;

    FcitxIM* im = FcitxInstanceGetIMByIndex(instance, index);
    // this contains delay support, so we don't use switch im by index here.
    if (im) {
        FcitxInstanceSwitchIMByName(instance, im->uniqueName);
    }
    return true;
}

void UpdateIMMenuItem(FcitxUIMenu *menu)
{
    FcitxInstance* instance = (FcitxInstance*) menu->priv;
    FcitxMenuClear(menu);

    FcitxIM* pim;
    UT_array* imes = &instance->imes;
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim))
        FcitxMenuAddMenuItem(&instance->imMenu, pim->strName, MENUTYPE_SIMPLE, NULL);

    menu->mark = instance->iIMIndex;
}

void HideInputSpeed(void* arg)
{
    FcitxInstance *instance = arg;
    FcitxInputState* input = instance->input;
    if (FcitxMessagesIsMessageChanged(input->msgAuxUp)
        || FcitxMessagesIsMessageChanged(input->msgAuxDown)
        || FcitxMessagesGetMessageCount(input->msgPreedit)
        || FcitxMessagesGetMessageCount(input->msgClientPreedit)
        || FcitxCandidateWordGetListSize(input->candList))
        return;
    FcitxUICloseInputWindow(instance);
}

void FcitxInstanceShowInputSpeed(FcitxInstance* instance, boolean force)
{
    FcitxInputState* input = instance->input;

    if (!instance->initialized)
        return;

    if (!force) {
        if (!instance->config->bShowInputWindowTriggering)
            return;

        if (FcitxInstanceGetCurrentState(instance) != IS_ACTIVE && instance->config->bShowInputWindowOnlyWhenActive)
            return;
    }

    if (FcitxMessagesGetMessageCount(input->msgAuxUp)
        || FcitxMessagesGetMessageCount(input->msgAuxDown)
        || FcitxMessagesGetMessageCount(input->msgPreedit)
        || FcitxMessagesGetMessageCount(input->msgClientPreedit)
        || FcitxCandidateWordGetListSize(input->candList))
        return;

    input->bShowCursor = false;

    FcitxInstanceCleanInputWindow(instance);

    FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
    if (!im)
        return;

    if (instance->config->bShowVersion) {
        FcitxMessagesAddMessageStringsAtLast(input->msgAuxUp, MSG_TIPS,
                                             "FCITX " VERSION " ");
    }
    if (im) {
        FcitxMessagesAddMessageStringsAtLast(input->msgAuxUp, MSG_TIPS,
                                             im->strName);
        const char* subModeName = im->GetSubModeName ? im->GetSubModeName(im->klass) : NULL;
        if (subModeName) {
            FcitxMessagesAddMessageStringsAtLast(input->msgAuxUp, MSG_TIPS, " - ",
                                                 subModeName);
        }
    }

    //显示打字速度
    if (instance->config->bShowUserSpeed) {
        double          timePassed;

        timePassed = instance->totaltime + difftime(time(NULL), instance->timeStart);
        if (((int) timePassed) == 0)
            timePassed = 1.0;

        FcitxMessagesSetMessageCount(input->msgAuxDown, 0);
        FcitxMessagesAddMessageStringsAtLast(input->msgAuxDown, MSG_OTHER,
                                             _("Input Speed: "));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_CODE, "%d", (int)(instance->iHZInputed * 60 / timePassed));
        FcitxMessagesAddMessageStringsAtLast(input->msgAuxDown, MSG_OTHER,
                                             _("/min  Time Used: "));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_CODE, "%d", (int) timePassed / 60);
        FcitxMessagesAddMessageStringsAtLast(input->msgAuxDown, MSG_OTHER,
                                             _("min Num of Characters: "));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_CODE, "%u", instance->iHZInputed);
    }

    FcitxUIUpdateInputWindow(instance);

    if (!FcitxInstanceCheckTimeoutByFunc(instance, HideInputSpeed))
        FcitxInstanceAddTimeout(instance, 1000, HideInputSpeed, instance);
}


INPUT_RETURN_VALUE ImProcessSaveAll(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    FcitxInstanceSaveAllIM(instance);
    return IRV_DO_NOTHING;
}


INPUT_RETURN_VALUE ImSwitchEmbeddedPreedit(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    instance->profile->bUsePreedit = !instance->profile->bUsePreedit;
    FcitxProfileSave(instance->profile);
    FcitxUIUpdateInputWindow(instance);
    return IRV_DO_NOTHING;
}

FCITX_EXPORT_API
void FcitxInstanceCleanInputWindow(FcitxInstance *instance)
{
    FcitxInstanceCleanInputWindowUp(instance);
    FcitxInstanceCleanInputWindowDown(instance);
}

FCITX_EXPORT_API
void FcitxInstanceCleanInputWindowUp(FcitxInstance *instance)
{
    FcitxInputState* input = instance->input;
    FcitxMessagesSetMessageCount(input->msgAuxUp, 0);
    FcitxMessagesSetMessageCount(input->msgPreedit, 0);
    FcitxMessagesSetMessageCount(input->msgClientPreedit, 0);
}

FCITX_EXPORT_API
void FcitxInstanceCleanInputWindowDown(FcitxInstance* instance)
{
    FcitxInputState* input = instance->input;
    FcitxCandidateWordReset(input->candList);
    FcitxMessagesSetMessageCount(input->msgAuxDown, 0);
}

FCITX_EXPORT_API
int FcitxHotkeyCheckChooseKey(FcitxKeySym sym, unsigned int state, const char* strChoose)
{
    return FcitxHotkeyCheckChooseKeyAndModifier(sym, state, strChoose, FcitxKeyState_None);
}

FCITX_EXPORT_API
int FcitxHotkeyCheckChooseKeyAndModifier(FcitxKeySym sym, unsigned int state,
                                         const char* strChoose, int candState)
{
    if (state != (unsigned)candState)
        return -1;

    char *p = strchr(strChoose, FcitxHotkeyPadToMain(sym));
    return p ? p - strChoose : -1;
}

FCITX_EXPORT_API
FcitxIM* FcitxInstanceGetCurrentIM(FcitxInstance* instance)
{
    return fcitx_array_eltptr(&instance->imes, instance->iIMIndex);
}

FCITX_EXPORT_API
FcitxIM* FcitxInstanceGetIMByIndex(FcitxInstance* instance, int index)
{
    /* utarray helps us to check the overflow */
    return fcitx_array_eltptr(&instance->imes, index);
}

FCITX_EXPORT_API
int FcitxInstanceGetIMIndexByName(FcitxInstance* instance, const char* imName)
{
    UT_array* imes = &instance->imes;
    FcitxIM *pim = FcitxInstanceGetIMByName(instance, imName);
    if (!pim)
        return -1;
    else
        return utarray_eltidx(imes, pim);
}

FCITX_EXPORT_API
FcitxIM* FcitxInstanceGetIMByName(FcitxInstance* instance, const char* imName)
{
    UT_array* imes = &instance->imes;
    FcitxIM *pim;
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim)) {
        if (strcmp(imName, pim->uniqueName) == 0) {
            return pim;
        }
    }
    return NULL;
}


FCITX_EXPORT_API
void FcitxInstanceNotifyUpdateSurroundingText(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (!ic)
        return;
    if (ic != instance->CurrentIC)
        return;

    FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
    if (!im)
        return;

    if (im->UpdateSurroundingText)
        im->UpdateSurroundingText(im->klass);
}

void UnusedIMItemFreeAll(UnusedIMItem* item)
{
    UnusedIMItem *cur;
    while (item) {
        cur = item;
        HASH_DEL(item, cur);
        fcitx_utils_free(cur->name);
        free(cur);
    }
}

static inline boolean MatchLanguage(const char* lang, const char* cur) {
    /* here we hard code some for chinese */
    if (strcmp(cur, "zh_HK") == 0) {
        cur = "zh_TW";
    }
    if (strcmp(cur, "en_HK") == 0) {
        cur = "zh_TW";
    }
    if (strcmp(lang, "zh_HK") == 0) {
        lang = "zh_TW";
    }

    if (strncmp(cur, "zh", 2) == 0 && strlen(cur) == 5) {
        return strcmp(lang, cur) == 0;
    }

    return strncmp(lang, cur, 2) == 0;
}

FCITX_EXPORT_API
void FcitxInstanceUpdateIMList(FcitxInstance* instance)
{
    if (!instance->imLoaded)
        return;

    UT_array* imList = fcitx_utils_split_string(instance->profile->imList, ',');
    utarray_sort(&instance->availimes, IMPriorityCmp);
    utarray_clear(&instance->imes);
    UnusedIMItemFreeAll(instance->unusedItem);
    instance->unusedItem = NULL;

    boolean imListIsEmpty = utarray_len(imList) == 0;


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
            ime = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, str);
            boolean status = (strcmp(pos, "True") == 0);
            if (status && ime)
                utarray_push_back(&instance->imes, ime);

            if (!ime) {
                UnusedIMItem* item;
                HASH_FIND_STR(instance->unusedItem, str, item);
                if (!item) {
                    item = fcitx_utils_new(UnusedIMItem);
                    item->name = strdup(str);
                    item->status = status;
                    HASH_ADD_KEYPTR(hh, instance->unusedItem, item->name, strlen(item->name), item);
                }
            }
        }
    }

    char* lang = fcitx_utils_get_current_langcode();
    for (ime = (FcitxIM*) utarray_front(&instance->availimes);
            ime != NULL;
            ime = (FcitxIM*) utarray_next(&instance->availimes, ime)) {
        if (!IMIsInIMNameList(imList, ime)) {
            /* ok, make all im priority larger than 100 disable by default */
            if (ime->iPriority == 0)
                utarray_push_front(&instance->imes, ime);
            else if (ime->iPriority < PRIORITY_DISABLE) {
                /*
                 * here, we use such logic, if user start fcitx for first time (or doing reset)
                 * then match everything by language, otherwise user might just install something,
                 * then add something else.
                 */
                if (!imListIsEmpty || MatchLanguage(ime->langCode, lang)) {
                    utarray_push_back(&instance->imes, ime);
                }
            }
        }
    }
    free(lang);

    utarray_free(imList);

    FcitxInstanceUpdateCurrentIM(instance, true, false);
    FcitxInstanceProcessUpdateIMListHook(instance);

    if (instance->globalIMName)
        FcitxProfileSave(instance->profile);
}

FCITX_EXPORT_API
FcitxIM* FcitxInstanceGetIMFromIMList(FcitxInstance* instance, FcitxIMAvailableStatus status, const char* name)
{
    UT_array* imes;
    if (status == IMAS_Enable)
        imes = &instance->imes;
    else
        imes = &instance->availimes;
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

void FreeIMEntry(FcitxIMEntry* entry)
{
    if (!entry)
        return ;
    FcitxConfigFreeConfigFile(entry->config.configFile);
    free(entry->name);
    free(entry->iconName);
    free(entry->langCode);
    free(entry->uniqueName);
    free(entry->parent);
    free(entry);
}

FCITX_EXPORT_API
INPUT_RETURN_VALUE FcitxStandardKeyBlocker(FcitxInputState* input, FcitxKeySym key, unsigned int state)
{
    if ((FcitxInputStateGetRawInputBufferSize(input) != 0
         || FcitxMessagesGetMessageCount(input->msgPreedit)
         || FcitxMessagesGetMessageCount(input->msgClientPreedit)
         || FcitxCandidateWordGetListSize(input->candList))
        && (FcitxHotkeyIsHotKeySimple(key, state)
        || FcitxHotkeyIsHotkeyCursorMove(key, state)
        || FcitxHotkeyIsHotKey(key, state, FCITX_SHIFT_SPACE)
        || FcitxHotkeyIsHotKey(key, state, FCITX_TAB)
        || FcitxHotkeyIsHotKey(key, state, FCITX_SHIFT_ENTER)
        ))
        return IRV_DO_NOTHING;
    else
        return IRV_TO_PROCESS;
}

FCITX_EXPORT_API
void FcitxInstanceShowCurrentIMInfo(FcitxInstance* instance)
{
    FcitxInstanceShowInputSpeed(instance, true);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
