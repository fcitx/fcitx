include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_AUTOENG_SOURCES
    AutoEng.c
)

fcitx_add_addon(fcitx-autoeng ${FCITX_AUTOENG_SOURCES})
fcitx_add_addon_conf_file(fcitx-autoeng.conf)
