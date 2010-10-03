#include <errno.h>

#include "core/fcitx.h"
#include "tools/tools.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/profile.h"
#include "fcitx-config/xdg.h"
#include "core/ime.h"

extern Display* dpy;
extern int iScreen;

FcitxProfile fcitxProfile;
ConfigFileDesc* fcitxProfileDesc = NULL;
static ConfigFileDesc* GetProfileDesc();

static void FilterCopyIMIndex(ConfigGroup *group, ConfigOption *option, void *data, ConfigSync sync, void* arg);
static void FilterScreenSizeX(ConfigGroup *group, ConfigOption *option, void *data, ConfigSync sync, void* arg);
static void FilterScreenSizeY(ConfigGroup *group, ConfigOption *option, void *data, ConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN(FcitxProfile);
CONFIG_BINDING_REGISTER_WITH_FILTER("Profile", "MainWindowOffsetX",  iMainWindowOffsetX, FilterScreenSizeX);
CONFIG_BINDING_REGISTER_WITH_FILTER("Profile", "MainWindowOffsetY", iMainWindowOffsetY, FilterScreenSizeY);
CONFIG_BINDING_REGISTER_WITH_FILTER("Profile", "InputWindowOffsetX", iInputWindowOffsetX, FilterScreenSizeX);
CONFIG_BINDING_REGISTER_WITH_FILTER("Profile", "InputWindowOffsetY", iInputWindowOffsetY, FilterScreenSizeY);
CONFIG_BINDING_REGISTER("Profile", "Corner", bCorner);
CONFIG_BINDING_REGISTER("Profile", "ChnPunc", bChnPunc);
CONFIG_BINDING_REGISTER("Profile", "TrackCursor", bTrackCursor);
CONFIG_BINDING_REGISTER("Profile", "UseLegend", bUseLegend);
CONFIG_BINDING_REGISTER_WITH_FILTER("Profile", "IMIndex", iIMIndex, FilterCopyIMIndex);
CONFIG_BINDING_REGISTER("Profile", "Locked", bLocked);
CONFIG_BINDING_REGISTER("Profile", "CompactMainWindow", bCompactMainWindow);
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

void FilterCopyIMIndex(ConfigGroup *group, ConfigOption *option, void *data, ConfigSync sync, void* arg)
{
    int* iIMIndex = (int*)data;
    switch(sync)
    {
        case Raw2Value:
            gs.iIMIndex = *iIMIndex;
            break;
        case Value2Raw:
            *iIMIndex = gs.iIMIndex;
            break;
    }
}
static void FilterScreenSizeX(ConfigGroup *group, ConfigOption *option, void *data, ConfigSync sync, void* arg)
{
    int* X = (int*)data;

    switch(sync)
    {
        case Raw2Value:
            if (*X >= DisplayWidth(dpy, iScreen))
                *X = DisplayWidth(dpy, iScreen) - 10;
            if (*X < 0)
                *X = 0;
            break;
        case Value2Raw:
            break;
    }

}

static void FilterScreenSizeY(ConfigGroup *group, ConfigOption *option, void *data, ConfigSync sync, void* arg)
{
    int* Y = (int*)data;

    switch(sync)
    {
        case Raw2Value:
            if (*Y >= DisplayHeight(dpy, iScreen))
                *Y = DisplayHeight(dpy, iScreen) - 3;
            if (*Y < 0)
                *Y = 0;
            break;
        case Value2Raw:
            break;
    }


}
