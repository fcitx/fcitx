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

/**
 * @file fcitx-config.c
 * @author CSSlayer wengxt@gmail.com
 * @date 2010-04-30
 *
 * ini style config file
 */
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <libintl.h>
#include <limits.h>
#include <locale.h>

#include "fcitx/fcitx.h"
#include "fcitx-config.h"
#include "fcitx-utils/log.h"
#include "hotkey.h"
#include "fcitx-utils/utils.h"


#define IsColorValid(c) ((c) >=0 && (c) <= 255)

#define RoundColor(c) ((c)>=0?((c)<=255?c:255):0)

/**
 * Config Option parse function
 **/
typedef FcitxConfigSyncResult(*FcitxConfigOptionFunc)(FcitxConfigOption *, FcitxConfigSync);

static FcitxConfigSyncResult FcitxConfigOptionInteger(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionBoolean(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionEnum(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionColor(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionString(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionHotkey(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionChar(FcitxConfigOption *option, FcitxConfigSync sync);
static FcitxConfigSyncResult FcitxConfigOptionI18NString(FcitxConfigOption *option, FcitxConfigSync sync);

/**
 * File type is basically a string, but can be a hint for config tool
 */
#define FcitxConfigOptionFile FcitxConfigOptionString

/**
 * Font type is basically a string, but can be a hint for config tool
 */
#define FcitxConfigOptionFont FcitxConfigOptionString

FCITX_EXPORT_API
FcitxConfigFile *FcitxConfigParseConfigFile(char *filename, FcitxConfigFileDesc* fileDesc)
{
    FILE* fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    FcitxConfigFile *cf = FcitxConfigParseConfigFileFp(fp, fileDesc);

    fclose(fp);

    return cf;
}

FCITX_EXPORT_API
FcitxConfigFile *FcitxConfigParseMultiConfigFile(char **filename, int len, FcitxConfigFileDesc*fileDesc)
{
    FILE **fp = malloc(sizeof(FILE*) * len);
    int i = 0;

    for (i = 0 ; i < len ; i++) {
        fp[i] = fopen(filename[i], "r");
    }

    FcitxConfigFile *cf = FcitxConfigParseMultiConfigFileFp(fp, len, fileDesc);

    for (i = 0 ; i < len ; i++)
        if (fp[i])
            fclose(fp[i]);

    free(fp);

    return cf;
}

FCITX_EXPORT_API
FcitxConfigFile *FcitxConfigParseMultiConfigFileFp(FILE **fp, int len, FcitxConfigFileDesc* fileDesc)
{
    FcitxConfigFile* cfile = NULL;
    int i = 0;

    for (i = 0 ; i < len ; i ++)
        cfile = FcitxConfigParseIniFp(fp[i], cfile);

    /* create a empty one, CheckConfig will do other thing for us */
    if (cfile == NULL)
        cfile = (FcitxConfigFile*) fcitx_utils_malloc0(sizeof(FcitxConfigFile));

    if (FcitxConfigCheckConfigFile(cfile, fileDesc)) {
        return cfile;
    }

    FcitxConfigFreeConfigFile(cfile);

    return NULL;

}

FCITX_EXPORT_API
FcitxConfigFile *FcitxConfigParseConfigFileFp(FILE *fp, FcitxConfigFileDesc* fileDesc)
{
    FcitxConfigFile *cfile = FcitxConfigParseIniFp(fp, NULL);

    /* create a empty one, CheckConfig will do other thing for us */

    if (cfile == NULL)
        cfile = (FcitxConfigFile*) fcitx_utils_malloc0(sizeof(FcitxConfigFile));

    if (FcitxConfigCheckConfigFile(cfile, fileDesc)) {
        return cfile;
    }

    FcitxConfigFreeConfigFile(cfile);

    return NULL;
}

FCITX_EXPORT_API
boolean FcitxConfigCheckConfigFile(FcitxConfigFile *cfile, FcitxConfigFileDesc* cfdesc)
{
    if (!cfile)
        return false;

    HASH_FOREACH(cgdesc, cfdesc->groupsDesc, FcitxConfigGroupDesc) {
        FcitxConfigGroup* group;
        HASH_FIND_STR(cfile->groups, cgdesc->groupName, group);

        if (!group) {
            group = fcitx_utils_malloc0(sizeof(FcitxConfigGroup));
            group->groupName = strdup(cgdesc->groupName);
            group->groupDesc = cgdesc;
            group->options = NULL;
            HASH_ADD_KEYPTR(hh, cfile->groups, group->groupName, strlen(group->groupName), group);
        }

        HASH_FOREACH(codesc, cgdesc->optionsDesc, FcitxConfigOptionDesc) {
            FcitxConfigOption *option;
            HASH_FIND_STR(group->options, codesc->optionName, option);

            if (!option) {
                if (!codesc->rawDefaultValue) {
                    FcitxLog(WARNING, "missing value: %s", codesc->optionName);
                    return false;
                }

                option = fcitx_utils_malloc0(sizeof(FcitxConfigOption));

                option->optionName = strdup(codesc->optionName);
                option->rawValue = strdup(codesc->rawDefaultValue);
                HASH_ADD_KEYPTR(hh, group->options, option->optionName, strlen(option->optionName), option);
            }

            option->optionDesc = codesc;
        }
    }

    cfile->fileDesc = cfdesc;

    return true;
}

/**
 * @brief
 *
 * @param filename
 *
 * @return
 */
FCITX_EXPORT_API
FcitxConfigFileDesc *FcitxConfigParseConfigFileDesc(char* filename)
{
    FILE* fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    FcitxConfigFileDesc *cfdesc = FcitxConfigParseConfigFileDescFp(fp);

    fclose(fp);

    return cfdesc;
}

/**
 * @brief
 *
 * @param fp
 *
 * @return
 */
FCITX_EXPORT_API
FcitxConfigFileDesc *FcitxConfigParseConfigFileDescFp(FILE *fp)
{
    FcitxConfigFile *cfile = FcitxConfigParseIniFp(fp, NULL);

    if (!cfile)
        return NULL;

    FcitxConfigFileDesc *cfdesc = fcitx_utils_malloc0(sizeof(FcitxConfigFileDesc));

    FcitxConfigGroup* group;

    for (group = cfile->groups;
            group != NULL;
            group = (FcitxConfigGroup*)group->hh.next) {
        FcitxConfigGroupDesc *cgdesc = NULL;
        FcitxConfigOption *options = group->options, *option = NULL;

        if (strcmp(group->groupName, "DescriptionFile") == 0) {
            HASH_FIND_STR(options, "LocaleDomain", option);
            cfdesc->domain = strdup(option->rawValue);
            continue;
        }

        char * p = strchr(group->groupName, '/');

        if (p == NULL)
            continue;

        unsigned int groupNameLen = p - group->groupName;

        unsigned int optionNameLen = strlen(p + 1);

        if (groupNameLen == 0 || optionNameLen == 0)
            continue;
        HASH_FIND(hh, cfdesc->groupsDesc, group->groupName,
                  groupNameLen, cgdesc);
        if (!cgdesc) {
            cgdesc = fcitx_utils_new(FcitxConfigGroupDesc);
            cgdesc->groupName = fcitx_utils_set_str_with_len(
                NULL, group->groupName, groupNameLen);
            cgdesc->optionsDesc = NULL;
            HASH_ADD_KEYPTR(hh, cfdesc->groupsDesc, cgdesc->groupName,
                            groupNameLen, cgdesc);
        }
        char *optionName = strdup(p + 1);
        FcitxConfigOptionDesc2 *codesc2 = fcitx_utils_new(FcitxConfigOptionDesc2);
        FcitxConfigOptionDesc *codesc = (FcitxConfigOptionDesc*) codesc2;

        codesc->optionName = optionName;

        codesc->rawDefaultValue = NULL;

        HASH_ADD_KEYPTR(hh, cgdesc->optionsDesc, codesc->optionName, optionNameLen, codesc);

        HASH_FIND_STR(options, "Advance", option);
        if (option && strcmp(option->rawValue, "True") == 0)
            codesc2->advance = true;
        else
            codesc2->advance = false;

        HASH_FIND_STR(options, "Description", option);
        if (option)
            codesc->desc = strdup(option->rawValue);
        else
            codesc->desc = strdup("");

        HASH_FIND_STR(options, "LongDescription", option);
        if (option)
            codesc2->longDesc = strdup(option->rawValue);
        else
            codesc2->longDesc = strdup("");


        /* Processing Type */
        HASH_FIND_STR(options, "Type", option);

        if (option) {
            if (!strcmp(option->rawValue, "Integer")) {
                codesc->type = T_Integer;
                FcitxConfigOption* coption;
                coption = NULL;
                HASH_FIND_STR(options, "Min", coption);
                if (coption) {
                    codesc2->constrain.integerConstrain.min = atoi(coption->rawValue);
                }
                else {
                    codesc2->constrain.integerConstrain.min = INT_MIN;
                }
                coption = NULL;
                HASH_FIND_STR(options, "Max", coption);
                if (coption) {
                    codesc2->constrain.integerConstrain.max = atoi(coption->rawValue);
                }
                else {
                    codesc2->constrain.integerConstrain.max = INT_MAX;
                }
            }
            else if (!strcmp(option->rawValue, "Color"))
                codesc->type = T_Color;
            else if (!strcmp(option->rawValue, "Char"))
                codesc->type = T_Char;
            else if (!strcmp(option->rawValue, "String")) {
                codesc->type = T_String;
                FcitxConfigOption* coption;
                coption = NULL;
                HASH_FIND_STR(options, "MaxLength", coption);
                if (coption) {
                    codesc2->constrain.stringConstrain.maxLength = atoi(coption->rawValue);
                }
            }
            else if (!strcmp(option->rawValue, "I18NString"))
                codesc->type = T_I18NString;
            else if (!strcmp(option->rawValue, "Boolean"))
                codesc->type = T_Boolean;
            else if (!strcmp(option->rawValue, "File"))
                codesc->type = T_File;
            else if (!strcmp(option->rawValue, "Font"))
                codesc->type = T_Font;
            else if (!strcmp(option->rawValue, "Hotkey"))
                codesc->type = T_Hotkey;
            else if (!strcmp(option->rawValue, "Enum")) {
                FcitxConfigOption *eoption;
                codesc->type = T_Enum;
                HASH_FIND_STR(options, "EnumCount", eoption);
                boolean enumError = false;
                int i = 0;

                if (eoption) {
                    int ecount = atoi(eoption->rawValue);

                    if (ecount > 0) {
                        char enumname[FCITX_INT_LEN + strlen("Enum") + 1];
                        memcpy(enumname, "Enum", strlen("Enum"));
                        codesc->configEnum.enumDesc = malloc(sizeof(char*) * ecount);
                        codesc->configEnum.enumCount = ecount;
                        size_t nel = 0;
                        for (i = 0; i < ecount; i++) {
                            sprintf(enumname + strlen("Enum"), "%d", i);
                            HASH_FIND_STR(options, enumname, eoption);

                            if (eoption) {
                                void* entry = lfind(eoption->rawValue, codesc->configEnum.enumDesc, &nel, sizeof(char*), (int (*)(const void *, const void *)) strcmp);

                                if (entry) {
                                    FcitxLog(WARNING, _("Enum option duplicated."));
                                }

                                codesc->configEnum.enumDesc[i] = strdup(eoption->rawValue);
                            } else {
                                enumError = true;
                                goto config_enum_final;
                            }
                        }
                    } else {
                        FcitxLog(WARNING, _("Enum option number must larger than 0"));
                        enumError = true;
                        goto config_enum_final;
                    }
                }

            config_enum_final:

                if (enumError) {
                    int j = 0;

                    for (; j < i; i++)
                        free(codesc->configEnum.enumDesc[j]);

                    FcitxLog(WARNING, _("Enum Option is invalid, take it as string"));

                    codesc->type = T_String;
                }

            } else {
                FcitxLog(WARNING, _("Unknown type, take it as string: %s"), option->rawValue);
                codesc->type = T_String;
            }
        } else {
            FcitxLog(WARNING, _("Missing type, take it as string"));
            codesc->type = T_String;
        }

        /* processing default value */
        HASH_FIND_STR(options, "DefaultValue", option);

        if (option)
            codesc->rawDefaultValue = strdup(option->rawValue);
    }

    FcitxConfigFreeConfigFile(cfile);

    return cfdesc;
}

FcitxConfigSyncResult FcitxConfigOptionInteger(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.integer)
        return SyncNoBinding;

    switch (sync) {

    case Raw2Value: {
        int value = atoi(option->rawValue);
        if (value > option->optionDesc2->constrain.integerConstrain.max || value < option->optionDesc2->constrain.integerConstrain.min)
            return SyncInvalid;
        *option->value.integer = value;
        return SyncSuccess;
    }

    case Value2Raw:
        if (*option->value.integer > option->optionDesc2->constrain.integerConstrain.max || *option->value.integer < option->optionDesc2->constrain.integerConstrain.min)
            return SyncInvalid;

        if (option->rawValue)
            free(option->rawValue);

        asprintf(&option->rawValue, "%d", *option->value.integer);

        return SyncSuccess;

    case ValueFree:
        return SyncSuccess;
    }

    return SyncInvalid;
}

FcitxConfigSyncResult FcitxConfigOptionBoolean(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.boolvalue)
        return SyncNoBinding;

    switch (sync) {

    case Raw2Value:

        if (strcmp(option->rawValue, "True") == 0)
            *option->value.boolvalue = true;
        else
            *option->value.boolvalue = false;

        return SyncSuccess;

    case Value2Raw:
        if (*option->value.boolvalue)
            fcitx_utils_string_swap(&option->rawValue, "True");
        else
            fcitx_utils_string_swap(&option->rawValue, "False");

        return SyncSuccess;

    case ValueFree:
        return SyncSuccess;
    }

    return SyncInvalid;
}

FcitxConfigSyncResult FcitxConfigOptionEnum(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.enumerate || !option->optionDesc)
        return SyncNoBinding;

    FcitxConfigOptionDesc *codesc = option->optionDesc;

    FcitxConfigEnum* cenum = &codesc->configEnum;

    int i = 0;

    switch (sync) {

    case Raw2Value:

        for (i = 0; i < cenum->enumCount; i++) {
            if (strcmp(cenum->enumDesc[i], option->rawValue) == 0) {
                *option->value.enumerate = i;
                return SyncSuccess;
            }
        }

        return SyncInvalid;

    case Value2Raw:

        if (*option->value.enumerate < 0 || *option->value.enumerate >= cenum->enumCount)
            return SyncInvalid;

        fcitx_utils_string_swap(&option->rawValue, cenum->enumDesc[*option->value.enumerate]);

        return SyncSuccess;

    case ValueFree:
        return SyncSuccess;
    }

    return SyncInvalid;
}

FcitxConfigSyncResult FcitxConfigOptionColor(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.color)
        return SyncNoBinding;

    FcitxConfigColor *color = option->value.color;

    int r = 0, g = 0, b = 0;

    switch (sync) {

    case Raw2Value:

        if (sscanf(option->rawValue, "%d %d %d", &r, &g, &b) != 3)
            return SyncInvalid;

        if (IsColorValid(r) && IsColorValid(g) && IsColorValid(b)) {
            color->r = r / 255.0;
            color->g = g / 255.0;
            color->b = b / 255.0;
            return SyncSuccess;
        }

        return SyncInvalid;

    case Value2Raw:
        r = (int)(color->r * 255);
        g = (int)(color->g * 255);
        b = (int)(color->b * 255);
        r = RoundColor(r);
        g = RoundColor(g);
        b = RoundColor(b);

        fcitx_utils_free(option->rawValue);
        option->rawValue = NULL;

        asprintf(&option->rawValue, "%d %d %d", r, g , b);

        return SyncSuccess;

    case ValueFree:
        return SyncSuccess;
    }

    return SyncInvalid;

}

FcitxConfigSyncResult FcitxConfigOptionString(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.string)
        return SyncNoBinding;

    switch (sync) {

    case Raw2Value:
        if (option->optionDesc2->constrain.stringConstrain.maxLength
            && strlen(option->rawValue) > option->optionDesc2->constrain.stringConstrain.maxLength)
            return SyncInvalid;
        fcitx_utils_string_swap(option->value.string, option->rawValue);

        return SyncSuccess;

    case Value2Raw:
        if (option->optionDesc2->constrain.stringConstrain.maxLength
            && strlen(*option->value.string) > option->optionDesc2->constrain.stringConstrain.maxLength)
            return SyncInvalid;
        fcitx_utils_string_swap(&option->rawValue, *option->value.string);

        return SyncSuccess;

    case ValueFree:
        fcitx_utils_free(*option->value.string);
        *option->value.string = NULL;
        return SyncSuccess;
    }

    return SyncInvalid;
}

FcitxConfigSyncResult FcitxConfigOptionI18NString(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.string)
        return SyncNoBinding;

    switch (sync) {

    case Raw2Value:
        fcitx_utils_string_swap(option->value.string, FcitxConfigOptionGetLocaleString(option));

        return SyncSuccess;

    case Value2Raw:
        /* read only */
        return SyncSuccess;

    case ValueFree:
        fcitx_utils_free(*option->value.string);
        *option->value.string = NULL;
        return SyncSuccess;
    }

    return SyncInvalid;
}

