set(FCITX_FREEDESKTOP_NOTIFY_SOURCES
    freedesktop-notify.c
)

fcitx_translate_add_sources(${FCITX_FREEDESKTOP_NOTIFY_SOURCES})

if(NOT _ENABLE_DBUS)
    return()
endif()

include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

fcitx_add_addon_full(freedesktop-notify
    SCAN
    SOURCES ${FCITX_FREEDESKTOP_NOTIFY_SOURCES}
    LINK_LIBS ${DBUS_LIBRARIES})

