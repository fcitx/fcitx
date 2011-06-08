#include "fcitx-config/fcitx-config.h"
#include "classicui.h"

extern FcitxClassicUI classicui;

static void FilterCopyUseTray(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void *value, ConfigSync sync, void *filterArg);

CONFIG_BINDING_BEGIN(FcitxClassicUI);
CONFIG_BINDING_REGISTER("ClassicUI", "Font", font);
CONFIG_BINDING_REGISTER("ClassicUI", "MenuFont", menuFont);
#ifndef _ENABLE_PANGO
CONFIG_BINDING_REGISTER("ClassicUI", "FontLocale", strUserLocale);
#endif
CONFIG_BINDING_REGISTER_WITH_FILTER("ClassicUI", "UseTray", bUseTrayIcon_, FilterCopyUseTray);
CONFIG_BINDING_REGISTER("ClassicUI", "ShowHintWindow", bHintWindow);
CONFIG_BINDING_REGISTER("ClassicUI", "SkinType", skinType);
CONFIG_BINDING_REGISTER("ClassicUI", "MainWindowHideMode", hideMainWindow);
CONFIG_BINDING_REGISTER("ClassicUI", "VerticalList", bVerticalList);
CONFIG_BINDING_END()

void FilterCopyUseTray(GenericConfig* config, ConfigGroup *group, ConfigOption *option, void *value, ConfigSync sync, void *filterArg) {
    static boolean firstRunOnUseTray = true;
    FcitxClassicUI *classicui = (FcitxClassicUI*) config;
    boolean *b = (boolean*)value;
    if (sync == Raw2Value && b)
    {
        if (firstRunOnUseTray)
            classicui->bUseTrayIcon = *b;
        firstRunOnUseTray = false;
    }
}