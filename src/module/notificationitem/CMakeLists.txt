if(_ENABLE_DBUS)
  include_directories(
    ${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
else()
  set(notificationitem_noinstall NO_INSTALL)
endif()

set(FCITX_NOTIFICATIONITEM_SOURCES
    notificationitem.c
    dbusmenu.c
    ../dbusstuff/property.c)
fcitx_add_addon_full(notificationitem ${notificationitem_noinstall}
  SCAN SCAN_PRIV
  HEADERS notificationitem.h
  SOURCES ${FCITX_NOTIFICATIONITEM_SOURCES}
  LINK_LIBS ${DBUS_LIBRARIES})