const char* FcitxConfigOptionGetLocaleString(FcitxConfigOption* option)
{
    char* locale = setlocale(LC_MESSAGES, NULL);
    char buf[40];
    char *p;
    size_t len;

    if ((p = strchr(locale, '.')) != NULL)
        len = p - locale;
    else
        len = strlen(locale);

    if (len > sizeof(buf))
        return option->rawValue;

    strncpy(buf, locale, len);

    buf[len] = '\0';

    FcitxConfigOptionSubkey* subkey = NULL;

    HASH_FIND_STR(option->subkey, buf, subkey);

    if (subkey)
        return subkey->rawValue;
    else
        return option->rawValue;
}

FcitxConfigSyncResult FcitxConfigOptionChar(FcitxConfigOption *option, FcitxConfigSync sync)
{
    if (!option->value.chr)
        return SyncNoBinding;

    switch (sync) {
    case Raw2Value:
        *option->value.chr = *option->rawValue;
        return SyncSuccess;

    case Value2Raw:
        option->rawValue = realloc(option->rawValue, 2);
        option->rawValue[0] = *option->value.chr;
        option->rawValue[1] = '\0';
        return SyncSuccess;

    case ValueFree:
        return SyncSuccess;
    }

    return SyncInvalid;
}

FcitxConfigSyncResult FcitxConfigOptionHotkey(FcitxConfigOption *option, FcitxConfigSync sync)
{
    /* we assume all hotkey can have 2 candiate key */
    if (!option->value.hotkey)
        return SyncNoBinding;

    switch (sync) {

    case Raw2Value:

        if (option->value.hotkey[0].desc) {
            free(option->value.hotkey[0].desc);
            option->value.hotkey[0].desc = NULL;
        }

        if (option->value.hotkey[1].desc) {
            free(option->value.hotkey[1].desc);
            option->value.hotkey[1].desc = NULL;
        }

        FcitxHotkeySetKey(option->rawValue, option->value.hotkey);

        return SyncSuccess;

    case Value2Raw:

        if (option->rawValue)
            free(option->rawValue);

        if (option->value.hotkey[1].desc) {
            fcitx_utils_alloc_cat_str(option->rawValue,
                                      option->value.hotkey[0].desc,
                                      " ", option->value.hotkey[1].desc);
        } else if (option->value.hotkey[0].desc) {
            option->rawValue = strdup(option->value.hotkey[0].desc);
        } else {
            option->rawValue = strdup("");
        }
        return SyncSuccess;

    case ValueFree:
        FcitxHotkeyFree(option->value.hotkey);
        return SyncSuccess;
    }

    return SyncInvalid;
}

