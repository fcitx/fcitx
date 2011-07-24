include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_QUICKPHRASE_SOURCES
    QuickPhrase.c
)

fcitx_add_addon(fcitx-quickphrase ${FCITX_QUICKPHRASE_SOURCES})
target_link_libraries( fcitx-quickphrase fcitx-config fcitx-core)
fcitx_add_addon_conf_file(fcitx-quickphrase.conf)