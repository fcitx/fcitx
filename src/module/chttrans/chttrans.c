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

#include "fcitx/fcitx.h"

#ifdef _ENABLE_OPENCC
#include <opencc.h>
#endif

#include <libintl.h>

#include "fcitx/module.h"
#include "fcitx-utils/utf8.h"
#include "fcitx-utils/uthash.h"
#include "fcitx-config/xdg.h"
#include <errno.h>
#include "fcitx/hook.h"
#include "fcitx/ui.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include <fcitx-utils/utils.h>

#define TABLE_GBKS2T "gbks2t.tab"

typedef struct _simple2trad_t
{
    int wc;
    char str[UTF8_MAX_LENGTH + 1];
    size_t len;
    UT_hash_handle hh;
} simple2trad_t;

typedef enum _ChttransEngine
{
    ENGINE_NATIVE,
    ENGINE_OPENCC
} ChttransEngine;


typedef struct _FcitxChttrans {
    GenericConfig gconfig;
    boolean enabled;
    ChttransEngine engine;
    HOTKEYS hkToggle[2];
    simple2trad_t* s2t_table;
    FcitxInstance* owner;
} FcitxChttrans;

static void* ChttransCreate(FcitxInstance* instance);
static char* ChttransOutputFilter(void* arg, const char* strin);
static void ReloadChttrans(void* arg);
static char *ConvertGBKSimple2Tradition (FcitxChttrans* transState, const char* strHZ);
static boolean GetChttransEnabled();
boolean LoadChttransConfig(FcitxChttrans* transState);
static ConfigFileDesc* GetChttransConfigDesc();
static void SaveChttransConfig(FcitxChttrans* transState);
static INPUT_RETURN_VALUE HotkeyToggleChttransState(void*);
static void ToggleChttransState(void*);

CONFIG_BINDING_BEGIN(FcitxChttrans)
CONFIG_BINDING_REGISTER("TraditionalChinese","TransEngine", engine)
CONFIG_BINDING_REGISTER("TraditionalChinese","Enabled", enabled)
CONFIG_BINDING_REGISTER("TraditionalChinese","Hotkey", hkToggle)
CONFIG_BINDING_END()

FCITX_EXPORT_API
FcitxModule module =
{
    ChttransCreate,
    NULL,
    NULL,
    NULL,
    ReloadChttrans
};

void* ChttransCreate(FcitxInstance* instance)
{
    FcitxChttrans* transState = fcitx_malloc0(sizeof(FcitxChttrans));
    transState->owner = instance;
    if (!LoadChttransConfig(transState))
    {
        free(transState);
        return NULL;
    }

    HotkeyHook hk;
    hk.arg = transState;
    hk.hotkey = transState->hkToggle;
    hk.hotkeyhandle = HotkeyToggleChttransState;

    StringFilterHook shk;
    shk.arg = transState;
    shk.func = ChttransOutputFilter;

    RegisterHotkeyFilter(instance, hk);
    RegisterOutputFilter(instance, shk);
    RegisterStatus(instance, transState, "chttrans", "Traditional Chinese Translate", "Traditional Chinese Translate", ToggleChttransState, GetChttransEnabled);
    return transState;
}

INPUT_RETURN_VALUE HotkeyToggleChttransState(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    UpdateStatus(transState->owner, "chttrans");
    return IRV_DO_NOTHING;
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
    if (transState->enabled)
        return ConvertGBKSimple2Tradition(transState, strin);
    else
        return NULL;
}

/**
 * 该函数装载data/gbks2t.tab的简体转繁体的码表，
 * 然后按码表将GBK字符转换成GBK繁体字符。
 *
 * WARNING： 该函数返回新分配内存字符串，请调用者
 * 注意释放。
 */
char *ConvertGBKSimple2Tradition (FcitxChttrans* transState, const char *strHZ)
{
    if (strHZ == NULL)
        return NULL;

    switch (transState->engine)
    {
    case ENGINE_OPENCC:
#ifdef _ENABLE_OPENCC
        {
            static opencc_t od = NULL;
            if (od == NULL)
            {
                od = opencc_open(OPENCC_DEFAULT_CONFIG_SIMP_TO_TRAD);
                if (od == (opencc_t) -1)
                {
                    opencc_perror(_("OpenCC initialization error"));
                    return NULL;
                }
            }

            char * res = opencc_convert_utf8(od, strHZ, (size_t) -1);

            if (res == (char *) -1)
            {
                opencc_perror(_("OpenCC error"));
                return NULL;
            }

            return res;
        }
#endif
    case ENGINE_NATIVE:
    {
        FILE           *fp;
        char           *ret;
        int             i, len, ret_len;
        char           *strBuf = NULL;
        size_t          bufLen = 0;
        const char     *ps;

        if (!transState->s2t_table) {
            len = 0;

            fp = GetXDGFileWithPrefix("data", TABLE_GBKS2T, "rb", NULL);
            if (!fp) {
                ret = (char *) malloc (sizeof (char) * (strlen (strHZ) + 1));
                strcpy (ret, strHZ);
                return ret;
            }
            while (getline(&strBuf, &bufLen, fp) != -1)
            {
                simple2trad_t *s2t;
                char *ps;
                int wc;

                ps = utf8_get_char(strBuf, &wc);
                s2t = (simple2trad_t*) malloc(sizeof(simple2trad_t));
                s2t->wc = wc;
                s2t->len = utf8_char_len(ps);
                strncpy(s2t->str, ps, s2t->len);
                s2t->str[s2t->len] = '\0';

                HASH_ADD_INT(transState->s2t_table, wc, s2t);
            }
            if (strBuf)
                free(strBuf);
        }

        i = 0;
        len = utf8_strlen (strHZ);
        ret_len = 0;
        ret = (char *) malloc (sizeof (char) * (UTF8_MAX_LENGTH * len + 1));
        ps = strHZ;
        ret[0] = '\0';
        for (; i < len; ++i) {
            int wc;
            simple2trad_t *s2t = NULL;
            int chr_len = utf8_char_len(ps);
            char *nps;
            nps = utf8_get_char(ps , &wc);
            HASH_FIND_INT(transState->s2t_table, &wc, s2t);

            if (s2t)
            {
                strcat(ret, s2t->str);
                ret_len += s2t->len;
            }
            else
            {
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
    ConfigFileDesc* configDesc = GetChttransConfigDesc();
    if (configDesc == NULL)
        return false;

    FILE *fp;
    char *file;
    fp = GetXDGFileUserWithPrefix("conf", "fcitx-chttrans.config", "rt", &file);
    FcitxLog(INFO, _("Load Config File %s"), file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveChttransConfig(transState);
    }

    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);

    FcitxChttransConfigBind(transState, cfile, configDesc);
    ConfigBindSync((GenericConfig*)transState);

    if (fp)
        fclose(fp);

    return true;
}

CONFIG_DESC_DEFINE(GetChttransConfigDesc, "fcitx-chttrans.desc")

void SaveChttransConfig(FcitxChttrans* transState)
{
    ConfigFileDesc* configDesc = GetChttransConfigDesc();
    char *file;
    FILE *fp = GetXDGFileUserWithPrefix("conf", "fcitx-chttrans.config", "wt", &file);
    FcitxLog(INFO, "Save Config to %s", file);
    SaveConfigFileFp(fp, &transState->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

void ReloadChttrans(void* arg)
{
    FcitxChttrans* transState = (FcitxChttrans*) arg;
    LoadChttransConfig(transState);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
