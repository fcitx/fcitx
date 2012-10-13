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
 * @defgroup FcitxConfig FcitxConfig
 *
 * FcitxConfig includes a lot of configuration related macro and function.
 * Macro can be easily used to bind a struct with configuration
 *
 * Fcitx configuration file can be easily mapped to corresponding user interface,
 * and you don't need to write any user interface at all.
 *
 * FcitxConfig can be also used to implement native user interface.
 *
 * Here is a common example for use macro binding with a struct
 *
 * @code
 *    typedef struct _FcitxProfile {
 *        FcitxGenericConfig gconfig;
 *        boolean bUseRemind;
 *        char* imName;
 *        boolean bUseWidePunc;
 *        boolean bUseFullWidthChar;
 *        boolean bUsePreedit;
 *        char* imList;
 *    } FcitxProfile;
 * @endcode
 *
 * A config struct need to put FcitxGenericConfig as first field.
 * Following code will define a function
 *
 * @code
 * CONFIG_BINDING_BEGIN_WITH_ARG(FcitxProfile, FcitxInstance* instance)
 * CONFIG_BINDING_REGISTER("Profile", "FullWidth", bUseFullWidthChar)
 * CONFIG_BINDING_REGISTER("Profile", "UseRemind", bUseRemind)
 * CONFIG_BINDING_REGISTER_WITH_FILTER_ARG("Profile", "IMName", imName, FilterIMName, instance)
 * CONFIG_BINDING_REGISTER("Profile", "WidePunc", bUseWidePunc)
 * CONFIG_BINDING_REGISTER("Profile", "PreeditStringInClientWindow", bUsePreedit)
 * CONFIG_BINDING_REGISTER_WITH_FILTER_ARG("Profile", "EnabledIMList", imList, FilterIMList, instance)
 * CONFIG_BINDING_END()
 * @endcode
 *
 * Then you will get following function:
 *
 * @code
 * void FcitxProfileConfigBind( FcitxProfile* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc, FcitxInstance* instance )
 * @endcode
 *
 * If you need forward declaration, you can used
 * @code
 * CONFIG_BINDING_DECLARE_WITH_ARG(FcitxProfile, FcitxInstance* instance)
 * @endcode
 *
 * The FcitxConfigFileDesc pointer is coresponding to the .desc file, which need to be placed
 * under share/fcitx/configdesc/
 *
 * You can use following macro to define a define to load FcitxConfigFileDesc* pointer,
 * second argument is the .desc file name.
 *
 * The FcitxConfigFileDesc pointer returned by this macro is a static variable, so it should not be
 * free'd, and will only load once.
 *
 * @code
 * CONFIG_DESC_DEFINE(GetProfileDesc, "profile.desc")
 * @endcode
 */

/**
 * @addtogroup FcitxConfig
 * @{
 */

/**
 * @file fcitx-config.h
 * @author CSSlayer wengxt@gmail.com
 * @date 2010-04-30
 *
 * Fcitx configure file read-write
 */

