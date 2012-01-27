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
 * @file fcitx-config.h
 * @author CSSlayer wengxt@gmail.com
 * @date 2010-04-30
 *
 * @brief Fcitx configure file read-write
 */

#ifndef _FCITX_FCITX_CONFIG_H_
#define _FCITX_FCITX_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <fcitx-utils/uthash.h>
#include <fcitx-utils/utils.h>

struct _FcitxHotkey;
/**
 * @brief fcitx boolean
 **/
typedef int32_t boolean;
/**
 * @brief fcitx true
 */
#define true (1)
/**
 * @brief fcitx true
 */
#define false (0)

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief The Color type in config file
     **/

    typedef struct _FcitxConfigColor {
        double r;
        double g;
        double b;
    } FcitxConfigColor;

    /**
     * @brief config value type
     **/
    typedef enum _FcitxConfigType {
        T_Integer,
        T_Color,
        T_String,
        T_Char,
        T_Boolean,
        T_Enum,
        T_File,
        T_Hotkey,
        T_Font,
        T_I18NString
    } FcitxConfigType;

    /**
     * @brief The sync direction
     **/
    typedef enum _FcitxConfigSync {
        Raw2Value,
        Value2Raw
    } FcitxConfigSync;

    /**
     * @brief Sync result
     **/
    typedef enum _FcitxConfigSyncResult {
        SyncSuccess,
        SyncNoBinding,
        SyncInvalid
    } FcitxConfigSyncResult;

    /**
     * @brief The value of config
     **/
    typedef union _FcitxConfigValueType {
        void *untype;
        int *integer;
        boolean *boolvalue;

        struct _FcitxHotkey *hotkey;
        FcitxConfigColor *color;
        int *enumerate;
        char **string;
        char *chr;
    } FcitxConfigValueType;

    typedef struct _FcitxConfigGroup FcitxConfigGroup;

    typedef struct _FcitxConfigOption FcitxConfigOption;

    typedef struct _FcitxConfigFileDesc FcitxConfigFileDesc;

    typedef struct _FcitxConfigGroupDesc FcitxConfigGroupDesc;

    typedef struct _FcitxConfigOptionDesc FcitxConfigOptionDesc;

    typedef struct _FcitxGenericConfig FcitxGenericConfig;

    typedef struct _FcitxConfigOptionSubkey FcitxConfigOptionSubkey;

    /**
     * @brief sync filter function
     **/
    typedef void (*FcitxSyncFilter)(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg);

    /**
     * @brief Enum value type description
     **/

    typedef struct _FcitxConfigEnum {
        char **enumDesc;
        int enumCount;
    } FcitxConfigEnum;

    /**
     * @brief Config file contains multiple config groups, and the opposite config file description
     **/

    typedef struct _FcitxConfigFile {
        FcitxConfigFileDesc *fileDesc;
        FcitxConfigGroup* groups;
    } FcitxConfigFile;


    /**
     * @brief A generic config struct, config struct can derive from it.
     *        Like: <br>
     *        struct TestConfig { <br>
     *            FcitxGenericConfig gconfig; <br>
     *            int own_value; <br>
     *        };
     **/

    struct _FcitxGenericConfig {
        /**
         * @brief config file pointer
         **/
        FcitxConfigFile *configFile;
    };

    /**
     * @brief Config Option Description, it describe a Key=Value entry in config file.
     **/

    struct _FcitxConfigOptionDesc {
        char *optionName;
        char *desc;
        FcitxConfigType type;
        char *rawDefaultValue;
        FcitxConfigEnum configEnum;

        UT_hash_handle hh;
    };

    /**
     * @brief Config Group Description, it describe a [Gruop] in config file
     **/

    struct _FcitxConfigGroupDesc {
        /**
         * @brief Group Name
         **/
        char *groupName;
        /**
         * @brief Hash table for option description
         **/
        FcitxConfigOptionDesc *optionsDesc;
        UT_hash_handle hh;
    };

    /**
     * @brief Description of a config file
     **/

    struct _FcitxConfigFileDesc {
        FcitxConfigGroupDesc *groupsDesc;
        char* domain;
    };

    /**
     * @brief Config Option in config file, Key=Value entry
     **/

    struct _FcitxConfigOption {
        char *optionName;
        char *rawValue;
        FcitxConfigValueType value;
        FcitxSyncFilter filter;
        void *filterArg;
        FcitxConfigOptionDesc *optionDesc;
        FcitxConfigOptionSubkey *subkey;
        UT_hash_handle hh;
    } ;

    /**
     * @brief Config Option subkey in config file, Key[Subkey]=Value entry
     **/

    struct _FcitxConfigOptionSubkey {
        char *subkeyName;
        char *rawValue;
        UT_hash_handle hh;
    };

    /**
     * @brief Config group in config file, [Group] Entry
     **/

    struct _FcitxConfigGroup {
        /**
         * @brief Group Name, unique in FcitxConfigFile
         **/
        char *groupName;
        /**
         * @brief Group Description
         **/
        FcitxConfigGroupDesc *groupDesc;
        /**
         * @brief Option store with a hash table
         **/
        FcitxConfigOption* options;
        /**
         * @brief UTHash handler
         **/
        UT_hash_handle hh;
    };

    /**
     * @brief declare the binding function
     **/
