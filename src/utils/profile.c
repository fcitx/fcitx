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

FcitxProfile fcitxProfile;
ConfigFileDesc* fcitxProfileDesc = NULL;
static ConfigFileDesc* GetProfileDesc();

CONFIG_BINDING_BEGIN(FcitxProfile);
CONFIG_BINDING_REGISTER("Profile", "Corner", bCorner);
CONFIG_BINDING_REGISTER("Profile", "ChnPunc", bChnPunc);
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
void LoadProfile(void)
{
    FILE *fp;
    fp = GetXDGFileUser( "profile", "rt", NULL);
    if (!fp) {
        if (errno == ENOENT)
        {
            SaveProfile();
            LoadProfile();
        }
        return;
    }

    ConfigFileDesc* profileDesc = GetProfileDesc();
    ConfigFile *cfile = ParseConfigFileFp(fp, profileDesc);
    
    FcitxProfileConfigBind(&fcitxProfile, cfile, profileDesc);
    ConfigBindSync((GenericConfig*)&fcitxProfile);

    fclose(fp);
    if (fcitxProfile.gconfig.configFile)
        SaveProfile();
}

ConfigFileDesc* GetProfileDesc()
{
    if (!fcitxProfileDesc)
    {
        FILE *tmpfp;
        tmpfp = GetXDGFileData("profile.desc", "r", NULL);
        fcitxProfileDesc = ParseConfigFileDescFp(tmpfp);
		fclose(tmpfp);
    }

    return fcitxProfileDesc;
}

void SaveProfile(void)
{
    ConfigFileDesc* profileDesc = GetProfileDesc();
    FILE* fp = GetXDGFileUser("profile", "wt", NULL);
    SaveConfigFileFp(fp, fcitxProfile.gconfig.configFile, profileDesc);
    fclose(fp);
}

boolean IsTrackCursor()
{
    return fcitxProfile.bTrackCursor;
}

int GetInputWindowOffsetX()
{
    return fcitxProfile.iInputWindowOffsetX;
}

int GetInputWindowOffsetY()
{
    return fcitxProfile.iInputWindowOffsetY;
}

void SetInputWindowOffsetX(int pos)
{
    fcitxProfile.iInputWindowOffsetX = pos;
}

void SetInputWindowOffsetY(int pos)
{
    fcitxProfile.iInputWindowOffsetY = pos;
}