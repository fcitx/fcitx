
include_directories(
    ${DBUS_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
)

link_directories(${DBUS_LIBRARY_DIRS})

set(fcitx_xkbdbus_SOURCES
    xkbdbus.c
    ../../im/keyboard/isocodes.c
)

fcitx_add_addon(fcitx-xkbdbus ${fcitx_xkbdbus_SOURCES})
target_link_libraries(fcitx-xkbdbus ${DBUS_LIBRARIES} ${LIBXML2_LIBRARIES})

fcitx_add_addon_conf_file(fcitx-xkbdbus.conf)