#define CONFIG_BINDING_DECLARE(config_type) \
    void config_type##ConfigBind(config_type* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc);

    /**
     * @brief define the binding function
     * each binding group for a config file will define a new function
     * the structure is like: <br>
     * CONFIG_BINDING_BEGIN <br>
     * CONFIG_BINDING_REGISTER <br>
     * .... <br>
     * CONFIG_BINDING_REGISTER <br>
     * CONFIG_BINDING_END
     **/
#define CONFIG_BINDING_BEGIN(config_type) \
    void config_type##ConfigBind(config_type* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc) { \
        (void) cfdesc; \
        FcitxGenericConfig *gconfig = (FcitxGenericConfig*) config; \
        if (gconfig->configFile) { \
            FcitxConfigFreeConfigFile(gconfig->configFile); \
        } \
        gconfig->configFile = cfile;

#define CONFIG_BINDING_BEGIN_WITH_ARG(config_type, arg...) \
    void config_type##ConfigBind(config_type* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc, arg) { \
        (void) cfdesc; \
        FcitxGenericConfig *gconfig = (FcitxGenericConfig*) config; \
        if (gconfig->configFile) { \
            FcitxConfigFreeConfigFile(gconfig->configFile); \
        } \
        gconfig->configFile = cfile;
    /**
     * @brief register a binding
     **/
#define CONFIG_BINDING_REGISTER(g, o, var) \
    do { \
        FcitxConfigBindValue(cfile, g, o, &config->var, NULL, NULL); \
    } while(0);

    /**
     * @brief register a binding with filter
     **/
#define CONFIG_BINDING_REGISTER_WITH_FILTER(g, o, var, filter_func) \
    do { \
        FcitxConfigBindValue(cfile, g, o, &config->var, filter_func, NULL); \
    } while(0);

    /**
     * @brief register a binding with filter and extra argument
     **/
#define CONFIG_BINDING_REGISTER_WITH_FILTER_ARG(g, o, var, filter_func, arg) \
    do { \
        FcitxConfigBindValue(cfile, g, o, &config->var, filter_func, arg); \
    } while(0);

    /**
     * @brief binding group end
     **/
#define CONFIG_BINDING_END() }

    /**
     * @brief define a singleton function to load config file description
     **/
