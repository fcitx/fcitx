set(TABLE_DATA
 ${CMAKE_CURRENT_BINARY_DIR}/db.mb
 ${CMAKE_CURRENT_BINARY_DIR}/erbi.mb
 ${CMAKE_CURRENT_BINARY_DIR}/qxm.mb
 ${CMAKE_CURRENT_BINARY_DIR}/wanfeng.mb
 ${CMAKE_CURRENT_BINARY_DIR}/wbpy.mb
 ${CMAKE_CURRENT_BINARY_DIR}/wbx.mb
 ${CMAKE_CURRENT_BINARY_DIR}/zrm.mb
 ${CMAKE_CURRENT_BINARY_DIR}/cj.mb
)
set(TABLEORG_DATA db.txt erbi.txt qxm.txt wanfeng.txt wbpy.txt wbx.txt zrm.txt cj.txt)
set(TABLE_NAME db erbi qxm wanfeng wbpy wbx zrm cj)
set(TABLE_CONF db.conf erbi.conf qxm.conf wanfeng.conf wbpy.conf wbx.conf zrm.conf cangjie.conf)
set(TABLE_URL http://fcitx.googlecode.com/files/table.tar.gz)
set(TABLE_TAR ${CMAKE_CURRENT_SOURCE_DIR}/table.tar.gz)

set(INSTALL_TABLE_CONF "")
foreach(tableconffile ${TABLE_CONF})
    intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/${tableconffile}.in ${CMAKE_CURRENT_BINARY_DIR}/${tableconffile} )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${tableconffile} DESTINATION ${pkgdatadir}/table )
    set(INSTALL_TABLE_CONF ${INSTALL_TABLE_CONF} ${CMAKE_CURRENT_BINARY_DIR}/${tableconffile})
endforeach(tableconffile ${TABLE_CONF})
add_custom_target(table_conf ALL DEPENDS ${INSTALL_TABLE_CONF} )
add_custom_target(table_data ALL DEPENDS ${TABLE_DATA})

add_custom_command(OUTPUT ${TABLEORG_DATA}
                   COMMAND ${PROJECT_SOURCE_DIR}/data/download.sh "${WGET}" "${TABLE_URL}" "${TABLE_TAR}"
                   COMMAND ${TAR} xzmvf ${TABLE_TAR})

foreach(tblname ${TABLE_NAME})
add_custom_command(OUTPUT ${tblname}.mb
                   DEPENDS ${tblname}.txt
                   COMMAND txt2mb ${tblname}.txt ${tblname}.mb)
endforeach()

install(FILES ${TABLE_DATA} DESTINATION ${pkgdatadir}/table )