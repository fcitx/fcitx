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
 * @brief  Process Keyboard Event and Input Method
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

static const char* GetStateName(INPUT_RETURN_VALUE retVal);
static const UT_icd ime_icd = {sizeof(FcitxIM), NULL , NULL, NULL};
static const UT_icd imclass_icd = {sizeof(FcitxAddon*), NULL , NULL, NULL};
static int IMPriorityCmp(const void *a, const void *b);
static boolean IMMenuAction(FcitxUIMenu* menu, int index);
static void UpdateIMMenuItem(FcitxUIMenu *menu);
static void FcitxInstanceEnableIMInternal(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState);
static void FcitxInstanceCloseIMInternal(FcitxInstance* instance, FcitxInputContext* ic);
static void FcitxInstanceChangeIMStateInternal(FcitxInstance* instance, FcitxInputContext* ic, FcitxContextState objectState);
static void FreeIMEntry(FcitxIMEntry* entry);
static INPUT_RETURN_VALUE FcitxStandardKeyBlocker(FcitxInputState* input, FcitxKeySym key, unsigned int state);

FCITX_GETTER_VALUE(FcitxInputState, IsInRemind, bIsInRemind, boolean)
FCITX_SETTER(FcitxInputState, IsInRemind, bIsInRemind, boolean)
FCITX_GETTER_VALUE(FcitxInputState, IsDoInputOnly, bIsDoInputOnly, boolean)
FCITX_SETTER(FcitxInputState, IsDoInputOnly, bIsDoInputOnly, boolean)
FCITX_GETTER_VALUE(FcitxInputState, OutputString, strStringGet, char*)
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
FCITX_SETTER(FcitxInputState, LastIsSingleChar, lastIsSingleHZ, boolean)
FCITX_SETTER(FcitxInputState, KeyReleased, keyReleased, KEY_RELEASED)

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
    hk.hotkey = FCITX_CTRL_5;
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
    utarray_init(&instance->imeclasses, &imclass_icd);
}

FCITX_EXPORT_API
void FcitxInstanceSaveAllIM(FcitxInstance* instance)
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

static const char* GetStateName(INPUT_RETURN_VALUE retVal)
{
    return "unknown";
}

void FcitxInstanceLoadIM(FcitxInstance* instance, FcitxAddon* addon)
{
    if (!addon)
        return;

    if (addon->type == AT_SHAREDLIBRARY) {
        char* modulePath;
        FILE *fp = FcitxXDGGetLibFile(addon->library, "r", &modulePath);
        void *handle;
        FcitxIMClass * imclass;
        if (!fp) {
            free(modulePath);
            return;
        }
        fclose(fp);
        handle = dlopen(modulePath, RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            FcitxLog(ERROR, _("IM: open %s fail %s") , modulePath , dlerror());
            free(modulePath);
            return;
        }
        free(modulePath);

        if (!CheckABIVersion(handle)) {
            FcitxLog(ERROR, "%s ABI Version Error", addon->name);
            dlclose(handle);
            return;
        }

        imclass = dlsym(handle, "ime");
        if (!imclass || !imclass->Create) {
            FcitxLog(ERROR, _("IM: bad im %s"), addon->name);
            dlclose(handle);
            return;
        }
        if ((addon->addonInstance = imclass->Create(instance)) == NULL) {
            dlclose(handle);
            return;
        }
        addon->imclass = imclass;
        utarray_push_back(&instance->imeclasses, &addon);
    }
}

