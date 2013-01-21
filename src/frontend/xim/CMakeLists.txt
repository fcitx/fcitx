set(FCITX_XIM_LINK_LIBS)
if(ENABLE_X11)
  include_directories(lib ${X11_X11_INCLUDE_PATH})
  set(FCITX_XIM_LINK_LIBS ${FCITX_XIM_LINK_LIBS} ${X11_LIBRARIES})
  set(xim_noinstall)
else()
  set(xim_noinstall NO_INSTALL)
endif()

set(FCITX_XIM_SOURCES
  IC.c
  xim.c
  ximhandler.c
  ximconfig.c
  ximqueue.c
  )

set(Ximd_SOURCES
  lib/FrameMgr.c
  lib/i18nAttr.c
  lib/i18nClbk.c
  lib/i18nIc.c
  lib/i18nIMProto.c
  lib/i18nMethod.c
  lib/i18nPtHdr.c
  lib/i18nUtil.c
  lib/i18nX.c
  lib/IMConn.c
  lib/IMMethod.c
  lib/IMValues.c
  )

set(Ximd_HEADERS
  lib/IMdkit.h
  lib/Xi18nX.h
  lib/Xi18n.h
  lib/FrameMgr.h
  lib/Xi18nTr.h
  lib/XimFunc.h
  lib/XimProto.h
  )

set(FCITX_XIM_HEADERS
  IC.h
  xim.h
  ximhandler.h
  ximqueue.h
  )

fcitx_add_addon_full(xim DESC SCAN SCAN_PRIV SCAN_NO_INSTALL ${xim_noinstall}
  SOURCES ${FCITX_XIM_SOURCES} ${Ximd_SOURCES}
  LINK_LIBS ${FCITX_XIM_LINK_LIBS}
  EXTRA_PO ${FCITX_XIM_HEADERS} ${Ximd_HEADERS})
