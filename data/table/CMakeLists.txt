set(TXT2MB_BIN "${PROJECT_BINARY_DIR}/tools/cli/txt2mb")

set(TABLE_DATA
  "${CMAKE_CURRENT_BINARY_DIR}/db.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/erbi.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/qxm.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/wanfeng.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/wbpy.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/wbx.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/zrm.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/cj.mb"
  )
set(TABLEORG_DATA db.txt erbi.txt qxm.txt wanfeng.txt
  wbpy.txt wbx.txt zrm.txt cj.txt)
set(TABLE_NAME db erbi qxm wanfeng wbpy wbx zrm cj)
set(TABLE_CONF db.conf erbi.conf qxm.conf wanfeng.conf
  wbpy.conf wbx.conf zrm.conf cangjie.conf)
set(TABLE_URL http://fcitx.googlecode.com/files/table.tar.gz)
set(TABLE_TAR ${CMAKE_CURRENT_SOURCE_DIR}/table.tar.gz)

foreach(tableconffile ${TABLE_CONF})
  fcitx_translate_add_apply_source(
    "${CMAKE_CURRENT_SOURCE_DIR}/${tableconffile}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${tableconffile}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${tableconffile}"
    DESTINATION "${pkgdatadir}/table")
endforeach()
add_custom_target(table_data ALL DEPENDS ${TABLE_DATA})

fcitx_download(table-data-download "${TABLE_URL}" "${TABLE_TAR}"
  MD5SUM acb0b112423474ab2c1a22cee590d636)
fcitx_extract(table-data-extract "${TABLE_TAR}" DEPENDS table-data-download
  OUTPUT ${TABLEORG_DATA})

foreach(tblname ${TABLE_NAME})
  add_custom_command(OUTPUT ${tblname}.mb
    DEPENDS ${tblname}.txt "${TXT2MB_BIN}" table-data-extract
    COMMAND "${TXT2MB_BIN}" ${tblname}.txt ${tblname}.mb)
endforeach()

install(FILES ${TABLE_DATA} DESTINATION ${pkgdatadir}/table )
