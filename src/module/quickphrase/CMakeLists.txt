set(FCITX_QUICKPHRASE_SOURCES
  quickphrase.c)

fcitx_add_addon_full(quickphrase SCAN DESC
  SOURCES ${FCITX_QUICKPHRASE_SOURCES}
  HEADERS quickphrase.h)
