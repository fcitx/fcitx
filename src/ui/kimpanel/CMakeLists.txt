include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

set(fcitx_kimpanel_ui_SOURCES
    kimpanel.c
)

fcitx_add_addon(fcitx-kimpanel-ui ${fcitx_kimpanel_ui_SOURCES})
target_link_libraries( fcitx-kimpanel-ui ${DBUS_LIBRARIES})
fcitx_add_addon_conf_file(fcitx-kimpanel-ui.conf)