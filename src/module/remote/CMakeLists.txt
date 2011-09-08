include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_REMOTE_MODULE_SOURCES
    remote.c
)

fcitx_add_addon(fcitx-remote-module ${FCITX_REMOTE_MODULE_SOURCES})

fcitx_add_addon_conf_file(fcitx-remote-module.conf)