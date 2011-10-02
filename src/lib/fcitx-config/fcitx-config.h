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

struct _HOTKEYS;
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

    typedef struct _ConfigColor {
        double r;
        double g;
        double b;
    } ConfigColor;

    /**
     * @brief config value type
     **/
    typedef enum _ConfigType {
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
    } ConfigType;

    /**
     * @brief The sync direction
     **/
    typedef enum _ConfigSync {
        Raw2Value,
        Value2Raw
    } ConfigSync;

    /**
     * @brief Sync result
     **/
    typedef enum _ConfigSyncResult {
        SyncSuccess,
        SyncNoBinding,
        SyncInvalid
    } ConfigSyncResult;

    /**
     * @brief The value of config
     **/
    typedef union _ConfigValueType {
        void *untype;
        int *integer;
        boolean *boolvalue;

        struct _HOTKEYS *hotkey;
        ConfigColor *color;
        int *enumerate;
        char **string;
        char *chr;
    } ConfigValueType;

    typedef struct _ConfigGroup ConfigGroup;

    typedef struct _ConfigOption ConfigOption;

    typedef struct _ConfigFileDesc ConfigFileDesc;

    typedef struct _ConfigGroupDesc ConfigGroupDesc;

    typedef struct _ConfigOptionDesc ConfigOptionDesc;

    typedef struct _GenericConfig GenericConfig;

    typedef struct _ConfigOptionSubkey ConfigOptionSubkey;

    /**
     * @brief sync filter function
     **/
    typedef void (*SyncFilter)(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void* value, ConfigSync sync, void* arg);

    /**
     * @brief Enum value type description
     **/

    typedef struct _ConfigEnum {
        char **enumDesc;
        int enumCount;
    } ConfigEnum;

    /**
     * @brief Config file contains multiple config groups, and the opposite config file description
     **/

    typedef struct _ConfigFile {
        ConfigFileDesc *fileDesc;
        ConfigGroup* groups;
    } ConfigFile;


    /**
     * @brief A generic config struct, config struct can derive from it.
     *        Like: <br>
     *        struct TestConfig { <br>
     *            GenericConfig gconfig; <br>
     *            int own_value; <br>
     *        };
     **/

    struct _GenericConfig {
        /**
         * @brief config file pointer
         **/
        ConfigFile *configFile;
    };

    /**
     * @brief Config Option Description, it describe a Key=Value entry in config file.
     **/

    struct _ConfigOptionDesc {
        char *optionName;
        char *desc;
        ConfigType type;
        char *rawDefaultValue;
        ConfigEnum configEnum;

        UT_hash_handle hh;
    };

    /**
     * @brief Config Group Description, it describe a [Gruop] in config file
     **/

    struct _ConfigGroupDesc {
        /**
         * @brief Group Name
         **/
        char *groupName;
        /**
         * @brief Hash table for option description
         **/
        ConfigOptionDesc *optionsDesc;
        UT_hash_handle hh;
    };

    /**
     * @brief Description of a config file
     **/

    struct _ConfigFileDesc {
        ConfigGroupDesc *groupsDesc;
        char* domain;
    };

    /**
     * @brief Config Option in config file, Key=Value entry
     **/

    struct _ConfigOption {
        char *optionName;
        char *rawValue;
        ConfigValueType value;
        SyncFilter filter;
        void *filterArg;
        ConfigOptionDesc *optionDesc;
        ConfigOptionSubkey *subkey;
        UT_hash_handle hh;
    } ;

    /**
     * @brief Config Option subkey in config file, Key[Subkey]=Value entry
     **/

    struct _ConfigOptionSubkey {
        char *subkeyName;
        char *rawValue;
        UT_hash_handle hh;
    };

    /**
     * @brief Config group in config file, [Group] Entry
     **/

    struct _ConfigGroup {
        /**
         * @brief Group Name, unique in ConfigFile
         **/
        char *groupName;
        /**
         * @brief Group Description
         **/
        ConfigGroupDesc *groupDesc;
        /**
         * @brief Option store with a hash table
         **/
        ConfigOption* options;
        /**
         * @brief UTHash handler
         **/
        UT_hash_handle hh;
    };

    /**
     * @brief declare the binding function
     **/