#ifndef _FCITX_FCITX_CONFIG_H_
#define _FCITX_FCITX_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcitx-utils/uthash.h>
#include <fcitx-utils/utils.h>
#include <fcitx-utils/log.h>
#include <fcitx-config/xdg.h>
#include <fcitx-config/hotkey.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * The Color type in config file
     **/
    typedef struct _FcitxConfigColor {
        double r; /**< red */
        double g; /**< green */
        double b; /**< blue */
    } FcitxConfigColor;

    /**
     * config value type
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
     * The sync direction
     **/
    typedef enum _FcitxConfigSync {
        Raw2Value,
        Value2Raw,
        ValueFree
    } FcitxConfigSync;

    /**
     * Sync result
     **/
    typedef enum _FcitxConfigSyncResult {
        SyncSuccess,
        SyncNoBinding,
        SyncInvalid
    } FcitxConfigSyncResult;

    /**
     * The value of config
     **/
    typedef union _FcitxConfigValueType {
        void *untype; /**< simple pointer */
        int *integer; /**< pointer to integer */
        boolean *boolvalue; /**< pointer to boolean */

        struct _FcitxHotkey *hotkey; /**< pointer to two hotkeys */
        FcitxConfigColor *color; /**< pointer to color */
        int *enumerate; /**< pointer to enum */
        char **string; /**< pointer to string */
        char *chr; /**< pointer to char */
    } FcitxConfigValueType;

    typedef struct _FcitxConfigGroup FcitxConfigGroup; /**< FcitxConfigGroup */

    typedef struct _FcitxConfigOption FcitxConfigOption; /**< FcitxConfigOption */

    typedef struct _FcitxConfigFileDesc FcitxConfigFileDesc; /**< FcitxConfigFileDesc */

    typedef struct _FcitxConfigGroupDesc FcitxConfigGroupDesc; /**< FcitxConfigGroupDesc */

    typedef struct _FcitxConfigOptionDesc FcitxConfigOptionDesc; /**< FcitxConfigOptionDesc */

    typedef struct _FcitxConfigOptionDesc2 FcitxConfigOptionDesc2; /**< FcitxConfigOptionDesc2 */

    typedef struct _FcitxGenericConfig FcitxGenericConfig; /**< FcitxGenericConfig */

    typedef struct _FcitxConfigOptionSubkey FcitxConfigOptionSubkey; /**< FcitxConfigOptionSubkey */

    typedef union _FcitxConfigConstrain FcitxConfigConstrain /** < FcitxConfigConstrain */;

    /**
     * sync filter function
     **/
    typedef void (*FcitxSyncFilter)(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg);

    /**
     * Enum value type description
     **/
    typedef struct _FcitxConfigEnum {
        char **enumDesc; /**< enum string description, a user visble string */
        int enumCount; /**< length of enumDesc */
    } FcitxConfigEnum;

    /**
     * Config file contains multiple config groups, and the opposite config file description
     **/
    typedef struct _FcitxConfigFile {
        FcitxConfigFileDesc *fileDesc; /**< configuration file description */
        FcitxConfigGroup* groups; /**< contained group */
    } FcitxConfigFile;


    /**
     * A generic config struct, config struct can derive from it.
     * @code
     *        struct TestConfig {
     *            FcitxGenericConfig gconfig;
     *            int own_value;
     *        };
     * @endcode
     **/
    struct _FcitxGenericConfig {
        /**
         * config file pointer
         **/
        FcitxConfigFile *configFile;
    };

    /**
     * Config Option Description, it describe a Key=Value entry in config file.
     **/
    struct _FcitxConfigOptionDesc {
        char *optionName; /**< option name */
        char *desc; /**< optiont description string, user visible */
        FcitxConfigType type; /**< value type */
        char *rawDefaultValue; /**< raw string default value */
        FcitxConfigEnum configEnum; /**< if type is enum, the enum item info */

        UT_hash_handle hh; /**< hash handle */
    };

    union _FcitxConfigConstrain {
        struct {
            int min;
            int max;
        } integerConstrain;

        struct {
            size_t maxLength;
        } stringConstrain;

        void* padding[10];
    };

    /**
     * Config option description v2
     */
    struct _FcitxConfigOptionDesc2 {
        struct _FcitxConfigOptionDesc optionDesc;
        boolean advance;
        FcitxConfigConstrain constrain;
        char* longDesc;
        void* padding[16];
    };

    /**
     * Config Group Description, it describe a [Gruop] in config file
     **/

    struct _FcitxConfigGroupDesc {
        char *groupName; /**< Group Name */
        FcitxConfigOptionDesc *optionsDesc; /**< Hash table for option description */
        UT_hash_handle hh; /**< hash handle */
    };

    /**
     * Description of a config file
     **/
    struct _FcitxConfigFileDesc {
        FcitxConfigGroupDesc *groupsDesc; /**< group description */
        char* domain; /**< domain for translation */
    };

    /**
     * Config Option in config file, Key=Value entry
     **/
    struct _FcitxConfigOption {
        char *optionName; /**< option name */
        char *rawValue; /**< raw string value */
        FcitxConfigValueType value; /**< value type */
        FcitxSyncFilter filter; /**< filter function */
        void *filterArg; /**< argument for filter function */
        union {
            FcitxConfigOptionDesc *optionDesc; /**< option description pointer */
            FcitxConfigOptionDesc2 *optionDesc2; /**< option description pointer */
        };
        FcitxConfigOptionSubkey *subkey; /**< subkey which only used with I18NString */
        UT_hash_handle hh; /**< hash handle */
    } ;

    /**
     * Config Option subkey in config file, Key[Subkey]=Value entry
     **/
    struct _FcitxConfigOptionSubkey {
        char *subkeyName; /**< subkey name */
        char *rawValue; /**< subkey raw value */
        UT_hash_handle hh; /**< hash handle */
    };

    /**
     * Config group in config file, [Group] Entry
     **/
    struct _FcitxConfigGroup {
        /**
         * Group Name, unique in FcitxConfigFile
         **/
        char *groupName;
        /**
         * Group Description
         **/
        FcitxConfigGroupDesc *groupDesc;
        /**
         * Option store with a hash table
         **/
        FcitxConfigOption* options;
        /**
         * UTHash handler
         **/
        UT_hash_handle hh;
    };

    /**
     * declare the binding function
     **/
