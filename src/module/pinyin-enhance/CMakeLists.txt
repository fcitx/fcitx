set(FCITX_PYENHANCE_SOURCES
  pinyin-enhance.c
  pinyin-enhance-spell.c
  pinyin-enhance-cfp.c
  pinyin-enhance-map.c
  pinyin-enhance-stroke.c
  pinyin-enhance-py.c
  pinyin-enhance-sym.c)

fcitx_add_addon_full(pinyin-enhance DESC SCAN SCAN_PRIV
  HEADERS pinyin-enhance.h
  SOURCES ${FCITX_PYENHANCE_SOURCES})