FCITX_EXPORT_API
void FcitxConfigFreeConfigFile(FcitxConfigFile* cfile)
{
    if (!cfile)
        return;

    FcitxConfigGroup *groups = cfile->groups, *curGroup;

    while (groups) {
        curGroup = groups;
        HASH_DEL(groups, curGroup);
        FcitxConfigFreeConfigGroup(curGroup);
    }

    free(cfile);
}

FCITX_EXPORT_API
void FcitxConfigFreeConfigFileDesc(FcitxConfigFileDesc* cfdesc)
{
    if (!cfdesc)
        return;

    FcitxConfigGroupDesc *cgdesc = cfdesc->groupsDesc, *curGroup;

    while (cgdesc) {
        curGroup = cgdesc;
        HASH_DEL(cgdesc, curGroup);
        FcitxConfigFreeConfigGroupDesc(curGroup);
    }

    if (cfdesc->domain)
        free(cfdesc->domain);
    free(cfdesc);
}

FCITX_EXPORT_API
void FcitxConfigFreeConfigGroup(FcitxConfigGroup *group)
{
    FcitxConfigOption *option = group->options, *curOption;

    while (option) {
        curOption = option;
        HASH_DEL(option, curOption);
        FcitxConfigFreeConfigOption(curOption);
    }

    free(group->groupName);

    free(group);
}

