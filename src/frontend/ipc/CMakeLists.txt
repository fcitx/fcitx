include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

set(fcitx_ipc_SOURCES
    ipc.c
)

fcitx_add_addon(fcitx-ipc ${fcitx_ipc_SOURCES})
target_link_libraries( fcitx-ipc ${DBUS_LIBRARIES} )
fcitx_add_addon_header(ipc ipc.h)
fcitx_add_addon_conf_file(fcitx-ipc.conf)

