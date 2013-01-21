set(FCITX_PUNC_SOURCES
  punc.c)
fcitx_add_addon_full(punc SCAN SCAN_PRIV
  SOURCES ${FCITX_PUNC_SOURCES}
  HEADERS punc.h)
