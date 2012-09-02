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

#include <string.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/log.h"
#include "pyconfig.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/xdg.h"
#include "PYFA.h"
#include <stdlib.h>
#include <errno.h>

static void FilterAnAng(FcitxGenericConfig* config, FcitxConfigGroup *group,
                        FcitxConfigOption *option, void* value,
                        FcitxConfigSync sync, void* arg);
static FcitxConfigFileDesc* GetPYConfigDesc();

CONFIG_BINDING_BEGIN(FcitxPinyinConfig)
CONFIG_BINDING_REGISTER("Pinyin", "DefaultShuangpinSchema", spscheme)
CONFIG_BINDING_REGISTER("Pinyin", "FixCursorAtHead", bFixCursorAtHead)
CONFIG_BINDING_REGISTER("Pinyin", "UseVForQuickPhrase", bUseVForQuickPhrase)
CONFIG_BINDING_REGISTER("Pinyin", "UseCompletePinyin", bFullPY)
CONFIG_BINDING_REGISTER("Pinyin", "AutoCreatePhrase", bPYCreateAuto)
CONFIG_BINDING_REGISTER("Pinyin", "SaveAutoPhrase", bPYSaveAutoAsPhrase)
CONFIG_BINDING_REGISTER("Pinyin", "AddFreqWordKey", hkPYAddFreq)
CONFIG_BINDING_REGISTER("Pinyin", "DeleteFreqWordKey", hkPYDelFreq)
CONFIG_BINDING_REGISTER("Pinyin", "DeleteUserPhraseKey", hkPYDelUserPhr)
CONFIG_BINDING_REGISTER("Pinyin", "BaseOrder", baseOrder)
CONFIG_BINDING_REGISTER("Pinyin", "PhraseOrder", phraseOrder)
CONFIG_BINDING_REGISTER("Pinyin", "FreqOrder", freqOrder)
CONFIG_BINDING_REGISTER_WITH_FILTER("Pinyin", "FuzzyAnAng", MHPY_C[0].bMode,
                                    FilterAnAng)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyEnEng", MHPY_C[1].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyIanIang", MHPY_C[2].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyInIng", MHPY_C[3].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyOuU", MHPY_C[4].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyUanUang", MHPY_C[5].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyCCh", MHPY_S[0].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyFH", MHPY_S[1].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyLN", MHPY_S[2].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzySSH", MHPY_S[3].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "FuzzyZZH", MHPY_S[4].bMode)
CONFIG_BINDING_REGISTER("Pinyin", "Misstype", bMisstypeNGGN)
CONFIG_BINDING_REGISTER("Pinyin", "MisstypeVU", MHPY_C[6].bMode)
CONFIG_BINDING_END()

void FilterAnAng(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg)
{
    FcitxPinyinConfig* pyconfig = (FcitxPinyinConfig*) config;
    if (sync == Raw2Value) {
        boolean *b = (boolean*)value;
        pyconfig->MHPY_S[5].bMode = *b;
    }
}

boolean LoadPYConfig(FcitxPinyinConfig *pyconfig)
{
    FcitxConfigFileDesc* configDesc = GetPYConfigDesc();
    if (configDesc == NULL)
        return false;
    FILE *fp;
    char *file;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-pinyin.config", "r", &file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SavePYConfig(pyconfig);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxPinyinConfigConfigBind(pyconfig, cfile, configDesc);

    FcitxConfigOption* option = FcitxConfigFileGetOption(cfile, "Pinyin", "DefaultShuangpinSchema");
    if (option != NULL && option->rawValue && option->optionDesc) {
        char* needfree = NULL;
        if (strcmp(option->rawValue, "自然码") == 0) {
            needfree = option->rawValue;
            option->rawValue = strdup(option->optionDesc->configEnum.enumDesc[SP_ZIRANMA]);
        } else if (strcmp(option->rawValue, "微软") == 0) {
            needfree = option->rawValue;
            option->rawValue = strdup(option->optionDesc->configEnum.enumDesc[SP_MS]);
        } else if (strcmp(option->rawValue, "紫光") == 0) {
            needfree = option->rawValue;
            option->rawValue = strdup(option->optionDesc->configEnum.enumDesc[SP_ZIGUANG]);
        } else if (strcmp(option->rawValue, "拼音加加") == 0) {
            needfree = option->rawValue;
            option->rawValue = strdup(option->optionDesc->configEnum.enumDesc[SP_PINYINJIAJIA]);
        } else if (strcmp(option->rawValue, "中文之星") == 0) {
            needfree = option->rawValue;
            option->rawValue = strdup(option->optionDesc->configEnum.enumDesc[SP_ZHONGWENZHIXING]);
        } else if (strcmp(option->rawValue, "智能ABC") == 0) {
            needfree = option->rawValue;
            option->rawValue = strdup(option->optionDesc->configEnum.enumDesc[SP_ABC]);
        }
        if (needfree)
            free(needfree);
    }

    FcitxConfigBindSync((FcitxGenericConfig*)pyconfig);

    if (fp)
        fclose(fp);
    return true;
}

void SavePYConfig(FcitxPinyinConfig* pyconfig)
{
    FcitxConfigFileDesc* configDesc = GetPYConfigDesc();
    char *file;
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-pinyin.config", "w", &file);
    FcitxConfigSaveConfigFileFp(fp, &pyconfig->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

CONFIG_DESC_DEFINE(GetPYConfigDesc, "fcitx-pinyin.desc")
// kate: indent-mode cstyle; space-indent on; indent-width 0;
