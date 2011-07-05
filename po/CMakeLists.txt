file(GLOB PO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.po) 
file(RELATIVE_PATH REL_SOURCE_ROOT ${fcitx_BINARY_DIR} ${fcitx_SOURCE_DIR})

set(POT_FILE ${CMAKE_CURRENT_BINARY_DIR}/fcitx.pot)

configure_file(POTFILES.in.in POTFILES.in)

add_custom_command(
    OUTPUT ${POT_FILE}
    COMMAND INTLTOOL_EXTRACT=${INTLTOOL_EXTRACT} srcdir=${CMAKE_CURRENT_BINARY_DIR} ${INTLTOOL_UPDATE} --gettext-package fcitx --pot
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

# Update .po files and compile them to binary .gmo files
gettext_create_translations(${POT_FILE} ALL ${PO_FILES})
