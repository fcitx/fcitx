set(FCITX_AUTOENG_SOURCES
  AutoEng.c)
fcitx_add_addon_full(autoeng DESC
  SOURCES ${FCITX_AUTOENG_SOURCES}
  EXTRA_PO AutoEng.h)
