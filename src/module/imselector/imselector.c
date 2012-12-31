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
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <libintl.h>
#include <errno.h>
#include "fcitx/instance.h"

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"
#include "fcitx/candidate.h"
#include "fcitx/keys.h"
#include <fcitx/context.h>
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"

#define SELECTOR_COUNT 9

typedef struct _IMSelector IMSelector;

typedef struct _SelectorHandle {
    int idx;
    boolean global;
    IMSelector* imselector;
} SelectorHandle;

struct _IMSelector {
    FcitxGenericConfig gconfig;
    FcitxHotkey localKey[2];
    FcitxHotkey globalKey[2];
    FcitxHotkey clearLocalKey[2];
    FcitxHotkey selectorKey[2][SELECTOR_COUNT][2];
    SelectorHandle handle[2][SELECTOR_COUNT];
    boolean triggered;
    boolean global;
    FcitxInstance* owner;
};

static void* IMSelectorCreate(FcitxInstance* instance);
static boolean IMSelectorPreFilter(void* arg, FcitxKeySym sym,
                                   unsigned int state,
                                   INPUT_RETURN_VALUE *retval
                                   );
static  void IMSelectorReset(void* arg);
static  void IMSelectorReload(void* arg);
static INPUT_RETURN_VALUE IMSelectorLocalTrigger(void* arg);
static INPUT_RETURN_VALUE IMSelectorGlobalTrigger(void* arg);
static INPUT_RETURN_VALUE IMSelectorClearLocal(void* arg);
static INPUT_RETURN_VALUE IMSelectorSelect(void* arg);
static boolean LoadIMSelectorConfig(IMSelector* imselector);
static void SaveIMSelectorConfig(IMSelector* imselector);
static void IMSelectorGetCands(IMSelector* imselector);

FCITX_DEFINE_PLUGIN(fcitx_imselector, module, FcitxModule) = {
    IMSelectorCreate,
    NULL,
    NULL,
    NULL,
    IMSelectorReload
};

CONFIG_BINDING_BEGIN(IMSelector)
CONFIG_BINDING_REGISTER("IMSelector", "LocalInputMethodSelectKey", localKey)
CONFIG_BINDING_REGISTER("IMSelector", "GlobalInputMethodSelectKey", globalKey)
CONFIG_BINDING_REGISTER("IMSelector", "ClearLocal", clearLocalKey)
CONFIG_BINDING_REGISTER("GlobalSelector", "IM1", selectorKey[1][0])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM2", selectorKey[1][1])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM3", selectorKey[1][2])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM4", selectorKey[1][3])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM5", selectorKey[1][4])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM6", selectorKey[1][5])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM7", selectorKey[1][6])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM8", selectorKey[1][7])
CONFIG_BINDING_REGISTER("GlobalSelector", "IM9", selectorKey[1][8])
CONFIG_BINDING_REGISTER("LocalSelector", "IM1", selectorKey[0][0])
CONFIG_BINDING_REGISTER("LocalSelector", "IM2", selectorKey[0][1])
CONFIG_BINDING_REGISTER("LocalSelector", "IM3", selectorKey[0][2])
CONFIG_BINDING_REGISTER("LocalSelector", "IM4", selectorKey[0][3])
CONFIG_BINDING_REGISTER("LocalSelector", "IM5", selectorKey[0][4])
CONFIG_BINDING_REGISTER("LocalSelector", "IM6", selectorKey[0][5])
CONFIG_BINDING_REGISTER("LocalSelector", "IM7", selectorKey[0][6])
CONFIG_BINDING_REGISTER("LocalSelector", "IM8", selectorKey[0][7])
CONFIG_BINDING_REGISTER("LocalSelector", "IM9", selectorKey[0][8])
CONFIG_BINDING_END()

