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

/**
 * @file conf.c
 * @author CSSlayer wengxt@gmail.com
 * @date 2010-04-30
 *
 * @brief 新配置文件读写
 */

#include "core/fcitx.h"

#include <stdio.h>
#include <string.h>
#include <search.h>
#include "fcitx-config/uthash.h"
#include "fcitx-config/sprintf.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/fcitx-config.h"
#include "tools/tools.h"

ConfigFile *ParseConfigFile(char *filename, ConfigFileDesc* fileDesc)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
        return NULL;
    ConfigFile *cf = ParseConfigFileFp(fp, fileDesc);
    fclose(fp);
    return cf;
}

ConfigFile *ParseMultiConfigFile(char **filename, int len, ConfigFileDesc*fileDesc)
{
    FILE **fp = malloc(sizeof(FILE*) * len);
    int i = 0;
    for (i = 0 ;i < len ; i++)
    {
        fp[i] = NULL;
        fp[i] = fopen(filename[i], "r");
    }

    ConfigFile *cf = ParseMultiConfigFileFp(fp, len, fileDesc);
    
    for (i = 0 ;i < len ; i++)
        if (fp[i])
            fclose(fp[i]);

    free(fp);

    return cf;
}

ConfigFile *ParseMultiConfigFileFp(FILE **fp, int len, ConfigFileDesc* fileDesc)
{
    ConfigFile* cfile = NULL;
    int i = 0;
    for (i = 0 ; i< len ;i ++)
        cfile = ParseIniFp(fp[i], cfile);
    if (CheckConfig(cfile, fileDesc))
    {
        return cfile;
    }
    FreeConfigFile(cfile);
    return NULL;

}

ConfigFile *ParseConfigFileFp(FILE *fp, ConfigFileDesc* fileDesc)
{
    ConfigFile *cfile = ParseIniFp(fp, NULL);
    if (CheckConfig(cfile, fileDesc))
    {
        return cfile;
    }
    FreeConfigFile(cfile);
    return NULL;
}

Bool CheckConfig(ConfigFile *cfile, ConfigFileDesc* cfdesc)
{
    if (!cfile)
        return False;
    ConfigGroupDesc *cgdesc = NULL;
    for(cgdesc = cfdesc->groupsDesc;
        cgdesc != NULL;
        cgdesc = (ConfigGroupDesc*)cgdesc->hh.next)
    {
        ConfigOptionDesc *codesc = NULL;
        ConfigGroup* group;
        HASH_FIND_STR(cfile->groups, cgdesc->groupName, group);
        if (!group)
        {
            group = malloc0(sizeof(ConfigGroup));
            group->groupName = strdup(cgdesc->groupName);
            group->groupDesc = cgdesc;
            group->options = NULL;
            HASH_ADD_KEYPTR(hh, cfile->groups, group->groupName, strlen(group->groupName), group);
        }
        for(codesc = cgdesc->optionsDesc;
            codesc != NULL;
            codesc = (ConfigOptionDesc*)codesc->hh.next)
        {
            ConfigOption *option;
            HASH_FIND_STR(group->options, codesc->optionName, option);
            if (!option)
            {
                if (!codesc->rawDefaultValue)
                {
                    FcitxLog(WARNING, "missing value: %s", codesc->optionName);
                    return False;
                }
                option = malloc0(sizeof(ConfigOption));
                option->optionName = strdup(codesc->optionName);
                option->rawValue = strdup(codesc->rawDefaultValue);
                HASH_ADD_KEYPTR(hh, group->options, option->optionName, strlen(option->optionName), option);
            }
            option->optionDesc = codesc;
        }
    }
    cfile->fileDesc = cfdesc;
    return True;
}

/** 
 * @brief 
 * 
 * @param filename
 * 
 * @return 
 */
