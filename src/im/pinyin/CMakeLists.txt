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
  
if(ENABLE_TABLE)
  set(pinyin_noinstall)
else()
  set(pinyin_noinstall NO_INSTALL)
endif()

fcitx_add_addon_full(pinyin SCAN SCAN_PRIV DESC ${pinyin_noinstall}
  SOURCES ${FCITX_PINYIN_SOURCES}
  HEADERS pydef.h
  IM_CONFIG pinyin.conf shuangpin.conf
  EXTRA_PO ${FCITX_PINYIN_HEADERS})

if (ENABLE_PINYIN)
add_subdirectory(data)
endif ()