void* IMSelectorCreate(FcitxInstance* instance)
{
    IMSelector* imselector = fcitx_utils_malloc0(sizeof(IMSelector));
    imselector->owner = instance;
    if (!LoadIMSelectorConfig(imselector)) {
        free(imselector);
        return NULL;
    }

    FcitxKeyFilterHook hk;
    hk.arg = imselector;
    hk.func = IMSelectorPreFilter;
    FcitxInstanceRegisterPreInputFilter(instance, hk);

    FcitxHotkeyHook hkhk;
    hkhk.arg = imselector;
    hkhk.hotkeyhandle = IMSelectorLocalTrigger;
    hkhk.hotkey = imselector->localKey;
    FcitxInstanceRegisterHotkeyFilter(instance, hkhk);

    hkhk.arg = imselector;
    hkhk.hotkeyhandle = IMSelectorGlobalTrigger;
    hkhk.hotkey = imselector->globalKey;
    FcitxInstanceRegisterHotkeyFilter(instance, hkhk);

    hkhk.arg = imselector;
    hkhk.hotkeyhandle = IMSelectorClearLocal;
    hkhk.hotkey = imselector->clearLocalKey;
    FcitxInstanceRegisterHotkeyFilter(instance, hkhk);

    /* this key is ignore the very first input method which is for inactive */
#define _ADD_HANDLE(X, ISGLOBAL) \
    do { \
        SelectorHandle* handle = &imselector->handle[ISGLOBAL][X - 1]; \
        handle->global = ISGLOBAL; \
        handle->idx = X; \
        handle->imselector = imselector; \
        hkhk.arg = handle; \
        hkhk.hotkeyhandle = IMSelectorSelect; \
        hkhk.hotkey = imselector->selectorKey[ISGLOBAL][X - 1]; \
        FcitxInstanceRegisterHotkeyFilter(instance, hkhk); \
    } while(0);

    _ADD_HANDLE(1, false);
    _ADD_HANDLE(2, false);
    _ADD_HANDLE(3, false);
    _ADD_HANDLE(4, false);
    _ADD_HANDLE(5, false);
    _ADD_HANDLE(6, false);
    _ADD_HANDLE(7, false);
    _ADD_HANDLE(8, false);
    _ADD_HANDLE(9, false);
    _ADD_HANDLE(1, true);
    _ADD_HANDLE(2, true);
    _ADD_HANDLE(3, true);
    _ADD_HANDLE(4, true);
    _ADD_HANDLE(5, true);
    _ADD_HANDLE(6, true);
    _ADD_HANDLE(7, true);
    _ADD_HANDLE(8, true);
    _ADD_HANDLE(9, true);

    FcitxIMEventHook resethk;
    resethk.arg = imselector;
    resethk.func = IMSelectorReset;
    FcitxInstanceRegisterResetInputHook(instance, resethk);
    return imselector;
}

boolean IMSelectorPreFilter(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    IMSelector* imselector = arg;
    FcitxInstance* instance = imselector->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxGlobalConfig* fc = FcitxInstanceGetGlobalConfig(instance);
    if (!imselector->triggered)
        return false;
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    if (FcitxHotkeyIsHotKey(sym, state,
                            FcitxConfigPrevPageKey(instance, fc))) {
        FcitxCandidateWordGoPrevPage(candList);
        *retval = IRV_DISPLAY_MESSAGE;
    } else if (FcitxHotkeyIsHotKey(sym, state,
                                   FcitxConfigNextPageKey(instance, fc))) {
        FcitxCandidateWordGoNextPage(candList);
        *retval = IRV_DISPLAY_MESSAGE;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
        if (FcitxCandidateWordPageCount(candList) != 0)
            *retval = FcitxCandidateWordChooseByIndex(candList, 0);
    } else if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
        int iKey = FcitxHotkeyCheckChooseKey(sym, state, DIGIT_STR_CHOOSE);
        if (iKey >= 0)
            *retval = FcitxCandidateWordChooseByIndex(candList, iKey);
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
        *retval = IRV_CLEAN;
    }
    if (*retval == IRV_TO_PROCESS)
        *retval = IRV_DO_NOTHING;
    return true;
}

INPUT_RETURN_VALUE IMSelectorGlobalTrigger(void* arg)
{
    IMSelector* imselector = arg;
    imselector->triggered = true;
    imselector->global = true;
    IMSelectorGetCands(imselector);
    return IRV_DISPLAY_MESSAGE;
}

INPUT_RETURN_VALUE IMSelectorLocalTrigger(void* arg)
{
    IMSelector* imselector = arg;
    imselector->triggered = true;
    imselector->global = false;
    IMSelectorGetCands(imselector);
    return IRV_DISPLAY_MESSAGE;
}

INPUT_RETURN_VALUE IMSelectorClearLocal(void* arg)
{
    IMSelector* imselector = arg;
    FcitxInstance* instance = imselector->owner;
    FcitxInstanceSetLocalIMName(instance, FcitxInstanceGetCurrentIC(instance), NULL);
    return IRV_CLEAN;
}

INPUT_RETURN_VALUE IMSelectorSelect(void* arg)
{
    SelectorHandle* handle = arg;
    IMSelector* imselector = handle->imselector;
    FcitxInstance* instance = imselector->owner;
    FcitxIM* im = FcitxInstanceGetIMByIndex(instance, handle->idx);
    if (!im)
        return IRV_TO_PROCESS;
    if (handle->global) {
        FcitxInstanceSwitchIMByIndex(instance, handle->idx);
    }
    else {
        FcitxInstanceSetLocalIMName(instance, FcitxInstanceGetCurrentIC(instance), im->uniqueName);

        if (FcitxInstanceGetCurrentState(instance) != IS_ACTIVE)
            FcitxInstanceEnableIM(instance, FcitxInstanceGetCurrentIC(instance), false);
    }
    return IRV_CLEAN;
}

