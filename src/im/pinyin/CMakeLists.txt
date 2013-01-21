set(FCITX_PINYIN_SOURCES
  py.c
  PYFA.c
  pyMapTable.c
  pyParser.c
  sp.c
  pyconfig.c
  )
set(FCITX_PINYIN_HEADERS
  PYFA.h
  pyconfig.h
  py.h
  pysplitdata.h
  pyParser.h
  spdata.h
  pyMapTable.h
  sp.h
  pydef.h
  )

fcitx_add_addon_full(pinyin SCAN SCAN_PRIV DESC
  SOURCES ${FCITX_PINYIN_SOURCES}
  HEADERS pydef.h
  IM_CONFIG pinyin.conf shuangpin.conf
  EXTRA_PO ${FCITX_PINYIN_HEADERS})