#define CONFIG_BINDING_DECLARE(config_type) \
    void config_type##ConfigBind(config_type* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc);

    /**
     * declare the binding function, with extra argument
     **/
#define CONFIG_BINDING_DECLARE_WITH_ARG(config_type, arg...) \
    void config_type##ConfigBind(config_type* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc, arg);

    /**
     * define the binding function
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

        /** register binding and call it with extra argument */
#define CONFIG_BINDING_BEGIN_WITH_ARG(config_type, arg...) \
    void config_type##ConfigBind(config_type* config, FcitxConfigFile* cfile, FcitxConfigFileDesc* cfdesc, arg) { \
        (void) cfdesc; \
        FcitxGenericConfig *gconfig = (FcitxGenericConfig*) config; \
        if (gconfig->configFile) { \
            FcitxConfigFreeConfigFile(gconfig->configFile); \
        } \
        gconfig->configFile = cfile;
    /**
     * register a binding
     **/
#define CONFIG_BINDING_REGISTER(g, o, var) \
    do { \
        FcitxConfigBindValue(cfile, g, o, &config->var, NULL, NULL); \
    } while(0);

    /**
     * register a binding with filter
     **/
#define CONFIG_BINDING_REGISTER_WITH_FILTER(g, o, var, filter_func) \
    do { \
        FcitxConfigBindValue(cfile, g, o, &config->var, filter_func, NULL); \
    } while(0);

    /**
     * register a binding with filter and extra argument
     **/
#define CONFIG_BINDING_REGISTER_WITH_FILTER_ARG(g, o, var, filter_func, arg) \
    do { \
        FcitxConfigBindValue(cfile, g, o, &config->var, filter_func, arg); \
    } while(0);

    /**
     * binding group end
     **/
