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

#include "fcitx/fcitx.h"
#include "config.h"

#ifdef _ENABLE_OPENCC
#include <opencc.h>
#endif

#include <libintl.h>
#include <errno.h>

#include "fcitx/module.h"
#include "fcitx-utils/utf8.h"
#include "fcitx-utils/uthash.h"
#include "fcitx-config/xdg.h"
#include "fcitx/hook.h"
#include "fcitx/ui.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx-utils/utils.h"
#include "chttrans.h"

#define TABLE_GBKS2T "gbks2t.tab"

typedef struct _simple2trad_t {
    int wc;
    char str[UTF8_MAX_LENGTH + 1];
    size_t len;
    UT_hash_handle hh;
} simple2trad_t;

typedef enum _ChttransEngine {
    ENGINE_NATIVE,
    ENGINE_OPENCC
} ChttransEngine;


typedef struct _FcitxChttrans {
    FcitxGenericConfig gconfig;
    boolean enabled;
    ChttransEngine engine;
    FcitxHotkey hkToggle[2];
    simple2trad_t* s2t_table;
    simple2trad_t* t2s_table;
#ifdef _ENABLE_OPENCC
    opencc_t ods2t;
    opencc_t odt2s;
#endif
    FcitxInstance* owner;
} FcitxChttrans;

static void* ChttransCreate(FcitxInstance* instance);
static char* ChttransOutputFilter(void* arg, const char* strin);
static void ReloadChttrans(void* arg);
static char *ConvertGBKSimple2Tradition(FcitxChttrans* transState, const char* strHZ);
static char *ConvertGBKTradition2Simple(FcitxChttrans* transState, const char *strHZ);
static boolean GetChttransEnabled(void* arg);
static void ChttransLanguageChanged(void* arg, const void* value);
boolean LoadChttransConfig(FcitxChttrans* transState);
static FcitxConfigFileDesc* GetChttransConfigDesc();
static void SaveChttransConfig(FcitxChttrans* transState);
static INPUT_RETURN_VALUE HotkeyToggleChttransState(void*);
static void ToggleChttransState(void*);
static void* ChttransS2T(void* arg, FcitxModuleFunctionArg args);
static void* ChttransT2S(void* arg, FcitxModuleFunctionArg args);

CONFIG_BINDING_BEGIN(FcitxChttrans)
CONFIG_BINDING_REGISTER("TraditionalChinese", "TransEngine", engine)
CONFIG_BINDING_REGISTER("TraditionalChinese", "Enabled", enabled)
CONFIG_BINDING_REGISTER("TraditionalChinese", "Hotkey", hkToggle)
CONFIG_BINDING_END()

FCITX_EXPORT_API
FcitxModule module = {
    ChttransCreate,
    NULL,
    NULL,
    NULL,
    ReloadChttrans
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* ChttransCreate(FcitxInstance* instance)
{
    FcitxChttrans* transState = fcitx_utils_malloc0(sizeof(FcitxChttrans));
    FcitxAddon* transAddon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_CHTTRANS_NAME);
    transState->owner = instance;
    if (!LoadChttransConfig(transState)) {
        free(transState);
        return NULL;
    }

    FcitxHotkeyHook hk;
    hk.arg = transState;
    hk.hotkey = transState->hkToggle;
    hk.hotkeyhandle = HotkeyToggleChttransState;

    FcitxStringFilterHook shk;
    shk.arg = transState;
    shk.func = ChttransOutputFilter;

    FcitxInstanceRegisterHotkeyFilter(instance, hk);
    FcitxInstanceRegisterOutputFilter(instance, shk);
    FcitxInstanceRegisterCommitFilter(instance, shk);
    FcitxUIRegisterStatus(instance, transState, "chttrans", _("Traditional Chinese Translate"), _("Traditional Chinese Translate"), ToggleChttransState, GetChttransEnabled);
    
    FcitxInstanceWatchContext(instance, CONTEXT_IM_LANGUAGE, ChttransLanguageChanged, transState);
    
    AddFunction(transAddon, ChttransS2T);
    AddFunction(transAddon, ChttransT2S);
    
    return transState;
}

INPUT_RETURN_VALUE HotkeyToggleChttransState(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;

    FcitxUIStatus *status = FcitxUIGetStatusByName(transState->owner, "chttrans");
    if (status->visible){
        FcitxUIUpdateStatus(transState->owner, "chttrans");
        return IRV_DO_NOTHING;
    }
    else
        return IRV_TO_PROCESS;
}