void FcitxRegisterEmptyEntry(FcitxInstance *instance,
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
        FcitxLog(ERROR, "%s already exists", uniqueName);
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
    if (priority <= 0)
        return ;
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

    entry->uniqueName = strdup(uniqueName);
    entry->strName = strdup(name);
    entry->strIconName = strdup(iconName);
    entry->Init = Init;
    entry->ResetIM = ResetIM;
    entry->DoInput = DoInput;
    entry->GetCandWords = GetCandWords;
    entry->PhraseTips = PhraseTips;
    entry->Save = Save;
    entry->ReloadConfig = ReloadConfig;
    entry->KeyBlocker = KeyBlocker;
    entry->klass = imclass;
    entry->iPriority = priority;
    if (langCode)
        strncpy(entry->langCode, langCode, LANGCODE_LENGTH);
    entry->langCode[LANGCODE_LENGTH] = 0;
    entry->initialized = true;
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
                FcitxRegisterEmptyEntry(instance,
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
        if (addon->bEnabled && addon->category == AC_INPUTMETHOD) {
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

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */
    if (FcitxInstanceGetCurrentIC(instance) == NULL)
        return IRV_TO_PROCESS;

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (event == FCITX_RELEASE_KEY) {
        if (FcitxInstanceGetCurrentState(instance) != IS_CLOSED) {
            if ((timestamp - input->lastKeyPressedTime) < 500 && (!input->bIsDoInputOnly)) {
                if (fc->bIMSwitchKey && (FcitxHotkeyIsHotKey(sym, state, FCITX_LCTRL_LSHIFT) || FcitxHotkeyIsHotKey(sym, state, FCITX_LCTRL_LSHIFT2))) {
                    if (FcitxInstanceGetCurrentState(instance) == IS_ACTIVE) {
                        if (input->keyReleased == KR_CTRL_SHIFT)
                            FcitxInstanceSwitchIM(instance, -1);
                    } else if (FcitxHotkeyIsHotKey(sym, state, fc->hkTrigger)) {
                            FcitxInstanceCloseIM(instance, FcitxInstanceGetCurrentIC(instance));
                    }
                } else if (FcitxHotkeyIsHotKey(sym, state, fc->switchKey) && input->keyReleased == KR_CTRL && !fc->bDoubleSwitchKey) {
                    retVal = IRV_DONOT_PROCESS;
                    if (fc->bSendTextWhenSwitchEng) {
                        if (input->iCodeInputCount != 0) {
                            strcpy(FcitxInputStateGetOutputString(input), FcitxInputStateGetRawInputBuffer(input));
                            retVal = IRV_ENG;
                        }
                    }
                    input->keyReleased = KR_OTHER;
                    if (FcitxInstanceGetCurrentState(instance) == IS_ENG)
                        FcitxInstanceShowInputSpeed(instance);
                    FcitxInstanceChangeIMState(instance, FcitxInstanceGetCurrentIC(instance));
                } else if (FcitxHotkeyIsHotKey(sym, state, fc->i2ndSelectKey) && input->keyReleased == KR_2ND_SELECTKEY) {
                    if (!input->bIsInRemind) {
                        retVal = FcitxCandidateWordChooseByIndex(input->candList, 1);
                    } else {
                        strcpy(FcitxInputStateGetOutputString(input), " ");
                        retVal = IRV_COMMIT_STRING;
                    }
                    input->keyReleased = KR_OTHER;
                } else if (FcitxHotkeyIsHotKey(sym, state, fc->i3rdSelectKey) && input->keyReleased == KR_3RD_SELECTKEY) {
                    if (!input->bIsInRemind) {
                        retVal = FcitxCandidateWordChooseByIndex(input->candList, 2);
                    } else {
                        strcpy(FcitxInputStateGetOutputString(input), "　");
                        retVal = IRV_COMMIT_STRING;
                    }

                    input->keyReleased = KR_OTHER;
                }
            }
        }
    }

#if 0
    /* Added by hubert_star AT forum.ubuntu.com.cn */
    if (event == FCITX_RELEASE_KEY
            && IsHotKeySimple(sym, state)
            && retVal == IRV_TO_PROCESS)
        return IRV_DO_NOTHING;
#endif

    if (retVal == IRV_TO_PROCESS) {
        /* process key event for switch key */
        if (event == FCITX_PRESS_KEY) {
            if (!FcitxHotkeyIsHotKey(sym, state, fc->switchKey))
                input->keyReleased = KR_OTHER;
            else {
                if ((input->keyReleased == KR_CTRL)
                        && (timestamp - input->lastKeyPressedTime < fc->iTimeInterval)
                        && fc->bDoubleSwitchKey) {
                    FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), FcitxInputStateGetRawInputBuffer(input));
                    FcitxInstanceChangeIMState(instance, FcitxInstanceGetCurrentIC(instance));
                }
            }

            input->lastKeyPressedTime = timestamp;
            if (FcitxHotkeyIsHotKey(sym, state, fc->switchKey)) {
                input->keyReleased = KR_CTRL;
                retVal = IRV_DO_NOTHING;
            } else if (fc->bIMSwitchKey && (FcitxHotkeyIsHotKey(sym, state, FCITX_LCTRL_LSHIFT) || FcitxHotkeyIsHotKey(sym, state, FCITX_LCTRL_LSHIFT2))) {
                input->keyReleased = KR_CTRL_SHIFT;
                retVal = IRV_DO_NOTHING;
            } else if (FcitxHotkeyIsHotKey(sym, state, fc->hkTrigger)) {
                /* trigger key has the highest priority, so we check it first */
                if (FcitxInstanceGetCurrentState(instance) == IS_ENG) {
                    FcitxInstanceChangeIMState(instance, FcitxInstanceGetCurrentIC(instance));
                    FcitxInstanceShowInputSpeed(instance);
                } else
                    FcitxInstanceCloseIM(instance, FcitxInstanceGetCurrentIC(instance));

                retVal = IRV_DO_NOTHING;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS && event != FCITX_PRESS_KEY)
        retVal = IRV_DONOT_PROCESS;

    /* the key processed before this phase is very important, we don't let any interrupt */
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE
        && retVal == IRV_TO_PROCESS
       ) {
        if (!input->bIsDoInputOnly) {
            FcitxInstanceProcessPreInputFilter(instance, sym, state, &retVal);
        }

        if (retVal == IRV_TO_PROCESS) {
            if (FcitxHotkeyIsHotKey(sym, state, fc->i2ndSelectKey)) {
                if (FcitxCandidateWordGetByIndex(input->candList, 1) != NULL) {
                    input->keyReleased = KR_2ND_SELECTKEY;
                    return IRV_DO_NOTHING;
                }
            } else if (FcitxHotkeyIsHotKey(sym, state, fc->i3rdSelectKey)) {
                if (FcitxCandidateWordGetByIndex(input->candList, 2) != NULL) {
                    input->keyReleased = KR_3RD_SELECTKEY;
                    return IRV_DO_NOTHING;
                }
            }

            if (!FcitxHotkeyIsHotKey(sym, state, FCITX_LCTRL_LSHIFT) && currentIM) {
                retVal = currentIM->DoInput(currentIM->klass, sym, state);
            }
        }

        /* check choose key first, because it might trigger update candidates */
        if (!input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
            int index = FcitxHotkeyCheckChooseKeyAndModifier(sym, state,
                                                  FcitxCandidateWordGetChoose(input->candList),
                                                  FcitxCandidateWordGetModifier(input->candList)
                                                 );
            if (index >= 0)
                retVal = FcitxCandidateWordChooseByIndex(input->candList, index);
        }
    }

    if (retVal != IRV_ASYNC) {
        return FcitxInstanceDoInputCallback(
                   instance,
                   retVal,
                   event,
                   timestamp,
                   sym,
                   state);
    } else
        return retVal;
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
    FcitxIM* currentIM = FcitxInstanceGetCurrentIM(instance);
    FcitxInputState *input = instance->input;

    FcitxGlobalConfig *fc = instance->config;

    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE && currentIM && (retVal & IRV_FLAG_UPDATE_CANDIDATE_WORDS)) {
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
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE && !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        const FcitxHotkey* hkPrevPage = FcitxInstanceGetContextHotkey(instance, CONTEXT_ALTERNATIVE_PREVPAGE_KEY);
        if (hkPrevPage == NULL)
            hkPrevPage = fc->hkPrevPage;
        
        const FcitxHotkey* hkNextPage = FcitxInstanceGetContextHotkey(instance, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY);
        if (hkNextPage == NULL)
            hkNextPage = fc->hkNextPage;
        
        if (FcitxHotkeyIsHotKey(sym, state, hkPrevPage)) {
            if (FcitxCandidateWordGoPrevPage(input->candList))
                retVal = IRV_DISPLAY_CANDWORDS;
        } else if (FcitxHotkeyIsHotKey(sym, state, hkNextPage)) {
            if (FcitxCandidateWordGoNextPage(input->candList))
                retVal = IRV_DISPLAY_CANDWORDS;
        }
    }

    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE && !input->bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
        FcitxInstanceProcessPostInputFilter(instance, sym, state, &retVal);
        
        if (retVal == IRV_TO_PROCESS) {
            if (currentIM && currentIM->KeyBlocker)
                retVal = currentIM->KeyBlocker(currentIM->klass, sym, state);
            else
                retVal = FcitxStandardKeyBlocker(input, sym, state);
        }
    }

    if (retVal == IRV_TO_PROCESS) {
        retVal = FcitxInstanceProcessHotkey(instance, sym, state);
    }

    FcitxLog(DEBUG, "ProcessKey Return State: %s", GetStateName(retVal));

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
        FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), FcitxInputStateGetOutputString(input));
        instance->iHZInputed += (int)(fcitx_utf8_strlen(FcitxInputStateGetOutputString(input)));
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
        FcitxMessagesAddMessageAtLast(input->msgAuxUp, MSG_INPUT, "%c", FcitxInputStateGetRawInputBuffer(input)[0]);
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_TIPS, "%s", FcitxInputStateGetOutputString(input));
    }

    if (retVal & IRV_FLAG_UPDATE_INPUT_WINDOW)
        FcitxUIUpdateInputWindow(instance);
}

