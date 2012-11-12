set(FCITX_KIMPANEL_LINK_LIBS)
if(_ENABLE_DBUS)
  include_directories(${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
  set(FCITX_KIMPANEL_LINK_LIBS ${FCITX_KIMPANEL_LINK_LIBS} ${DBUS_LIBRARIES})
  set(kimpanel_noinstall)
else()
  set(kimpanel_noinstall NO_INSTALL)
endif()

set(FCITX_KIMPANEL_UI_SOURCES
  kimpanel.c
  )

fcitx_add_addon_full(kimpanel-ui ${kimpanel_noinstall}
  SOURCES ${FCITX_KIMPANEL_UI_SOURCES}
  LINK_LIBS ${FCITX_KIMPANEL_LINK_LIBS})
