set(FCITX_IPC_LINK_LIBS)
if(_ENABLE_DBUS)
  include_directories(${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
  set(FCITX_IPC_LINK_LIBS ${FCITX_IPC_LINK_LIBS} ${DBUS_LIBRARIES})
  set(ipc_noinstall)
else()
  set(ipc_noinstall NO_INSTALL)
endif()

set(FCITX_IPC_SOURCES
  ipc.c
  ../../module/dbusstuff/property.c
  )

fcitx_add_addon_full(ipc ${ipc_noinstall}
  SOURCES ${FCITX_IPC_SOURCES}
  HEADERS ipc.h
  LINK_LIBS ${FCITX_IPC_LINK_LIBS})