#define CONFIG_BINDING_END() }

    /**
     * define a singleton function to load config file description
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

#define CONFIG_DEFINE_LOAD_AND_SAVE(name, type, config_name) \
CONFIG_DESC_DEFINE(Get##name##Desc, config_name ".desc") \
void name##SaveConfig(type* _cfg) \
{ \
    FcitxConfigFileDesc* configDesc = Get##name##Desc(); \
    char *file; \
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", config_name ".config", "w", &file); \
    FcitxLog(DEBUG, "Save Config to %s", file); \
    FcitxConfigSaveConfigFileFp(fp, &_cfg->gconfig, configDesc); \
    free(file); \
    if (fp) \
        fclose(fp); \
} \
boolean name##LoadConfig(type* _cfg) { \
    FcitxConfigFileDesc* configDesc = Get##name##Desc(); \
    if (configDesc == NULL) \
        return false; \
    \
    FILE *fp; \
    char *file; \
    fp = FcitxXDGGetFileUserWithPrefix("conf", config_name ".config", "r", &file); \
    FcitxLog(DEBUG, "Load Config File %s", file); \
    free(file); \
    if (!fp) { \
        if (errno == ENOENT) \
            name##SaveConfig(_cfg); \
    } \
    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc); \
    type##ConfigBind(_cfg, cfile, configDesc); \
    FcitxConfigBindSync((FcitxGenericConfig*)_cfg); \
    if (fp) \
        fclose(fp); \
    return true; \
} \

    /**
     * parse a config file with file name.
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
     * parse multi config file, the main difference
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
     * same with ParseConfigFile, the input is FILE*
     *
     * @see ParseConfigFile
     * @param fp file pointer
     * @param fileDesc config file description
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile *FcitxConfigParseConfigFileFp(FILE* fp, FcitxConfigFileDesc* fileDesc);

    /**
     * same with FcitxConfigParseMultiConfigFileFp, the input is array of FILE*
     *
     * @see FcitxConfigParseMultiConfigFileFp
     * @param fp array of file pointers
     * @param len lenght of fp
     * @param fileDesc config file description
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile *FcitxConfigParseMultiConfigFileFp(FILE **fp, int len, FcitxConfigFileDesc* fileDesc);

    /**
     * Check the raw FcitxConfigFile and try to fill the default value
     *
     * @param configFile config file
     * @param fileDesc config file description
     * @return boolean
     **/
    boolean FcitxConfigCheckConfigFile(FcitxConfigFile *configFile, FcitxConfigFileDesc* fileDesc);

    /**
     * parse config file description from file
     *
     * @param filename filename
     * @return FcitxConfigFileDesc*
     **/
    FcitxConfigFileDesc *FcitxConfigParseConfigFileDesc(char* filename);

    /**
     * parse config file description from file pointer
     *
     * @see ParseConfigFileDesc
     * @param fp file pointer
     * @return FcitxConfigFileDesc*
     **/
    FcitxConfigFileDesc *FcitxConfigParseConfigFileDescFp(FILE* fp);

    /**
     * internal raw file parse, it can merge the config to existing config file
     *
     * @param filename file
     * @param reuse NULL or existing config file
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile* FcitxConfigParseIni(char* filename, FcitxConfigFile* reuse);

    /**
     * internal raw file parse, it can merge the config to existing config file
     *
     * @see ParseIni
     * @param fp file pointer
     * @param reuse NULL or existing config file
     * @return FcitxConfigFile*
     **/
    FcitxConfigFile* FcitxConfigParseIniFp(FILE* fp, FcitxConfigFile* reuse);

    /**
     * free a config file
     *
     * @param cfile config file
     * @return void
     **/
    void FcitxConfigFreeConfigFile(FcitxConfigFile* cfile);

    /**
     * free a config file description
     *
     * @param cfdesc config file description
     * @return void
     **/
    void FcitxConfigFreeConfigFileDesc(FcitxConfigFileDesc* cfdesc);

    /**
     * free a config group
     *
     * @param group config group
     * @return void
     **/
    void FcitxConfigFreeConfigGroup(FcitxConfigGroup *group);

    /**
     * free a config group description
     *
     * @param cgdesc config group description
     * @return void
     **/
    void FcitxConfigFreeConfigGroupDesc(FcitxConfigGroupDesc *cgdesc);

    /**
     * free a config option
     *
     * @param option config option
     * @return void
     **/
    void FcitxConfigFreeConfigOption(FcitxConfigOption *option);

    /**
     * free a config option description
     *
     * @param codesc config option description
     * @return void
     **/
    void FcitxConfigFreeConfigOptionDesc(FcitxConfigOptionDesc *codesc);

    /**
     * Save config file to fp, it will do the Value2Raw sync
     *
     * @param filename file name
     * @param cfile config
     * @param cdesc config file description
     * @return boolean
     **/
    boolean FcitxConfigSaveConfigFile(char *filename, FcitxGenericConfig *cfile, FcitxConfigFileDesc* cdesc);

    /**
     * Save config file to fp
     *
     * @see SaveConfigFile
     * @param fp file pointer
     * @param cfile config
     * @param cdesc config file dsecription
     * @return boolean
     **/
    boolean FcitxConfigSaveConfigFileFp(FILE* fp, FcitxGenericConfig *cfile, FcitxConfigFileDesc* cdesc);

    /**
     * sync a single value
     *
     * @param config config
     * @param group config group
     * @param option config option
     * @param sync sync direction
     * @return Svoid
     **/
    void FcitxConfigSyncValue(FcitxGenericConfig* config, FcitxConfigGroup* group, FcitxConfigOption* option, FcitxConfigSync sync);

    /**
     * Get the binded value type
     *
     * @param config config
     * @param group group name
     * @param option option name
     * @return FcitxConfigValueType
     **/
    FcitxConfigValueType FcitxConfigGetBindValue(FcitxGenericConfig *config, const char *group, const char* option);

    /**
     * Get a option description from config file description, return NULL if not found.
     *
     * @param cfdesc config file description
     * @param groupName gropu name
     * @param optionName option name
     * @return const FcitxConfigOptionDesc*
     **/
    const FcitxConfigOptionDesc* FcitxConfigDescGetOptionDesc(FcitxConfigFileDesc* cfdesc, const char* groupName, const char* optionName);


    /**
     * Get a option description from config file description, return NULL if not found.
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
     * Get the I18NString value from current locale
     *
     * @param option config option
     * @return const char*
     **/
    const char* FcitxConfigOptionGetLocaleString(FcitxConfigOption* option);

    /**
     * do the Raw2Value sync for config
     *
     * @param config config
     * @return void
     **/
    void FcitxConfigBindSync(FcitxGenericConfig* config);

    /**
     * reset a config to default value
     *
     * @param config config
     * @return Svoid
     **/
    void FcitxConfigResetConfigToDefaultValue(FcitxGenericConfig* config);

    /**
     * bind a value with a struct, normally you should use
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

    /**
     * free a binded config struct with all related value
     * @param config config
     * @return void
     */
    void FcitxConfigFree(FcitxGenericConfig* config);

#ifdef __cplusplus
}

#endif

#endif

/**
 * @}
 */

// kate: indent-mode cstyle; space-indent on; indent-width 0;