FCITX_EXPORT_API
void FcitxConfigFreeConfigGroupDesc(FcitxConfigGroupDesc *cgdesc)
{
    FcitxConfigOptionDesc *codesc = cgdesc->optionsDesc, *curOption;

    while (codesc) {
        curOption = codesc;
        HASH_DEL(codesc, curOption);
        FcitxConfigFreeConfigOptionDesc(curOption);
    }

    free(cgdesc->groupName);

    free(cgdesc);
}

FCITX_EXPORT_API
void FcitxConfigFreeConfigOption(FcitxConfigOption *option)
{
    free(option->optionName);

    FcitxConfigOptionSubkey* item = option->subkey;
    while (item) {
        FcitxConfigOptionSubkey* curitem = item;
        HASH_DEL(item, curitem);
        free(curitem->rawValue);
        free(curitem->subkeyName);
        free(curitem);
    }

    if (option->rawValue)
        free(option->rawValue);

    free(option);
}

FCITX_EXPORT_API
void FcitxConfigFreeConfigOptionDesc(FcitxConfigOptionDesc *codesc)
{
    FcitxConfigOptionDesc2* codesc2 = (FcitxConfigOptionDesc2*) codesc;
    free(codesc->optionName);

    if (codesc->configEnum.enumCount > 0) {
        int i = 0;

        for (i = 0 ; i < codesc->configEnum.enumCount; i ++) {
            free(codesc->configEnum.enumDesc[i]);
        }

        free(codesc->configEnum.enumDesc);
    }

    if (codesc->rawDefaultValue)
        free(codesc->rawDefaultValue);

    free(codesc->desc);
    free (codesc2->longDesc);

    free(codesc);
}

