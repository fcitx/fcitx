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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <errno.h>

#include "fcitx/fcitx.h"
#include "configfile.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx/keys.h"
#include "instance.h"

static FcitxConfigFileDesc* GetConfigDesc();
static void Filter2nd3rdKey(FcitxGenericConfig* config, FcitxConfigGroup* group, FcitxConfigOption* option, void* value, FcitxConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN(FcitxGlobalConfig)
CONFIG_BINDING_REGISTER("Program", "DelayStart", iDelayStart)
CONFIG_BINDING_REGISTER("Program", "ShareStateAmongWindow", shareState)
CONFIG_BINDING_REGISTER("Program", "DefaultInputMethodState", _defaultIMState)
CONFIG_BINDING_REGISTER("Output", "HalfPuncAfterNumber", bEngPuncAfterNumber)
CONFIG_BINDING_REGISTER("Output", "EnterAction", enterToDo)
CONFIG_BINDING_REGISTER("Output", "RemindModeDisablePaging", bDisablePagingInRemind)
CONFIG_BINDING_REGISTER("Output", "SendTextWhenSwitchEng", bSendTextWhenSwitchEng)
CONFIG_BINDING_REGISTER("Output", "CandidateWordNumber", iMaxCandWord)
CONFIG_BINDING_REGISTER("Output", "PhraseTips", bPhraseTips)
CONFIG_BINDING_REGISTER("Output", "DontCommitPreeditWhenUnfocus", bDontCommitPreeditWhenUnfocus)
CONFIG_BINDING_REGISTER("Appearance", "ShowInputWindowOnlyWhenActive", bShowInputWindowOnlyWhenActive)
CONFIG_BINDING_REGISTER("Appearance", "ShowInputWindowWhenFocusIn", bShowInputWindowWhenFocusIn)
CONFIG_BINDING_REGISTER("Appearance", "ShowInputWindowAfterTriggering", bShowInputWindowTriggering)
CONFIG_BINDING_REGISTER("Appearance", "ShowInputSpeed", bShowUserSpeed)
CONFIG_BINDING_REGISTER("Appearance", "ShowVersion", bShowVersion)
CONFIG_BINDING_REGISTER("Appearance", "HideInputWindowWhenOnlyPreeditString", bHideInputWindowWhenOnlyPreeditString);
CONFIG_BINDING_REGISTER("Appearance", "HideInputWindowWhenOnlyOneCandidate", bHideInputWindowWhenOnlyOneCandidate);
CONFIG_BINDING_REGISTER("Hotkey", "TriggerKey", hkTrigger)
CONFIG_BINDING_REGISTER("Hotkey", "ActivateKey", hkActivate)
CONFIG_BINDING_REGISTER("Hotkey", "InactivateKey", hkInactivate)
CONFIG_BINDING_REGISTER("Hotkey", "UseExtraTriggerKeyOnlyWhenUseItToInactivate", bUseExtraTriggerKeyOnlyWhenUseItToInactivate)
CONFIG_BINDING_REGISTER("Hotkey", "IMSwitchKey", bIMSwitchKey)
CONFIG_BINDING_REGISTER("Hotkey", "IMSwitchIncludeInactive", bIMSwitchIncludeInactive)
CONFIG_BINDING_REGISTER("Hotkey", "IMSwitchHotkey", iIMSwitchKey)
CONFIG_BINDING_REGISTER("Hotkey", "SwitchKey", iSwitchKey)
CONFIG_BINDING_REGISTER("Hotkey", "DoubleSwitchKey", bDoubleSwitchKey)
CONFIG_BINDING_REGISTER("Hotkey", "TimeInterval", iTimeInterval)
CONFIG_BINDING_REGISTER("Hotkey", "VKSwitchKey", hkVK)
CONFIG_BINDING_REGISTER("Hotkey", "RemindSwitchKey", hkRemind)
CONFIG_BINDING_REGISTER("Hotkey", "FullWidthSwitchKey", hkFullWidthChar)
CONFIG_BINDING_REGISTER("Hotkey", "PuncSwitchKey", hkPunc)
CONFIG_BINDING_REGISTER("Hotkey", "PrevPageKey", hkPrevPage)
CONFIG_BINDING_REGISTER("Hotkey", "NextPageKey", hkNextPage)
CONFIG_BINDING_REGISTER_WITH_FILTER("Hotkey", "SecondThirdCandWordKey", str2nd3rdCand, Filter2nd3rdKey)
CONFIG_BINDING_REGISTER("Hotkey", "SaveAllKey", hkSaveAll)
CONFIG_BINDING_REGISTER("Hotkey", "SwitchPreedit", hkSwitchEmbeddedPreedit);
CONFIG_BINDING_REGISTER("Hotkey", "PrevWord", prevWord);
CONFIG_BINDING_REGISTER("Hotkey", "NextWord", nextWord);
CONFIG_BINDING_REGISTER("Hotkey", "ReloadConfig", hkReloadConfig);
CONFIG_BINDING_END()

void Filter2nd3rdKey(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg)
{
    FCITX_UNUSED(group);
    FCITX_UNUSED(option);
    FCITX_UNUSED(arg);
    FcitxGlobalConfig* fc = (FcitxGlobalConfig*) config;
    char *pstr = *(char**) value;

    if (sync == Raw2Value) {
        if (!strcasecmp(pstr, "SHIFT")) {
            fc->i2ndSelectKey[0].sym = fc->i2ndSelectKey[1].sym = FcitxKey_Shift_L;        //左SHIFT的扫描码
            fc->i2ndSelectKey[0].state = FcitxKeyState_Shift ;        //左SHIFT的扫描码
            fc->i2ndSelectKey[1].state = FcitxKeyState_None ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[0].sym = fc->i3rdSelectKey[1].sym = FcitxKey_Shift_R;        //右SHIFT的扫描码
            fc->i3rdSelectKey[0].state = FcitxKeyState_Shift ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[1].state = FcitxKeyState_None;
        } else if (!strcasecmp(pstr, "CTRL")) {
            fc->i2ndSelectKey[0].sym = fc->i2ndSelectKey[1].sym = FcitxKey_Control_L;        //左CTRL的扫描码
            fc->i2ndSelectKey[0].state = FcitxKeyState_Ctrl ;        //左SHIFT的扫描码
            fc->i2ndSelectKey[1].state = FcitxKeyState_None ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[0].sym = fc->i3rdSelectKey[1].sym = FcitxKey_Control_R;       //右CTRL的扫描码
            fc->i3rdSelectKey[0].state = FcitxKeyState_Ctrl ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[1].state = FcitxKeyState_None;
        } else {
            if (pstr[0]) {
                fc->i2ndSelectKey[0].sym = pstr[0] & 0xFF;
                fc->i2ndSelectKey[0].state = FcitxKeyState_None;
            } else {
                fc->i2ndSelectKey[0].sym = 0;
                fc->i2ndSelectKey[0].state = FcitxKeyState_None;
            }
            if (pstr[0] && pstr[1]) {
                fc->i3rdSelectKey[0].sym = pstr[1] & 0xFF;
                fc->i3rdSelectKey[0].state = FcitxKeyState_None;
            } else {
                fc->i3rdSelectKey[0].sym = 0;
                fc->i3rdSelectKey[0].state = FcitxKeyState_None;
            }
        }
    }
}

FCITX_EXPORT_API
boolean FcitxGlobalConfigLoad(FcitxGlobalConfig* fc)
{
    FcitxConfigFileDesc* configDesc = GetConfigDesc();

    if (configDesc == NULL)
        return false;

    /* removed option */
    fc->bPointAfterNumber = true;

    FILE *fp;
    char *file;
    boolean newconfig = false;
    fp = FcitxXDGGetFileUserWithPrefix("", "config", "r", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            FcitxGlobalConfigSave(fc);
        newconfig = true;
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxGlobalConfigConfigBind(fc, cfile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*)fc);
    if (fc->_defaultIMState == 0) {
        fc->defaultIMState = IS_INACTIVE;
    }
    else
        fc->defaultIMState = IS_ACTIVE;

    if (newconfig) {
        char *p = fcitx_utils_get_current_langcode();
        if (strncmp(p, "ja", 2) == 0) {
            fc->hkTrigger[1].desc = strdup("ZENKAKUHANKAKU");
            fc->hkTrigger[1].sym = FcitxKey_Zenkaku_Hankaku;
            fc->hkTrigger[1].state = FcitxKeyState_None;
        }
        if (strncmp(p, "ko", 2) == 0) {
            fc->hkTrigger[1].desc = strdup("HANGUL");
            fc->hkTrigger[1].sym = FcitxKey_Hangul;
            fc->hkTrigger[1].state = FcitxKeyState_None;
        }
        FcitxGlobalConfigSave(fc);
        free(p);
    }

    if (fp)
        fclose(fp);
    return true;
}

CONFIG_DESC_DEFINE(GetConfigDesc, "config.desc")

FCITX_EXPORT_API
void FcitxGlobalConfigSave(FcitxGlobalConfig* fc)
{
    FcitxConfigFileDesc* configDesc = GetConfigDesc();
    char *file;
    FILE *fp = FcitxXDGGetFileUserWithPrefix("", "config", "w", &file);
    FcitxLog(DEBUG, "Save Config to %s", file);
    FcitxConfigSaveConfigFileFp(fp, &fc->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
