add_subdirectory(fcitx-config)
add_subdirectory(fcitx-utils)
add_subdirectory(fcitx)
if(_ENABLE_DBUS AND ENABLE_GLIB2)
  PKG_CHECK_MODULES(GIO2 "gio-2.0>=2.26")
endif()
add_subdirectory(fcitx-gclient)
if (ENABLE_QT)
add_subdirectory(fcitx-qt)
endif (ENABLE_QT)