FCITX_EXPORT_API
FcitxConfigFile* FcitxConfigParseIni(char* filename, FcitxConfigFile* reuse)
{
    FILE* fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    FcitxConfigFile *cf = FcitxConfigParseIniFp(fp, reuse);

    fclose(fp);

    return cf;
}

FCITX_EXPORT_API
FcitxConfigFile* FcitxConfigParseIniFp(FILE *fp, FcitxConfigFile *cfile)
{
    char *line = NULL, *buf = NULL;
    size_t len = 0;
    int lineLen = 0;

    if (!fp)
        return cfile;

    if (!cfile)
        cfile = fcitx_utils_new(FcitxConfigFile);

    FcitxConfigGroup* curGroup = NULL;

    int lineNo = 0;

    while (getline(&buf, &len, fp) != -1) {
        lineNo ++;

        if (line)
            free(line);
        line = fcitx_utils_trim(buf);

        lineLen = strlen(line);

        if (lineLen == 0 || line[0] == '#')
            continue;

        if (line[0] == '[') {
            if (!(line[lineLen - 1] == ']' && lineLen != 2)) {
                FcitxLog(ERROR, _("Configure group name error: line %d"), lineNo);
                return NULL;
            }

            size_t grp_len = lineLen - 2;
            HASH_FIND(hh, cfile->groups, line + 1, grp_len, curGroup);
            if (curGroup) {
                FcitxLog(DEBUG, _("Duplicate group name, "
                                  "merge with the previous: %s :line %d"),
                         curGroup->groupName, lineNo);
                continue;
            }

            char *groupName;
            groupName = fcitx_utils_set_str_with_len(NULL, line + 1, grp_len);
            curGroup = fcitx_utils_malloc0(sizeof(FcitxConfigGroup));
            curGroup->groupName = groupName;
            curGroup->options = NULL;
            curGroup->groupDesc = NULL;
            HASH_ADD_KEYPTR(hh, cfile->groups, curGroup->groupName,
                            grp_len, curGroup);
        } else {
            if (curGroup == NULL)
                continue;

            char *value = strchr(line, '=');

            if (!value) {
                FcitxLog(WARNING, _("Invalid Entry: line %d missing '='"), lineNo);
                goto next_line;
            }

            if (line == value)
                goto next_line;

            *value = '\0';

            value ++;

            char *name = line;

            /* check subkey */
            char *subkeyname = NULL;

            if ((subkeyname = strchr(name, '[')) != NULL) {
                size_t namelen = strlen(name);

                if (name[namelen - 1] == ']') {
                    /* there is a subkey */
                    *subkeyname = '\0';
                    subkeyname++;
                    name[namelen - 1] = '\0';
                }
            }

            FcitxConfigOption *option;

            HASH_FIND_STR(curGroup->options, name, option);

            if (option) {
                if (subkeyname) {
                    FcitxConfigOptionSubkey* subkey = NULL;
                    HASH_FIND_STR(option->subkey, subkeyname, subkey);

                    if (subkey) {
                        free(subkey->rawValue);
                        subkey->rawValue = strdup(value);
                    } else {
                        subkey = fcitx_utils_malloc0(sizeof(FcitxConfigOptionSubkey));
                        subkey->subkeyName = strdup(subkeyname);
                        subkey->rawValue = strdup(value);
                        HASH_ADD_KEYPTR(hh, option->subkey, subkey->subkeyName, strlen(subkey->subkeyName), subkey);
                    }
                } else {
                    FcitxLog(DEBUG, _("Duplicate option, overwrite: line %d"), lineNo);
                    free(option->rawValue);
                    option->rawValue = strdup(value);
                }
            } else {
                option = fcitx_utils_malloc0(sizeof(FcitxConfigOption));
                option->optionName = strdup(name);
                option->rawValue = strdup(value);
                HASH_ADD_KEYPTR(hh, curGroup->options, option->optionName, strlen(option->optionName), option);

                /* if the subkey is new, and no default key exists, so we can assign it to default value */

                if (subkeyname) {
                    FcitxConfigOptionSubkey* subkey = NULL;
                    subkey = fcitx_utils_malloc0(sizeof(FcitxConfigOptionSubkey));
                    subkey->subkeyName = strdup(subkeyname);
                    subkey->rawValue = strdup(value);
                    HASH_ADD_KEYPTR(hh, option->subkey, subkey->subkeyName, strlen(subkey->subkeyName), subkey);
                }
            }
        }

    next_line:

        continue;
    }

    if (line)
        free(line);

    if (buf)
        free(buf);

    return cfile;
}

