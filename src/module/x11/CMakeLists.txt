set(FCITX_X11_LINK_LIBS)
if(ENABLE_X11)
  include_directories(${X11_X11_INCLUDE_PATH})
  set(FCITX_X11_LINK_LIBS ${FCITX_X11_LINK_LIBS}
    ${X11_LIBRARIES} ${X11_Xrender_LIB})
  if(HAVE_XINERAMA)
    set(FCITX_X11_LINK_LIBS ${FCITX_X11_LINK_LIBS} ${X11_Xinerama_LIB})
  endif()
  if(HAVE_XFIXES)
    set(FCITX_X11_LINK_LIBS ${FCITX_X11_LINK_LIBS} ${X11_Xfixes_LIB})
  endif(HAVE_XFIXES)
  set(x11_noinstall)
else()
  set(x11_noinstall NO_INSTALL)
endif()

set(FCITX_X11_SOURCES
  xerrorhandler.c
  x11stuff.c
  x11selection.c)
fcitx_add_addon_full(x11 SCAN_IN SCAN_PRIV ${x11_noinstall}
  SOURCES ${FCITX_X11_SOURCES}
  HEADERS x11stuff.h
  LINK_LIBS ${FCITX_X11_LINK_LIBS}
  EXTRA_PO xerrorhandler.h)
