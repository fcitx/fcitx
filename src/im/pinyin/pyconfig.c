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

#include <string.h>

#include "pyconfig.h"
#include "fcitx-config/fcitx-config.h"
#include "PYFA.h"

static void FilterGetWordFromPhrase(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);
static void FilterAnAng(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN(FcitxPinyinConfig);
CONFIG_BINDING_REGISTER("Pinyin", "DefaultShuangpinSchema", strDefaultSP);
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
CONFIG_BINDING_REGISTER("Pinyin", "Misstype", bMisstype);
CONFIG_BINDING_END()


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
        pyconfig.cPYYCDZ[0] = a;
        pyconfig.cPYYCDZ[1] = b;
    }
}

void FilterAnAng(ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg)
{
    if (sync == Raw2Value)
    {
        boolean *b = (boolean*)value;
        pyconfig.MHPY_S[5].bMode = *b;
    }
}