INPUT_RETURN_VALUE IMSelectorGetCand(void* arg, FcitxCandidateWord* candWord)
{
    IMSelector* imselector = arg;
    FcitxInstance* instance = imselector->owner;

    if (!candWord->priv) {
        FcitxInstanceSetLocalIMName(instance, FcitxInstanceGetCurrentIC(instance), NULL);
        return IRV_CLEAN;
    }

    int index = FcitxInstanceGetIMIndexByName(imselector->owner, (char*) candWord->priv);

    if (index == 0)
        FcitxInstanceCloseIM(instance, FcitxInstanceGetCurrentIC(instance));
    else {
        if (imselector->global)
            FcitxInstanceSwitchIMByIndex(instance, index);
        else
            FcitxInstanceSetLocalIMName(instance, FcitxInstanceGetCurrentIC(instance), (char*) candWord->priv);

        if (FcitxInstanceGetCurrentState(instance) != IS_ACTIVE)
            FcitxInstanceEnableIM(instance, FcitxInstanceGetCurrentIC(instance), false);
    }
    return IRV_CLEAN;
}

void IMSelectorGetCands(IMSelector* imselector)
{
    FcitxInstance* instance = imselector->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);
    FcitxIM* pim;
    UT_array* imes = FcitxInstanceGetIMEs(instance);
    FcitxInstanceCleanInputWindow(instance);
    FcitxCandidateWordSetPageSize(FcitxInputStateGetCandidateList(input), 10);
    FcitxCandidateWordSetChoose(FcitxInputStateGetCandidateList(input), DIGIT_STR_CHOOSE);
    FcitxInputStateSetShowCursor(input, false);

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(instance);
    FcitxInputContext2* ic2 = (FcitxInputContext2*) ic;
    if (!ic)
        return;

    FcitxMessages *aux_up = FcitxInputStateGetAuxUp(input);
    FcitxMessagesAddMessageStringsAtLast(aux_up, MSG_TIPS, imselector->global ?
                                         _("Select global input method: ") :
                                         _("Select local input method: "));
    if (ic2->imname) {
        int idx = FcitxInstanceGetIMIndexByName(instance, ic2->imname);
        FcitxIM *im = fcitx_array_eltptr(imes, idx);
        if (im) {
            FcitxMessagesAddMessageStringsAtLast(aux_up, MSG_TIPS,
                _("Current local input method is "), im->strName);
        }
    } else {
        FcitxMessagesAddMessageStringsAtLast(aux_up, MSG_TIPS,
                                             _("No local input method"));
    }
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim)) {
        FcitxCandidateWord candWord;
        candWord.callback = IMSelectorGetCand;
        candWord.owner = imselector;
        candWord.strExtra = NULL;
        if (ic2->imname && strcmp(ic2->imname, pim->uniqueName) == 0) {
            candWord.priv = NULL;
            candWord.strWord = strdup(_("Clear local input method"));
        }
        else {
            candWord.priv = strdup(pim->uniqueName);
            candWord.strWord = strdup(pim->strName);
        }
        candWord.wordType = MSG_OTHER;
        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }
}

void IMSelectorReset(void* arg)
{
    IMSelector* imselector = arg;
    imselector->triggered = false;
}


void IMSelectorReload(void* arg)
{
    IMSelector* imselector = arg;
    LoadIMSelectorConfig(imselector);
}

CONFIG_DESC_DEFINE(GetIMSelectorConfig, "fcitx-imselector.desc")

boolean LoadIMSelectorConfig(IMSelector* imselector)
{
    FcitxConfigFileDesc* configDesc = GetIMSelectorConfig();
    if (configDesc == NULL)
        return false;

    FILE *fp;
    char *file;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-imselector.config", "r", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveIMSelectorConfig(imselector);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    IMSelectorConfigBind(imselector, cfile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*)imselector);

    if (fp)
        fclose(fp);

    return true;
}

void SaveIMSelectorConfig(IMSelector* imselector)
{
    FcitxConfigFileDesc* configDesc = GetIMSelectorConfig();
    char *file;
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-imselector.config", "w", &file);
    FcitxLog(DEBUG, "Save Config to %s", file);
    FcitxConfigSaveConfigFileFp(fp, &imselector->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}