FCITX_EXPORT_API
boolean FcitxConfigSaveConfigFile(char *filename, FcitxGenericConfig *cfile, FcitxConfigFileDesc* cdesc)
{
    FILE* fp = fopen(filename, "w");

    if (!fp)
        return false;

    boolean result = FcitxConfigSaveConfigFileFp(fp, cfile, cdesc);

    fclose(fp);

    return result;
}

FCITX_EXPORT_API
void FcitxConfigFree(FcitxGenericConfig* config)
{
    FcitxConfigFile *cfile = config->configFile;
    FcitxConfigFileDesc *cdesc = NULL;

    if (!cfile)
        return;

    cdesc = cfile->fileDesc;

    HASH_FOREACH(groupdesc, cdesc->groupsDesc, FcitxConfigGroupDesc) {
        FcitxConfigGroup *group = NULL;
        HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);
        HASH_FOREACH(optiondesc, groupdesc->optionsDesc, FcitxConfigOptionDesc) {
            FcitxConfigOption *option = NULL;

            if (group)
                HASH_FIND_STR(group->options, optiondesc->optionName, option);

            FcitxConfigSyncValue(config, group, option, ValueFree);
        }
    }

    FcitxConfigFreeConfigFile(cfile);
}

FCITX_EXPORT_API
void FcitxConfigBindSync(FcitxGenericConfig* config)
{
    FcitxConfigFile *cfile = config->configFile;
    FcitxConfigFileDesc *cdesc = NULL;

    if (!cfile)
        return;

    cdesc = cfile->fileDesc;

    HASH_FOREACH(groupdesc, cdesc->groupsDesc, FcitxConfigGroupDesc) {
        FcitxConfigGroup *group = NULL;
        HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);

        HASH_FOREACH(optiondesc, groupdesc->optionsDesc, FcitxConfigOptionDesc) {
            FcitxConfigOption *option = NULL;

            if (group)
                HASH_FIND_STR(group->options, optiondesc->optionName, option);

            FcitxConfigSyncValue(config, group, option, Raw2Value);
        }
    }
}

