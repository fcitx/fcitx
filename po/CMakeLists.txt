set(_FCITX_GETDESCPO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/getdescpo)

file(GLOB PO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.po)
file(RELATIVE_PATH REL_SOURCE_ROOT ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
if ("${REL_SOURCE_ROOT}" STREQUAL "")
    set(REL_SOURCE_ROOT ".")
endif("${REL_SOURCE_ROOT}" STREQUAL "")

set(POT_FILE fcitx.pot)

configure_file(POTFILES.in.in ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in)

extract_fcitx_addon_conf_postring()
extract_fcitx_desc_file_postring()

add_custom_target(
    pot
    COMMAND INTLTOOL_EXTRACT=${INTLTOOL_EXTRACT} srcdir=${CMAKE_CURRENT_BINARY_DIR} ${INTLTOOL_UPDATE} --gettext-package fcitx --pot
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/conf.po ${CMAKE_CURRENT_BINARY_DIR}/desc.po
    )

# Update .po files and compile them to binary .gmo files
gettext_create_translations(${POT_FILE} ALL ${PO_FILES})

install(FILES getdescpo DESTINATION share/cmake/fcitx PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
