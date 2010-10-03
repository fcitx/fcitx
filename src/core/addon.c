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

#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "core/fcitx.h"
#include "fcitx-config/uthash.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/xdg.h"
#include "tools/utarray.h"
#include "core/addon.h"
#include "tools/tools.h"


CONFIG_BINDING_BEGIN(FcitxAddon);
CONFIG_BINDING_REGISTER("Addon", "Name", name);
CONFIG_BINDING_REGISTER("Addon", "Category", category);
CONFIG_BINDING_REGISTER("Addon", "Enabled", bEnabled);
CONFIG_BINDING_REGISTER("Addon", "Module", module);
CONFIG_BINDING_REGISTER("Addon", "Type", type);
CONFIG_BINDING_END()

UT_icd addon_icd = {sizeof(FcitxAddon), NULL ,NULL, FreeAddon};
UT_array *addons;
static ConfigFileDesc *addonConfigDesc = NULL;
static ConfigFileDesc* GetAddonConfigDesc();

/** 
 * @brief 加载Addon信息
 */
void LoadAddonInfo(void)
{
    if (!fc.bEnableAddons)
        return;
        
    char **addonPath;
    size_t len;
    char pathBuf[PATH_MAX];
    int i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;

	StringHashSet* sset = NULL;

    if (addons)
    {
        utarray_free(addons);
        addons = NULL;
    }

    addons = malloc(sizeof(UT_array));
    utarray_init(addons, &addon_icd);

    addonPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/addon" , DATADIR, "fcitx/data/addon" );

    for(i = 0; i< len; i++)
    {
        snprintf(pathBuf, sizeof(pathBuf), "%s", addonPath[i]);
        pathBuf[sizeof(pathBuf) - 1] = '\0';

        dir = opendir(pathBuf);
        if (dir == NULL)
            continue;

		/* collect all *.conf files */
        while((drt = readdir(dir)) != NULL)
        {
            size_t nameLen = strlen(drt->d_name);
            if (nameLen <= strlen(".conf") )
                continue;
            memset(pathBuf,0,sizeof(pathBuf));

            if (strcmp(drt->d_name + nameLen -strlen(".conf"), ".conf") != 0)
                continue;
            snprintf(pathBuf, sizeof(pathBuf), "%s/%s", addonPath[i], drt->d_name );

            if (stat(pathBuf, &fileStat) == -1)
                continue;

            if (fileStat.st_mode & S_IFREG)
            {
				StringHashSet *string;
				HASH_FIND_STR(sset, drt->d_name, string);
				if (!string)
				{
					char *bStr = strdup(drt->d_name);
					string = malloc(sizeof(StringHashSet));
                    memset(string, 0, sizeof(StringHashSet));
					string->name = bStr;
					HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
				}
            }
        }

        closedir(dir);
    }

    char **paths = malloc(sizeof(char*) *len);
    for (i = 0;i < len ;i ++)
        paths[i] = malloc(sizeof(char) *PATH_MAX);
    StringHashSet* string;
    for (string = sset;
         string != NULL;
         string = (StringHashSet*)string->hh.next)
    {
        int i = 0;
        for (i = len -1; i >= 0; i--)
        {
            snprintf(paths[i], PATH_MAX, "%s/%s", addonPath[len - i - 1], string->name);
            FcitxLog(DEBUG, "Load Addon Config File:%s", paths[i]);
        }
        FcitxLog(INFO, _("Load Addon Config File:%s"), string->name);
        ConfigFile* cfile = ParseMultiConfigFile(paths, len, GetAddonConfigDesc());
        if (cfile)
        {
            FcitxAddon addon;
            memset(&addon, 0, sizeof(FcitxAddon));
            utarray_push_back(addons, &addon);
            FcitxAddon *a = (FcitxAddon*) utarray_back(addons);
            FcitxAddonConfigBind(a, cfile, GetAddonConfigDesc());
            ConfigBindSync((GenericConfig*)a);
            FcitxLog(DEBUG, _("Addon Config %s is %s"),string->name, (a->bEnabled)?"Enabled":"Disabled");
            if (!a->bEnabled)
                utarray_pop_back(addons);
        }
    }

    for (i = 0;i < len ;i ++)
        free(paths[i]);
    free(paths);
		

    FreeXDGPath(addonPath);

	StringHashSet *curStr;
	while(sset)
	{
		curStr = sset;
		HASH_DEL(sset, curStr);
		free(curStr->name);
        free(curStr);
	}
}

/** 
 * @brief 加载Addon的配置文件描述
 * 
 * @return 单例的配置文件描述
 */
ConfigFileDesc* GetAddonConfigDesc()
{
	if (!addonConfigDesc)
	{
		FILE *tmpfp;
		tmpfp = GetXDGFileData("addon.desc", "r", NULL);
		addonConfigDesc = ParseConfigFileDescFp(tmpfp);
		fclose(tmpfp);
	}

	return addonConfigDesc;
}

/** 
 * @brief 释放单个Addon信息的内存
 * 
 * @param addon的指针
 */
void FreeAddon(void *v)
{
    FcitxAddon *addon = (FcitxAddon*) v;
    if (!addon)
        return ;
    FreeConfigFile(addon->config.configFile);
    free(addon->name);
    free(addon->module);
}