ConfigFileDesc *ParseConfigFileDesc(char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
        return NULL;

    ConfigFileDesc *cfdesc = ParseConfigFileDescFp(fp);

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
ConfigFileDesc *ParseConfigFileDescFp(FILE *fp)
{
    ConfigFile *cfile = ParseIniFp(fp, NULL);
    if (!cfile)
        return NULL;
    ConfigFileDesc *cfdesc = malloc0(sizeof(ConfigFileDesc));
    ConfigGroup* group;
    for(group = cfile->groups;
        group != NULL;
        group = (ConfigGroup*)group->hh.next)
    {
        ConfigGroupDesc *cgdesc = NULL;
        ConfigOption *options = group->options, *option = NULL;
        
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
        if (!cgdesc)
        {
            cgdesc = malloc0(sizeof(ConfigGroupDesc));
            cgdesc->groupName = groupName;
            cgdesc->optionsDesc = NULL;
            HASH_ADD_KEYPTR(hh, cfdesc->groupsDesc, cgdesc->groupName, groupNameLen, cgdesc);
        }
        else
            free(groupName);

        ConfigOptionDesc *codesc = malloc0(sizeof(ConfigOptionDesc));
        codesc->optionName = optionName;
        codesc->rawDefaultValue = NULL;
        HASH_ADD_KEYPTR(hh, cgdesc->optionsDesc, codesc->optionName, optionNameLen, codesc);
        HASH_FIND_STR(options, "Description", option);
        if (option)
            codesc->desc= strdup(option->rawValue);
        else
            codesc->desc = strdup("");
        /* Processing Type */
        HASH_FIND_STR(options, "Type", option);
        if (option)
        {
            if (!strcmp(option->rawValue, "Integer"))
                codesc->type = T_Integer;
            else if (!strcmp(option->rawValue, "Color"))
                codesc->type = T_Color;
            else if (!strcmp(option->rawValue, "Char"))
                codesc->type = T_Char;
            else if (!strcmp(option->rawValue, "String"))
                codesc->type = T_String;
            else if (!strcmp(option->rawValue, "Boolean"))
                codesc->type = T_Boolean;
            else if (!strcmp(option->rawValue, "File"))
                codesc->type = T_File;
            else if (!strcmp(option->rawValue, "Font"))
                codesc->type = T_Font;
            else if (!strcmp(option->rawValue, "Image"))
                codesc->type = T_Image;
            else if (!strcmp(option->rawValue, "Hotkey"))
                codesc->type = T_Hotkey;
            else if (!strcmp(option->rawValue, "Enum"))
            {
                ConfigOption *eoption;
                codesc->type = T_Enum;
                HASH_FIND_STR(options, "EnumCount", eoption);
                Bool enumError = False;
                int i = 0;
                if (eoption)
                {
                    int ecount = atoi(eoption->rawValue);
                    if (ecount > 0)
                    {
                        /* 10个选项基本足够了，以后有需求再添加 */
                        if (ecount > 10)
                            ecount = 10;
                        char *enumname = strdup("Enum0");
                        codesc->configEnum.enumDesc = malloc(sizeof(char*) * ecount);
                        codesc->configEnum.enumCount = ecount;
                        size_t nel = 0;
                        for (i=0; i < ecount; i++) {
                            enumname[4] = '0' + i;
                            HASH_FIND_STR(options, enumname, eoption);
                            if (eoption)
                            {
                                void* entry = lfind(eoption->rawValue, codesc->configEnum.enumDesc, &nel, sizeof(char*), (int (*)(const void *, const void *)) strcmp);
                                if (entry)
                                {
                                    FcitxLog(WARNING, _("Enum option duplicated."));
                                }
                                codesc->configEnum.enumDesc[i] = strdup(eoption->rawValue);
                            }
                            else {
                                enumError = True;
                                free(enumname);
                                goto config_enum_final;
                            }
                        }
                        free(enumname);
                    }
                    else {
                        FcitxLog(WARNING, _("Enum option number must larger than 0"));
                        enumError = True;
                        goto config_enum_final;
                    }
                }
config_enum_final:
                if (enumError) {
                    int j =0;
                    for (;j < i;i++)
                        free(codesc->configEnum.enumDesc[j]);
                    FcitxLog(WARNING, _("Enum Option is invalid, take it as string"));
                    codesc->type = T_String;
                }
                 
            }
            else
            {
                FcitxLog(WARNING, _("Unknown type, take it as string: %s"), option->rawValue);
                codesc->type = T_String;
            }
        }
        else
        {
            FcitxLog(WARNING, _("Missing type, take it as string"));
            codesc->type = T_String;
        }

        /* processing default value */
        HASH_FIND_STR(options, "DefaultValue", option);
        if (option)
            codesc->rawDefaultValue = strdup(option->rawValue);
    }
    FreeConfigFile(cfile);
    return cfdesc;
}

ConfigSyncResult ConfigOptionInteger(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.integer)
        return SyncNoBinding;
    switch(sync)
    {
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

ConfigSyncResult ConfigOptionBoolean(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.boolean)
        return SyncNoBinding;
    switch(sync)
    {
        case Raw2Value:
            if (strcmp(option->rawValue, "True") == 0)
                *option->value.boolean = True;
            else
                *option->value.boolean = False;
            return SyncSuccess;
        case Value2Raw:
            if (option->rawValue)
                free(option->rawValue);
            if (*option->value.boolean)
                option->rawValue = strdup("True");
            else
                option->rawValue = strdup("False");
            return SyncSuccess;
    }

    return SyncInvalid;
}

ConfigSyncResult ConfigOptionEnum(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.enumerate || !option->optionDesc)
        return SyncNoBinding;

    ConfigOptionDesc *codesc = option->optionDesc;
    ConfigEnum* cenum = &codesc->configEnum;
    int i = 0;

    switch(sync)
    {
        case Raw2Value:
            for (i = 0; i< cenum->enumCount; i++)
            {
                if ( strcmp(cenum->enumDesc[i], option->rawValue) == 0)
                {
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

ConfigSyncResult ConfigOptionColor(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.color)
        return SyncNoBinding;

    ConfigColor *color = option->value.color;
    int r = 0,g = 0,b = 0;

    switch(sync)
    {
        case Raw2Value:
            if(sscanf(option->rawValue, "%d %d %d",&r,&g,&b) !=3)
                return SyncInvalid;
            if (IsColorValid(r) && IsColorValid(g) && IsColorValid(b))
            {
                color->r=r/255.0;
                color->g=g/255.0;
                color->b=b/255.0;
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
            asprintf(&option->rawValue, "%d %d %d", r, g ,b);
            return SyncSuccess;
    }

    return SyncInvalid;

}

ConfigSyncResult ConfigOptionString(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.string)
        return SyncNoBinding;
    switch(sync)
    {
        case Raw2Value:
            if (*option->value.string)
                free(*option->value.string);
            *option->value.string = strdup(option->rawValue);
            return SyncSuccess;
        case Value2Raw:
            if (option->rawValue)
                free(option->rawValue);
            option->rawValue =strdup(*option->value.string);
            return SyncSuccess;
    }
    return SyncInvalid;
}

ConfigSyncResult ConfigOptionChar(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.chr)
        return SyncNoBinding;
    switch(sync)
    {
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

ConfigSyncResult ConfigOptionImage(ConfigOption *option, ConfigSync sync)
{
    if (!option->value.image)
        return SyncNoBinding;

    FcitxImage *img = option->value.image;

    switch(sync)
    {
        case Raw2Value:
            memset(img, 0 , sizeof(FcitxImage));
            if(sscanf(option->rawValue, "%s %d %d %d %d %d %d %d %d", img->img_name,&img->position_x,\
		&img->position_y,&img->width,&img->height,&img->response_x,&img->response_y,\
		&img->response_w,&img->response_h) <= 0 )
            {
                strcpy(img->img_name, "");
            }
            else
            {
                if( img->response_x ==0 && img->response_y ==0)
                {
                    img->response_x=img->position_x;
                    img->response_y=img->position_y;
                }
            }
            return SyncSuccess;
        case Value2Raw:
            break;
    }

    return SyncInvalid;
}

ConfigSyncResult ConfigOptionHotkey(ConfigOption *option, ConfigSync sync)
{
    /* we assume all hotkey can have 2 candiate key */
    if (!option->value.hotkey)
        return SyncNoBinding;

    switch(sync)
    {
        case Raw2Value:
            if (option->value.hotkey[0].desc)
            {
                free(option->value.hotkey[0].desc);
                option->value.hotkey[0].desc = NULL;
            }
            if (option->value.hotkey[1].desc)
            {
                free(option->value.hotkey[1].desc);
                option->value.hotkey[0].desc = NULL;
            }
            SetHotKey(option->rawValue, option->value.hotkey);
            return SyncSuccess;
        case Value2Raw:
            if (option->rawValue)
                free(option->rawValue);
            if (option->value.hotkey[1].desc)
                asprintf(&option->rawValue, "%s %s", option->value.hotkey[0].desc, option->value.hotkey[1].desc);
            else if (option->value.hotkey[0].desc)
            {
                option->rawValue = strdup(option->value.hotkey[0].desc);
            }
            else
                option->rawValue = strdup("");
            return SyncSuccess;
    }

    return SyncInvalid;
}

void FreeConfigFile(ConfigFile* cfile)
{
    if (!cfile)
        return;
    ConfigGroup *groups = cfile->groups, *curGroup;
    while(groups)
    {
        curGroup = groups;
        HASH_DEL(groups, curGroup);
        FreeConfigGroup(curGroup);
    }
    free(cfile);
}

void FreeConfigFileDesc(ConfigFileDesc* cfdesc)
{
    if (!cfdesc)
        return;
    ConfigGroupDesc *cgdesc = cfdesc->groupsDesc, *curGroup;
    while(cgdesc)
    {
        curGroup = cgdesc;
        HASH_DEL(cgdesc, curGroup);
        FreeConfigGroupDesc(curGroup);
    }
    free(cfdesc);
}

void FreeConfigGroup(ConfigGroup *group)
{
    ConfigOption *option = group->options, *curOption;
    while(option)
    {
        curOption = option;
        HASH_DEL(option, curOption);
        FreeConfigOption(curOption);
    }
    free(group->groupName);
    free(group);
}

void FreeConfigGroupDesc(ConfigGroupDesc *cgdesc)
{
    ConfigOptionDesc *codesc = cgdesc->optionsDesc, *curOption;
    while(codesc)
    {
        curOption = codesc;
        HASH_DEL(codesc, curOption);
        FreeConfigOptionDesc(curOption);
    }
    free(cgdesc->groupName);
    free(cgdesc);
}

void FreeConfigOption(ConfigOption *option)
{
    free(option->optionName);
    if (option->rawValue)
        free(option->rawValue);
    free(option);
}

void FreeConfigOptionDesc(ConfigOptionDesc *codesc)
{
    free(codesc->optionName);
    if (codesc->configEnum.enumCount > 0)
    {
        int i = 0;
        for (i =0 ;i< codesc->configEnum.enumCount; i ++)
        {
            free(codesc->configEnum.enumDesc[i]);
        }
        free(codesc->configEnum.enumDesc);
    }
    if (codesc->rawDefaultValue)
        free(codesc->rawDefaultValue);
    free(codesc->desc);
    free(codesc);
}

/** 
 * @brief 
 * 
 * @param filename
 * 
 * @return 
 */
ConfigFile* ParseIni(char* filename, ConfigFile* reuse)
{
    FILE* fp = fopen(filename, "r");
    if (!fp)
        return NULL;
    ConfigFile *cf = ParseIniFp(fp, reuse);
    fclose(fp);
    return cf;
}

ConfigFile* ParseIniFp(FILE *fp, ConfigFile* reuse)
{
    char *line = NULL, *buf = NULL;
    size_t len = 0;
    int lineLen = 0;
    ConfigFile* cfile;

    if (!fp)
    {
        if (reuse)
            return reuse;
        else
            return NULL;
    }
    if (reuse)
        cfile = reuse;
    else
        cfile = malloc0(sizeof(ConfigFile));
    ConfigGroup* curGroup = NULL;
    int lineNo = 0;
    while(getline(&buf, &len, fp) != -1)
    {
        lineNo ++;
        if (line)
            free(line);
        line = trim(buf);
        lineLen = strlen(line);

        if (lineLen == 0 || line[0] == '#')
            continue;

        if (line[0] == '[')
        {
            if (!(line[lineLen - 1] == ']' && lineLen != 2))
            {
                FcitxLog(ERROR, _("Configure group name error: line %d"), lineNo);
                return NULL;
            }
            char *groupName;
            groupName = malloc(sizeof(char) * (lineLen - 2 + 1) );
            strncpy(groupName, line + 1, lineLen - 2);
            groupName[lineLen - 2] = '\0';
            HASH_FIND_STR(cfile->groups, groupName, curGroup);
            if (curGroup)
            {
                FcitxLog(DEBUG, _("Duplicate group name, merge with the previous: %s :line %d"),groupName, lineNo);
                free(groupName);
                continue;
            }
            curGroup = malloc0(sizeof(ConfigGroup));
            curGroup->groupName = groupName;
            curGroup->options = NULL;
            curGroup->groupDesc = NULL;
            HASH_ADD_KEYPTR(hh, cfile->groups, curGroup->groupName, strlen(curGroup->groupName), curGroup);
        }
        else
        {
            if (curGroup == NULL)
                continue;
            char *value = strchr(line, '=');
            if (!value)
            {
                FcitxLog(WARNING, _("Invalid Entry: line %d missing '='"), lineNo);
                goto next_line;
            }
            *value = '\0';
            value ++;
            char *name = line;
            ConfigOption *option;
            HASH_FIND_STR(curGroup->options, name, option);
            if (option)
            {
                FcitxLog(DEBUG, _("Duplicate option, overwrite: line %d"), lineNo);
                free(option->rawValue);
                option->rawValue = strdup(value);
            }
            else
            {
                option = malloc0(sizeof(ConfigOption));
                option->optionName = strdup(name);
                option->rawValue = strdup(value);
                HASH_ADD_KEYPTR(hh, curGroup->options, option->optionName, strlen(option->optionName), option);
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

Bool SaveConfigFile(char *filename, ConfigFile *cfile, ConfigFileDesc* cdesc)
{
    FILE* fp = fopen(filename, "w");
    if (!fp)
        return False;
    Bool result = SaveConfigFileFp(fp, cfile, cdesc);
    fclose(fp);
    return result;
}

void ConfigBindSync(GenericConfig* config)
{
    ConfigFile *cfile = config->configFile;
    ConfigFileDesc *cdesc = cfile->fileDesc;
    ConfigGroupDesc* groupdesc = NULL;
    for(groupdesc = cdesc->groupsDesc;
        groupdesc != NULL;
        groupdesc = (ConfigGroupDesc*)groupdesc->hh.next)
    {
        ConfigOptionDesc *optiondesc;
        ConfigGroup *group = NULL;
        if (cfile)
            HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);
        for(optiondesc = groupdesc->optionsDesc;
            optiondesc != NULL;
            optiondesc = (ConfigOptionDesc*)optiondesc->hh.next)
        {
            ConfigOption *option = NULL;
            if (group)
                HASH_FIND_STR(group->options, optiondesc->optionName, option);

            ConfigSyncValue(group, option, Raw2Value);
        }
    }
}

void ConfigSyncValue(ConfigGroup* group, ConfigOption *option, ConfigSync sync)
{
    ConfigOptionDesc *codesc = option->optionDesc;

    ConfigOptionFunc f = NULL;

    if (codesc == NULL)
        return;

    if (sync == Value2Raw)
        if (option->filter)
            option->filter(group, option, option->value.untype, sync, option->filterArg);

    switch (codesc->type)
    {
        case T_Integer:
            f = ConfigOptionInteger;
            break;
        case T_Color:
            f = ConfigOptionColor;
            break;
        case T_Boolean:
            f = ConfigOptionBoolean;
            break;
        case T_Enum:
            f = ConfigOptionEnum;
            break;
        case T_String:
            f = ConfigOptionString;
            break;
        case T_Hotkey:
            f = ConfigOptionHotkey;
            break;
        case T_Image:
            f = ConfigOptionImage;
            break;
        case T_File:
            f = ConfigOptionFile;
            break;
        case T_Font:
            f = ConfigOptionFont;
            break;
        case T_Char:
            f = ConfigOptionChar;
            break;
    }
    ConfigSyncResult r = SyncNoBinding;
    if (f)
        r = f(option, sync);
    if (r == SyncInvalid)
    {
        if (codesc->rawDefaultValue)
        {
            FcitxLog(WARNING, _("Option %s is Invalid, Use Default Value %s"), option->optionName, codesc->rawDefaultValue);
            if (option->rawValue)
                free(option->rawValue);
            option->rawValue = strdup(codesc->rawDefaultValue);
            if (sync == Raw2Value)
                f(option, sync);
        }
        else
        {
            FcitxLog(ERROR, _("Option %s is Invalid."), option->optionName);
        }
    }

    if (sync == Raw2Value)
        if (option->filter)
            option->filter(group, option, option->value.untype, sync, option->filterArg);
}

Bool SaveConfigFileFp(FILE* fp, ConfigFile *cfile, ConfigFileDesc* cdesc)
{
    ConfigGroupDesc* groupdesc = NULL;
    for(groupdesc = cdesc->groupsDesc;
        groupdesc != NULL;
        groupdesc = (ConfigGroupDesc*)groupdesc->hh.next)
    {
        ConfigOptionDesc *optiondesc;
        fprintf(fp, "[%s]\n", groupdesc->groupName);

        ConfigGroup *group = NULL;
        if (cfile)
            HASH_FIND_STR(cfile->groups, groupdesc->groupName, group);
        for(optiondesc = groupdesc->optionsDesc;
            optiondesc != NULL;
            optiondesc = (ConfigOptionDesc*)optiondesc->hh.next)
        {
            ConfigOption *option = NULL;
            if (group)
                HASH_FIND_STR(group->options, optiondesc->optionName, option);

            if (!option)
            {
                if (optiondesc->rawDefaultValue)
                    fprintf(fp, "%s=%s\n", optiondesc->optionName, optiondesc->rawDefaultValue);
                else
                    FcitxLog(FATAL, _("no default option for %s/%s"), groupdesc->groupName, optiondesc->optionName);
            }
            else 
            {
                ConfigSyncValue(group, option, Value2Raw);
                fprintf(fp, "%s=%s\n", option->optionName, option->rawValue);
            }
        }
    }
    return True;
}

void ConfigBindValue(ConfigFile* cfile, char *groupName, char *optionName, void* var, SyncFilter filter, void *arg)
{
    ConfigGroup *group = NULL;
    HASH_FIND_STR(cfile->groups, groupName, group);
    if (group)
    {
        ConfigOption *option = NULL;
        HASH_FIND_STR(group->options, optionName, option);
        if (option)
        {
            ConfigOptionDesc* codesc = option->optionDesc;
            option->filter = filter;
            option->filterArg = arg;
            if (!codesc)
            {
                FcitxLog(WARNING, "Unknown Option: %s/%s", groupName, optionName);
                return;
            }
            switch(codesc->type)
            {
                case T_Image:
                    option->value.image = (FcitxImage*)var;
                    break;
                case T_Char:
                    option->value.chr = (char*) var;
                    break;
                case T_Integer:
                    option->value.integer = (int*) var;
                    break;
                case T_Color:
                    option->value.color = (ConfigColor*) var;
                    break;
                case T_Boolean:
                    option->value.boolean = (Bool*) var;
                    break;
                case T_Hotkey:
                    option->value.hotkey = (HOTKEYS*) var;
                    break;
                case T_Enum:
                    option->value.enumerate = (int*) var;
                    break;
                case T_String:
                case T_File:
                case T_Font:
                    option->value.string = (char**) var;
                    break;
            }
        }
    }
}

