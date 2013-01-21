set(FCITX_DBUS_LINK_LIBS)
if(_ENABLE_DBUS)
  include_directories(${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
  set(FCITX_DBUS_LINK_LIBS ${FCITX_DBUS_LINK_LIBS}
    ${DBUS_LIBRARIES})
  set(dbus_noinstall)
else()
  set(dbus_noinstall NO_INSTALL)
endif()

set(FCITX_DBUS_SOURCES
  dbussocket.c
  dbusstuff.c
  dbuslauncher.c
  )

fcitx_add_addon_full(dbus SCAN SCAN_PRIV ${dbus_noinstall}
  SOURCES ${FCITX_DBUS_SOURCES}
  HEADERS dbusstuff.h
  LINK_LIBS ${FCITX_DBUS_LINK_LIBS})

set(FCITX_DBUS_WATCHER_SOURCES
  dbussocket.c
  watcher.c
  )
set(FCITX_DBUS_REMOTE_SOURCES
  dbusremote.c
  )

fcitx_translate_add_sources(${FCITX_DBUS_REMOTE_SOURCES}
  ${FCITX_DBUS_WATCHER_SOURCES})

if(_ENABLE_DBUS)
  add_executable(fcitx-dbus-watcher ${FCITX_DBUS_WATCHER_SOURCES})
  target_link_libraries(fcitx-dbus-watcher ${DBUS_LIBRARIES} fcitx-utils)
  install(TARGETS fcitx-dbus-watcher DESTINATION "${bindir}")
  add_executable(fcitx-dbus-remote ${FCITX_DBUS_REMOTE_SOURCES})
  set_target_properties(fcitx-dbus-remote PROPERTIES OUTPUT_NAME fcitx-remote)
  target_link_libraries(fcitx-dbus-remote ${DBUS_LIBRARIES}
    fcitx-config fcitx-utils)
  install(TARGETS fcitx-dbus-remote DESTINATION "${bindir}")
endif()
