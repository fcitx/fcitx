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
#include "fcitx-utils/stringmap.h"
#include "chttrans.h"
#include "chttrans_p.h"
#ifdef ENABLE_OPENCC
#include "chttrans-opencc.h"
#endif
#include "module/freedesktop-notify/fcitx-freedesktop-notify.h"

#define TABLE_GBKS2T "gbks2t.tab"

static void* ChttransCreate(FcitxInstance* instance);
static char* ChttransOutputFilter(void* arg, const char* strin);
static void ChttransIMChanged(void* arg);
static void ReloadChttrans(void* arg);
static char *ConvertGBKSimple2Tradition(FcitxChttrans* transState,
                                        const char* strHZ);
static char *ConvertGBKTradition2Simple(FcitxChttrans* transState,
                                        const char *strHZ);
static boolean GetChttransEnabled(void* arg);
static void ChttransLanguageChanged(void* arg, const void* value);
boolean LoadChttransConfig(FcitxChttrans* transState);
static FcitxConfigFileDesc* GetChttransConfigDesc();
static void SaveChttransConfig(FcitxChttrans* transState);
static INPUT_RETURN_VALUE HotkeyToggleChttransState(void*);
static void ToggleChttransState(void*);
static void ChttransEnabledForIMFilter(FcitxGenericConfig *config,
                                       FcitxConfigGroup *group,
                                       FcitxConfigOption *option, void* value,
                                       FcitxConfigSync sync, void* arg);

DECLARE_ADDFUNCTIONS(Chttrans)

CONFIG_BINDING_BEGIN(FcitxChttrans)
CONFIG_BINDING_REGISTER("TraditionalChinese", "TransEngine", engine)
CONFIG_BINDING_REGISTER("TraditionalChinese", "Hotkey", hkToggle)
CONFIG_BINDING_REGISTER_WITH_FILTER("TraditionalChinese", "EnabledForIM",
                                    strEnableForIM, ChttransEnabledForIMFilter)
CONFIG_BINDING_END()

FCITX_DEFINE_PLUGIN(fcitx_chttrans, module, FcitxModule) = {
    ChttransCreate,
    NULL,
    NULL,
    NULL,
    ReloadChttrans
};

static boolean
ChttransEnabled(FcitxChttrans *chttrans)
{
    boolean result = false;
    FcitxIM* im = FcitxInstanceGetCurrentIM(chttrans->owner);
    if (im) {
        boolean defaultValue = false;
        if (strcmp(im->langCode, "zh_TW") == 0 ||
            strcmp(im->langCode, "en_HK") == 0 ||
            strcmp(im->langCode, "zh_HK") == 0) {
            defaultValue = true;
        }
        result = fcitx_string_map_get(chttrans->enableIM, im->uniqueName,
                                      defaultValue);
    }
    return result;
}

static void
ChttransEnabledForIMFilter(FcitxGenericConfig *config, FcitxConfigGroup *group,
                           FcitxConfigOption *option, void *value,
                           FcitxConfigSync sync, void *arg)
{
    FCITX_UNUSED(group);
    FCITX_UNUSED(option);
    FCITX_UNUSED(arg);
    FcitxChttrans *chttrans = (FcitxChttrans*)config;
    char **enableForIM = (char**)value;
    if (sync == Value2Raw) {
        fcitx_utils_free(*enableForIM);
        *enableForIM = fcitx_string_map_to_string(chttrans->enableIM, ',');
    } else if (sync == Raw2Value) {
        if (*enableForIM) {
            fcitx_string_map_from_string(chttrans->enableIM, *enableForIM, ',');
        }
    }
}

