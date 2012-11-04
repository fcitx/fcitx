set(FCITX_CHTTRANS_SOURCES
  chttrans.c)
if(ENABLE_OPENCC)
  set(FCITX_CHTTRANS_SOURCES ${FCITX_CHTTRANS_SOURCES} chttrans-opencc.c)
endif(ENABLE_OPENCC)

fcitx_add_addon_full(chttrans SCAN DESC
  SOURCES ${FCITX_CHTTRANS_SOURCES}
  HEADERS chttrans.h
  EXTRA_PO chttrans-opencc.c)