FCITX_EXPORT_API
void FcitxConfigSyncValue(FcitxGenericConfig* config, FcitxConfigGroup* group, FcitxConfigOption *option, FcitxConfigSync sync)
{
    FcitxConfigOptionDesc *codesc = option->optionDesc;

    FcitxConfigOptionFunc f = NULL;

    if (codesc == NULL)
        return;

    if (sync == Value2Raw)
        if (option->filter)
            option->filter(config, group, option, option->value.untype, sync, option->filterArg);

    switch (codesc->type) {

    case T_Integer:
        f = FcitxConfigOptionInteger;
        break;

    case T_Color:
        f = FcitxConfigOptionColor;
        break;

    case T_Boolean:
        f = FcitxConfigOptionBoolean;
        break;

    case T_Enum:
        f = FcitxConfigOptionEnum;
        break;

    case T_String:
        f = FcitxConfigOptionString;
        break;

    case T_I18NString:
        f = FcitxConfigOptionI18NString;
        break;

    case T_Hotkey:
        f = FcitxConfigOptionHotkey;
        break;

    case T_File:
        f = FcitxConfigOptionFile;
        break;

    case T_Font:
        f = FcitxConfigOptionFont;
        break;

    case T_Char:
        f = FcitxConfigOptionChar;
        break;
    }

    FcitxConfigSyncResult r = SyncNoBinding;

    if (f)
        r = f(option, sync);

    if (r == SyncInvalid) {
        if (codesc->rawDefaultValue) {
            FcitxLog(WARNING, _("Option %s is Invalid, Use Default Value %s"),
                     option->optionName, codesc->rawDefaultValue);
            fcitx_utils_free(option->rawValue);
            option->rawValue = strdup(codesc->rawDefaultValue);

            if (sync == Raw2Value)
                f(option, sync);
        } else {
            FcitxLog(ERROR, _("Option %s is Invalid."), option->optionName);
        }
    }

    if (sync == Raw2Value)
        if (option->filter)
            option->filter(config, group, option, option->value.untype, sync, option->filterArg);
}

FCITX_EXPORT_API
boolean FcitxConfigSaveConfigFileFp(FILE* fp, FcitxGenericConfig *config, FcitxConfigFileDesc* cdesc)
{
    if (!fp)
        return false;

    FcitxConfigFile* cfile = config->configFile;

    HASH_FOREACH(groupdesc, cdesc->groupsDesc, FcitxConfigGroupDesc) {
        fprintf(fp, "[%s]\n", groupdesc->groupName);

        FcitxConfigGroup *group = NULL;

        if (cfile)
            HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);

        HASH_FOREACH(optiondesc, groupdesc->optionsDesc, FcitxConfigOptionDesc) {
            FcitxConfigOption *option = NULL;

            if (group)
                HASH_FIND_STR(group->options, optiondesc->optionName, option);

            if (optiondesc->desc && strlen(optiondesc->desc) != 0)
                fprintf(fp, "# %s\n", dgettext(cdesc->domain, optiondesc->desc));

            switch (optiondesc->type) {
            case T_Enum: {
                fprintf(fp, "# %s\n", _("Available Value:"));
                fprintf(fp, "#");
                int i;
                for (i = 0; i < optiondesc->configEnum.enumCount; i++)
                    fprintf(fp, " %s", optiondesc->configEnum.enumDesc[i]);
                fprintf(fp, "\n");
            }
            break;
            case T_Boolean: {
                fprintf(fp, "# %s\n", _("Available Value:"));
                fprintf(fp, "# True False\n");
            }
            break;
            default:
                break;
            }

            if (!option) {
                if (optiondesc->rawDefaultValue)
                    fprintf(fp, "#%s=%s\n", optiondesc->optionName,
                            optiondesc->rawDefaultValue);
                else
                    FcitxLog(FATAL, _("no default option for %s/%s"),
                             groupdesc->groupName, optiondesc->optionName);
            } else {
                FcitxConfigSyncValue(config, group, option, Value2Raw);
                /* comment out the default value, for future automatical change */
                if (optiondesc->rawDefaultValue && strcmp(option->rawValue, optiondesc->rawDefaultValue) == 0)
                    fprintf(fp, "#");
                fprintf(fp, "%s=%s\n", option->optionName, option->rawValue);
                HASH_FOREACH(subkey, option->subkey, FcitxConfigOptionSubkey) {
                    fprintf(fp, "%s[%s]=%s\n", option->optionName, subkey->subkeyName, subkey->rawValue);
                }
            }
        }

        fprintf(fp, "\n");
    }

    return true;
}