FCITX_EXPORT_API
void FcitxInstanceForwardKey(FcitxInstance* instance, FcitxInputContext *ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state)
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
void FcitxInstanceSwitchIM(FcitxInstance* instance, int index)
{
    FcitxInstanceSwitchIMInternal(instance, index, instance->config->firstAsInactive);
}

void FcitxInstanceSwitchIMInternal(FcitxInstance* instance, int index, boolean skipZero)
{
    UT_array* imes = &instance->imes;
    int iIMCount = utarray_len(imes);

    FcitxInstanceCleanInputWindow(instance);
    FcitxInstanceResetInput(instance);
    FcitxUIUpdateInputWindow(instance);

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
    
    if (skipZero && instance->iIMIndex == 0)
        instance->iIMIndex = 1;

    if (instance->iIMIndex >= iIMCount || instance->iIMIndex < 0)
        newIM = NULL;
    else {
        newIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    }

    if (lastIM && lastIM->Save)
        lastIM->Save(lastIM->klass);

    /* lazy load */
    if (newIM && !newIM->initialized) {
        char* name = strdup(newIM->uniqueName);
        FcitxInstanceLoadIM(instance, newIM->owner);
        FcitxIM* im = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, name);
        if (!im->initialized) {
            im->initialized = true;
            int index = utarray_eltidx(&instance->availimes, im);
            utarray_erase(&instance->availimes, index, 1);
        }

        FcitxInstanceUpdateIMList(instance);
        instance->iIMIndex = FcitxInstanceGetIMIndexByName(instance, name);
        newIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
        free(name);
    }
    
    if (newIM && newIM->Init) {
        FcitxInstanceResetContext(instance, FCF_ResetOnInputMethodChange);
        FcitxInstanceSetContext(instance, CONTEXT_IM_LANGUAGE, newIM->langCode);
        newIM->Init(newIM->klass);
    }
    
    if (instance->iIMIndex != 0 && instance->config->firstAsInactive)
        instance->lastIMIndex = instance->iIMIndex;

    FcitxInstanceResetInput(instance);
    
    FcitxProfileSave(instance->profile);
}