void *ChttransCreate(FcitxInstance* instance)
{
    FcitxChttrans *transState = fcitx_utils_new(FcitxChttrans);
    transState->owner = instance;
    transState->enableIM = fcitx_string_map_new(NULL, '\0');
    if (!LoadChttransConfig(transState)) {
        fcitx_string_map_free(transState->enableIM);
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

    FcitxIMEventHook imhk;
    imhk.arg = transState;
    imhk.func = ChttransIMChanged;

    FcitxInstanceRegisterHotkeyFilter(instance, hk);
    FcitxInstanceRegisterOutputFilter(instance, shk);
    FcitxInstanceRegisterCommitFilter(instance, shk);
    FcitxInstanceRegisterIMChangedHook(instance, imhk);
    FcitxUIRegisterStatus(instance, transState, "chttrans",
                          ChttransEnabled(transState) ? _("Traditional Chinese") :  _("Simplified Chinese"),
                          _("Toggle Simp/Trad Chinese Conversion"),
                          ToggleChttransState,
                          GetChttransEnabled);

    FcitxInstanceWatchContext(instance, CONTEXT_IM_LANGUAGE,
                              ChttransLanguageChanged, transState);

    FcitxChttransAddFunctions(instance);
    return transState;
}

INPUT_RETURN_VALUE HotkeyToggleChttransState(void* arg)
{
    FcitxChttrans *transState = (FcitxChttrans*)arg;
    FcitxInstance *instance = transState->owner;

    FcitxUIStatus *status = FcitxUIGetStatusByName(instance, "chttrans");
    if (status->visible){
        FcitxUIUpdateStatus(instance, "chttrans");
        boolean enabled = ChttransEnabled(transState);
        FcitxFreeDesktopNotifyShowAddonTip(
            instance, "fcitx-chttrans-toggle",
            enabled ? "fcitx-chttrans-active" :
                      "fcitx-chttrans-inactive",
            _("Simplified Chinese To Traditional Chinese"),
            enabled ? _("Traditional Chinese is enabled.") :
                      _("Simplified Chinese is enabled."));
        return IRV_DO_NOTHING;
    } else {
        return IRV_TO_PROCESS;
    }
}

void ToggleChttransState(void* arg)
{
    FcitxChttrans *transState = (FcitxChttrans*)arg;
    FcitxInstance *instance = transState->owner;
    FcitxIM* im = FcitxInstanceGetCurrentIM(transState->owner);
    if (!im)
        return;
    boolean enabled = !ChttransEnabled(transState);

    fcitx_string_map_set(transState->enableIM, im->uniqueName, enabled);
    FcitxUISetStatusString(instance, "chttrans",
                           enabled ? _("Traditional Chinese") :
                           _("Simplified Chinese"),
                          _("Toggle Simp/Trad Chinese Conversion"));
    FcitxUIUpdateInputWindow(instance);
    SaveChttransConfig(transState);
}

boolean GetChttransEnabled(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    return ChttransEnabled(transState);
}

char* ChttransOutputFilter(void* arg, const char *strin)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    FcitxIM* im = FcitxInstanceGetCurrentIM(transState->owner);

    /* don't trans for "zh" */
    if (!im || strncmp(im->langCode, "zh", 2) != 0 || strlen(im->langCode) == 2)
        return NULL;

    if (ChttransEnabled(transState)) {
        if (strcmp(im->langCode, "zh_HK") == 0 ||
            strcmp(im->langCode, "zh_TW") == 0) {
            return NULL;
        } else {
            return ConvertGBKSimple2Tradition(transState, strin);
        }
    } else {
        if (strcmp(im->langCode, "zh_CN") == 0) {
            return NULL;
        } else {
            return ConvertGBKTradition2Simple(transState, strin);
        }
    }
    return NULL;
}

