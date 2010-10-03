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

#include "core/fcitx.h"
#include "tools/tools.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/xdg.h"
#include "im/pinyin/PYFA.h"
#include "ui/font.h"
#include "core/ime.h"
#include <errno.h>
#include <ctype.h>

extern MHPY MHPY_C[];
extern MHPY MHPY_S[];
extern Display* dpy;
extern int iTriggerKeyCount;
extern XIMTriggerKey* Trigger_Keys;

static Bool IsReloadConfig = False;

FcitxConfig fc;
ConfigFileDesc* fcitxConfigDesc = NULL;
static ConfigFileDesc* GetConfigDesc();
static void FilterAnAng(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void FilterSwitchKey(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void FilterTriggerKey(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void FilterCopyFontEn(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void FilterCopyFontZh(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void Filter2nd3rdKey(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void SetTriggerKeys (char **str, int length);
static Bool MyStrcmp (char *str1, char *str2);
static void FilterGetWordFromPhrase(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);

#ifdef _ENABLE_TRAY
FilterNextTimeEffectBool(UseTray, fc.bUseTrayIcon)
#endif
FilterNextTimeEffectBool(UseDBus, fc.bUseDBus)

CONFIG_BINDING_BEGIN(FcitxConfig);
CONFIG_BINDING_REGISTER_WITH_FILTER("Program", "FontEn", fontEn, FilterCopyFontEn);
CONFIG_BINDING_REGISTER_WITH_FILTER("Program", "FontZh", fontZh, FilterCopyFontZh);
CONFIG_BINDING_REGISTER("Program", "FontLocale", strUserLocale);
#ifdef _ENABLE_RECORDING
CONFIG_BINDING_REGISTER("Program", "RecordFile", strRecordingPath);
#endif
#ifdef _ENABLE_TRAY
CONFIG_BINDING_REGISTER_WITH_FILTER("Program", "UseTray", bUseTrayIcon_, FilterCopyUseTray);
#endif
#ifdef _ENABLE_DBUS
CONFIG_BINDING_REGISTER_WITH_FILTER("Program", "UseDBus", bUseDBus_, FilterCopyUseDBus);
#endif
CONFIG_BINDING_REGISTER("Program", "EnableAddons", bEnableAddons);
CONFIG_BINDING_REGISTER("Output", "HalfPuncAfterNumber", bEngPuncAfterNumber);
CONFIG_BINDING_REGISTER("Output", "EnterAction", enterToDo);
CONFIG_BINDING_REGISTER("Output", "SemiColonAction", semicolonToDo);
CONFIG_BINDING_REGISTER("Output", "InputEngByCapitalChar", bEngAfterCap);
CONFIG_BINDING_REGISTER("Output", "ConvertPunc", bConvertPunc);
CONFIG_BINDING_REGISTER("Output", "LegendModeDisablePaging", bDisablePagingInLegend);
CONFIG_BINDING_REGISTER("Output", "SendTextWhenSwitchEng", bSendTextWhenSwitchEng);
CONFIG_BINDING_REGISTER("Appearance", "CandidateWordNumber", iMaxCandWord);
CONFIG_BINDING_REGISTER("Appearance", "MainWindowHideMode", hideMainWindow);
CONFIG_BINDING_REGISTER("Appearance", "CenterInputWindow", bCenterInputWindow);
CONFIG_BINDING_REGISTER("Appearance", "ShowInputWindowAfterTriggering", bSendTextWhenSwitchEng);
CONFIG_BINDING_REGISTER("Appearance", "ShowPointAfterIndex", bPointAfterNumber);
CONFIG_BINDING_REGISTER("Appearance", "ShowInputSpeed", bShowUserSpeed);
CONFIG_BINDING_REGISTER("Appearance", "ShowVersion", bShowVersion);
CONFIG_BINDING_REGISTER("Appearance", "ShowHintWindow", bHintWindow);
CONFIG_BINDING_REGISTER("Appearance", "SkinType", skinType);
CONFIG_BINDING_REGISTER_WITH_FILTER("Hotkey", "TriggerKey", hkTrigger, FilterTriggerKey);
CONFIG_BINDING_REGISTER_WITH_FILTER("Hotkey", "ChnEngSwitchKey", iSwitchKey, FilterSwitchKey);
CONFIG_BINDING_REGISTER("Hotkey", "DoubleSwitchKey", bDoubleSwitchKey);
CONFIG_BINDING_REGISTER("Hotkey", "TimeInterval", iTimeInterval);
CONFIG_BINDING_REGISTER("Hotkey", "FollowCursorKey", hkTrack);
CONFIG_BINDING_REGISTER("Hotkey", "HideMainWindowKey", hkHideMainWindow);
CONFIG_BINDING_REGISTER("Hotkey", "VKSwitchKey", hkVK);
CONFIG_BINDING_REGISTER("Hotkey", "TraditionalChnSwitchKey", hkGBT);
CONFIG_BINDING_REGISTER("Hotkey", "LegendSwitchKey", hkLegend);
CONFIG_BINDING_REGISTER("Hotkey", "LookupPinyinKey", hkGetPY);
CONFIG_BINDING_REGISTER("Hotkey", "FullWidthSwitchKey", hkCorner);
CONFIG_BINDING_REGISTER("Hotkey", "ChnPuncSwitchKey", hkPunc);
CONFIG_BINDING_REGISTER("Hotkey", "PrevPageKey", hkPrevPage);
CONFIG_BINDING_REGISTER("Hotkey", "NextPageKey", hkNextPage);
CONFIG_BINDING_REGISTER_WITH_FILTER("Hotkey", "SecondThirdCandWordKey", str2nd3rdCand, Filter2nd3rdKey);
CONFIG_BINDING_REGISTER("Hotkey", "SaveAllKey", hkSaveAll);
#ifdef _ENABLE_RECORDING
CONFIG_BINDING_REGISTER("Hotkey", "SetRecordingKey", hkRecording);
CONFIG_BINDING_REGISTER("Hotkey", "ResetRecordingKey", hkResetRecording);
#endif
CONFIG_BINDING_REGISTER("InputMethod", "PinyinOrder", inputMethods[IM_PY]);
CONFIG_BINDING_REGISTER("InputMethod", "ShuangpinOrder", inputMethods[IM_SP]);
CONFIG_BINDING_REGISTER("InputMethod", "DefaultShuangpinSchema", strDefaultSP);
CONFIG_BINDING_REGISTER("InputMethod", "QuweiOrder", inputMethods[IM_QW]);
CONFIG_BINDING_REGISTER("InputMethod", "TableOrder", inputMethods[IM_TABLE]);
CONFIG_BINDING_REGISTER("InputMethod", "PhraseTips", bPhraseTips);
CONFIG_BINDING_REGISTER("Pinyin", "UseCompletePinyin", bFullPY);
CONFIG_BINDING_REGISTER("Pinyin", "AutoCreatePhrase", bPYCreateAuto);
CONFIG_BINDING_REGISTER("Pinyin", "SaveAutoPhrase", bPYSaveAutoAsPhrase);
CONFIG_BINDING_REGISTER("Pinyin", "AddFreqWordKey", hkPYAddFreq);
CONFIG_BINDING_REGISTER("Pinyin", "DeleteFreqWordKey", hkPYDelFreq);
CONFIG_BINDING_REGISTER("Pinyin", "DeleteUserPhraseKey", hkPYDelUserPhr);
CONFIG_BINDING_REGISTER_WITH_FILTER("Pinyin", "InputWordFromPhraseKey", strPYGetWordFromPhrase, FilterGetWordFromPhrase);
CONFIG_BINDING_REGISTER("Pinyin", "BaseOrder", baseOrder);
CONFIG_BINDING_REGISTER("Pinyin", "PhraseOrder", phraseOrder);
CONFIG_BINDING_REGISTER("Pinyin", "FreqOrder", freqOrder);
CONFIG_BINDING_REGISTER_WITH_FILTER("Pinyin", "FuzzyAnAng", MHPY_C[0].bMode, FilterAnAng);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyEnEng", MHPY_C[1].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyIanIang", MHPY_C[2].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyInIng", MHPY_C[3].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyOuU", MHPY_C[4].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyUanUang", MHPY_C[5].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyCCh", MHPY_S[0].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyFH", MHPY_S[1].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyLN", MHPY_S[2].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzySSH", MHPY_S[3].bMode);
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyZZH", MHPY_S[4].bMode);
CONFIG_BINDING_END()

Bool MyStrcmp (char *str1, char *str2)
{
        return !strncmp (str1, str2, strlen (str2));
}

void FilterCopyFontEn(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    char *pstr = *(char **)value;
    if (sync == Raw2Value)
    {
        if (gs.fontEn)
            free(gs.fontEn);
        gs.fontEn = strdup(pstr);
    }
}

void FilterCopyFontZh(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    char *pstr = *(char **)value;
    if (sync == Raw2Value)
    {
        if (gs.fontZh)
            free(gs.fontZh);
        gs.fontZh = strdup(pstr);
    }
}

void FilterGetWordFromPhrase(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    char *pstr = *(char**) value;
    if(sync == Raw2Value){
        char a = '\0';
        char b = '\0';
        if (strlen(pstr) >= 1)
            a = pstr[0];
        if (strlen(pstr) >= 2)
            b = pstr[1];
        fc.cPYYCDZ[0] = a;
        fc.cPYYCDZ[1] = b;
    }
}

void Filter2nd3rdKey(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    char *pstr = *(char**) value;

    if(sync == Raw2Value){
        if (!strcasecmp (pstr, "SHIFT")) {
            fc.i2ndSelectKey = 50;        //左SHIFT的扫描码
            fc.i3rdSelectKey = 62;        //右SHIFT的扫描码
        }
        else if (!strcasecmp (pstr, "CTRL")) {
            fc.i2ndSelectKey = 37;        //左CTRL的扫描码
            fc.i3rdSelectKey = 109;       //右CTRL的扫描码
        }
        else {
            if (pstr[0] && pstr[0]!='0')
                fc.i2ndSelectKey = pstr[0] ^ 0xFF;
            else
                fc.i2ndSelectKey = 0;
        
            if (pstr[1] && pstr[1]!='0')
                fc.i3rdSelectKey = pstr[1] ^ 0xFF;
            else
                fc.i3rdSelectKey = 0;
        }
    }
}

void FilterTriggerKey(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    HOTKEYS *hotkey = (HOTKEYS*)value;
    if (sync == Raw2Value)
    {
        if (hotkey[0].desc == NULL && hotkey[1].desc == NULL)
        {
            hotkey[0].desc = strdup("CTRL_SPACE");
            hotkey[0].iKeyCode = CTRL_SPACE;
        }
        char *strkey[2];
        int i = 0;
        for (;i<2;i++)
        {
            if (hotkey[i].desc == NULL)
                break;
            strkey[i] = hotkey[i].desc;
        }
        SetTriggerKeys(strkey, i);
    }
}

void FilterSwitchKey(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    if (sync == Raw2Value)
    {
        SWITCHKEY *s = (SWITCHKEY*)value;
        switch(*s)
        {
            case S_R_CTRL:
                fc.switchKey = XKeysymToKeycode (dpy, XK_Control_R);
                break;
            case S_R_SHIFT:
                fc.switchKey = XKeysymToKeycode (dpy, XK_Shift_R);
                break;
            case S_L_SHIFT:
                fc.switchKey = XKeysymToKeycode (dpy, XK_Shift_L);
                break;
            case S_R_SUPER:
                fc.switchKey = XKeysymToKeycode (dpy, XK_Super_R);
                break;
            case S_L_SUPER:
                fc.switchKey = XKeysymToKeycode (dpy, XK_Super_L);
                break;
            case S_L_CTRL:
                fc.switchKey = XKeysymToKeycode (dpy, XK_Control_L);
                break;
        }
    }
}
void FilterAnAng(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    if (sync == Raw2Value)
    {
        Bool *b = (Bool*)value;
        fc.MHPY_S[5].bMode = *b;
    }
}

void LoadConfig()
{
    FILE *fp;
    char *file;
    fc.MHPY_C = MHPY_C;
    fc.MHPY_S = MHPY_S;
    fp = GetXDGFileUser( "config", "rt", &file);
    FcitxLog(INFO, _("Load Config File %s"), file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
        {
            SaveConfig();
            LoadConfig();
        }
        return;
    }
    
    ConfigFileDesc* configDesc = GetConfigDesc();
    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);
    
    FcitxConfigConfigBind(&fc, cfile, configDesc);
    ConfigBindSync((GenericConfig*)&fc);

    IsReloadConfig = True;
    
    fclose(fp);
    
    CreateFont();
}

ConfigFileDesc* GetConfigDesc()
{
    if (!fcitxConfigDesc)
    {
        FILE *tmpfp;
        tmpfp = GetXDGFileData("config.desc", "r", NULL);
        fcitxConfigDesc = ParseConfigFileDescFp(tmpfp);
		fclose(tmpfp);
    }

    return fcitxConfigDesc;
}

void SetTriggerKeys (char **strKey, int length)
{
    int             i;
    char           *p;

    if (length == 0)
    {
        strKey[0] = "CTRL_SPACE";
        length = 1;
    }

    iTriggerKeyCount = length - 1;

    if (Trigger_Keys)
        free(Trigger_Keys);

    Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
    for (i = 0; i <= (iTriggerKeyCount + 1); i++) {
        Trigger_Keys[i].keysym = 0;
        Trigger_Keys[i].modifier = 0;
        Trigger_Keys[i].modifier_mask = 0;
    }

    for (i = 0; i <= iTriggerKeyCount; i++) {
        if (MyStrcmp (strKey[i], "CTRL_")) {
            Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | ControlMask;
            Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | ControlMask;
        }
        else if (MyStrcmp (strKey[i], "SHIFT_")) {
            Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | ShiftMask;
            Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | ShiftMask;
        }
        else if (MyStrcmp (strKey[i], "ALT_")) {
            Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | Mod1Mask;
            Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | Mod1Mask;
        }
        else if (MyStrcmp (strKey[i], "SUPER_")) {
            Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | Mod4Mask;
            Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | Mod4Mask;
        }

        if (Trigger_Keys[i].modifier == 0) {
            Trigger_Keys[i].modifier = ControlMask;
            Trigger_Keys[i].modifier_mask = ControlMask;
        }

        p = strKey[i] + strlen (strKey[i]) - 1;
        while (*p != '_') {
            p--;
            if (p == strKey[i] || (p == strKey[i] + strlen (strKey[i]) - 1)) {
                Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
                Trigger_Keys[i].keysym = XK_space;
                return;
            }
        }
        p++;

        if (strlen (p) == 1)
            Trigger_Keys[i].keysym = tolower (*p);
        else if (!strcasecmp (p, "LCTRL"))
            Trigger_Keys[i].keysym = XK_Control_L;
        else if (!strcasecmp (p, "RCTRL"))
            Trigger_Keys[i].keysym = XK_Control_R;
        else if (!strcasecmp (p, "LSHIFT"))
            Trigger_Keys[i].keysym = XK_Shift_L;
        else if (!strcasecmp (p, "RSHIFT"))
            Trigger_Keys[i].keysym = XK_Shift_R;
        else if (!strcasecmp (p, "LALT"))
            Trigger_Keys[i].keysym = XK_Alt_L;
        else if (!strcasecmp (p, "RALT"))
            Trigger_Keys[i].keysym = XK_Alt_R;
        else if (!strcasecmp (p, "LSUPER"))
            Trigger_Keys[i].keysym = XK_Super_L;
        else if (!strcasecmp (p, "RSUPER"))
            Trigger_Keys[i].keysym = XK_Super_R;
        else
            Trigger_Keys[i].keysym = XK_space;
    }
}


void SaveConfig()
{
    ConfigFileDesc* configDesc = GetConfigDesc();
    char *file;
    FILE *fp = GetXDGFileUser("config", "wt", &file);
    FcitxLog(INFO, "Save Config to %s", file);
    SaveConfigFileFp(fp, fc.gconfig.configFile, configDesc);
    free(file);
    fclose(fp);
}