/**
 * @brief 重置输入状态
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

    FcitxIM* currentIM = (FcitxIM*) utarray_eltptr(ims, instance->iIMIndex);

    if (currentIM && currentIM->ResetIM)
        currentIM->ResetIM(currentIM->klass);

    FcitxInstanceProcessResetInputHook(instance);
}

void FcitxInstanceDoPhraseTips(FcitxInstance* instance)
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
    FcitxGlobalConfig *fc = instance->config;

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
            FcitxInstanceCleanInputWindow(instance);
            strcpy(FcitxInputStateGetOutputString(input), FcitxInputStateGetRawInputBuffer(input));
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
    FcitxUIUpdateStatus(instance, "remind");
    return IRV_DONOT_PROCESS;
}

INPUT_RETURN_VALUE ImProcessReload(void *arg)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    FcitxInstanceReloadConfig(instance);
    return IRV_DO_NOTHING;
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
FcitxIM* FcitxInstanceGetCurrentIM(FcitxInstance* instance)
{
    UT_array* imes = &instance->imes;
    FcitxIM* pcurrentIM = (FcitxIM*) utarray_eltptr(imes, instance->iIMIndex);
    return pcurrentIM;
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
                UT_array* frontends = &instance->frontends;
                FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
                if (pfrontend) {
                    FcitxFrontend* frontend = (*pfrontend)->frontend;
                    if (frontend->CheckICFromSameApplication &&
                            frontend->CheckICFromSameApplication((*pfrontend)->addonInstance, rec, ic))
                        flag = true;
                }
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
    
    if (instance->config->firstAsInactive
        && FcitxInstanceGetCurrentState(instance) == IS_ACTIVE
        && instance->iIMIndex == 0
    ) {
        FcitxInstanceSwitchIM(instance, instance->lastIMIndex);
    }
}


void FcitxInstanceEnableIMInternal(FcitxInstance* instance, FcitxInputContext* ic, boolean keepState)
{
    if (ic == NULL)
        return ;
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    FcitxContextState oldstate = ic->state;
    ic->state = IS_ACTIVE;
    if (oldstate == IS_CLOSED)
        frontend->EnableIM((*pfrontend)->addonInstance, ic);

    if (ic == FcitxInstanceGetCurrentIC(instance)) {
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
    
    if (instance->config->firstAsInactive && !(ic->contextCaps & CAPACITY_CLIENT_SIDE_CONTROL_STATE)) {
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
                UT_array* frontends = &instance->frontends;
                FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
                if (pfrontend) {
                    FcitxFrontend* frontend = (*pfrontend)->frontend;
                    if (frontend->CheckICFromSameApplication &&
                            frontend->CheckICFromSameApplication((*pfrontend)->addonInstance, rec, ic))
                        flag = true;
                }
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
    UT_array* frontends = &instance->frontends;
    FcitxAddon** pfrontend = (FcitxAddon**) utarray_eltptr(frontends, ic->frontendid);
    if (pfrontend == NULL)
        return;
    FcitxFrontend* frontend = (*pfrontend)->frontend;
    ic->state = IS_CLOSED;
    frontend->CloseIM((*pfrontend)->addonInstance, ic);

    if (ic == FcitxInstanceGetCurrentIC(instance)) {
        FcitxUIOnTriggerOff(instance);
        FcitxUICloseInputWindow(instance);
    }
}

/**
 * @brief 更改输入法状态
 *
 * @param _connect_id
 */
