include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

link_directories(${DBUS_LIBRARY_DIRS})

set(FCITX_FREEDESKTOP_NOTIFY_SOURCES
    notify.c
)

fcitx_add_addon(fcitx-freedesktop-notify ${FCITX_FREEDESKTOP_NOTIFY_SOURCES})
target_link_libraries( fcitx-freedesktop-notify ${DBUS_LIBRARIES})
#fcitx_add_addon_header(freedesktop-notify freedesktop-notify.h)
fcitx_add_addon_conf_file(fcitx-freedesktop-notify.conf)