void ToggleChttransState(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    transState->enabled = !transState->enabled;
    SaveChttransConfig(transState);
}

boolean GetChttransEnabled(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    return transState->enabled;
}

char* ChttransOutputFilter(void* arg, const char *strin)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    if (transState->enabled) {
        FcitxIM* im = FcitxInstanceGetCurrentIM(transState->owner);
        if (im && (strcmp(im->langCode, "zh_HK") == 0 || strcmp(im->langCode, "zh_TW") == 0))
            return NULL;
        else
            return ConvertGBKSimple2Tradition(transState, strin);
    }
    else {
        FcitxIM* im = FcitxInstanceGetCurrentIM(transState->owner);
        if (im && strcmp(im->langCode, "zh_CN") == 0)
            return NULL;
        else
            return ConvertGBKTradition2Simple(transState, strin);
    }
    return NULL;
}

void* ChttransS2T(void* arg, FcitxModuleFunctionArg args)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    const char* s = args.args[0];
    
    return ConvertGBKSimple2Tradition(transState, s);
}

void* ChttransT2S(void* arg, FcitxModuleFunctionArg args)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    const char* s = args.args[0];
    
    return ConvertGBKTradition2Simple(transState, s);
}

/**
 * 该函数装载data/gbks2t.tab的简体转繁体的码表，
 * 然后按码表将GBK字符转换成GBK繁体字符。
 *
 * WARNING： 该函数返回新分配内存字符串，请调用者
 * 注意释放。
 */
char *ConvertGBKSimple2Tradition(FcitxChttrans* transState, const char *strHZ)
{
    if (strHZ == NULL)
        return NULL;

    switch (transState->engine) {
    case ENGINE_OPENCC:
#ifdef _ENABLE_OPENCC
        {
            if (transState->ods2t == NULL) {
                transState->ods2t = opencc_open(OPENCC_DEFAULT_CONFIG_SIMP_TO_TRAD);
                if (transState->ods2t == NULL) {
                    opencc_perror(_("OpenCC initialization error"));
                    return NULL;
                }
            }

            char * res = opencc_convert_utf8(transState->ods2t, strHZ, (size_t) - 1);

            if (res == (char *) - 1) {
                opencc_perror(_("OpenCC error"));
                return NULL;
            }

            return res;
        }
#endif
    case ENGINE_NATIVE: {
        FILE           *fp;
        char           *ret;
        int             i, len, ret_len;
        char           *strBuf = NULL;
        size_t          bufLen = 0;
        const char     *ps;

        if (!transState->s2t_table) {
            len = 0;

            fp = FcitxXDGGetFileWithPrefix("data", TABLE_GBKS2T, "rb", NULL);
            if (!fp) {
                ret = (char *) malloc(sizeof(char) * (strlen(strHZ) + 1));
                strcpy(ret, strHZ);
                return ret;
            }
            while (getline(&strBuf, &bufLen, fp) != -1) {
                simple2trad_t *s2t;
                char *ps;
                int wc;

                ps = fcitx_utf8_get_char(strBuf, &wc);
                s2t = (simple2trad_t*) malloc(sizeof(simple2trad_t));
                s2t->wc = wc;
                s2t->len = fcitx_utf8_char_len(ps);
                strncpy(s2t->str, ps, s2t->len);
                s2t->str[s2t->len] = '\0';

                HASH_ADD_INT(transState->s2t_table, wc, s2t);
            }
            if (strBuf)
                free(strBuf);
        }

        i = 0;
        len = fcitx_utf8_strlen(strHZ);
        ret_len = 0;
        ret = (char *) fcitx_utils_malloc0(sizeof(char) * (UTF8_MAX_LENGTH * len + 1));
        ps = strHZ;
        ret[0] = '\0';
        for (; i < len; ++i) {
            int wc;
            simple2trad_t *s2t = NULL;
            int chr_len = fcitx_utf8_char_len(ps);
            char *nps;
            nps = fcitx_utf8_get_char(ps , &wc);
            HASH_FIND_INT(transState->s2t_table, &wc, s2t);

            if (s2t) {
                strcat(ret, s2t->str);
                ret_len += s2t->len;
            } else {
                strncat(ret, ps, chr_len);
                ret_len += chr_len;
            }

            ps = nps;

        }
        ret[ret_len] = '\0';

        return ret;
    }
    }
    return NULL;
}

/**
 * 该函数装载data/gbks2t.tab的简体转繁体的码表，
 * 然后按码表将GBK字符转换成GBK繁体字符。
 *
 * WARNING： 该函数返回新分配内存字符串，请调用者
 * 注意释放。
 */