#define CONFIG_DESC_DEFINE(funcname, path) \
    FcitxConfigFileDesc *funcname() \
    { \
        static FcitxConfigFileDesc *configDesc = NULL; \
        if (!configDesc) \
        { \
            FILE *tmpfp; \
            tmpfp = FcitxXDGGetFileWithPrefix("configdesc", path, "r", NULL); \
            if (tmpfp == NULL) \
            { \
                FcitxLog(ERROR, "Load Config Description File %s Erorr, Please Check your install.", path); \
                return NULL; \
            } \
            configDesc = FcitxConfigParseConfigFileDescFp(tmpfp); \
            fclose(tmpfp); \
        } \
        return configDesc; \
    }

    /**
     * @brief parse a config file with file name.
     * even the file cannot be read, or with wrong format,
     * it will try to return a usable FcitxConfigFile (missing
     * entry with defaul value).
     *
     * @param filename file name of a configfile
     * @param cfdesc config file description
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile *FcitxConfigParseConfigFile(char *filename, FcitxConfigFileDesc* cfdesc);

    /**
     * @brief parse multi config file, the main difference
     * between ParseConfigFile is that it parse multiple file
     * and the duplicate entry will be overwritten with the
     * file behind the previous one.
     *
     * @see ParseConfigFile
     * @param filename filenames
     * @param len len of filenames
     * @param cfdesc config file description
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile *FcitxConfigParseMultiConfigFile(char **filename, int len, FcitxConfigFileDesc* cfdesc);

    /**
     * @brief same with ParseConfigFile, the input is FILE*
     *
     * @see ParseConfigFile
     * @param fp file pointer
     * @param fileDesc config file description
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile *FcitxConfigParseConfigFileFp(FILE* fp, FcitxConfigFileDesc* fileDesc);

    /**
     * @brief same with FcitxConfigParseMultiConfigFileFp, the input is array of FILE*
     *
     * @see FcitxConfigParseMultiConfigFileFp
     * @param fp array of file pointers
     * @param len lenght of fp
     * @param fileDesc config file description
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile *FcitxConfigParseMultiConfigFileFp(FILE **fp, int len, FcitxConfigFileDesc* fileDesc);

    /**
     * @brief Check the raw FcitxConfigFile and try to fill the default value
     *
     * @param configFile config file
     * @param fileDesc config file description
     * @return boolean
     **/
    boolean FcitxConfigCheckConfigFile(FcitxConfigFile *configFile, FcitxConfigFileDesc* fileDesc);

    /**
     * @brief parse config file description from file
     *
     * @param filename filename
     * @return FcitxConfigFileDesc*
     **/
    FcitxConfigFileDesc *FcitxConfigParseConfigFileDesc(char* filename);

    /**
     * @brief parse config file description from file pointer
     *
     * @see ParseConfigFileDesc
     * @param fp file pointer
     * @return FcitxConfigFileDesc*
     **/
    FcitxConfigFileDesc *FcitxConfigParseConfigFileDescFp(FILE* fp);

    /**
     * @brief internal raw file parse, it can merge the config to existing config file
     *
     * @param filename file
     * @param reuse NULL or existing config file
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile* FcitxConfigParseIni(char* filename, FcitxConfigFile* reuse);

    /**
     * @brief internal raw file parse, it can merge the config to existing config file
     *
     * @see ParseIni
     * @param fp file pointer
     * @param reuse NULL or existing config file
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile* FcitxConfigParseIniFp(FILE* fp, FcitxConfigFile* reuse);

    /**
     * @brief free a config file
     *
     * @param cfile config file
     * @return void
     **/
    void FcitxConfigFreeConfigFile(FcitxConfigFile* cfile);

    /**
     * @brief free a config file description
     *
     * @param cfdesc config file description
     * @return void
     **/
    void FcitxConfigFreeConfigFileDesc(FcitxConfigFileDesc* cfdesc);

    /**
     * @brief free a config group
     *
     * @param group config group
     * @return void
     **/
    void FcitxConfigFreeConfigGroup(FcitxConfigGroup *group);

    /**
     * @brief free a config group description
     *
     * @param cgdesc config group description
     * @return void
     **/
    void FcitxConfigFreeConfigGroupDesc(FcitxConfigGroupDesc *cgdesc);

    /**
     * @brief free a config option
     *
     * @param option config option
     * @return void
     **/
    void FcitxConfigFreeConfigOption(FcitxConfigOption *option);

    /**
     * @brief free a config option description
     *
     * @param codesc config option description
     * @return void
     **/
    void FcitxConfigFreeConfigOptionDesc(FcitxConfigOptionDesc *codesc);

    /**
     * @brief Save config file to fp, it will do the Value2Raw sync
     *
     * @param filename file name
     * @param cfile config
     * @param cdesc config file description
     * @return boolean
     **/
    boolean FcitxConfigSaveConfigFile(char *filename, FcitxGenericConfig *cfile, FcitxConfigFileDesc* cdesc);

    /**
     * @brief Save config file to fp
     *
     * @see SaveConfigFile
     * @param fp file pointer
     * @param cfile config
     * @param cdesc config file dsecription
     * @return boolean
     **/
    boolean FcitxConfigSaveConfigFileFp(FILE* fp, FcitxGenericConfig *cfile, FcitxConfigFileDesc* cdesc);

    /**
     * @brief sync a single value
     *
     * @param config config
     * @param group config group
     * @param option config option
     * @param sync sync direction
     * @return Svoid
     **/
    void FcitxConfigSyncValue(FcitxGenericConfig* config, FcitxConfigGroup* group, FcitxConfigOption* option, FcitxConfigSync sync);

    /**
     * @brief Get the binded value type
     *
     * @param config config
     * @param group group name
     * @param option option name
     * @return FcitxConfigValueType
     **/
    FcitxConfigValueType FcitxConfigGetBindValue(FcitxGenericConfig *config, const char *group, const char* option);

    /**
     * @brief Get a option description from config file description, return NULL if not found.
     *
     * @param cfdesc config file description
     * @param groupName gropu name
     * @param optionName option name
     * @return const FcitxConfigOptionDesc*
     **/
    const FcitxConfigOptionDesc* FcitxConfigDescGetOptionDesc(FcitxConfigFileDesc* cfdesc, const char* groupName, const char* optionName);


    /**
     * @brief Get a option description from config file description, return NULL if not found.
     *
     * @param cfile config file
     * @param groupName gropu name
     * @param optionName option name
     * @return const FcitxConfigOptionDesc*
     *
     * @since 4.1.2
     **/
    FcitxConfigOption* FcitxConfigFileGetOption(FcitxConfigFile* cfile, const char* groupName, const char* optionName);


    /**
     * @brief Get the I18NString value from current locale
     *
     * @param option config option
     * @return const char*
     **/
    const char* FcitxConfigOptionGetLocaleString(FcitxConfigOption* option);

    /**
     * @brief do the Raw2Value sync for config
     *
     * @param config config
     * @return void
     **/
    void FcitxConfigBindSync(FcitxGenericConfig* config);

    /**
     * @brief reset a config to default value
     *
     * @param config config
     * @return Svoid
     **/
    void FcitxConfigResetConfigToDefaultValue(FcitxGenericConfig* config);

    /**
     * @brief bind a value with a struct, normally you should use
     * CONFIG_BINDING_ series macro, not directly this function.
     *
     * @param cfile config file
     * @param groupName group name
     * @param optionName option name
     * @param var pointer to value
     * @param filter filter function
     * @param arg extra argument
     * @return void
     **/
    void FcitxConfigBindValue(FcitxConfigFile* cfile, const char *groupName, const char *optionName, void* var, FcitxSyncFilter filter, void *arg);

#ifdef __cplusplus
}

#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
