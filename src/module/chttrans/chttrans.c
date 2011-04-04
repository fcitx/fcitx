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

#include "core/fcitx.h"

#ifdef HAVE_OPENCC
#include <opencc.h>
#endif

#include <libintl.h>

#include "core/module.h"
#include "utils/utf8.h"
#include "fcitx-config/uthash.h"
#include "fcitx-config/xdg.h"
#include <errno.h>
#include <core/hook.h>
#include <core/ui.h>
#include <fcitx-config/cutils.h>

#define TABLE_GBKS2T "gbks2t.tab"

static boolean ChttransInit();
typedef struct simple2trad_t
{
    int wc;
    char str[UTF8_MAX_LENGTH + 1];
    size_t len;
    UT_hash_handle hh;
} simple2trad_t;

typedef enum ChttransEngine
{
    ENGINE_NATIVE,
    ENGINE_OPENCC
} ChttransEngine;


typedef struct FcitxChttrans{
    GenericConfig gconfig;
    boolean enabled;
    ChttransEngine engine;
    HOTKEYS hkToggle[2];
} FcitxChttrans;

static char* ChttransOutputFilter(char *strin);
static char *ConvertGBKSimple2Tradition (char *strHZ, ChttransEngine engine);
static boolean GetChttransEnabled();
static void LoadChttransConfig();
static ConfigFileDesc* GetChttransConfigDesc();
static void SaveChttransConfig();
static INPUT_RETURN_VALUE HotkeyToggleChttransState();
static void ToggleChttransState();

FcitxChttrans transState;
simple2trad_t* s2t_table = NULL;

CONFIG_BINDING_BEGIN(FcitxChttrans);
CONFIG_BINDING_REGISTER("TraditionalChinese","TransEngine", engine);
CONFIG_BINDING_REGISTER("TraditionalChinese","Enabled", enabled);
CONFIG_BINDING_REGISTER("TraditionalChinese","Hotkey", hkToggle);
CONFIG_BINDING_END()

FCITX_EXPORT_API
FcitxModule module =
{
    ChttransInit,
    NULL
};

boolean ChttransInit()
{
    LoadChttransConfig();
    
    HotkeyHook hk;
    hk.hotkey = transState.hkToggle;
    hk.hotkeyhandle = HotkeyToggleChttransState;
    
    RegisterHotkeyFilter(hk);
    RegisterOutputFilter(ChttransOutputFilter);
    RegisterStatus("chttrans", ToggleChttransState, GetChttransEnabled);
    return true;
}

INPUT_RETURN_VALUE HotkeyToggleChttransState()
{
    UpdateStatus("chttrans");
    return IRV_DO_NOTHING;
}

void ToggleChttransState()
{
    transState.enabled = !transState.enabled;
}

boolean GetChttransEnabled()
{
    return transState.enabled;
}

char* ChttransOutputFilter(char *strin)
{
    if (transState.enabled)
        return ConvertGBKSimple2Tradition(strin, transState.engine);
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
char *ConvertGBKSimple2Tradition (char *strHZ, ChttransEngine engine)
{
    if (strHZ == NULL)
        return NULL;

    switch(engine)
    {
        case ENGINE_OPENCC:
#ifdef HAVE_OPENCC
        {
            static opencc_t od = NULL;
            if (od == NULL)
            {
                od = opencc_open(OPENCC_DEFAULT_CONFIG_SIMP_TO_TRAD);
                if (od == (opencc_t) -1)
                {
                    opencc_perror(_("OpenCC initialization error"));
                    exit(1);
                }
            }

            char * res = opencc_convert_utf8(od, strHZ, (size_t) -1);

            if (res == (char *) -1)
            {
                opencc_perror(_("OpenCC error"));
                exit(1);
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
            char           *ps;

            if (!s2t_table) {
                len = 0;

                fp = GetXDGFileData(TABLE_GBKS2T, "rb", NULL);
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

                    HASH_ADD_INT(s2t_table, wc, s2t);
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
                HASH_FIND_INT(s2t_table, &wc, s2t);

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

void LoadChttransConfig()
{
    FILE *fp;
    char *file;
    fp = GetXDGFileUser( "addon/fcitx-chttrans.config", "rt", &file);
    FcitxLog(INFO, _("Load Config File %s"), file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
        {
            SaveChttransConfig();
            LoadChttransConfig();
        }
        return;
    }
    
    ConfigFileDesc* configDesc = GetChttransConfigDesc();
    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);
    
    FcitxChttransConfigBind(&transState, cfile, configDesc);
    ConfigBindSync((GenericConfig*)&transState);

    fclose(fp);
}

ConfigFileDesc* GetChttransConfigDesc()
{
    static ConfigFileDesc* chttransConfigDesc = NULL;
    if (!chttransConfigDesc)
    {
        FILE *tmpfp;
        tmpfp = GetXDGFileData("addon/fcitx-chttrans.desc", "r", NULL);
        chttransConfigDesc = ParseConfigFileDescFp(tmpfp);
        fclose(tmpfp);
    }

    return chttransConfigDesc;
}

void SaveChttransConfig()
{
    ConfigFileDesc* configDesc = GetChttransConfigDesc();
    char *file;
    FILE *fp = GetXDGFileUser("addon/fcitx-chttrans.config", "wt", &file);
    FcitxLog(INFO, "Save Config to %s", file);
    SaveConfigFileFp(fp, transState.gconfig.configFile, configDesc);
    free(file);
    fclose(fp);
}