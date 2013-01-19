set(FCITX_WAYLAND_LINK_LIBS)
if(ENABLE_WAYLAND)
  include_directories(${WAYLAND_CLIENT_INCLUDE_DIRS})
  set(FCITX_WAYLAND_LINK_LIBS ${FCITX_WAYLAND_LINK_LIBS}
    ${WAYLAND_CLIENT_LIBRARIES})
  set(wayland_noinstall)
else()
  set(wayland_noinstall NO_INSTALL)
endif()

set(FCITX_WAYLAND_SOURCES
  wayland.c
  wayland-global.c
  epoll-utils.c)
fcitx_add_addon_full(wayland SCAN_IN SCAN_PRIV ${wayland_noinstall}
  SOURCES ${FCITX_WAYLAND_SOURCES}
  HEADERS wayland-defs.h
  LINK_LIBS ${FCITX_WAYLAND_LINK_LIBS}
  EXTRA_PO wayland-internal.h)
