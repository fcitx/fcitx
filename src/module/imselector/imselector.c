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

typedef struct _IMSelector {
    FcitxGenericConfig gconfig;
    FcitxHotkey localKey[2];
    FcitxHotkey globalKey[2];
    boolean triggered;
    boolean global;
    FcitxInstance* owner;
} IMSelector;

static void* IMSelectorCreate(FcitxInstance* instance);
static boolean IMSelectorPreFilter(void* arg, FcitxKeySym sym,
                                    unsigned int state,
                                    INPUT_RETURN_VALUE *retval
                                   );
static  void IMSelectorReset(void* arg);
static  void IMSelectorReload(void* arg);
static INPUT_RETURN_VALUE IMSelectorLocalTrigger(void* arg);
static INPUT_RETURN_VALUE IMSelectorGlobalTrigger(void* arg);
static boolean LoadIMSelectorConfig(IMSelector* imselector);
static void SaveIMSelectorConfig(IMSelector* imselector);
static void IMSelectorGetCands(IMSelector* imselector);

FCITX_EXPORT_API
FcitxModule module = {
    IMSelectorCreate,
    NULL,
    NULL,
    NULL,
    IMSelectorReload
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

CONFIG_BINDING_BEGIN(IMSelector)
CONFIG_BINDING_REGISTER("IMSelector", "LocalInputMethodSelectKey", localKey)
CONFIG_BINDING_REGISTER("IMSelector", "GlobalInputMethodSelectKey", globalKey)
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

    const FcitxHotkey* hkPrevPage = FcitxInstanceGetContextHotkey(instance, CONTEXT_ALTERNATIVE_PREVPAGE_KEY);
    if (hkPrevPage == NULL)
        hkPrevPage = fc->hkPrevPage;

    const FcitxHotkey* hkNextPage = FcitxInstanceGetContextHotkey(instance, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY);
    if (hkNextPage == NULL)
        hkNextPage = fc->hkNextPage;

    if (FcitxHotkeyIsHotKey(sym, state, hkPrevPage)) {
        if (FcitxCandidateWordGoPrevPage(FcitxInputStateGetCandidateList(input)))
            *retval = IRV_DISPLAY_MESSAGE;
    } else if (FcitxHotkeyIsHotKey(sym, state, hkNextPage)) {
        if (FcitxCandidateWordGoNextPage(FcitxInputStateGetCandidateList(input)))
            *retval = IRV_DISPLAY_MESSAGE;
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
        if (FcitxCandidateWordPageCount(FcitxInputStateGetCandidateList(input)) != 0)
            *retval = FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
    } if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
        int iKey = FcitxHotkeyCheckChooseKey(sym, state, DIGIT_STR_CHOOSE);
        if (iKey >= 0)
            *retval = FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), iKey);
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_ESCAPE)) {
        FcitxInstanceResetInput(instance);
    } else {
        *retval = IRV_DO_NOTHING;
    }
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

INPUT_RETURN_VALUE IMSelectorGetCand(void* arg, FcitxCandidateWord* candWord)
{
    IMSelector* imselector = arg;
    FcitxInstance* instance = imselector->owner;

    if (!candWord->priv) {
        FcitxInstanceSetLocalIMName(instance, FcitxInstanceGetCurrentIC(instance), NULL);
        return IRV_DO_NOTHING;
    }

    int index = FcitxInstanceGetIMIndexByName(imselector->owner, (char*) candWord->priv);

    if (index == 0 && FcitxInstanceGetGlobalConfig(instance)->firstAsInactive)
        FcitxInstanceCloseIM(instance, FcitxInstanceGetCurrentIC(instance));
    else {
        if (imselector->global)
            FcitxInstanceSwitchIM(instance, index);
        else
            FcitxInstanceSetLocalIMName(instance, FcitxInstanceGetCurrentIC(instance), (char*) candWord->priv);

        if (FcitxInstanceGetCurrentState(instance) != IS_ACTIVE)
            FcitxInstanceEnableIM(instance, FcitxInstanceGetCurrentIC(instance), false);
    }
    return IRV_DO_NOTHING;
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
    if (!imselector->global) {
        FcitxCandidateWord candWord;
        candWord.callback = IMSelectorGetCand;
        candWord.owner = imselector;
        candWord.priv = NULL;
        candWord.strExtra = NULL;
        candWord.strWord = strdup(_("Clear local state"));
        candWord.wordType = MSG_OTHER;
        FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
    }


    FcitxMessagesAddMessageAtLast(FcitxInputStateGetAuxUp(input), MSG_TIPS, "%s", imselector->global ? _("Select global input method") : _("Select local input method"));
    for (pim = (FcitxIM *) utarray_front(imes);
            pim != NULL;
            pim = (FcitxIM *) utarray_next(imes, pim)) {
        FcitxCandidateWord candWord;
        candWord.callback = IMSelectorGetCand;
        candWord.owner = imselector;
        candWord.priv = strdup(pim->uniqueName);
        candWord.strExtra = NULL;
        candWord.strWord = strdup(pim->strName);
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