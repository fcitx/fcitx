include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

link_directories(${DBUS_LIBRARY_DIRS})

set(FCITX_DBUS_SOURCES
    dbussocket.c
    dbusstuff.c
    dbuslauncher.c
)

fcitx_scan_addon(dbus)
fcitx_add_addon(fcitx-dbus ${FCITX_DBUS_SOURCES})
target_link_libraries( fcitx-dbus ${DBUS_LIBRARIES})
fcitx_add_addon_header(dbus dbusstuff.h)
fcitx_add_addon_conf_file(fcitx-dbus.conf)

set(FCITX_DBUS_WATCHER_SOURCES
    dbussocket.c
    watcher.c
)

add_executable(fcitx-dbus-watcher ${FCITX_DBUS_WATCHER_SOURCES})
target_link_libraries( fcitx-dbus-watcher ${DBUS_LIBRARIES} fcitx-utils)
install(TARGETS fcitx-dbus-watcher DESTINATION ${bindir})

set(FCITX_DBUS_REMOTE_SOURCES
    dbusremote.c
)

add_executable(fcitx-dbus-remote
               ${FCITX_DBUS_REMOTE_SOURCES})
set_target_properties(fcitx-dbus-remote PROPERTIES OUTPUT_NAME fcitx-remote)
target_link_libraries( fcitx-dbus-remote ${DBUS_LIBRARIES} fcitx-config fcitx-utils)

install(TARGETS fcitx-dbus-remote DESTINATION ${bindir})
