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

#ifndef FCITX_CONFIG_H
#define FCITX_CONFIG_H

#include <X11/Xlib.h>
#include <stdio.h>
#include <fcitx-config/hotkey.h>

#ifndef UTHASH_H

typedef struct UT_hash_handle {
   struct UT_hash_table *tbl;
   void *prev;                       /* prev element in app order      */
   void *next;                       /* next element in app order      */
   struct UT_hash_handle *hh_prev;   /* previous hh in bucket order    */
   struct UT_hash_handle *hh_next;   /* next hh in bucket order        */
   void *key;                        /* ptr to enclosing struct's key  */
   unsigned keylen;                  /* enclosing struct's key len     */
   unsigned hashv;                   /* result of hash-fcn(key)        */
} UT_hash_handle;

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

typedef struct FcitxImage
{
	char img_name[32];
	//图片绘画区域
	int  position_x;
	int  position_y;
	int  width;
	int  height;
	//按键响应区域
	int  response_x;
	int  response_y;
	int  response_w;
	int  response_h;
	//鼠标不同状态mainnMenuwindow不同的显示状态.
	MouseE mouse;
} FcitxImage;

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
    T_Font,
    T_Image
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

typedef struct ConfigGroup ConfigGroup;
typedef struct ConfigOption ConfigOption;

typedef void (*SyncFilter)(ConfigGroup *, ConfigOption *, void* , ConfigSync, void* );

typedef struct ConfigEnum
{
    char **enumDesc;
    int enumCount;
} ConfigEnum;

typedef struct ConfigOptionDesc
{
    char *optionName;
    char *desc;
    ConfigType type;
    char *rawDefaultValue;
    ConfigEnum configEnum;

    UT_hash_handle hh;
} ConfigOptionDesc;

typedef struct ConfigGroupDesc
{
    char *groupName;
    ConfigOptionDesc *optionsDesc;
    UT_hash_handle hh;
} ConfigGroupDesc;

typedef struct ConfigFileDesc
{
    ConfigGroupDesc *groupsDesc;
} ConfigFileDesc;

struct ConfigOption
{
    char *optionName;
    char *rawValue;
    union {
        void *untype;
        int *integer;
        Bool *boolean;
        HOTKEYS *hotkey;
        ConfigColor *color;
        int *enumerate;
        char **string;
        char *chr;
        FcitxImage* image;
    } value;
    SyncFilter filter;
    void *filterArg;
    ConfigOptionDesc *optionDesc;
    UT_hash_handle hh;
} ;

struct ConfigGroup
{
    char *groupName;
    ConfigGroupDesc *groupDesc;
    ConfigOption* options;
    UT_hash_handle hh;
} ;

typedef struct ConfigFile
{
    ConfigFileDesc *fileDesc;
    ConfigGroup* groups;
} ConfigFile;

struct GenericConfig;

typedef void(*ConfigBindingFunc)(struct GenericConfig*);

typedef struct GenericConfig
{
    ConfigFile *configFile;
} GenericConfig;

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

#define IsColorValid(c) ((c) >=0 && (c) <= 255)
#define RoundColor(c) ((c)>=0?((c)<=255?c:255):0)

#define FilterNextTimeEffectBool(name, var) \
    static Bool firstRunOn##name = True; \
    void FilterCopy##name(ConfigGroup *group, ConfigOption *option, void *value, ConfigSync sync, void *filterArg) { \
        Bool *b = (Bool*)value; \
        if (sync == Raw2Value && b) \
        { \
            if (firstRunOn##name) \
                var = *b; \
            firstRunOn##name = False; \
        } \
    }



ConfigFile *ParseConfigFile(char *filename, ConfigFileDesc*);
ConfigFile *ParseMultiConfigFile(char **filename, int len, ConfigFileDesc*);
ConfigFile *ParseConfigFileFp(FILE* fp, ConfigFileDesc* fileDesc);
ConfigFile *ParseMultiConfigFileFp(FILE **fp, int len, ConfigFileDesc* fileDesc);
Bool CheckConfig(ConfigFile *configFile, ConfigFileDesc* fileDesc);
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
Bool SaveConfigFile(char *filename, ConfigFile *cfile, ConfigFileDesc* cdesc);
Bool SaveConfigFileFp(FILE* fp, ConfigFile *cfile, ConfigFileDesc* cdesc);
void ConfigSyncValue(ConfigGroup* group, ConfigOption *option, ConfigSync sync);
void ConfigBindSync(GenericConfig* config);

typedef ConfigSyncResult (*ConfigOptionFunc)(ConfigOption *, ConfigSync);
ConfigSyncResult ConfigOptionInteger(ConfigOption *option, ConfigSync sync);
ConfigSyncResult ConfigOptionImage(ConfigOption *option, ConfigSync sync);
ConfigSyncResult ConfigOptionBoolean(ConfigOption *option, ConfigSync sync);
ConfigSyncResult ConfigOptionEnum(ConfigOption *option, ConfigSync sync);
ConfigSyncResult ConfigOptionColor(ConfigOption *option, ConfigSync sync);
ConfigSyncResult ConfigOptionString(ConfigOption *option, ConfigSync sync);
ConfigSyncResult ConfigOptionHotkey(ConfigOption *option, ConfigSync sync);
#define ConfigOptionFile ConfigOptionString
#define ConfigOptionFont ConfigOptionString

void ConfigBindValue(ConfigFile* cfile, char *groupName, char *optionName, void* var, SyncFilter filter, void *arg);

#endif
