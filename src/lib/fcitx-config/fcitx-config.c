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
 * @brief ini style config file
 */
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx-config.h"
#include "fcitx-utils/log.h"
#include "hotkey.h"
#include <fcitx-utils/utils.h>
#include <locale.h>


#define IsColorValid(c) ((c) >=0 && (c) <= 255)

#define RoundColor(c) ((c)>=0?((c)<=255?c:255):0)

/**
 * @brief Config Option parse function
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
 * @brief File type is basically a string, but can be a hint for config tool
 */
#define FcitxConfigOptionFile FcitxConfigOptionString

/**
 * @brief Font type is basically a string, but can be a hint for config tool
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
        fp[i] = NULL;
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

    FcitxConfigGroupDesc *cgdesc = NULL;

    for (cgdesc = cfdesc->groupsDesc;
            cgdesc != NULL;
            cgdesc = (FcitxConfigGroupDesc*)cgdesc->hh.next) {
        FcitxConfigOptionDesc *codesc = NULL;
        FcitxConfigGroup* group;
        HASH_FIND_STR(cfile->groups, cgdesc->groupName, group);

        if (!group) {
            group = fcitx_utils_malloc0(sizeof(FcitxConfigGroup));
            group->groupName = strdup(cgdesc->groupName);
            group->groupDesc = cgdesc;
            group->options = NULL;
            HASH_ADD_KEYPTR(hh, cfile->groups, group->groupName, strlen(group->groupName), group);
        }

        for (codesc = cgdesc->optionsDesc;
                codesc != NULL;
                codesc = (FcitxConfigOptionDesc*)codesc->hh.next) {
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

        int groupNameLen = p - group->groupName;

        int optionNameLen = strlen(p + 1);

        if (groupNameLen == 0 || optionNameLen == 0)
            continue;

        char *groupName = malloc(sizeof(char) * (groupNameLen + 1));

        strncpy(groupName, group->groupName, groupNameLen);

        groupName[groupNameLen] = '\0';

        char *optionName = strdup(p + 1);

        HASH_FIND_STR(cfdesc->groupsDesc, groupName, cgdesc);

        if (!cgdesc) {
            cgdesc = fcitx_utils_malloc0(sizeof(FcitxConfigGroupDesc));
            cgdesc->groupName = groupName;
            cgdesc->optionsDesc = NULL;
            HASH_ADD_KEYPTR(hh, cfdesc->groupsDesc, cgdesc->groupName, groupNameLen, cgdesc);
        } else
            free(groupName);

        FcitxConfigOptionDesc *codesc = fcitx_utils_malloc0(sizeof(FcitxConfigOptionDesc));

        codesc->optionName = optionName;

        codesc->rawDefaultValue = NULL;

        HASH_ADD_KEYPTR(hh, cgdesc->optionsDesc, codesc->optionName, optionNameLen, codesc);

        HASH_FIND_STR(options, "Description", option);

        if (option)
            codesc->desc = strdup(option->rawValue);
        else
            codesc->desc = strdup("");

        /* Processing Type */
        HASH_FIND_STR(options, "Type", option);

        if (option) {
            if (!strcmp(option->rawValue, "Integer"))
                codesc->type = T_Integer;
            else if (!strcmp(option->rawValue, "Color"))
                codesc->type = T_Color;
            else if (!strcmp(option->rawValue, "Char"))
                codesc->type = T_Char;
            else if (!strcmp(option->rawValue, "String"))
                codesc->type = T_String;
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
                        /* 10个选项基本足够了，以后有需求再添加 */
                        if (ecount > 10)
                            ecount = 10;

                        char *enumname = strdup("Enum0");

                        codesc->configEnum.enumDesc = malloc(sizeof(char*) * ecount);

                        codesc->configEnum.enumCount = ecount;

                        size_t nel = 0;

                        for (i = 0; i < ecount; i++) {
                            enumname[4] = '0' + i;
                            HASH_FIND_STR(options, enumname, eoption);

                            if (eoption) {
                                void* entry = lfind(eoption->rawValue, codesc->configEnum.enumDesc, &nel, sizeof(char*), (int (*)(const void *, const void *)) strcmp);

                                if (entry) {
                                    FcitxLog(WARNING, _("Enum option duplicated."));
                                }

                                codesc->configEnum.enumDesc[i] = strdup(eoption->rawValue);
                            } else {
                                enumError = true;
                                free(enumname);
                                goto config_enum_final;
                            }
                        }

                        free(enumname);
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

    case Raw2Value:
        *option->value.integer = atoi(option->rawValue);
        return SyncSuccess;

    case Value2Raw:

        if (option->rawValue)
            free(option->rawValue);

        asprintf(&option->rawValue, "%d", *option->value.integer);

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
        if (option->rawValue)
            free(option->rawValue);

        if (*option->value.boolvalue)
            option->rawValue = strdup("True");
        else
            option->rawValue = strdup("False");

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

        if (option->rawValue)
            free(option->rawValue);

        option->rawValue = strdup(cenum->enumDesc[*option->value.enumerate]);

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

        if (option->rawValue)
            free(option->rawValue);

        option->rawValue = NULL;

        asprintf(&option->rawValue, "%d %d %d", r, g , b);

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

        if (*option->value.string)
            free(*option->value.string);

        *option->value.string = strdup(option->rawValue);

        return SyncSuccess;

    case Value2Raw:
        if (option->rawValue)
            free(option->rawValue);

        option->rawValue = strdup(*option->value.string);

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

        if (*option->value.string)
            free(*option->value.string);

        *option->value.string = strdup(FcitxConfigOptionGetLocaleString(option));

        return SyncSuccess;

    case Value2Raw:
        /* read only */
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

        if (strlen(option->rawValue) == 0)
            *option->value.chr = '\0';
        else
            *option->value.chr = option->rawValue[0];

        return SyncSuccess;

    case Value2Raw:
        if (option->rawValue)
            free(option->rawValue);

        asprintf(&option->rawValue, "%c", *option->value.chr);

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
            option->value.hotkey[0].desc = NULL;
        }

        FcitxHotkeySetKey(option->rawValue, option->value.hotkey);

        return SyncSuccess;

    case Value2Raw:

        if (option->rawValue)
            free(option->rawValue);

        if (option->value.hotkey[1].desc)
            asprintf(&option->rawValue, "%s %s", option->value.hotkey[0].desc, option->value.hotkey[1].desc);
        else if (option->value.hotkey[0].desc) {
            option->rawValue = strdup(option->value.hotkey[0].desc);
        } else
            option->rawValue = strdup("");

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
FcitxConfigFile* FcitxConfigParseIniFp(FILE *fp, FcitxConfigFile* reuse)
{
    char *line = NULL, *buf = NULL;
    size_t len = 0;
    int lineLen = 0;
    FcitxConfigFile* cfile;

    if (!fp) {
        if (reuse)
            return reuse;
        else
            return NULL;
    }

    if (reuse)
        cfile = reuse;
    else
        cfile = fcitx_utils_malloc0(sizeof(FcitxConfigFile));

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

            char *groupName;

            groupName = malloc(sizeof(char) * (lineLen - 2 + 1));
            strncpy(groupName, line + 1, lineLen - 2);
            groupName[lineLen - 2] = '\0';
            HASH_FIND_STR(cfile->groups, groupName, curGroup);

            if (curGroup) {
                FcitxLog(DEBUG, _("Duplicate group name, merge with the previous: %s :line %d"), groupName, lineNo);
                free(groupName);
                continue;
            }

            curGroup = fcitx_utils_malloc0(sizeof(FcitxConfigGroup));

            curGroup->groupName = groupName;
            curGroup->options = NULL;
            curGroup->groupDesc = NULL;
            HASH_ADD_KEYPTR(hh, cfile->groups, curGroup->groupName, strlen(curGroup->groupName), curGroup);
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
void FcitxConfigBindSync(FcitxGenericConfig* config)
{
    FcitxConfigFile *cfile = config->configFile;
    FcitxConfigFileDesc *cdesc = NULL;
    FcitxConfigGroupDesc* groupdesc = NULL;

    if (!cfile)
        return;

    cdesc = cfile->fileDesc;

    for (groupdesc = cdesc->groupsDesc;
            groupdesc != NULL;
            groupdesc = (FcitxConfigGroupDesc*)groupdesc->hh.next) {
        FcitxConfigOptionDesc *optiondesc;
        FcitxConfigGroup *group = NULL;
        HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);

        for (optiondesc = groupdesc->optionsDesc;
                optiondesc != NULL;
                optiondesc = (FcitxConfigOptionDesc*)optiondesc->hh.next) {
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
            FcitxLog(WARNING, _("Option %s is Invalid, Use Default Value %s"), option->optionName, codesc->rawDefaultValue);

            if (option->rawValue)
                free(option->rawValue);

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

    FcitxConfigGroupDesc* groupdesc = NULL;

    for (groupdesc = cdesc->groupsDesc;
            groupdesc != NULL;
            groupdesc = (FcitxConfigGroupDesc*)groupdesc->hh.next) {
        FcitxConfigOptionDesc *optiondesc;
        fprintf(fp, "[%s]\n", groupdesc->groupName);

        FcitxConfigGroup *group = NULL;

        if (cfile)
            HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);

        for (optiondesc = groupdesc->optionsDesc;
                optiondesc != NULL;
                optiondesc = (FcitxConfigOptionDesc*)optiondesc->hh.next) {
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
                    fprintf(fp, "%s=%s\n", optiondesc->optionName, optiondesc->rawDefaultValue);
                else
                    FcitxLog(FATAL, _("no default option for %s/%s"), groupdesc->groupName, optiondesc->optionName);
            } else {
                FcitxConfigSyncValue(config, group, option, Value2Raw);
                fprintf(fp, "%s=%s\n", option->optionName, option->rawValue);
                FcitxConfigOptionSubkey* subkey;

                for (subkey = option->subkey;
                        subkey != NULL;
                        subkey = subkey->hh.next) {
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

    FcitxConfigGroupDesc *cgdesc = NULL;

    for (cgdesc = cfdesc->groupsDesc;
            cgdesc != NULL;
            cgdesc = (FcitxConfigGroupDesc*)cgdesc->hh.next) {
        FcitxConfigOptionDesc *codesc = NULL;
        FcitxConfigGroup* group;
        HASH_FIND_STR(cfile->groups, cgdesc->groupName, group);

        if (!group) {
            /* should not happen */
            continue;
        }

        for (codesc = cgdesc->optionsDesc;
                codesc != NULL;
                codesc = (FcitxConfigOptionDesc*)codesc->hh.next) {
            FcitxConfigOption *option;
            HASH_FIND_STR(group->options, codesc->optionName, option);

            if (!option) {
                /* should not happen */
                continue;
            }

            if (!codesc->rawDefaultValue) {
                /* ignore it, actually if it doesn't have a default value, the reset is meaning less */
                continue;
            }

            if (option->rawValue)
                free(option->rawValue);

            option->rawValue = strdup(codesc->rawDefaultValue);
        }
    }
}
// kate: indent-mode cstyle; space-indent on; indent-width 4;