void ChttransIMChanged(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    FcitxIM* im = FcitxInstanceGetCurrentIM(transState->owner);
    if (!im)
        return;
    FcitxUIRefreshStatus(transState->owner, "chttrans");
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
#ifdef ENABLE_OPENCC
        do {
            if (transState->ods2t == NULL) {
                OpenCCInit(transState);
                if (transState->ods2t == NULL) {
                    /* break to native as fallback */
                    break;
                }
            }

            char * res = OpenCCConvert(transState->ods2t, strHZ, (size_t) - 1);

            if (!res || res == (char *) - 1) {
                return NULL;
            }

            return res;
        } while(0);
#endif
    case ENGINE_NATIVE: {
        FILE           *fp;
        char           *ret;
        int             i, len, ret_len;
        const char     *ps;

        if (!transState->s2t_table) {
            char *strBuf = NULL;
            size_t bufLen = 0;
            fp = FcitxXDGGetFileWithPrefix("data", TABLE_GBKS2T, "r", NULL);
            if (!fp) {
                ret = (char *) malloc(sizeof(char) * (strlen(strHZ) + 1));
                strcpy(ret, strHZ);
                return ret;
            }
            while (getline(&strBuf, &bufLen, fp) != -1) {
                simple2trad_t *s2t = NULL;
                char *ps;
                uint32_t wc;

                ps = fcitx_utf8_get_char(strBuf, &wc);

                HASH_FIND_INT(transState->s2t_table, &wc, s2t);
                if (s2t) {
                    continue;
                }

                s2t = (simple2trad_t*) malloc(sizeof(simple2trad_t));
                s2t->wc = wc;
                s2t->len = fcitx_utf8_char_len(ps);
                strncpy(s2t->str, ps, s2t->len);
                s2t->str[s2t->len] = '\0';

                HASH_ADD_INT(transState->s2t_table, wc, s2t);
            }
            fcitx_utils_free(strBuf);
        }

        i = 0;
        len = fcitx_utf8_strlen(strHZ);
        ret_len = 0;
        ret = fcitx_utils_malloc0(UTF8_MAX_LENGTH * len + 1);
        ps = strHZ;
        ret[0] = '\0';
        for (; i < len; ++i) {
            uint32_t wc;
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
#ifdef ENABLE_OPENCC
        do {
            if (transState->odt2s == NULL) {
                OpenCCInit(transState);
                if (transState->odt2s == NULL) {
                    break;
                }
            }

            char * res = OpenCCConvert(transState->odt2s, strHZ, (size_t) - 1);

            if (!res || res == (char *) - 1) {
                return NULL;
            }

            return res;
        } while(0);
#endif
    case ENGINE_NATIVE: {
        FILE           *fp;
        char           *ret;
        int             i, len, ret_len;
        const char     *ps;

        if (!transState->t2s_table) {
            char *strBuf = NULL;
            size_t bufLen = 0;
            fp = FcitxXDGGetFileWithPrefix("data", TABLE_GBKS2T, "r", NULL);
            if (!fp) {
                ret = (char *) malloc(sizeof(char) * (strlen(strHZ) + 1));
                strcpy(ret, strHZ);
                return ret;
            }
            while (getline(&strBuf, &bufLen, fp) != -1) {
                simple2trad_t *t2s = NULL;
                char *ps;
                uint32_t wc;

                ps = fcitx_utf8_get_char(strBuf, &wc);
                HASH_FIND_INT(transState->s2t_table, &wc, t2s);
                if (t2s) {
                    continue;
                }
                t2s = (simple2trad_t*) malloc(sizeof(simple2trad_t));

                fcitx_utf8_get_char(ps, &wc);
                t2s->wc = wc;
                t2s->len = fcitx_utf8_char_len(strBuf);
                strncpy(t2s->str, strBuf, t2s->len);
                t2s->str[t2s->len] = '\0';

                HASH_ADD_INT(transState->t2s_table, wc, t2s);
            }
            fcitx_utils_free(strBuf);
        }

        i = 0;
        len = fcitx_utf8_strlen(strHZ);
        ret_len = 0;
        ret = fcitx_utils_malloc0(UTF8_MAX_LENGTH * len + 1);
        ps = strHZ;
        ret[0] = '\0';
        for (; i < len; ++i) {
            uint32_t wc;
            simple2trad_t *t2s = NULL;
            int chr_len = fcitx_utf8_char_len(ps);
            char *nps;
            nps = fcitx_utf8_get_char(ps , &wc);
            HASH_FIND_INT(transState->t2s_table, &wc, t2s);

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
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-chttrans.config",
                                       "r", NULL);
    if (!fp) {
        if (errno == ENOENT)
            SaveChttransConfig(transState);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxChttransConfigBind(transState, cfile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*)transState);

    if (fp)
        fclose(fp);

    return true;
}

CONFIG_DESC_DEFINE(GetChttransConfigDesc, "fcitx-chttrans.desc")

void SaveChttransConfig(FcitxChttrans* transState)
{
    FcitxConfigFileDesc* configDesc = GetChttransConfigDesc();
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-chttrans.config",
                                             "w", NULL);
    FcitxConfigSaveConfigFileFp(fp, &transState->gconfig, configDesc);
    if (fp) {
        fclose(fp);
    }
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
    if (lang && strncmp(lang, "zh", 2) == 0 && lang[2])
        visible = true;

    FcitxUISetStatusVisable(transState->owner, "chttrans", visible);
}

#include "fcitx-chttrans-addfunctions.h"
