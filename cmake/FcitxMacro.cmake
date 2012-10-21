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

MACRO(EXTRACT_FCITX_DESC_FILE_POSTRING)
    IF(NOT _FCITX_GETDESCPO_PATH)
        FIND_FILE(_FCITX_GETDESCPO_PATH NAMES getdescpo
            HINTS ${FCITX4_PREFIX}/share/cmake/fcitx
            )
    ENDIF(NOT _FCITX_GETDESCPO_PATH)

    IF(NOT _FCITX_GETDESCPO_PATH)
        MESSAGE(FATAL_ERROR "getdescpo not found")
    ENDIF(NOT _FCITX_GETDESCPO_PATH)

    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/desc.po
                       COMMAND ${_FCITX_GETDESCPO_PATH} ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
ENDMACRO(EXTRACT_FCITX_DESC_FILE_POSTRING)

function(__FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME _name _unique_name)
   set(propertyName "_FCITX_UNIQUE_COUNTER_${_name}")
   get_property(currentCounter GLOBAL PROPERTY "${propertyName}")
   if(NOT currentCounter)
      set(currentCounter 1)
   endif()
   set(${_unique_name} "${_name}_${currentCounter}" PARENT_SCOPE)
   math(EXPR currentCounter "${currentCounter} + 1")
   set_property(GLOBAL PROPERTY ${propertyName} ${currentCounter} )
endfunction()

MACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)
    intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
    __FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME(fcitx_addon_conf targetname)
    add_custom_target(${targetname} ALL DEPENDS ${conffilename})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${conffilename} DESTINATION ${FCITX4_ADDON_CONFIG_INSTALL_DIR})
ENDMACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)

MACRO(FCITX_ADD_INPUTMETHOD_CONF_FILE conffilename)
    intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
    __FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME(fcitx_inputmethod_conf targetname)
    add_custom_target(${targetname} ALL DEPENDS ${conffilename})
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
        if ( ${EXTRACTFILE} MATCHES ".conf.in")
            get_filename_component(_EXTRAFILENAME ${EXTRACTFILE} NAME)
            add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h
                            COMMAND ${INTLTOOL_EXTRACT} --srcdir=${PROJECT_BINARY_DIR} --local --type=gettext/ini ${EXTRACTFILE}
                            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                            )
            set(TEMPHEADER ${TEMPHEADER} "${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h")
        endif ( ${EXTRACTFILE} MATCHES ".conf.in")
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

function(FCITX_ADD_ADDON_HEADER subdir)
  set(_SRCS ${ARGN})
  # for fcitx_scanner_addon
  foreach(src ${_SRCS})
    get_filename_component(fullsrc "${src}" ABSOLUTE)
    execute_process(COMMAND ln -sft "${CMAKE_CURRENT_BINARY_DIR}" "${fullsrc}")
  endforeach()
  install(FILES ${_SRCS}
    DESTINATION "${includedir}/${FCITX4_PACKAGE_NAME}/module/${subdir}")
endfunction()

function(fcitx_scan_addon subdir)
  fcitx_scan_addon_with_name("${subdir}" "fcitx-${subdir}")
endfunction()

function(fcitx_scan_addon_with_name subdir name)
  get_property(FCITX_INTERNAL_BUILD GLOBAL PROPERTY "__FCITX_INTERNAL_BUILD")
  # too lazy to set variables instead of simply copy the command twice here...
  if(FCITX_INTERNAL_BUILD)
    add_custom_command(
      COMMAND "${PROJECT_BINARY_DIR}/tools/dev/fcitx-scanner" "${name}.fxaddon"
      "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
      DEPENDS "${name}.fxaddon" fcitx-scanner
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    add_custom_command(
      COMMAND fcitx-scanner "${name}.fxaddon"
      "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
      DEPENDS "${name}.fxaddon"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${name}.h"
    DESTINATION "${includedir}/${FCITX4_PACKAGE_NAME}/module/${subdir}")
  add_custom_target(${name}.target ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.h")
endfunction()
