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

include(CMakeParseArguments)
find_package(Gettext REQUIRED)

set(FCITX_MACRO_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(FCITX_TRANSLATION_SCAN_POT "${FCITX_MACRO_CMAKE_DIR}/fcitx-scan-pot.sh")
set(FCITX_TRANSLATION_MERGE_CONFIG
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-merge-config.sh")
set(FCITX_TRANSLATION_EXTRACT_CPP
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-cpp.sh")
set(FCITX_TRANSLATION_EXTRACT_DESKTOP
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-desktop.sh")
set(FCITX_TRANSLATION_EXTRACT_CONFDESC
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-confdesc.sh")

# Function to create a unique target in certain namespace
# Useful when it is hard to determine a unique legal target name
function(__fcitx_addon_get_unique_name name unique_name)
  set(property_name "_FCITX_ADDON_UNIQUE_COUNTER_${name}")
  get_property(current_counter GLOBAL PROPERTY "${property_name}")
  if(NOT current_counter)
    set(current_counter 1)
  endif()
  set(${unique_name} "fcitx-addon-${name}-${current_counter}" PARENT_SCOPE)
  math(EXPR current_counter "${current_counter} + 1")
  set_property(GLOBAL PROPERTY "${property_name}" "${current_counter}")
endfunction()

# Add global targets
# (run only once even if the file is included many times)
function(__fcitx_cmake_init)
  set(property_name "_FCITX_CMAKE_INITIATED")
  get_property(fcitx_initiated GLOBAL PROPERTY "${property_name}")
  if(fcitx_initiated)
    return()
  endif()
  add_custom_target(fcitx-modules.target ALL)
  add_custom_target(fcitx-scan-addons.target)
  add_dependencies(fcitx-modules.target fcitx-scan-addons.target)
  set(translation_cache_base "${PROJECT_BINARY_DIR}")
  set_property(GLOBAL PROPERTY "__FCITX_TRANSLATION_CACHE_BASE"
    "${translation_cache_base}")
  get_property(full_name GLOBAL
    PROPERTY "__FCITX_TRANSLATION_TARGET_FILE")
  execute_process(COMMAND "${FCITX_TRANSLATION_SCAN_POT}"
    "${translation_cache_base}"
    "${full_name}" --clean)
  set_property(GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET" 0)
  set_property(GLOBAL PROPERTY "${property_name}" 1)
endfunction()
__fcitx_cmake_init()

# options:
#     NO_INSTALL: Do not install any files or compile any sources
#         useful when the module is disabled but the generated header is needed
#     SCAN: Generate api header, the path of the input file is determined by the
#         FXADDON_SRC arguement and the name of the generated header file is
#         determined by the FXADDON_GEN arguement.
#     DESC: Install main config-desc file, the path of the file (with the .desc
#         suffix) is determined by the DESC_SRC arguement.
# single value arguments:
#     HEADER_DIR: The subdirectory under fcitx/module to which the header files
#         of the addon will be installed (default ${short_name})
#     LIB_NAME: The name of the addon binary (default fcitx-${short_name})
#     FXADDON_SRC: the path of the fxaddon file. Setting this argument will
#          automatically set the SCAN option as well.
#          (default: fcitx-${short_name}.fxaddon)
#     FXADDON_GEN: the name of the generated header file. (path will be ignored)
#          Setting this argument will automatically set the SCAN option as well.
#          (default: fcitx-${short_name}.h)
#     CONF_SRC: the path of the addon conf file (without the .in suffix)
#         (default: fcitx-${short_name}.conf)
#     DESC_SRC: the path of the main config-desc file. Setting this argument
#          will automatically set the DESC option as well.
#          (default: fcitx-${short_name}.desc)
#     UNIQUE_NAME: the unique name of the addon.
#          (default: fcitx-${short_name})
# multiple values arguments:
#     SOURCES: source files to compile the addon.
#     HEADERS: addon header files to install (besides the generated one).
#     EXTRA_DESC: extra config-desc files to install.
#     EXTRA_PO: extra files to scan string for translation
#     LINK_LIBS: external libraies to link
#     DEPENDS: extra targets or files the addon library should depend on.
function(fcitx_add_addon_full short_name)
  set(options NO_INSTALL SCAN DESC)
  set(one_value_args HEADER_DIR FXADDON_SRC FXADDON_GEN
    CONF_SRC DESC_SRC UNIQUE_NAME LIB_NAME)
  set(multi_value_args SOURCES HEADERS EXTRA_DESC EXTRA_PO LINK_LIBS
    DEPENDS IM_CONFIG)
  cmake_parse_arguments(FCITX_ADDON
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  set(files_to_translate)
  set(all_conf_descs)
  message(STATUS "Adding Fcitx Addon ${short_name}")
  if(NOT FCITX_ADDON_UNIQUE_NAME)
    set(FCITX_ADDON_UNIQUE_NAME "fcitx-${short_name}")
  endif()
  if(NOT FCITX_ADDON_HEADER_DIR)
    set(FCITX_ADDON_HEADER_DIR "${short_name}")
  endif()
  if(NOT FCITX_ADDON_CONF_SRC)
    set(FCITX_ADDON_CONF_SRC "fcitx-${short_name}.conf")
  endif()
  if(NOT FCITX_ADDON_LIB_NAME)
    set(FCITX_ADDON_LIB_NAME "fcitx-${short_name}")
  endif()
  set(files_to_translate ${files_to_translate} "${FCITX_ADDON_CONF_SRC}.in")
  foreach(im_config ${FCITX_ADDON_IM_CONFIG})
    set(files_to_translate ${files_to_translate} "${im_config}.in")
  endforeach()
  if(FCITX_ADDON_SCAN OR FCITX_ADDON_FXADDON_SRC OR FCITX_ADDON_FXADDON_GEN)
    if(NOT FCITX_ADDON_FXADDON_SRC)
      set(FCITX_ADDON_FXADDON_SRC "fcitx-${short_name}.fxaddon")
    endif()
    if(NOT FCITX_ADDON_FXADDON_GEN)
      set(FCITX_ADDON_FXADDON_GEN
        "${CMAKE_CURRENT_BINARY_DIR}/fcitx-${short_name}.h")
    else()
      get_filename_component(FCITX_ADDON_FXADDON_GEN
        "${FCITX_ADDON_FXADDON_GEN}" NAME)
      set(FCITX_ADDON_FXADDON_GEN
        "${CMAKE_CURRENT_BINARY_DIR}/${FCITX_ADDON_FXADDON_GEN}")
    endif()
  endif()
  if(FCITX_ADDON_DESC OR FCITX_ADDON_DESC_SRC)
    if(NOT FCITX_ADDON_DESC_SRC)
      set(FCITX_ADDON_DESC_SRC "fcitx-${short_name}.desc")
    endif()
    set(files_to_translate ${files_to_translate} "${FCITX_ADDON_DESC_SRC}")
    set(all_conf_descs ${all_conf_descs} "${FCITX_ADDON_DESC_SRC}")
  endif()
  if(NOT FCITX_ADDON_SOURCES)
    MESSAGE(FATAL_ERROR "No source file for addon ${short_name}.")
  endif()
  set(files_to_translate ${files_to_translate}
    ${FCITX_ADDON_SOURCES} ${FCITX_ADDON_HEADERS}
    ${FCITX_ADDON_EXTRA_DESC} ${FCITX_ADDON_EXTRA_PO})
  set(all_conf_descs ${all_conf_descs} ${FCITX_ADDON_EXTRA_DESC})

  __fcitx_link_addon_headers(${FCITX_ADDON_HEADERS})
  if(FCITX_ADDON_FXADDON_GEN)
    __fcitx_scan_addon("${FCITX_ADDON_FXADDON_SRC}"
      "${FCITX_ADDON_FXADDON_GEN}")
    set(FCITX_ADDON_HEADERS ${FCITX_ADDON_HEADERS}
      "${FCITX_ADDON_FXADDON_GEN}")
  endif()
  fcitx_translate_add_sources(${files_to_translate})
  if(FCITX_ADDON_NO_INSTALL)
    return()
  endif()

  __fcitx_addon_get_unique_name(add-addon target_name)
  add_custom_target("${target_name}" ALL)

  __fcitx_addon_config_file("${target_name}" "${FCITX_ADDON_CONF_SRC}"
    "${FCITX4_ADDON_CONFIG_INSTALL_DIR}")
  foreach(im_config ${FCITX_ADDON_IM_CONFIG})
    __fcitx_addon_config_file("${target_name}" "${im_config}"
      "${FCITX4_INPUTMETHOD_CONFIG_INSTALL_DIR}")
  endforeach()
  __fcitx_add_addon_lib("${FCITX_ADDON_LIB_NAME}" ${FCITX_ADDON_SOURCES})
  if(FCITX_ADDON_LINK_LIBS)
    target_link_libraries("${FCITX_ADDON_LIB_NAME}" ${FCITX_ADDON_LINK_LIBS})
  endif()
  __fcitx_install_addon_headers("${FCITX_ADDON_LIB_NAME}"
    "${FCITX_ADDON_HEADER_DIR}"
    ${FCITX_ADDON_HEADERS})
  __fcitx_install_addon_desc("${target_name}" ${all_conf_descs})
  if(FCITX_ADDON_DEPENDS)
    __fcitx_addon_get_unique_name(addon-depends depends_target)
    add_custom_target("${depends_target}" DEPENDS ${FCITX_ADDON_DEPENDS})
    add_dependencies("${FCITX_ADDON_LIB_NAME}" "${depends_target}")
  endif()
endfunction()

function(__fcitx_add_addon_lib lib_name)
  set(sources ${ARGN})
  add_library("${lib_name}" MODULE ${sources})
  add_dependencies(fcitx-modules.target "${lib_name}")
  set_target_properties("${lib_name}" PROPERTIES PREFIX ""
    COMPILE_FLAGS "-fvisibility=hidden")
  add_dependencies("${lib_name}" fcitx-scan-addons.target)
  install(TARGETS "${lib_name}" DESTINATION "${FCITX4_ADDON_INSTALL_DIR}")
endfunction()

function(__fcitx_install_addon_desc target_name)
  set(descs ${ARGN})
  __fcitx_addon_get_unique_name(addon-desc desc_target)
  add_custom_target("${desc_target}" DEPENDS ${descs})
  add_dependencies("${target_name}" "${desc_target}")
  install(FILES ${descs}
    DESTINATION "${FCITX4_CONFIGDESC_INSTALL_DIR}")
endfunction()

function(fcitx_install_addon_desc)
  __fcitx_addon_get_unique_name(install-desc desc_target)
  add_custom_target("${desc_target}" ALL)
  fcitx_translate_add_sources(${ARGN})
  __fcitx_install_addon_desc("${desc_target}" ${ARGN})
endfunction()

function(__fcitx_install_addon_headers target_name subdir)
  set(headers ${ARGN})
  __fcitx_addon_get_unique_name(addon-headers header_target)
  add_custom_target("${header_target}" DEPENDS ${headers})
  add_dependencies("${target_name}" "${header_target}")
  install(FILES ${headers}
    DESTINATION "${includedir}/${FCITX4_PACKAGE_NAME}/module/${subdir}")
endfunction()

function(__fcitx_link_addon_headers)
  set(headers ${ARGN})
  foreach(header ${headers})
    get_filename_component(fullsrc "${header}" ABSOLUTE)
    get_filename_component(basename "${fullsrc}" NAME)
    set(link_target "${CMAKE_CURRENT_BINARY_DIR}/${basename}")
    if(NOT EXISTS "${link_target}")
      execute_process(COMMAND ln -sfT "${fullsrc}" "${link_target}")
    endif()
  endforeach()
endfunction()

function(__fcitx_scan_addon in_file out_file)
  get_property(FCITX_INTERNAL_BUILD GLOBAL PROPERTY "__FCITX_INTERNAL_BUILD")
  # too lazy to set variables instead of simply copy the command twice here...
  if(FCITX_INTERNAL_BUILD)
    add_custom_command(
      COMMAND "${PROJECT_BINARY_DIR}/tools/dev/fcitx-scanner"
      "${in_file}" "${out_file}"
      OUTPUT "${out_file}" DEPENDS "${in_file}" fcitx-scanner
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    add_custom_command(
      COMMAND fcitx-scanner "${in_file}" "${out_file}"
      OUTPUT "${out_file}" DEPENDS "${in_file}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  __fcitx_addon_get_unique_name(scan-addon target_name)
  add_custom_target("${target_name}" DEPENDS "${out_file}")
  add_dependencies(fcitx-scan-addons.target "${target_name}")
endfunction()

function(__fcitx_addon_config_file target conf_file dest_dir)
  set(SRC_FILE "${conf_file}.in")
  get_filename_component(base_name "${conf_file}" NAME)
  set(TGT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${base_name}")
  fcitx_translate_add_apply_source("${SRC_FILE}" "${TGT_FILE}")
  __fcitx_addon_get_unique_name(addon-conf conf_target)
  add_custom_target("${conf_target}" DEPENDS "${TGT_FILE}")
  add_dependencies("${target}" "${conf_target}")
  install(FILES "${TGT_FILE}" DESTINATION "${dest_dir}")
endfunction()

function(__fcitx_translate_add_sources_internal)
  if(NOT ARGN)
    return()
  endif()
  get_property(translation_cache_base GLOBAL
    PROPERTY "__FCITX_TRANSLATION_CACHE_BASE")
  get_property(full_name GLOBAL
    PROPERTY "__FCITX_TRANSLATION_TARGET_FILE")
  get_property(target GLOBAL
    PROPERTY "__FCITX_TRANSLATION_TARGET_NAME")
  execute_process(COMMAND "${FCITX_TRANSLATION_SCAN_POT}"
    "${translation_cache_base}"
    "${full_name}" --add-sources ${ARGN}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endfunction()

# Add files to be translated
function(fcitx_translate_add_sources)
  set(sources ${ARGN})
  set(new_sources)
  foreach(source ${sources})
    get_filename_component(full_name "${source}" ABSOLUTE)
    list(APPEND new_sources "${full_name}")
  endforeach()
  set_property(GLOBAL APPEND PROPERTY "FCITX_TRANSLATION_SOURCES"
    ${new_sources})
  get_property(pot_target_set GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET")
  if(NOT pot_target_set)
    return()
  endif()
  __fcitx_translate_add_sources_internal(${new_sources})
endfunction()

# Should be used in cmake modules, add a handler script to generate a po file
# from a set of certain type of files, the script must be executable
function(_fcitx_translate_add_handler script)
  get_property(translation_cache_base GLOBAL
    PROPERTY "__FCITX_TRANSLATION_CACHE_BASE")
  get_property(full_name GLOBAL
    PROPERTY "__FCITX_TRANSLATION_TARGET_FILE")
  execute_process(COMMAND "${FCITX_TRANSLATION_SCAN_POT}"
    "${translation_cache_base}"
    "${full_name}" --add-handler "${script}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endfunction()
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_CPP}")
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_DESKTOP}")
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_CONFDESC}")

# Add files to apply translation
function(fcitx_translate_add_apply_source in_file out_file)
  get_property(scripts GLOBAL
    PROPERTY FCITX_TRANSLATION_APPLY_HANDLERS)
  get_filename_component(in_file "${in_file}" ABSOLUTE)
  get_filename_component(out_file "${out_file}" ABSOLUTE)
  get_property(translation_cache_base GLOBAL
    PROPERTY "__FCITX_TRANSLATION_CACHE_BASE")
  set(po_cache "${translation_cache_base}/fcitx-translation-po-cache.txt")

  get_property(po_lang_files GLOBAL PROPERTY "FCITX_TRANSLATION_PO_FILES")
  set(all_po_files)
  foreach(po_lang_file ${po_lang_files})
    string(REGEX REPLACE "^[^ ]* " "" po_file "${po_lang_file}")
    string(REGEX REPLACE " .*$" "" po_lang "${po_lang_file}")
    list(APPEND all_po_files "${po_file}")
  endforeach()

  if(NOT all_po_files)
    message(WARNING "No po files added.")
  endif()

  foreach(script ${scripts})
    execute_process(COMMAND "${script}" "${translation_cache_base}"
      "${po_cache}" -c "${in_file}" "${out_file}"
      RESULT_VARIABLE result)
    if(NOT result)
      add_custom_command(OUTPUT "${out_file}"
        COMMAND "${script}" "${translation_cache_base}" "${po_cache}"
        -w "${in_file}" "${out_file}"
        DEPENDS "${in_file}" "${script}" ${all_po_files}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      return()
    endif()
  endforeach()
  message(FATAL_ERROR
    "Cannot find a rule to convert ${in_file} to ${out_file}")
endfunction()

# Should be used in cmake modules, add a handler script to update a set of
# certain type of files from the translated mo files.
# the script must be executable
function(_fcitx_translate_add_apply_handler script)
  get_filename_component(script "${script}" ABSOLUTE)
  set_property(GLOBAL APPEND PROPERTY "FCITX_TRANSLATION_APPLY_HANDLERS"
    "${script}")
endfunction()
_fcitx_translate_add_apply_handler("${FCITX_TRANSLATION_MERGE_CONFIG}")

# Set the pot file and the target to update it
# (the second time it is called in one project will raise an error)
function(fcitx_translate_set_pot_target target domain pot_file)
  get_property(pot_target_set GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET")
  if(pot_target_set)
    message(FATAL_ERROR "Duplicate pot targets.")
  endif()
  set_property(GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET" 1)
  get_property(translation_cache_base GLOBAL
    PROPERTY "__FCITX_TRANSLATION_CACHE_BASE")
  get_filename_component(full_name "${pot_file}" ABSOLUTE)
  set_property(GLOBAL PROPERTY "__FCITX_TRANSLATION_TARGET_FILE" "${full_name}")
  set_property(GLOBAL PROPERTY "__FCITX_TRANSLATION_TARGET_NAME" "${target}")
  set_property(GLOBAL PROPERTY "__FCITX_TRANSLATION_TARGET_DOMAIN" "${domain}")
  add_custom_target(fcitx-translate-pot.target
    COMMAND "${FCITX_TRANSLATION_SCAN_POT}"
    "${translation_cache_base}"
    "${full_name}" --pot
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  add_custom_target("${target}")
  add_dependencies("${target}" fcitx-translate-pot.target)
  get_property(sources GLOBAL PROPERTY "FCITX_TRANSLATION_SOURCES")
  __fcitx_translate_add_sources_internal(${sources})
  get_property(po_lang_files GLOBAL PROPERTY "FCITX_TRANSLATION_PO_FILES")
  set(all_mo_files)
  foreach(po_lang_file ${po_lang_files})
    string(REGEX REPLACE "^[^ ]* " "" po_file "${po_lang_file}")
    string(REGEX REPLACE " .*$" "" po_lang "${po_lang_file}")
    set(po_dir "${translation_cache_base}/fcitx_mo/${po_lang}")
    add_custom_command(OUTPUT "${po_dir}/${domain}.mo"
      COMMAND mkdir -p "${po_dir}"
      COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
      -o "${po_dir}/${domain}.mo" "${po_file}"
      DEPENDS "${po_file}")
    install(FILES "${po_dir}/${domain}.mo"
      DESTINATION "share/locale/${po_lang}/LC_MESSAGES")
    list(APPEND all_mo_files "${po_dir}/${domain}.mo")
  endforeach()
  add_custom_target(fcitx-compile-mo.target ALL
    DEPENDS ${all_mo_files})
endfunction()

# Add translated po files to be updated and installed
# need to be done before any files that needs to be merged is added
function(fcitx_translate_add_po_file locale po_file)
  get_filename_component(po_file "${po_file}" ABSOLUTE)
  get_property(translation_cache_base GLOBAL
    PROPERTY "__FCITX_TRANSLATION_CACHE_BASE")
  get_property(full_name GLOBAL
    PROPERTY "__FCITX_TRANSLATION_TARGET_FILE")
  set_property(GLOBAL APPEND PROPERTY "FCITX_TRANSLATION_PO_FILES"
    "${locale} ${po_file}")
  execute_process(COMMAND "${FCITX_TRANSLATION_SCAN_POT}"
    "${translation_cache_base}"
    "${full_name}" --add-po "${locale}" "${po_file}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  get_property(pot_target_set GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET")
  if(NOT pot_target_set)
    return()
  endif()
  get_property(domain GLOBAL PROPERTY "__FCITX_TRANSLATION_TARGET_DOMAIN")
  set(po_dir "${translation_cache_base}/fcitx_mo/${locale}")
  add_custom_command(OUTPUT "${po_dir}/${domain}.mo"
    COMMAND mkdir -p "${po_dir}"
    COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
    -o "${po_dir}/${domain}.mo" "${po_file}"
    DEPENDS "${po_file}")
  install(FILES "${po_dir}/${domain}.mo"
    DESTINATION "share/locale/${locale}/LC_MESSAGES")
  __fcitx_addon_get_unique_name(translated-po po_target)
  add_custom_target("${po_target}" ALL DEPENDS "${po_dir}/${domain}.mo")
endfunction()


function(__fcitx_conf_file_get_unique_target_name name unique_name)
  set(propertyName "_FCITX_UNIQUE_COUNTER_${name}")
  get_property(currentCounter GLOBAL PROPERTY "${propertyName}")
  if(NOT currentCounter)
    set(currentCounter 1)
  endif()
  set(${unique_name} "${name}_${currentCounter}" PARENT_SCOPE)
  math(EXPR currentCounter "${currentCounter} + 1")
  set_property(GLOBAL PROPERTY ${propertyName} ${currentCounter})
endfunction()

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


MACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)
  intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in ${CMAKE_CURRENT_BINARY_DIR}/${conffilename})
  __FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME(fcitx_addon_conf targetname)
  add_custom_target(${targetname} ALL DEPENDS ${conffilename})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${conffilename} DESTINATION ${FCITX4_ADDON_CONFIG_INSTALL_DIR})
ENDMACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)

MACRO(FCITX_ADD_INPUTMETHOD_CONF_FILE conffilename)
  intltool_merge_translation("${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${conffilename}")
  __FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME(fcitx_inputmethod_conf targetname)
  add_custom_target(${targetname} ALL DEPENDS ${conffilename})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${conffilename} DESTINATION ${FCITX4_INPUTMETHOD_CONFIG_INSTALL_DIR})
ENDMACRO()

MACRO(FCITX_ADD_CONFIGDESC_FILE)
  fcitx_install_addon_desc(${ARGN})
ENDMACRO()

MACRO(EXTRACT_FCITX_ADDON_CONF_POSTRING)
  file(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in POTFILES_IN)
  set(TEMPHEADER "")
  foreach(EXTRACTFILE ${POTFILES_IN})
    if ("${EXTRACTFILE}" MATCHES ".conf.in")
      get_filename_component(_EXTRAFILENAME ${EXTRACTFILE} NAME)
      add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h
        COMMAND ${INTLTOOL_EXTRACT} --srcdir=${PROJECT_BINARY_DIR}
        --local --type=gettext/ini ${EXTRACTFILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
      set(TEMPHEADER ${TEMPHEADER}
        "${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h")
    endif()
  endforeach()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/conf.po
    COMMAND xgettext --add-comments --output=conf.po --keyword=N_ ${TEMPHEADER}
    DEPENDS ${TEMPHEADER})
ENDMACRO()

MACRO(FCITX_ADD_ADDON target)
  __fcitx_add_addon_lib("${target}" ${ARGN})
ENDMACRO()

function(FCITX_ADD_ADDON_HEADER subdir)
  # for fcitx_scanner_addon
  __fcitx_link_addon_headers(${ARGN})
  set(headers ${ARGN})
  install(FILES ${headers}
    DESTINATION "${includedir}/${FCITX4_PACKAGE_NAME}/module/${subdir}")
endfunction()
