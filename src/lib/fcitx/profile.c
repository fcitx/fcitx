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
#include "fcitx-utils/log.h"
#include "profile.h"
#include "fcitx-config/xdg.h"

static ConfigFileDesc* GetProfileDesc();

CONFIG_BINDING_BEGIN(FcitxProfile)
CONFIG_BINDING_REGISTER("Profile", "FullWidth", bUseFullWidthChar)
CONFIG_BINDING_REGISTER("Profile", "UseRemind", bUseRemind)
CONFIG_BINDING_REGISTER("Profile", "IMIndex", iIMIndex)
CONFIG_BINDING_REGISTER("Profile", "WidePunc", bUseWidePunc)
CONFIG_BINDING_REGISTER("Profile", "PreeditStringInClientWindow", bUsePreedit)
CONFIG_BINDING_END()

/**
 * @brief 加载配置文件
 */
FCITX_EXPORT_API
boolean LoadProfile(FcitxProfile* profile)
{
    ConfigFileDesc* profileDesc = GetProfileDesc();
    if (!profileDesc)
        return false;

    FILE *fp;
    fp = GetXDGFileUserWithPrefix("", "profile", "rt", NULL);
    if (!fp) {
        if (errno == ENOENT)
            SaveProfile(profile);
    }

    ConfigFile *cfile = ParseConfigFileFp(fp, profileDesc);

    FcitxProfileConfigBind(profile, cfile, profileDesc);
    ConfigBindSync(&profile->gconfig);

    if (fp)
        fclose(fp);

    return true;
}

CONFIG_DESC_DEFINE(GetProfileDesc, "profile.desc")

FCITX_EXPORT_API
void SaveProfile(FcitxProfile* profile)
{
    ConfigFileDesc* profileDesc = GetProfileDesc();
    FILE* fp = GetXDGFileUserWithPrefix("", "profile", "wt", NULL);
    SaveConfigFileFp(fp, &profile->gconfig, profileDesc);
    if (fp)
        fclose(fp);
}

FCITX_EXPORT_API
boolean UseRemind(FcitxProfile* profile)
{
    return profile->bUseRemind;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
