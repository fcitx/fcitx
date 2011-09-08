include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

link_directories(${DBUS_LIBRARY_DIRS})

set(FCITX_DBUS_SOURCES
	dbusstuff.c
)

fcitx_add_addon(fcitx-dbus ${FCITX_DBUS_SOURCES})
target_link_libraries( fcitx-dbus ${DBUS_LIBRARIES})
fcitx_add_addon_header(dbus dbusstuff.h)
fcitx_add_addon_conf_file(fcitx-dbus.conf)