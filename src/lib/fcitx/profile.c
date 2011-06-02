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

#include "profile.h"
#include "fcitx-config/xdg.h"

static ConfigFileDesc* GetProfileDesc();

CONFIG_BINDING_BEGIN(FcitxProfile);
CONFIG_BINDING_REGISTER("Profile", "TrackCursor", bTrackCursor);
CONFIG_BINDING_REGISTER("Profile", "UseLegend", bUseLegend);
CONFIG_BINDING_REGISTER("Profile", "IMIndex", iIMIndex);
CONFIG_BINDING_REGISTER("Profile", "UseGBKT", bUseGBKT);
#ifdef _ENABLE_RECORDING
CONFIG_BINDING_REGISTER("Profile", "Recording", bRecording);
#endif
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

boolean UseLegend(FcitxProfile* profile)
{
    return profile->bUseLegend;
}

boolean IsTrackCursor(FcitxProfile* profile)
{
    return profile->bTrackCursor;
}

int GetInputWindowOffsetX(FcitxProfile* profile)
{
    return profile->iInputWindowOffsetX;
}

int GetInputWindowOffsetY(FcitxProfile* profile)
{
    return profile->iInputWindowOffsetY;
}

void SetInputWindowOffsetX(FcitxProfile* profile, int pos)
{
    profile->iInputWindowOffsetX = pos;
}

void SetInputWindowOffsetY(FcitxProfile* profile, int pos)
{
    profile->iInputWindowOffsetY = pos;
}