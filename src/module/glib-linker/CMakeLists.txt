
PKG_CHECK_MODULES(GOBJECT2 "gobject-2.0" REQUIRED)

include_directories(${GOBJECT2_INCLUDE_DIRS})
link_directories(${GOBJECT2_LIBRARY_DIRS})

set(FCITX_GLIB_LINKER_SOURCES
  glib-linker.c)

fcitx_add_addon_full(glib-linker
  SOURCES ${FCITX_GLIB_LINKER_SOURCES}
  LINK_LIBS ${GOBJECT2_LIBRARIES})

