# This file is included by FindFcitx4.cmake, don't include it directly.
# - Useful macro for fcitx development
#
# Usage:
#   INTLTOOL_MERGE_TRANSLATION([INFILE] [OUTFILE])
#     merge translation to fcitx config and desktop file
#
#   FCITX_ADD_ADDON_CONF_FILE([conffilename])
#     merge addon .conf.in translation and install it to correct path
#     you shouldn't put .in in filename, just put foobar.conf
#
#   FCITX_ADD_CONFIGDESC_FILE([filename]*)
#     install configuration description file to correct path
#
#   EXTRACT_FCITX_ADDON_CONF_POSTRING()
#     extract fcitx addon conf translatable string from POFILES.in from
#     ${CMAKE_CURRENT_BINARY_DIR}, the file need end with ,conf.in
#

#==============================================================================
# Copyright 2011 Xuetian Weng
#
# Distributed under the GPLv2 License
# see accompanying file COPYRIGHT for details
#==============================================================================
# (To distribute this file outside of Fcitx, substitute the full
#  License text for the above reference.)

FIND_PROGRAM(INTLTOOL_EXTRACT intltool-extract)
FIND_PROGRAM(INTLTOOL_UPDATE intltool-update)
FIND_PROGRAM(INTLTOOL_MERGE intltool-merge)

MACRO(INTLTOOL_MERGE_TRANSLATION infile outfile)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${outfile}
        COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${PROJECT_SOURCE_DIR}/po ${infile} ${outfile}
        DEPENDS ${infile}
    )
ENDMACRO(INTLTOOL_MERGE_TRANSLATION)

MACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)
    intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
    add_custom_target(${conffilename}.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${conffilename} DESTINATION ${FCITX4_ADDON_CONFIG_INSTALL_DIR})
ENDMACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)

MACRO(FCITX_ADD_INPUTMETHOD_CONF_FILE conffilename)
    intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
    add_custom_target(${conffilename}.target ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${conffilename} DESTINATION ${FCITX4_INPUTMETHOD_CONFIG_INSTALL_DIR})
ENDMACRO(FCITX_ADD_INPUTMETHOD_CONF_FILE conffilename)

MACRO(FCITX_ADD_CONFIGDESC_FILE)
    set(_args ${ARGN})
    set(_DESCFILES ${_args})
    install(FILES ${_DESCFILES} DESTINATION ${FCITX4_CONFIGDESC_INSTALL_DIR})
ENDMACRO(FCITX_ADD_CONFIGDESC_FILE)

MACRO(EXTRACT_FCITX_ADDON_CONF_POSTRING)
    file(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in POTFILES_IN)
    set(TEMPHEADER "")
    foreach(EXTRACTFILE ${POTFILES_IN})
        if ( ${EXTRACTFILE} MATCHES ".conf.in$")
            get_filename_component(_EXTRAFILENAME ${EXTRACTFILE} NAME)
            add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h
                            COMMAND ${INTLTOOL_EXTRACT} --srcdir=${PROJECT_BINARY_DIR} --local --type=gettext/ini ${EXTRACTFILE}
                            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                            )
            set(TEMPHEADER ${TEMPHEADER} "${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h")
        endif ( ${EXTRACTFILE} MATCHES ".conf.in$")
    endforeach(EXTRACTFILE ${POTFILES_IN})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/conf.po
        COMMAND xgettext --add-comments --output=conf.po --keyword=N_ ${TEMPHEADER}
        DEPENDS ${TEMPHEADER})
ENDMACRO(EXTRACT_FCITX_ADDON_CONF_POSTRING)

MACRO(FCITX_ADD_ADDON target)
    set(_args ${ARGN})
    set(_SRCS ${_args})
    add_library(${target} MODULE ${_SRCS})
    set_target_properties( ${target} PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
    install(TARGETS ${target} DESTINATION ${FCITX4_ADDON_INSTALL_DIR})
ENDMACRO(FCITX_ADD_ADDON)

MACRO(FCITX_ADD_ADDON_HEADER subdir)
    set(_args ${ARGN})
    set(_SRCS ${_args})
    install(FILES ${_SRCS} DESTINATION ${includedir}/${FCITX4_PACKAGE_NAME}/module/${subdir})
ENDMACRO(FCITX_ADD_ADDON_HEADER)