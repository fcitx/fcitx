if (ENABLE_TABLE)
    add_subdirectory(table)
endif (ENABLE_TABLE)

add_subdirectory(script)

set(FCITX_DESC_FILES
    addon.desc
    config.desc
    profile.desc
)

set(FCITX_DATA_FILES
    AutoEng.dat
    gbks2t.tab
    punc.mb
    vk.conf
)

set(FCITX_PY_FILES
    pySym.mb
    sp.dat
)

set(FCITX_DSEKTOP_FILES
    fcitx.desktop
    fcitx-kimpanel.desktop
    fcitx-configtool.desktop
)

set(PY_ORGDATA pyPhrase.org)
set(PY_DATA ${CMAKE_CURRENT_BINARY_DIR}/pybase.mb ${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb)
set(PY_TAR ${CMAKE_CURRENT_SOURCE_DIR}/pinyin.tar.gz)
set(PY_URL http://fcitx.googlecode.com/files/pinyin.tar.gz)


add_subdirectory(png)

intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/fcitx-configtool.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop )
intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/fcitx-kimpanel.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-kimpanel.desktop )
intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/fcitx.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop )

add_custom_target(pinyin_data ALL DEPENDS ${PY_DATA})

add_custom_command(OUTPUT ${PY_ORGDATA}
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/download.sh "${WGET}" "${PY_URL}" "${PY_TAR}"
                   COMMAND ${TAR} xzmvf ${PY_TAR})
add_custom_command(OUTPUT ${PY_DATA}
                   DEPENDS ${PY_ORGDATA}
                   COMMAND createPYMB ${CMAKE_CURRENT_SOURCE_DIR}/gbkpy.org ${CMAKE_CURRENT_BINARY_DIR}/${PY_ORGDATA})

add_custom_target(desktopfile ALL DEPENDS ${FCITX_DSEKTOP_FILES})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop DESTINATION share/applications/)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop DESTINATION share/applications/)
if (_ENABLE_DBUS)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-kimpanel.desktop DESTINATION share/applications/)
endif(_ENABLE_DBUS)

install(FILES ${FCITX_DATA_FILES} DESTINATION ${pkgdatadir}/data)
fcitx_add_configdesc_file(${FCITX_DESC_FILES})
install(FILES ${FCITX_PY_FILES} ${CMAKE_CURRENT_BINARY_DIR}/pybase.mb ${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb DESTINATION ${pkgdatadir}/pinyin)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/pyERROR,${CMAKE_CURRENT_BINARY_DIR}/pyPhrase.ok")
