include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_FULLWIDTH_CHAR_SOURCES
    fullwidthchar.c
)

fcitx_add_addon(fcitx-fullwidth-char ${FCITX_FULLWIDTH_CHAR_SOURCES})
fcitx_add_addon_conf_file(fcitx-fullwidth-char.conf)