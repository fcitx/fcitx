set(FCITX_XKB_LINK_LIBS)
if(_ENABLE_XKB)
  include_directories (
    ${X11_INCLUDE_DIR}
    ${X11_Xkb_INCLUDE_PATH}
    ${X11_Xkblib_INCLUDE_PATH}
    ${LIBINTL_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR})
  set(FCITX_XKB_LINK_LIBS ${FCITX_XKB_LINK_LIBS}
    ${X11_X11_LIB} ${XKBFILE_LIBRARIES} ${LIBXML2_LIBRARIES})
  set(xkb_noinstall)
else()
  set(xkb_noinstall NO_INSTALL)
endif()

set(FCITX_XKB_SOURCES
  xkb.c
  rules.c
  xkbconfig.c)

fcitx_add_addon_full(xkb SCAN SCAN_PRIV DESC ${xkb_noinstall}
  SOURCES ${FCITX_XKB_SOURCES}
  HEADERS xkb.h
  LINK_LIBS ${FCITX_XKB_LINK_LIBS}
  EXTRA_PO rules.h)