char *ConvertGBKTradition2Simple(FcitxChttrans* transState, const char *strHZ)
{
    if (strHZ == NULL)
        return NULL;

    switch (transState->engine) {
    case ENGINE_OPENCC:
#ifdef _ENABLE_OPENCC
        {
            if (transState->odt2s == NULL) {
                transState->odt2s = opencc_open(OPENCC_DEFAULT_CONFIG_TRAD_TO_SIMP);
                if (transState->odt2s == NULL) {
                    opencc_perror(_("OpenCC initialization error"));
                    return NULL;
                }
            }

            char * res = opencc_convert_utf8(transState->odt2s, strHZ, (size_t) - 1);

            if (res == (char *) - 1) {
                opencc_perror(_("OpenCC error"));
                return NULL;
            }

            return res;
        }
#endif
    case ENGINE_NATIVE: {
        FILE           *fp;
        char           *ret;
        int             i, len, ret_len;
        char           *strBuf = NULL;
        size_t          bufLen = 0;
        const char     *ps;

        if (!transState->t2s_table) {
            len = 0;

            fp = FcitxXDGGetFileWithPrefix("data", TABLE_GBKS2T, "rb", NULL);
            if (!fp) {
                ret = (char *) malloc(sizeof(char) * (strlen(strHZ) + 1));
                strcpy(ret, strHZ);
                return ret;
            }
            while (getline(&strBuf, &bufLen, fp) != -1) {
                simple2trad_t *t2s;
                char *ps;
                int wc;

                ps = fcitx_utf8_get_char(strBuf, &wc);
                t2s = (simple2trad_t*) malloc(sizeof(simple2trad_t));
                
                fcitx_utf8_get_char(ps, &wc);
                t2s->wc = wc;
                t2s->len = fcitx_utf8_char_len(strBuf);
                strncpy(t2s->str, strBuf, t2s->len);
                t2s->str[t2s->len] = '\0';

                HASH_ADD_INT(transState->t2s_table, wc, t2s);
            }
            if (strBuf)
                free(strBuf);
        }

        i = 0;
        len = fcitx_utf8_strlen(strHZ);
        ret_len = 0;
        ret = (char *) fcitx_utils_malloc0(sizeof(char) * (UTF8_MAX_LENGTH * len + 1));
        ps = strHZ;
        ret[0] = '\0';
        for (; i < len; ++i) {
            int wc;
            simple2trad_t *t2s = NULL;
            int chr_len = fcitx_utf8_char_len(ps);
            char *nps;
            nps = fcitx_utf8_get_char(ps , &wc);
            HASH_FIND_INT(transState->s2t_table, &wc, t2s);

            if (t2s) {
                strcat(ret, t2s->str);
                ret_len += t2s->len;
            } else {
                strncat(ret, ps, chr_len);
                ret_len += chr_len;
            }

            ps = nps;

        }
        ret[ret_len] = '\0';

        return ret;
    }
    }
    return NULL;
}

boolean LoadChttransConfig(FcitxChttrans* transState)
{
    FcitxConfigFileDesc* configDesc = GetChttransConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp;
    char *file;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-chttrans.config", "rt", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    boolean newconfig = false;
    if (!fp) {
        if (errno == ENOENT)
            SaveChttransConfig(transState);
        newconfig = true;
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxChttransConfigBind(transState, cfile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*)transState);
    
    if (newconfig) {
        char *p = fcitx_utils_get_current_langcode();
        if (strcmp(p, "zh_TW") == 0 || strcmp(p, "zh_HK") == 0) {
            transState->enabled = true;
            SaveChttransConfig(transState);
        }
        free(p);
    }

    if (fp)
        fclose(fp);

    return true;
}

CONFIG_DESC_DEFINE(GetChttransConfigDesc, "fcitx-chttrans.desc")

void SaveChttransConfig(FcitxChttrans* transState)
{
    FcitxConfigFileDesc* configDesc = GetChttransConfigDesc();
    char *file;
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-chttrans.config", "wt", &file);
    FcitxLog(DEBUG, "Save Config to %s", file);
    FcitxConfigSaveConfigFileFp(fp, &transState->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

void ReloadChttrans(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    LoadChttransConfig(transState);
}

void ChttransLanguageChanged(void* arg, const void* value)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    const char* lang = (const char*) value;
    boolean visible = false;
    if (lang && strncmp(lang, "zh", 2) == 0)
        visible = true;
    
    FcitxUISetStatusVisable(transState->owner, "chttrans", visible);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
