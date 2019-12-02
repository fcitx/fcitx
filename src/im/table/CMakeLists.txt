fcitx_translate_add_sources(
  data/qxm.conf.in
  data/wbx.conf.in
  data/wanfeng.conf.in
  data/db.conf.in
  data/wbpy.conf.in
  data/erbi.conf.in
  data/zrm.conf.in
  data/cangjie.conf.in
  )

set(FCITX_TABLE_SOURCES
  tabledict.c
  table.c
  tableconfig.c
  )
set(FCITX_TABLE_HEADERS
  tabledict.h
  table.h
  utf8_in_gb18030.h
  )
if(ENABLE_TABLE)
  set(table_noinstall)
  add_subdirectory(data)
else()
  set(table_noinstall NO_INSTALL)
endif()

fcitx_add_addon_full(table DESC ${table_noinstall}
  SOURCES ${FCITX_TABLE_SOURCES}
  EXTRA_DESC table.desc
  EXTRA_PO ${FCITX_TABLE_HEADERS})

