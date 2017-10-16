set(FCITX_IPCPORTAL_LINK_LIBS)
if(_ENABLE_DBUS)
  include_directories(${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
  pkg_check_modules(LibUUID "uuid" IMPORTED_TARGET REQUIRED)
  set(FCITX_IPCPORTAL_LINK_LIBS ${FCITX_IPCPORTAL_LINK_LIBS} ${DBUS_LIBRARIES} PkgConfig::LibUUID)
  set(ipcportal_noinstall)
else()
  set(ipcportal_noinstall NO_INSTALL)
endif()

set(FCITX_IPCPORTAL_SOURCES
  ipcportal.c
  )

fcitx_add_addon_full(ipcportal ${ipcportal_noinstall}
  SOURCES ${FCITX_IPCPORTAL_SOURCES}
  LINK_LIBS ${FCITX_IPCPORTAL_LINK_LIBS})
