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

#include <errno.h>
#include <libintl.h>

#include "fcitx.h"
#include "fcitx-utils/cutils.h"
#include "profile.h"
#include "fcitx-config/xdg.h"

static ConfigFileDesc* GetProfileDesc();

CONFIG_BINDING_BEGIN(FcitxProfile)
CONFIG_BINDING_REGISTER("Profile", "FullWidth", bUseFullWidthChar)
CONFIG_BINDING_REGISTER("Profile", "UseRemind", bUseRemind)
CONFIG_BINDING_REGISTER("Profile", "IMIndex", iIMIndex)
CONFIG_BINDING_REGISTER("Profile", "WidePunc", bUseWidePunc)
CONFIG_BINDING_END()

/** 
 * @brief 加载配置文件
 */
void LoadProfile(FcitxProfile* profile)
{
    FILE *fp;
    fp = GetXDGFileUser( "profile", "rt", NULL);
    if (!fp) {
        if (errno == ENOENT)
        {
            SaveProfile(profile);
            LoadProfile(profile);
        }
        return;
    }

    ConfigFileDesc* profileDesc = GetProfileDesc();
    ConfigFile *cfile = ParseConfigFileFp(fp, profileDesc);
    
    FcitxProfileConfigBind(profile, cfile, profileDesc);
    ConfigBindSync(&profile->gconfig);

    fclose(fp);
    if (profile->gconfig.configFile)
        SaveProfile(profile);
}

CONFIG_DESC_DEFINE(GetProfileDesc, "profile.desc")

void SaveProfile(FcitxProfile* profile)
{
    ConfigFileDesc* profileDesc = GetProfileDesc();
    FILE* fp = GetXDGFileUser("profile", "wt", NULL);
    SaveConfigFileFp(fp, &profile->gconfig, profileDesc);
    fclose(fp);
}

boolean UseRemind(FcitxProfile* profile)
{
    return profile->bUseRemind;
}