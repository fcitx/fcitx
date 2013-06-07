if(_ENABLE_DBUS)
  include_directories(
    ${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
else()
  set(freedesktop_notify_noinstall NO_INSTALL)
endif()

set(FCITX_FREEDESKTOP_NOTIFY_SOURCES
  freedesktop-notify.c
  )

fcitx_add_addon_full(freedesktop-notify ${freedesktop_notify_noinstall}
    SCAN SCAN_PRIV
    HEADERS freedesktop-notify.h
    SOURCES ${FCITX_FREEDESKTOP_NOTIFY_SOURCES}
    LINK_LIBS ${DBUS_LIBRARIES})