#define CONFIG_BINDING_DECLARE(config_type) \
    void config_type##ConfigBind(config_type* config, ConfigFile* cfile, ConfigFileDesc* cfdesc);

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
    void config_type##ConfigBind(config_type* config, ConfigFile* cfile, ConfigFileDesc* cfdesc) { \
        (void) cfdesc; \
        GenericConfig *gconfig = (GenericConfig*) config; \
        if (gconfig->configFile) { \
            FreeConfigFile(gconfig->configFile); \
        } \
        gconfig->configFile = cfile;

#define CONFIG_BINDING_BEGIN_WITH_ARG(config_type, arg...) \
    void config_type##ConfigBind(config_type* config, ConfigFile* cfile, ConfigFileDesc* cfdesc, arg) { \
        (void) cfdesc; \
        GenericConfig *gconfig = (GenericConfig*) config; \
        if (gconfig->configFile) { \
            FreeConfigFile(gconfig->configFile); \
        } \
        gconfig->configFile = cfile;
    /**
     * @brief register a binding
     **/
#define CONFIG_BINDING_REGISTER(g, o, var) \
    do { \
        ConfigBindValue(cfile, g, o, &config->var, NULL, NULL); \
    } while(0);

    /**
     * @brief register a binding with filter
     **/
#define CONFIG_BINDING_REGISTER_WITH_FILTER(g, o, var, filter_func) \
    do { \
        ConfigBindValue(cfile, g, o, &config->var, filter_func, NULL); \
    } while(0);

    /**
     * @brief register a binding with filter and extra argument
     **/
#define CONFIG_BINDING_REGISTER_WITH_FILTER_ARG(g, o, var, filter_func, arg) \
    do { \
        ConfigBindValue(cfile, g, o, &config->var, filter_func, arg); \
    } while(0);

    /**
     * @brief binding group end
     **/
#define CONFIG_BINDING_END() }

    /**
     * @brief define a singleton function to load config file description
     **/
