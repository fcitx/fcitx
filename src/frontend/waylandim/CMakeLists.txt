set(FCITX_WAYLANDIM_LINK_LIBS)
if(ENABLE_WAYLAND)
  include_directories(${WAYLAND_CLIENT_INCLUDE_DIRS} ${XKBCOMMON_INCLUDE_DIRS})
  link_directories(${WAYLAND_CLIENT_LIBRARY_DIRS})
  set(FCITX_WAYLANDIM_LINK_LIBS ${FCITX_WAYLANDIM_LINK_LIBS} ${WAYLAND_CLIENT_LIBRARIES} ${XKBCOMMON_LIBRARIES})
  set(waylandim_noinstall)
else()
  set(waylandim_noinstall NO_INSTALL)
endif()

set(FCITX_WAYLANDIM_SOURCES
  waylandim.c
  protocol.c
  )

fcitx_add_addon_full(waylandim ${waylandim_noinstall}
  SOURCES ${FCITX_WAYLANDIM_SOURCES}
  LINK_LIBS ${FCITX_WAYLANDIM_LINK_LIBS})