FCITX_EXPORT_API
void FcitxConfigBindValue(FcitxConfigFile* cfile, const char *groupName, const char *optionName, void* var, FcitxSyncFilter filter, void *arg)
{
    FcitxConfigGroup *group = NULL;
    HASH_FIND_STR(cfile->groups, groupName, group);

    if (group) {
        FcitxConfigOption *option = NULL;
        HASH_FIND_STR(group->options, optionName, option);

        if (option) {
            FcitxConfigOptionDesc* codesc = option->optionDesc;
            option->filter = filter;
            option->filterArg = arg;

            if (!codesc) {
                FcitxLog(WARNING, "Unknown Option: %s/%s", groupName, optionName);
                return;
            }

            switch (codesc->type) {

            case T_Char:
                option->value.chr = (char*) var;
                break;

            case T_Integer:
                option->value.integer = (int*) var;
                break;

            case T_Color:
                option->value.color = (FcitxConfigColor*) var;
                break;

            case T_Boolean:
                option->value.boolvalue = (boolean*) var;
                break;

            case T_Hotkey:
                option->value.hotkey = (FcitxHotkey*) var;
                break;

            case T_Enum:
                option->value.enumerate = (int*) var;
                break;

            case T_I18NString:

            case T_String:

            case T_File:

            case T_Font:
                option->value.string = (char**) var;
                break;
            }
        }
    }
}

FCITX_EXPORT_API
FcitxConfigValueType FcitxConfigGetBindValue(FcitxGenericConfig *config, const char *groupName, const char* optionName)
{
    FcitxConfigFile* cfile = config->configFile;
    FcitxConfigGroup *group = NULL;
    FcitxConfigValueType null;
    memset(&null, 0, sizeof(FcitxConfigValueType));
    HASH_FIND_STR(cfile->groups, groupName, group);

    if (group) {
        FcitxConfigOption *option = NULL;
        HASH_FIND_STR(group->options, optionName, option);

        if (option) {
            return option->value;
        }
    }

    return null;

}

FCITX_EXPORT_API
const FcitxConfigOptionDesc* FcitxConfigDescGetOptionDesc(FcitxConfigFileDesc* cfdesc, const char* groupName, const char* optionName)
{
    FcitxConfigGroupDesc* groupDesc;
    HASH_FIND_STR(cfdesc->groupsDesc, groupName, groupDesc);

    if (groupDesc) {
        FcitxConfigOptionDesc *optionDesc = NULL;
        HASH_FIND_STR(groupDesc->optionsDesc, optionName, optionDesc);

        if (optionDesc) {
            return optionDesc;
        }
    }

    return NULL;
}

FCITX_EXPORT_API
FcitxConfigOption* FcitxConfigFileGetOption(FcitxConfigFile* cfile, const char* groupName, const char* optionName)
{
    FcitxConfigGroup* group;
    HASH_FIND_STR(cfile->groups, groupName, group);

    if (group) {
        FcitxConfigOption *option = NULL;
        HASH_FIND_STR(group->options, optionName, option);

        if (option) {
            return option;
        }
    }

    return NULL;
}


FCITX_EXPORT_API
void FcitxConfigResetConfigToDefaultValue(FcitxGenericConfig* config)
{
    FcitxConfigFile* cfile = config->configFile;

    if (!cfile)
        return;

    FcitxConfigFileDesc* cfdesc = cfile->fileDesc;

    if (!cfdesc)
        return;

    HASH_FOREACH(cgdesc, cfdesc->groupsDesc, FcitxConfigGroupDesc) {
        FcitxConfigGroup* group;
        HASH_FIND_STR(cfile->groups, cgdesc->groupName, group);

        if (!group) {
            /* should not happen */
            continue;
        }

        HASH_FOREACH(codesc, cgdesc->optionsDesc, FcitxConfigOptionDesc) {
            FcitxConfigOption *option;
            HASH_FIND_STR(group->options, codesc->optionName, option);

            if (!option) {
                /* should not happen */
                continue;
            }

            if (!codesc->rawDefaultValue) {
                /* ignore it, actually the reset is meaningless
                 * if it doesn't have a default value. */
                continue;
            }

            if (option->rawValue)
                free(option->rawValue);

            option->rawValue = strdup(codesc->rawDefaultValue);
        }
    }
}
// kate: indent-mode cstyle; space-indent on; indent-width 4;