#define CONFIG_DESC_DEFINE(funcname, path) \
    ConfigFileDesc *funcname() \
    { \
        static ConfigFileDesc *configDesc = NULL; \
        if (!configDesc) \
        { \
            FILE *tmpfp; \
            tmpfp = GetXDGFileWithPrefix("configdesc", path, "r", NULL); \
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

    /**
     * @brief check the rgb within 0 - 255 or not
     **/
#define IsColorValid(c) ((c) >=0 && (c) <= 255)

    /**
     * @brief round the color within 0 - 255
     **/
#define RoundColor(c) ((c)>=0?((c)<=255?c:255):0)

    /**
     * @brief parse a config file with file name.
     * even the file cannot be read, or with wrong format,
     * it will try to return a usable ConfigFile (missing
     * entry with defaul value).
     *
     * @param filename file name of a configfile
     * @param cfdesc config file description
     * @return ConfigFile*
     **/
    ConfigFile *ParseConfigFile(char *filename, ConfigFileDesc* cfdesc);

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
     * @return ConfigFile*
     **/
    ConfigFile *ParseMultiConfigFile(char **filename, int len, ConfigFileDesc* cfdesc);

    /**
     * @brief same with ParseConfigFile, the input is FILE*
     *
     * @see ParseConfigFile
     * @param fp file pointer
     * @param fileDesc config file description
     * @return ConfigFile*
     **/
    ConfigFile *ParseConfigFileFp(FILE* fp, ConfigFileDesc* fileDesc);

    /**
     * @brief same with ParseMultiConfigFileFp, the input is array of FILE*
     *
     * @see ParseMultiConfigFileFp
     * @param fp array of file pointers
     * @param len lenght of fp
     * @param fileDesc config file description
     * @return ConfigFile*
     **/
    ConfigFile *ParseMultiConfigFileFp(FILE **fp, int len, ConfigFileDesc* fileDesc);

    /**
     * @brief Check the raw ConfigFile and try to fill the default value
     *
     * @param configFile config file
     * @param fileDesc config file description
     * @return boolean
     **/
    boolean CheckConfig(ConfigFile *configFile, ConfigFileDesc* fileDesc);

    /**
     * @brief parse config file description from file
     *
     * @param filename filename
     * @return ConfigFileDesc*
     **/
    ConfigFileDesc *ParseConfigFileDesc(char* filename);

    /**
     * @brief parse config file description from file pointer
     *
     * @see ParseConfigFileDesc
     * @param fp file pointer
     * @return ConfigFileDesc*
     **/
    ConfigFileDesc *ParseConfigFileDescFp(FILE* fp);

    /**
     * @brief internal raw file parse, it can merge the config to existing config file
     *
     * @param filename file
     * @param reuse NULL or existing config file
     * @return ConfigFile*
     **/
    ConfigFile* ParseIni(char* filename, ConfigFile* reuse);

    /**
     * @brief internal raw file parse, it can merge the config to existing config file
     *
     * @see ParseIni
     * @param fp file pointer
     * @param reuse NULL or existing config file
     * @return ConfigFile*
     **/
    ConfigFile* ParseIniFp(FILE* fp, ConfigFile* reuse);

    /**
     * @brief free a config file
     *
     * @param cfile config file
     * @return void
     **/
    void FreeConfigFile(ConfigFile* cfile);

    /**
     * @brief free a config file description
     *
     * @param cfdesc config file description
     * @return void
     **/
    void FreeConfigFileDesc(ConfigFileDesc* cfdesc);

    /**
     * @brief free a config group
     *
     * @param group config group
     * @return void
     **/
    void FreeConfigGroup(ConfigGroup *group);

    /**
     * @brief free a config group description
     *
     * @param cgdesc config group description
     * @return void
     **/
    void FreeConfigGroupDesc(ConfigGroupDesc *cgdesc);

    /**
     * @brief free a config option
     *
     * @param option config option
     * @return void
     **/
    void FreeConfigOption(ConfigOption *option);

    /**
     * @brief free a config option description
     *
     * @param codesc config option description
     * @return void
     **/
    void FreeConfigOptionDesc(ConfigOptionDesc *codesc);

    /**
     * @brief Save config file to fp, it will do the Value2Raw sync
     *
     * @param filename file name
     * @param cfile config
     * @param cdesc config file description
     * @return boolean
     **/
    boolean SaveConfigFile(char *filename, GenericConfig *cfile, ConfigFileDesc* cdesc);

    /**
     * @brief Save config file to fp
     *
     * @see SaveConfigFile
     * @param fp file pointer
     * @param cfile config
     * @param cdesc config file dsecription
     * @return boolean
     **/
    boolean SaveConfigFileFp(FILE* fp, GenericConfig *cfile, ConfigFileDesc* cdesc);

    /**
     * @brief sync a single value
     *
     * @param config config
     * @param group config group
     * @param option config option
     * @param sync sync direction
     * @return Svoid
     **/
    void ConfigSyncValue(GenericConfig* config, ConfigGroup* group, ConfigOption* option, ConfigSync sync);

    /**
     * @brief Get the binded value type
     *
     * @param config config
     * @param group group name
     * @param option option name
     * @return ConfigValueType
     **/
    ConfigValueType ConfigGetBindValue(GenericConfig *config, const char *group, const char* option);

    /**
     * @brief Get a option description from config file description, return NULL if not found.
     *
     * @param cfdesc config file description
     * @param groupName gropu name
     * @param optionName option name
     * @return const ConfigOptionDesc*
     **/
    const ConfigOptionDesc* ConfigDescGetOptionDesc(ConfigFileDesc* cfdesc, const char* groupName, const char* optionName);


    /**
     * @brief Get a option description from config file description, return NULL if not found.
     *
     * @param cfile config file
     * @param groupName gropu name
     * @param optionName option name
     * @return const ConfigOptionDesc*
     *
     * @since 4.1.2
     **/
    ConfigOption* ConfigFileGetOption(ConfigFile* cfile, const char* groupName, const char* optionName);


    /**
     * @brief Get the I18NString value from current locale
     *
     * @param option config option
     * @return const char*
     **/
    const char* ConfigOptionGetLocaleString(ConfigOption* option);

    /**
     * @brief do the Raw2Value sync for config
     *
     * @param config config
     * @return void
     **/
    void ConfigBindSync(GenericConfig* config);

    /**
     * @brief reset a config to default value
     *
     * @param config config
     * @return Svoid
     **/
    void ResetConfigToDefaultValue(GenericConfig* config);

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
    void ConfigBindValue(ConfigFile* cfile, const char *groupName, const char *optionName, void* var, SyncFilter filter, void *arg);

#ifdef __cplusplus
}

#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
