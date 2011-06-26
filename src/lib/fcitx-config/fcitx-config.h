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
 * @file config.h
 * @author CSSlayer wengxt@gmail.com
 * @date 2010-04-30
 *
 * @brief 新配置文件读写
 */

#ifndef _FCITX_FCITX_CONFIG_H_
#define _FCITX_FCITX_CONFIG_H_

#include <stdint.h>
#include <stdio.h>

struct HOTKEYS;
typedef int8_t boolean;
#define true (1)
#define false (0)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ConfigColor
{
    double r;
    double g;
    double b;
} ConfigColor;

typedef enum
{
    RELEASE,//鼠标释放状态
    PRESS,//鼠标按下
    MOTION//鼠标停留
} MouseE;

typedef enum ConfigType
{
    T_Integer,
    T_Color,
    T_String,
    T_Char,
    T_Boolean,
    T_Enum,
    T_File,
    T_Hotkey,
    T_Font
} ConfigType;

typedef enum ConfigSync
{
    Raw2Value,
    Value2Raw
} ConfigSync;

typedef enum ConfigSyncResult
{
    SyncSuccess,
    SyncNoBinding,
    SyncInvalid
} ConfigSyncResult;

typedef union ConfigValueType{
    void *untype;
    int *integer;
    boolean *boolvalue;
    struct HOTKEYS *hotkey;
    ConfigColor *color;
    int *enumerate;
    char **string;
    char *chr;
} ConfigValueType;

typedef struct ConfigGroup ConfigGroup;
typedef struct ConfigOption ConfigOption;
typedef struct ConfigFileDesc ConfigFileDesc;
typedef struct ConfigGroupDesc ConfigGroupDesc;
typedef struct ConfigOptionDesc ConfigOptionDesc;
typedef struct GenericConfig GenericConfig;

typedef void (*SyncFilter)(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);

typedef struct ConfigEnum
{
    char **enumDesc;
    int enumCount;
} ConfigEnum;

typedef struct ConfigFile
{
    ConfigFileDesc *fileDesc;
    ConfigGroup* groups;
} ConfigFile;


struct GenericConfig
{
    ConfigFile *configFile;
};

typedef void(*ConfigBindingFunc)(GenericConfig*);

#define CONFIG_BINDING_DECLARE(config_type) \
    void config_type##ConfigBind(config_type* config, ConfigFile* cfile, ConfigFileDesc* cfdesc);
#define CONFIG_BINDING_BEGIN(config_type) \
    void config_type##ConfigBind(config_type* config, ConfigFile* cfile, ConfigFileDesc* cfdesc) { \
        GenericConfig *gconfig = (GenericConfig*) config; \
        if (gconfig->configFile) { \
            FreeConfigFile(gconfig->configFile); \
        } \
        gconfig->configFile = cfile
#define CONFIG_BINDING_REGISTER(g, o, var) \
        do { \
            ConfigBindValue(cfile, g, o, &config->var, NULL, NULL); \
        } while(0)      

#define CONFIG_BINDING_REGISTER_WITH_FILTER(g, o, var, filter_func) \
        do { \
            ConfigBindValue(cfile, g, o, &config->var, filter_func, NULL); \
        } while(0)
#define CONFIG_BINDING_REGISTER_WITH_FILTER_ARG(g, o, var, filter_func, arg) \
        do { \
            ConfigBindValue(cfile, g, o, &config->var, filter_func, arg); \
        } while(0)
#define CONFIG_BINDING_END() }

#define CONFIG_DESC_DEFINE(funcname, path) \
ConfigFileDesc *funcname() \
{ \
    static ConfigFileDesc *configDesc = NULL; \
    if (!configDesc) \
    { \
        FILE *tmpfp; \
        tmpfp = GetXDGFileData(path, "r", NULL); \
        if (tmpfp == NULL) \
        { \
            FcitxLog(ERROR, _("Load Config Description File %s Erorr, Please Check your install."), path); \
            return NULL; \
        } \
        configDesc = ParseConfigFileDescFp(tmpfp); \
        fclose(tmpfp); \
    } \
    return configDesc; \
}

#define IsColorValid(c) ((c) >=0 && (c) <= 255)
#define RoundColor(c) ((c)>=0?((c)<=255?c:255):0)

ConfigFile *ParseConfigFile(char *filename, ConfigFileDesc*);
ConfigFile *ParseMultiConfigFile(char **filename, int len, ConfigFileDesc*);
ConfigFile *ParseConfigFileFp(FILE* fp, ConfigFileDesc* fileDesc);
ConfigFile *ParseMultiConfigFileFp(FILE **fp, int len, ConfigFileDesc* fileDesc);
boolean CheckConfig(ConfigFile *configFile, ConfigFileDesc* fileDesc);
ConfigFileDesc *ParseConfigFileDesc(char* filename);
ConfigFileDesc *ParseConfigFileDescFp(FILE* filename);
ConfigFile* ParseIni(char* filename, ConfigFile* reuse);
ConfigFile* ParseIniFp(FILE* filename, ConfigFile* reuse);
void FreeConfigFile(ConfigFile* cfile);
void FreeConfigFileDesc(ConfigFileDesc* cfdesc);
void FreeConfigGroup(ConfigGroup *group);
void FreeConfigGroupDesc(ConfigGroupDesc *cgdesc);
void FreeConfigOption(ConfigOption *option);
void FreeConfigOptionDesc(ConfigOptionDesc *codesc);
boolean SaveConfigFile(char *filename, GenericConfig *cfile, ConfigFileDesc* cdesc);
boolean SaveConfigFileFp(FILE* fp, GenericConfig *cfile, ConfigFileDesc* cdesc);
void ConfigSyncValue(GenericConfig* config, ConfigGroup* group, ConfigOption* option, ConfigSync sync);
ConfigValueType ConfigGetBindValue(GenericConfig *config, const char *group, const char* option);
void ConfigBindSync(GenericConfig* config);

typedef ConfigSyncResult (*ConfigOptionFunc)(ConfigOption *, ConfigSync);

void ConfigBindValue(ConfigFile* cfile, const char *groupName, const char *optionName, void* var, SyncFilter filter, void *arg);

#ifdef __cplusplus
}
#endif

#endif
