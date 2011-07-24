include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_AUTOENG_SOURCES
    AutoEng.c
)

fcitx_add_addon(fcitx-autoeng ${FCITX_AUTOENG_SOURCES})
target_link_libraries( fcitx-autoeng fcitx-config fcitx-core)
fcitx_add_addon_conf_file(fcitx-autoeng.conf)
