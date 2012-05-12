include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_imselector_SOURCES
    imselector.c
)

fcitx_add_addon(fcitx-imselector ${FCITX_imselector_SOURCES})
fcitx_add_addon_conf_file(fcitx-imselector.conf)
fcitx_add_configdesc_file(fcitx-imselector.desc)
