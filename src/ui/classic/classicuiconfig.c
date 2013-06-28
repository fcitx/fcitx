#include "fcitx-config/fcitx-config.h"
#include "classicui.h"

static void FilterCopyUseTray(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void *value, FcitxConfigSync sync, void *filterArg);

CONFIG_BINDING_BEGIN(FcitxClassicUI)
CONFIG_BINDING_REGISTER("ClassicUI", "MainWindowOffsetX", iMainWindowOffsetX)
CONFIG_BINDING_REGISTER("ClassicUI", "MainWindowOffsetY", iMainWindowOffsetY)
CONFIG_BINDING_REGISTER("ClassicUI", "FontSize", fontSize)
CONFIG_BINDING_REGISTER("ClassicUI", "Font", font)
CONFIG_BINDING_REGISTER("ClassicUI", "MenuFont", menuFont)
#ifndef _ENABLE_PANGO
CONFIG_BINDING_REGISTER("ClassicUI", "FontLocale", strUserLocale)
#endif
CONFIG_BINDING_REGISTER_WITH_FILTER("ClassicUI", "UseTray", bUseTrayIcon_, FilterCopyUseTray)
CONFIG_BINDING_REGISTER("ClassicUI", "SkinType", skinType)
CONFIG_BINDING_REGISTER("ClassicUI", "MainWindowHideMode", hideMainWindow)
CONFIG_BINDING_REGISTER("ClassicUI", "VerticalList", bVerticalList)
CONFIG_BINDING_END()

void FilterCopyUseTray(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void *value, FcitxConfigSync sync, void *filterArg)
{
    static boolean firstRunOnUseTray = true;
    FcitxClassicUI *classicui = (FcitxClassicUI*) config;
    boolean *b = (boolean*)value;
    if (sync == Raw2Value && b) {
        if (firstRunOnUseTray)
            classicui->bUseTrayIcon = *b;
        firstRunOnUseTray = false;
    }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