FCITX_EXPORT_API
void FcitxInstanceChangeIMState(FcitxInstance* instance, FcitxInputContext* ic)
{
    if (ic == NULL)
        return;
    FcitxContextState objectState;
    if (ic->state == IS_ENG)
        objectState = IS_ACTIVE;
    else
        objectState = IS_ENG;
    
    if (instance->config->firstAsInactive) {
        if (objectState == IS_ACTIVE)
            FcitxInstanceSwitchIM(instance, instance->lastIMIndex);
        else {
            FcitxInstanceSwitchIMInternal(instance, 0, false);
        }
    }

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

            if (flag && (rec == ic || !(rec->contextCaps & CAPACITY_CLIENT_SIDE_CONTROL_STATE)))
                FcitxInstanceChangeIMStateInternal(instance, rec, objectState);
            rec = rec->next;
        }
    }
    break;
    case ShareState_None:
        FcitxInstanceChangeIMStateInternal(instance, ic, objectState);
        break;
    }
}

void FcitxInstanceChangeIMStateInternal(FcitxInstance* instance, FcitxInputContext* ic, FcitxContextState objectState)
{
    if (!ic)
        return;
    if (ic->state == objectState)
        return;
    ic->state = objectState;
    if (ic == FcitxInstanceGetCurrentIC(instance)) {
        if (objectState == IS_ACTIVE) {
            FcitxInstanceResetInput(instance);
        } else {
            FcitxInstanceResetInput(instance);
            FcitxUICloseInputWindow(instance);
        }
    }
}

