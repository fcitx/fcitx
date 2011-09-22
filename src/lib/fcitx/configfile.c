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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
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

static ConfigFileDesc* GetConfigDesc();
static void FilterSwitchKey(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void Filter2nd3rdKey(GenericConfig* config, ConfigGroup* group, ConfigOption* option, void* value, ConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN(FcitxConfig)
CONFIG_BINDING_REGISTER("Program", "DelayStart", iDelayStart)
CONFIG_BINDING_REGISTER("Program", "FirstRun", bFirstRun)
CONFIG_BINDING_REGISTER("Program", "ShareStateAmongWindow", shareState)
CONFIG_BINDING_REGISTER("Program", "DefaultInputMethodState", defaultIMState)
CONFIG_BINDING_REGISTER("Output", "HalfPuncAfterNumber", bEngPuncAfterNumber)
CONFIG_BINDING_REGISTER("Output", "EnterAction", enterToDo)
CONFIG_BINDING_REGISTER("Output", "RemindModeDisablePaging", bDisablePagingInRemind)
CONFIG_BINDING_REGISTER("Output", "SendTextWhenSwitchEng", bSendTextWhenSwitchEng)
CONFIG_BINDING_REGISTER("Output", "CandidateWordNumber", iMaxCandWord)
CONFIG_BINDING_REGISTER("Output", "PhraseTips", bPhraseTips)
CONFIG_BINDING_REGISTER("Appearance", "ShowInputWindowAfterTriggering", bShowInputWindowTriggering)
CONFIG_BINDING_REGISTER("Appearance", "ShowPointAfterIndex", bPointAfterNumber)
CONFIG_BINDING_REGISTER("Appearance", "ShowInputSpeed", bShowUserSpeed)
CONFIG_BINDING_REGISTER("Appearance", "ShowVersion", bShowVersion)
CONFIG_BINDING_REGISTER("Appearance", "HideInputWindowWhenOnlyPreeditString", bHideInputWindowWhenOnlyPreeditString);
CONFIG_BINDING_REGISTER("Appearance", "HideInputWindowWhenOnlyOneCandidate", bHideInputWindowWhenOnlyOneCandidate);
CONFIG_BINDING_REGISTER("Hotkey", "TriggerKey", hkTrigger)
CONFIG_BINDING_REGISTER("Hotkey", "IMSwitchKey", bIMSwitchKey)
CONFIG_BINDING_REGISTER_WITH_FILTER("Hotkey", "SwitchKey", iSwitchKey, FilterSwitchKey)
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
CONFIG_BINDING_END()

void Filter2nd3rdKey(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    FCITX_UNUSED(group);
    FCITX_UNUSED(option);
    FCITX_UNUSED(arg);
    FcitxConfig* fc = (FcitxConfig*) config;
    char *pstr = *(char**) value;

    if (sync == Raw2Value) {
        if (!strcasecmp(pstr, "SHIFT")) {
            fc->i2ndSelectKey[0].sym = fc->i2ndSelectKey[1].sym = Key_Shift_L;        //左SHIFT的扫描码
            fc->i2ndSelectKey[0].state = KEY_SHIFT_COMP ;        //左SHIFT的扫描码
            fc->i2ndSelectKey[1].state = KEY_NONE ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[0].sym = fc->i3rdSelectKey[1].sym = Key_Shift_R;        //右SHIFT的扫描码
            fc->i3rdSelectKey[0].state = KEY_SHIFT_COMP ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[1].state = KEY_NONE;
        } else if (!strcasecmp(pstr, "CTRL")) {
            fc->i2ndSelectKey[0].sym = fc->i2ndSelectKey[1].sym = Key_Control_L;        //左CTRL的扫描码
            fc->i2ndSelectKey[0].state = KEY_CTRL_COMP ;        //左SHIFT的扫描码
            fc->i2ndSelectKey[1].state = KEY_NONE ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[0].sym = fc->i3rdSelectKey[1].sym = Key_Control_R;       //右CTRL的扫描码
            fc->i3rdSelectKey[0].state = KEY_CTRL_COMP ;        //左SHIFT的扫描码
            fc->i3rdSelectKey[1].state = KEY_NONE;
        } else {
            if (pstr[0] && pstr[0] != '0') {
                fc->i2ndSelectKey[0].sym = pstr[0] & 0xFF;
                fc->i2ndSelectKey[0].state = KEY_NONE;
            } else {
                fc->i2ndSelectKey[0].sym = 0;
                fc->i2ndSelectKey[0].state = KEY_NONE;
            }
            if (pstr[1] && pstr[1] != '0') {
                fc->i3rdSelectKey[0].sym = pstr[1] & 0xFF;
                fc->i3rdSelectKey[0].state = KEY_NONE;
            } else {
                fc->i3rdSelectKey[0].sym = 0;
                fc->i3rdSelectKey[0].state = KEY_NONE;
            }
        }
    }
}

void FilterSwitchKey(GenericConfig* config, ConfigGroup* group, ConfigOption* option, void* value, ConfigSync sync, void* arg)
{
    FCITX_UNUSED(group);
    FCITX_UNUSED(option);
    FCITX_UNUSED(arg);
    FcitxConfig* fc = (FcitxConfig*) config;
    HOTKEYS* hkey = NULL;
    if (sync == Raw2Value) {
        SWITCHKEY *s = (SWITCHKEY*)value;
        switch (*s) {
        case SWITCHKEY_R_CTRL:
            hkey = FCITX_RCTRL;
            break;
        case SWITCHKEY_R_SHIFT:
            hkey = FCITX_RSHIFT;
            break;
        case SWITCHKEY_L_SHIFT:
            hkey = FCITX_LSHIFT;
            break;
        case SWITCHKEY_L_CTRL:
            hkey = FCITX_LCTRL;
            break;
        case SWITCHKEY_NONE:
            hkey = FCITX_NONE_KEY;
        }
    }
    if (hkey != NULL) {
        fc->switchKey[0] = hkey[0];
        fc->switchKey[1] = hkey[1];
    }
}

FCITX_EXPORT_API
boolean LoadConfig(FcitxConfig* fc)
{
    ConfigFileDesc* configDesc = GetConfigDesc();

    if (configDesc == NULL)
        return false;

    FILE *fp;
    char *file;
    fp = GetXDGFileUserWithPrefix("", "config", "rt", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveConfig(fc);
    }

    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);

    FcitxConfigConfigBind(fc, cfile, configDesc);
    ConfigBindSync((GenericConfig*)fc);

    if (fp)
        fclose(fp);
    return true;
}

CONFIG_DESC_DEFINE(GetConfigDesc, "config.desc")

FCITX_EXPORT_API
void SaveConfig(FcitxConfig* fc)
{
    ConfigFileDesc* configDesc = GetConfigDesc();
    char *file;
    FILE *fp = GetXDGFileUserWithPrefix("", "config", "wt", &file);
    FcitxLog(DEBUG, "Save Config to %s", file);
    SaveConfigFileFp(fp, &fc->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

FCITX_EXPORT_API
int ConfigGetMaxCandWord(FcitxConfig* fc)
{
    return fc->iMaxCandWord;
}

FCITX_EXPORT_API
boolean ConfigGetPointAfterNumber(FcitxConfig* fc)
{
    return fc->bPointAfterNumber;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
