set(FCITX_DESC_FILES
    addon.desc
    config.desc
    profile.desc
)

set(FCITX_DATA_FILES
    AutoEng.dat
    gbks2t.tab
    punc.mb
    pySym.mb
    vk.conf
    sp.dat
)

set(FCITX_DSEKTOP_FILES
    fcitx.desktop
    fcitx-configtool.desktop
)

set(PY_ORGDATA pyPhrase.org)
set(PY_DATA ${CMAKE_CURRENT_BINARY_DIR}/pybase.mb ${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb)
set(PY_TAR ${CMAKE_CURRENT_SOURCE_DIR}/pinyin.tar.gz)
set(PY_URL http://fcitx.googlecode.com/files/pinyin.tar.gz)


add_subdirectory(png)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/desc.po
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/getdescpo ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(desc ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/desc.po)

add_custom_command(OUTPUT fcitx-configtool.desktop
                   COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_CURRENT_SOURCE_DIR}/../po ${CMAKE_CURRENT_SOURCE_DIR}/fcitx-configtool.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop )

add_custom_command(OUTPUT fcitx.desktop
                   COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${CMAKE_CURRENT_SOURCE_DIR}/../po ${CMAKE_CURRENT_SOURCE_DIR}/fcitx.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop )

add_custom_target(pinyin_data ALL DEPENDS ${PY_DATA})

add_custom_command(OUTPUT ${PY_TAR}
                   COMMAND ${WGET} -O ${PY_TAR} ${PY_URL})
add_custom_command(OUTPUT ${PY_ORGDATA}
                   DEPENDS ${PY_TAR}
                   COMMAND ${TAR} xzmvf ${PY_TAR})
add_custom_command(OUTPUT ${PY_DATA}
                   DEPENDS ${PY_ORGDATA}
                   COMMAND createPYMB ${CMAKE_CURRENT_SOURCE_DIR}/gbkpy.org ${CMAKE_CURRENT_BINARY_DIR}/${PY_ORGDATA})

add_custom_target(desktopfile ALL DEPENDS ${FCITX_DSEKTOP_FILES})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop DESTINATION share/applications/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop DESTINATION share/applications/)

install(FILES ${FCITX_DESC_FILES} ${FCITX_DATA_FILES} DESTINATION ${pkgdatadir}/data)
install(FILES fcitx-config DESTINATION ${bindir})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pybase.mb ${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb DESTINATION ${pkgdatadir}/data)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/pyERROR,${CMAKE_CURRENT_BINARY_DIR}/pyPhrase.ok")