void FcitxInstanceInitIMMenu(FcitxInstance* instance)
{
    instance->imMenu.candStatusBind = strdup("im");
    instance->imMenu.name = strdup(_("Input Method"));

    instance->imMenu.UpdateMenu = UpdateIMMenuItem;
    instance->imMenu.MenuAction = IMMenuAction;
    instance->imMenu.priv = instance;
    instance->imMenu.isSubMenu = false;
}

boolean IMMenuAction(FcitxUIMenu *menu, int index)
{
    FcitxInstance* instance = (FcitxInstance*) menu->priv;
    FcitxInstanceSwitchIM(instance, index);
    return true;
}

void UpdateIMMenuItem(FcitxUIMenu *menu)
{
    FcitxInstance* instance = (FcitxInstance*) menu->priv;
    FcitxMenuClear(menu);

    FcitxIM* pim;
    UT_array* imes = &instance->imes;
    FcitxMenuInit(&instance->imMenu);
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim))
        FcitxMenuAddMenuItem(&instance->imMenu, pim->strName, MENUTYPE_SIMPLE, NULL);

    menu->mark = instance->iIMIndex;
}

void FcitxInstanceShowInputSpeed(FcitxInstance* instance)
{
    FcitxInputState* input = instance->input;

    if (!instance->config->bShowInputWindowTriggering)
        return;

    input->bShowCursor = false;

    FcitxInstanceCleanInputWindow(instance);
    if (instance->config->bShowVersion) {
        FcitxMessagesAddMessageAtLast(input->msgAuxUp, MSG_TIPS, "FCITX " VERSION);
        FcitxIM* im = FcitxInstanceGetCurrentIM(instance);
        if (im) {
            FcitxMessagesAddMessageAtLast(input->msgAuxUp, MSG_TIPS, " %s", im->strName);
        }
    }

    //显示打字速度
    if (instance->config->bShowUserSpeed) {
        double          timePassed;

        timePassed = instance->totaltime + difftime(time(NULL), instance->timeStart);
        if (((int) timePassed) == 0)
            timePassed = 1.0;

        FcitxMessagesSetMessageCount(input->msgAuxDown, 0);
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_OTHER, _("Input Speed: "));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_CODE, "%d", (int)(instance->iHZInputed * 60 / timePassed));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_OTHER, _("/min  Time Used: "));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_CODE, "%d", (int) timePassed / 60);
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_OTHER, _("min Num of Characters: "));
        FcitxMessagesAddMessageAtLast(input->msgAuxDown, MSG_CODE, "%u", instance->iHZInputed);

    }

    FcitxUIUpdateInputWindow(instance);
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
int FcitxHotkeyCheckChooseKey(FcitxKeySym sym, int state, const char* strChoose)
{
    return FcitxHotkeyCheckChooseKeyAndModifier(sym, state, strChoose, FcitxKeyState_None);
}

FCITX_EXPORT_API
int FcitxHotkeyCheckChooseKeyAndModifier(FcitxKeySym sym, int state, const char* strChoose, int candState)
{
    if (state != candState)
        return -1;

    sym = FcitxHotkeyPadToMain(sym);

    int i = 0;

    while (strChoose[i]) {
        if (sym == strChoose[i])
            return i;
        i++;
    }

    return -1;
}

FCITX_EXPORT_API
int FcitxInstanceGetIMIndexByName(FcitxInstance* instance, const char* imName)
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
void FcitxInstanceUpdateIMList(FcitxInstance* instance)
{
    if (!instance->imLoaded)
        return;

    UT_array* imList = fcitx_utils_split_string(instance->profile->imList, ',');
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
                ime = FcitxInstanceGetIMFromIMList(instance, IMAS_Disable, str);

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

    instance->iIMIndex = FcitxInstanceGetIMIndexByName(instance, instance->profile->imName);

    FcitxInstanceSwitchIM(instance, instance->iIMIndex);
    FcitxInstanceProcessUpdateIMListHook(instance);
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

INPUT_RETURN_VALUE FcitxStandardKeyBlocker(FcitxInputState* input, FcitxKeySym key, unsigned int state)
{
    if (FcitxInputStateGetRawInputBufferSize(input) != 0
        && (FcitxHotkeyIsHotKeySimple(key, state) || FcitxHotkeyIsHotkeyCursorMove(key, state)))
        return IRV_DO_NOTHING;
    else
        return IRV_TO_PROCESS;
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
