include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

link_directories(${DBUS_LIBRARY_DIRS})

set(FCITX_DBUSIM_SOURCES
    inputbus.c
)

fcitx_add_addon(fcitx-inputbus ${FCITX_DBUSIM_SOURCES})
target_link_libraries( fcitx-inputbus ${DBUS_LIBRARIES})
fcitx_add_addon_header(inputbus inputbus.h)
fcitx_add_addon_conf_file(fcitx-inputbus.conf)