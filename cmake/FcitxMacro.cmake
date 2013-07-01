# This file is included by FindFcitx4.cmake, don't include it directly.
# - Providing Useful cmake functions for fcitx development
#
# Functions for handling source files and building the project:
#     fcitx_parse_arguments
#     fcitx_add_addon_full
#     fcitx_install_addon_desc
#     fcitx_translate_add_sources
#     fcitx_translate_add_apply_source
#     fcitx_translate_set_pot_target
#     fcitx_translate_add_po_file
#     _fcitx_add_uninstall_target
#     fcitx_download
# Functions to extend fcitx's build (mainly translation) system:
#     _fcitx_translate_add_handler
#     _fcitx_translate_add_apply_handler
#
# Please refer to the descriptions before each functions' definition
# for usage.

#==============================================================================
# Copyright 2011, 2012 Xuetian Weng
# Copyright 2012, 2013 Yichao Yu
#
# Distributed under the GPLv2 License
# see accompanying file COPYRIGHT for details
#==============================================================================
# (To distribute this file outside of Fcitx, substitute the full
#  License text for the above reference.)

find_program(GETTEXT_MSGFMT_EXECUTABLE msgfmt)

set(_FCITX_DISABLE_GETTEXT "Off" CACHE INTERNAL "Disable Gettext" FORCE)
function(fcitx_disable_gettext)
  set(_FCITX_DISABLE_GETTEXT "On" CACHE INTERNAL "Disable Gettext" FORCE)
endfunction()
if(NOT GETTEXT_MSGFMT_EXECUTABLE)
  fcitx_disable_gettext()
endif()

# The following definition of fcitx_parse_arguments is copied from
# CMakeParseArguments.cmake, which is a standard cmake module since 2.8.3,
# with some minor format changes and is renamed to avoid conflict with newer
# cmake versions. (see cmake documentation for usage)
# It is included to lower the required cmake version.

# This file incorporates work covered by the following copyright and
# permission notice:
#
#    =========================================================================
#     Copyright 2010 Alexander Neundorf <neundorf@kde.org>
#
#     Distributed under the OSI-approved BSD License (the "License");
#     see accompanying file Copyright.txt for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even the
#     implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#     See the License for more information.
#    =========================================================================
#     (To distribute this file outside of CMake, substitute the full
#      License text for the above reference.)

function(fcitx_parse_arguments prefix _optionNames
    _singleArgNames _multiArgNames)
  # first set all result variables to empty/FALSE
  foreach(arg_name ${_singleArgNames} ${_multiArgNames})
    set(${prefix}_${arg_name})
  endforeach()

  foreach(option ${_optionNames})
    set(${prefix}_${option} FALSE)
  endforeach()

  set(${prefix}_UNPARSED_ARGUMENTS)

  set(insideValues FALSE)
  set(currentArgName)

  # now iterate over all arguments and fill the result variables
  foreach(currentArg ${ARGN})
    # ... then this marks the end of the arguments belonging to this keyword
    list(FIND _optionNames "${currentArg}" optionIndex)
    # ... then this marks the end of the arguments belonging to this keyword
    list(FIND _singleArgNames "${currentArg}" singleArgIndex)
    # ... then this marks the end of the arguments belonging to this keyword
    list(FIND _multiArgNames "${currentArg}" multiArgIndex)

    if(${optionIndex} EQUAL -1 AND
        ${singleArgIndex} EQUAL -1 AND
        ${multiArgIndex} EQUAL -1)
      if(insideValues)
        if("${insideValues}" STREQUAL "SINGLE")
          set(${prefix}_${currentArgName} ${currentArg})
          set(insideValues FALSE)
        elseif("${insideValues}" STREQUAL "MULTI")
          list(APPEND ${prefix}_${currentArgName} ${currentArg})
        endif()
      else()
        list(APPEND ${prefix}_UNPARSED_ARGUMENTS ${currentArg})
      endif()
    else()
      if(NOT ${optionIndex} EQUAL -1)
        set(${prefix}_${currentArg} TRUE)
        set(insideValues FALSE)
      elseif(NOT ${singleArgIndex} EQUAL -1)
        set(currentArgName ${currentArg})
        set(${prefix}_${currentArgName})
        set(insideValues "SINGLE")
      elseif(NOT ${multiArgIndex} EQUAL -1)
        set(currentArgName ${currentArg})
        set(${prefix}_${currentArgName})
        set(insideValues "MULTI")
      endif()
    endif()
  endforeach()

  # propagate the result variables to the caller:
  foreach(arg_name ${_singleArgNames} ${_multiArgNames} ${_optionNames})
    set(${prefix}_${arg_name}  ${${prefix}_${arg_name}} PARENT_SCOPE)
  endforeach()
  set(${prefix}_UNPARSED_ARGUMENTS ${${prefix}_UNPARSED_ARGUMENTS} PARENT_SCOPE)
endfunction()


get_filename_component(FCITX_MACRO_CMAKE_DIR
  "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(FCITX_CMAKE_HELPER_SCRIPT "${FCITX_MACRO_CMAKE_DIR}/fcitx-cmake-helper.sh"
  CACHE INTERNAL "fcitx-cmake-helper" FORCE)
mark_as_advanced(FORCE FCITX_CMAKE_HELPER_SCRIPT)
set(FCITX_TRANSLATION_MERGE_CONFIG
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-merge-config.sh")
set(FCITX_TRANSLATION_EXTRACT_GETTEXT
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-gettext.sh")
set(FCITX_TRANSLATION_EXTRACT_DESKTOP
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-desktop.sh")
set(FCITX_TRANSLATION_EXTRACT_CONFDESC
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-confdesc.sh")
set(FCITX_TRANSLATION_EXTRACT_PO
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-po.sh")
set(FCITX_TRANSLATION_EXTRACT_QT
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-qt.sh")
set(FCITX_TRANSLATION_EXTRACT_KDE
  "${FCITX_MACRO_CMAKE_DIR}/fcitx-extract-kde.sh")

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
  get_property(FCITX_INTERNAL_BUILD GLOBAL PROPERTY "__FCITX_INTERNAL_BUILD")
  add_custom_target(fcitx-modules.target ALL)
  add_custom_target(fcitx-scan-addons.target)
  add_custom_target(fcitx-extract-traslation.dependency)
  add_custom_target(fcitx-parse-pos.dependency)
  add_dependencies(fcitx-modules.target fcitx-scan-addons.target)
  set(fcitx_cmake_cache_base "${PROJECT_BINARY_DIR}/fcitx_cmake_cache")
  file(MAKE_DIRECTORY "${fcitx_cmake_cache_base}")
  set_property(GLOBAL PROPERTY "__FCITX_CMAKE_CACHE_BASE"
    "${fcitx_cmake_cache_base}")
  if(FCITX_INTERNAL_BUILD)
    set(FCITX_SCANNER_EXECUTABLE
      "${PROJECT_BINARY_DIR}/tools/dev/fcitx-scanner"
      CACHE INTERNAL "fcitx-scanner" FORCE)
    set(FCITX_PO_PARSER_EXECUTABLE
      "${PROJECT_BINARY_DIR}/tools/dev/fcitx-po-parser"
      CACHE INTERNAL "fcitx-po-parser" FORCE)
    set(FCITX4_FCITX_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include"
      CACHE INTERNAL "include dir" FORCE)
  else()
    set(FCITX_SCANNER_EXECUTABLE
      "${FCITX4_ADDON_INSTALL_DIR}/libexec/fcitx-scanner"
      CACHE INTERNAL "fcitx-scanner" FORCE)
    set(FCITX_PO_PARSER_EXECUTABLE
      "${FCITX4_ADDON_INSTALL_DIR}/libexec/fcitx-po-parser"
      CACHE INTERNAL "fcitx-po-parser" FORCE)
    execute_process(COMMAND env ${FCITX_PO_PARSER_EXECUTABLE}
      --gettext-support RESULT_VARIABLE result)
    if(result)
      # not 0
      fcitx_disable_gettext()
    endif()
  endif()
  set(__FCITX_CMAKE_HELPER_EXPORT
    "_FCITX_MACRO_CMAKE_DIR=${FCITX_MACRO_CMAKE_DIR}"
    "_FCITX_PO_PARSER_EXECUTABLE=${FCITX_PO_PARSER_EXECUTABLE}"
    "FCITX_HELPER_CMAKE_CMD=${CMAKE_COMMAND}"
    "FCITX_CMAKE_CACHE_BASE=${fcitx_cmake_cache_base}"
    CACHE INTERNAL "fcitx cmake helper export" FORCE)
  execute_process(COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
    "${FCITX_CMAKE_HELPER_SCRIPT}" --clean)
  set_property(GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET" 0)
  set_property(GLOBAL PROPERTY "${property_name}" 1)
  set_property(GLOBAL PROPERTY "__FCITX_APPLY_TRANLATION_FILE_ADDED" 0)
endfunction()
__fcitx_cmake_init()

# options:
#     NO_INSTALL: Do not install any files or compile any sources
#         useful when the module is disabled but the generated header is needed
#     SCAN: Generate api header, the path of the input file is determined by the
#         FXADDON_SRC arguement and the name of the generated header file is
#         determined by the FXADDON_GEN arguement.
#     SCAN_PRIV: Generate add function header, the path of the input file is
#         determined by the FXADDON_SRC arguement and the name of the
#         generated header file is determined by the ADDFUNCTIONS_GEN arguement.
#     SCAN_IN: Generate api header from .in, the path of the fxaddon.in file
#         (without the .in suffix) is determined by the
#         FXADDON_SRC arguement and the name of the generated header file is
#         determined by the FXADDON_GEN arguement.
#     DESC: Install main config-desc file, the path of the file (with the .desc
#         suffix) is determined by the DESC_SRC arguement.
#     SCAN_NO_INSTALL: Do not install generated api header.
# single value arguments:
#     HEADER_DIR: The subdirectory under fcitx/module to which the header files
#         of the addon will be installed (default ${short_name})
#     LIB_NAME: The name of the addon binary (default fcitx-${short_name})
#     FXADDON_SRC: the path of the fxaddon file. Setting this argument will
#          automatically set the SCAN option as well, when using with SCAN_IN
#          the value of this argument shouldn't contain the .in suffix
#          (default: fcitx-${short_name}.fxaddon)
#     FXADDON_GEN: the name of the generated header file. (path will be ignored)
#          Setting this argument will automatically set the SCAN option as well.
#          (default: fcitx-${short_name}.h)
#     ADDFUNCTIONS_GEN: the name of the generated add function header file.
#          (path will be ignored) Setting this argument will automatically
#          set the SCAN_PRIV option as well.
#          (default: fcitx-${short_name}-addfunctions.h)
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
#     IM_CONFIG: input method config files.
function(fcitx_add_addon_full short_name)
  set(options NO_INSTALL SCAN DESC SCAN_IN SCAN_NO_INSTALL SCAN_PRIV)
  set(one_value_args HEADER_DIR FXADDON_SRC FXADDON_GEN
    CONF_SRC DESC_SRC UNIQUE_NAME LIB_NAME ADDFUNCTIONS_GEN)
  set(multi_value_args SOURCES HEADERS EXTRA_DESC EXTRA_PO LINK_LIBS
    DEPENDS IM_CONFIG)
  fcitx_parse_arguments(FCITX_ADDON
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  if(FCITX_ADDON_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unknown arguments given to fcitx_add_addon_full "
      "\"${FCITX_ADDON_UNPARSED_ARGUMENTS}\"")
  endif()
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
  list(APPEND files_to_translate "${FCITX_ADDON_CONF_SRC}.in")
  foreach(im_config ${FCITX_ADDON_IM_CONFIG})
    list(APPEND files_to_translate "${im_config}.in")
  endforeach()
  if(FCITX_ADDON_SCAN OR FCITX_ADDON_SCAN_IN OR
      FCITX_ADDON_FXADDON_SRC OR FCITX_ADDON_FXADDON_GEN)
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
    if(FCITX_ADDON_SCAN_IN)
      set(fxaddon_in "${FCITX_ADDON_FXADDON_SRC}.in")
      get_filename_component(FCITX_ADDON_FXADDON_SRC
        "${FCITX_ADDON_FXADDON_SRC}" NAME)
      set(FCITX_ADDON_FXADDON_SRC
        "${CMAKE_CURRENT_BINARY_DIR}/${FCITX_ADDON_FXADDON_SRC}")
      configure_file("${fxaddon_in}" "${FCITX_ADDON_FXADDON_SRC}"
        IMMEDIATE @ONLY)
    endif()
  endif()
  if(FCITX_ADDON_SCAN_PRIV OR FCITX_ADDON_ADDFUNCTIONS_GEN)
    if(NOT FCITX_ADDON_FXADDON_SRC)
      set(FCITX_ADDON_FXADDON_SRC "fcitx-${short_name}.fxaddon")
    endif()
    if(NOT FCITX_ADDON_ADDFUNCTIONS_GEN)
      set(FCITX_ADDON_ADDFUNCTIONS_GEN
        "${CMAKE_CURRENT_BINARY_DIR}/fcitx-${short_name}-addfunctions.h")
    else()
      get_filename_component(FCITX_ADDON_ADDFUNCTIONS_GEN
        "${FCITX_ADDON_ADDFUNCTIONS_GEN}" NAME)
      set(FCITX_ADDON_ADDFUNCTIONS_GEN
        "${CMAKE_CURRENT_BINARY_DIR}/${FCITX_ADDON_ADDFUNCTIONS_GEN}")
    endif()
  endif()
  if(FCITX_ADDON_DESC OR FCITX_ADDON_DESC_SRC)
    if(NOT FCITX_ADDON_DESC_SRC)
      set(FCITX_ADDON_DESC_SRC "fcitx-${short_name}.desc")
    endif()
    list(APPEND files_to_translate "${FCITX_ADDON_DESC_SRC}")
    list(APPEND all_conf_descs "${FCITX_ADDON_DESC_SRC}")
  endif()
  if(NOT FCITX_ADDON_SOURCES)
    MESSAGE(FATAL_ERROR "No source file for addon ${short_name}.")
  endif()
  list(APPEND files_to_translate
    ${FCITX_ADDON_SOURCES} ${FCITX_ADDON_HEADERS}
    ${FCITX_ADDON_EXTRA_DESC} ${FCITX_ADDON_EXTRA_PO})
  list(APPEND all_conf_descs ${FCITX_ADDON_EXTRA_DESC})

  __fcitx_link_addon_headers(${FCITX_ADDON_HEADERS})
  if(FCITX_ADDON_FXADDON_GEN)
    __fcitx_scan_addon("${FCITX_ADDON_UNIQUE_NAME}"
      "${FCITX_ADDON_FXADDON_SRC}"
      "${FCITX_ADDON_FXADDON_GEN}")
    if(NOT FCITX_ADDON_SCAN_NO_INSTALL)
      list(APPEND FCITX_ADDON_HEADERS "${FCITX_ADDON_FXADDON_GEN}")
    endif()
  endif()
  fcitx_translate_add_sources(${files_to_translate})
  if(FCITX_ADDON_NO_INSTALL)
    return()
  endif()

  if(FCITX_ADDON_ADDFUNCTIONS_GEN)
    __fcitx_scan_addon_priv("${FCITX_ADDON_UNIQUE_NAME}"
      "${FCITX_ADDON_FXADDON_SRC}"
      "${FCITX_ADDON_ADDFUNCTIONS_GEN}")
  endif()
  include_directories("${CMAKE_CURRENT_BINARY_DIR}")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
  set(target_name "${FCITX_ADDON_UNIQUE_NAME}--addon")
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
    set(depends_target "fcitx-addon-depends--${FCITX_ADDON_UNIQUE_NAME}")
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
  __fcitx_addon_get_unique_name("${target_name}--desc" desc_target)
  add_custom_target("${desc_target}" DEPENDS ${descs})
  add_dependencies("${target_name}" "${desc_target}")
  install(FILES ${descs}
    DESTINATION "${FCITX4_CONFIGDESC_INSTALL_DIR}")
endfunction()

# Add additional config-desc files, the file will be added to extracte
# po strings from and will be installed
function(fcitx_install_addon_desc)
  __fcitx_addon_get_unique_name(install-desc desc_target)
  add_custom_target("${desc_target}" ALL)
  fcitx_translate_add_sources(${ARGN})
  __fcitx_install_addon_desc("${desc_target}" ${ARGN})
endfunction()

function(__fcitx_install_addon_headers target_name subdir)
  set(headers ${ARGN})
  __fcitx_addon_get_unique_name("${target_name}--headers" header_target)
  add_custom_target("${header_target}" DEPENDS ${headers})
  add_dependencies("${target_name}" "${header_target}")
  install(FILES ${headers}
    DESTINATION
    "${FCITX4_FCITX_INCLUDEDIR}/${FCITX4_PACKAGE_NAME}/module/${subdir}")
endfunction()

function(__fcitx_link_addon_headers)
  set(headers ${ARGN})
  foreach(header ${headers})
    get_filename_component(fullsrc "${header}" ABSOLUTE)
    get_filename_component(basename "${fullsrc}" NAME)
    set(link_target "${CMAKE_CURRENT_BINARY_DIR}/${basename}")
    if(NOT EXISTS "${link_target}")
      execute_process(COMMAND "${CMAKE_COMMAND}" -E create_symlink
        "${fullsrc}" "${link_target}")
    endif()
  endforeach()
endfunction()

function(__fcitx_scan_addon name in_file out_file)
  if(NOT FCITX_SCANNER_EXECUTABLE)
    message(FATAL_ERROR "Cannot find fcitx-scanner")
  endif()
  get_filename_component(dir "${out_file}" PATH)
  add_custom_command(
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
    COMMAND "${FCITX_SCANNER_EXECUTABLE}" --api "${in_file}" "${out_file}"
    OUTPUT "${out_file}" DEPENDS "${in_file}" "${FCITX_SCANNER_EXECUTABLE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  __fcitx_addon_get_unique_name("${name}--scan" target_name)
  add_custom_target("${target_name}" DEPENDS "${out_file}")
  add_dependencies(fcitx-scan-addons.target "${target_name}")
endfunction()

function(__fcitx_scan_addon_priv name in_file out_file)
  if(NOT FCITX_SCANNER_EXECUTABLE)
    message(FATAL_ERROR "Cannot find fcitx-scanner")
  endif()
  get_filename_component(dir "${out_file}" PATH)
  add_custom_command(
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
    COMMAND "${FCITX_SCANNER_EXECUTABLE}" --private "${in_file}" "${out_file}"
    OUTPUT "${out_file}" DEPENDS "${in_file}" "${FCITX_SCANNER_EXECUTABLE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  __fcitx_addon_get_unique_name("${name}--scan-priv" target_name)
  add_custom_target("${target_name}" DEPENDS "${out_file}")
  add_dependencies(fcitx-scan-addons.target "${target_name}")
endfunction()

function(__fcitx_addon_config_file target conf_file dest_dir)
  set(SRC_FILE "${conf_file}.in")
  get_filename_component(base_name "${conf_file}" NAME)
  set(TGT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${base_name}")
  fcitx_translate_add_apply_source("${SRC_FILE}" "${TGT_FILE}" NOT_ALL)
  __fcitx_addon_get_unique_name("${target}--conf" conf_target)
  add_custom_target("${conf_target}" DEPENDS "${TGT_FILE}")
  add_dependencies("${target}" "${conf_target}")
  install(FILES "${TGT_FILE}" DESTINATION "${dest_dir}")
endfunction()

function(fcitx_translate_set_extract_type type)
  if(type STREQUAL "")
    set(type "generic")
  endif()
  foreach(file ${ARGN})
    get_filename_component(file "${file}" ABSOLUTE)
    set_source_files_properties("${file}" PROPERTIES
      "__FCITX_TRANSLATE_EXTRACT_TYPE" "${type}")
  endforeach()
endfunction()

function(_fcitx_translate_get_extract_type var file)
  get_filename_component(file "${file}" ABSOLUTE)
  get_source_file_property(type "${file}" "__FCITX_TRANSLATE_EXTRACT_TYPE")
  if(NOT type)
    set(type)
  endif()
  set("${var}" "${type}" PARENT_SCOPE)
endfunction()

function(fcitx_translate_set_ignore)
  foreach(file ${ARGN})
    get_filename_component(file "${file}" ABSOLUTE)
    set_source_files_properties("${file}" PROPERTIES
      "__FCITX_TRANSLATE_EXTRACT_IGNORE" 1)
  endforeach()
endfunction()

function(_fcitx_translate_get_ignore var file)
  get_filename_component(file "${file}" ABSOLUTE)
  get_source_file_property(ignore "${file}" "__FCITX_TRANSLATE_EXTRACT_IGNORE")
  set("${var}" "${ignore}" PARENT_SCOPE)
endfunction()

function(__fcitx_translate_add_sources_internal)
  if(NOT ARGN)
    return()
  endif()
  foreach(source ${ARGN})
    _fcitx_translate_get_extract_type(type "${source}")
    _fcitx_translate_get_ignore(ignore "${source}")
    if(ignore)
      continue()
    endif()
    file(RELATIVE_PATH source "${PROJECT_SOURCE_DIR}" "${source}")
    execute_process(COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
      "${FCITX_CMAKE_HELPER_SCRIPT}" --add-source "${source}" "${type}"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  endforeach()
endfunction()

# Add files to be translated
function(fcitx_translate_add_sources)
  set(options)
  set(one_value_args EXTRACT_TYPE)
  set(multi_value_args)
  fcitx_parse_arguments(FCITX_ADD_SOURCES
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  set(sources ${FCITX_ADD_SOURCES_UNPARSED_ARGUMENTS})
  set(new_sources)
  foreach(source ${sources})
    get_filename_component(full_name "${source}" ABSOLUTE)
    if(FCITX_ADD_SOURCES_EXTRACT_TYPE)
      _fcitx_translate_get_extract_type(type "${full_name}")
      if(NOT type)
        fcitx_translate_set_extract_type(
          "${FCITX_ADD_SOURCES_EXTRACT_TYPE}" "${full_name}")
      endif()
    endif()
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
# from a set of certain type of files, the script must be executable.
#
# When classifying source files, the script will be called with -c as the first
# arguement and the file name as the second one. The script should return 0 in
# this case to indicate a source file can be handled and should return non-zero
# otherwise.
#
# When extracting po strings from source files, the script will be called with
# first argument -w, the second argument the file name of the output po file
# followed by a list of source files to extract po strings from. The script
# should extract po strings from the given source files and write the result
# to the po file.
function(_fcitx_translate_add_handler script)
  __fcitx_addon_get_unique_name(translation-handler target)
  add_custom_target("${target}" DEPENDS "${script}")
  add_dependencies(fcitx-extract-traslation.dependency "${target}")
  execute_process(COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
    "${FCITX_CMAKE_HELPER_SCRIPT}" --add-handler "${script}" ${ARGN}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endfunction()
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_GETTEXT}" gettext)
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_DESKTOP}" desktop)
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_CONFDESC}" confdesc)
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_PO}" po)
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_QT}" qt)
_fcitx_translate_add_handler("${FCITX_TRANSLATION_EXTRACT_KDE}" kde)

# Add files to apply translation
# this will generate a rule to generate ${out_file} from ${in_file} by
# l10n it using the handler script found.
# Proper handler script has to be added before this macro is called, or there
# will be a FATAL_ERROR
function(fcitx_translate_add_apply_source in_file out_file)
  set(options NOT_ALL)
  set(one_value_args)
  set(multi_value_args)
  fcitx_parse_arguments(FCITX_APPLY_SRC
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  get_property(scripts GLOBAL
    PROPERTY FCITX_TRANSLATION_APPLY_HANDLERS)
  get_filename_component(in_file "${in_file}" ABSOLUTE)
  get_filename_component(out_file "${out_file}" ABSOLUTE)
  get_property(po_lang_files GLOBAL PROPERTY "FCITX_TRANSLATION_PO_FILES")
  get_property(pot_target_set GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET")
  set(all_po_files)
  foreach(po_lang_file ${po_lang_files})
    string(REGEX REPLACE "^[^ ]* " "" po_file "${po_lang_file}")
    string(REGEX REPLACE " .*$" "" po_lang "${po_lang_file}")
    list(APPEND all_po_files "${po_file}")
  endforeach()

  set_property(GLOBAL PROPERTY "__FCITX_APPLY_TRANLATION_FILE_ADDED" 1)

  foreach(script ${scripts})
    execute_process(COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
      "${FCITX_CMAKE_HELPER_SCRIPT}" --check-apply-handler
      "${script}" "${in_file}" "${out_file}" RESULT_VARIABLE result)
    if(NOT result)
      get_filename_component(dir "${out_file}" PATH)
      add_custom_command(OUTPUT "${out_file}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
        COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
        "${FCITX_CMAKE_HELPER_SCRIPT}" --apply-po-merge
        "${script}" "${in_file}" "${out_file}"
        DEPENDS fcitx-parse-pos.dependency "${in_file}" "${script}"
        "${FCITX_CMAKE_HELPER_SCRIPT}" ${all_po_files}
        "${FCITX_PO_PARSER_EXECUTABLE}"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
      if(NOT FCITX_APPLY_SRC_NOT_ALL)
        __fcitx_addon_get_unique_name("apply-translation" target_name)
        add_custom_target("${target_name}" ALL DEPENDS "${out_file}")
      endif()
      return()
    endif()
  endforeach()
  message(FATAL_ERROR
    "Cannot find a rule to convert ${in_file} to ${out_file}")
endfunction()

# Should be used in cmake modules, add a handler script to update a set of
# certain type of files from the translated mo files.
# The script must be executable.
#
# When classifying source files, the script will be called with first arguement
# the directory name po's parse results are in, the second arguement a file
# containing space separated language code and translated po file name
# in each line, the third argument -c, followed by the file name of the input
# and output files (argument 4 and 5). The script should decide from the
# filename of input and output files to decide whether it can handle this
# conversion and should return 0 if it can or non-zero otherwise.
#
# When doing the l18n conversion, the script will be called with arguments
# the same with the previous case except the thrid argument is -w. The script
# should read language code list from the file given in the second argument
# fetch translation from either files in the first argument or po files in
# the second argument and then use these to l18n input file to output files.
function(_fcitx_translate_add_apply_handler script)
  get_filename_component(script "${script}" ABSOLUTE)
  set_property(GLOBAL APPEND PROPERTY "FCITX_TRANSLATION_APPLY_HANDLERS"
    "${script}")
endfunction()
_fcitx_translate_add_apply_handler("${FCITX_TRANSLATION_MERGE_CONFIG}")

# Set the pot file and the target to update it
# (the second time it is called in one project will raise an error)
# A target with name ${target} will be added to update ${pot_file} and
# po files added by fcitx_translate_add_po_file from all the sources added by
# `fcitx_translate_add_sources`.
# This also generate rules and targets to generate mo files from po files
# and install them to the given ${domain}
function(fcitx_translate_set_pot_target target domain pot_file)
  get_property(pot_target_set GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET")
  if(pot_target_set)
    message(FATAL_ERROR "Duplicate pot targets.")
  endif()

  set(options)
  set(one_value_args BUGADDR)
  set(multi_value_args)
  fcitx_parse_arguments(FCITX_SET_POT
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  set_property(GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET" 1)
  get_property(fcitx_cmake_cache_base GLOBAL
    PROPERTY "__FCITX_CMAKE_CACHE_BASE")
  get_filename_component(full_name "${pot_file}" ABSOLUTE)
  set_property(GLOBAL PROPERTY "__FCITX_TRANSLATION_TARGET_DOMAIN" "${domain}")
  set(__FCITX_CMAKE_HELPER_EXPORT
    ${__FCITX_CMAKE_HELPER_EXPORT}
    "_FCITX_TRANSLATION_TARGET_FILE=${full_name}"
    CACHE INTERNAL "fcitx cmake helper export" FORCE)

  if(NOT FCITX_SET_POT_BUGADDR)
    set(FCITX_SET_POT_BUGADDR "fcitx-dev@googlegroups.com")
  endif()

  # make pot will require bash, but this is only a dev time dependency.
  add_custom_target(fcitx-translate-pot.target
    COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
    bash "${FCITX_CMAKE_HELPER_SCRIPT}" --pot "${FCITX_SET_POT_BUGADDR}"
    DEPENDS "${FCITX_PO_PARSER_EXECUTABLE}" fcitx-extract-traslation.dependency
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  add_custom_target("${target}")
  add_dependencies("${target}" fcitx-translate-pot.target)
  get_property(sources GLOBAL PROPERTY "FCITX_TRANSLATION_SOURCES")
  __fcitx_translate_add_sources_internal(${sources})
  get_property(po_lang_files GLOBAL PROPERTY "FCITX_TRANSLATION_PO_FILES")
  set(all_mo_files)
  set(all_po_files)
  if(NOT _FCITX_DISABLE_GETTEXT)
    foreach(po_lang_file ${po_lang_files})
      string(REGEX REPLACE "^[^ ]* " "" po_file "${po_lang_file}")
      string(REGEX REPLACE " .*$" "" po_lang "${po_lang_file}")
      set(po_dir "${fcitx_cmake_cache_base}/mo/${po_lang}")
      add_custom_command(OUTPUT "${po_dir}/${domain}.mo"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${po_dir}"
        COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
        -o "${po_dir}/${domain}.mo" "${po_file}"
        DEPENDS "${po_file}")
      install(FILES "${po_dir}/${domain}.mo"
        DESTINATION "share/locale/${po_lang}/LC_MESSAGES")
      list(APPEND all_mo_files "${po_dir}/${domain}.mo")
      list(APPEND all_po_files "${po_file}")
    endforeach()
    add_custom_target(fcitx-compile-mo.target ALL
      DEPENDS ${all_mo_files})
  endif()
  add_custom_target(fcitx-parse-pos.target
    COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
    "${FCITX_CMAKE_HELPER_SCRIPT}" --parse-pos
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    DEPENDS "${FCITX_PO_PARSER_EXECUTABLE}" ${all_po_files})
  add_dependencies(fcitx-parse-pos.dependency fcitx-parse-pos.target)
endfunction()

# Add translated po files to be updated and installed.
# Need to be done before any files that needs to be l10n'ized is added.
# (or the recompile dependency won't be correct although there shouldn't be
# any problem for one-time build.)
function(fcitx_translate_add_po_file locale po_file)
  get_filename_component(po_file "${po_file}" ABSOLUTE)
  get_property(fcitx_cmake_cache_base GLOBAL
    PROPERTY "__FCITX_CMAKE_CACHE_BASE")
  set_property(GLOBAL APPEND PROPERTY "FCITX_TRANSLATION_PO_FILES"
    "${locale} ${po_file}")
  execute_process(COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
    "${FCITX_CMAKE_HELPER_SCRIPT}" --add-po "${locale}" "${po_file}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
  get_property(pot_target_set GLOBAL PROPERTY "FCITX_TRANSLATION_TARGET_SET")
  get_property(trans_files_added GLOBAL
    PROPERTY "__FCITX_APPLY_TRANLATION_FILE_ADDED")
  if(trans_files_added)
    message(WARNING
      "PO files should be added before any files to apply translation are added.")
  endif()
  if(NOT pot_target_set)
    return()
  endif()
  # message(WARNING "PO files should be added before the pot target is set.")
  get_property(domain GLOBAL PROPERTY "__FCITX_TRANSLATION_TARGET_DOMAIN")
  set(po_dir "${fcitx_cmake_cache_base}/fcitx_mo/${locale}")
  if(_FCITX_DISABLE_GETTEXT)
    return()
  endif()
  add_custom_command(OUTPUT "${po_dir}/${domain}.mo"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${po_dir}"
    COMMAND "${GETTEXT_MSGFMT_EXECUTABLE}"
    -o "${po_dir}/${domain}.mo" "${po_file}"
    DEPENDS "${po_file}")
  install(FILES "${po_dir}/${domain}.mo"
    DESTINATION "share/locale/${locale}/LC_MESSAGES")
  __fcitx_addon_get_unique_name(translated-po po_target)
  add_custom_target("${po_target}" ALL DEPENDS "${po_dir}/${domain}.mo")
endfunction()

# Add a uninstall target to the project
function(_fcitx_add_uninstall_target)
  add_custom_target(uninstall
    COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
    "${FCITX_CMAKE_HELPER_SCRIPT}" --uninstall
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
endfunction()

# Download a file
# The file will not be download if the target file already exist. An optional
# md5sum can be provided in which case the md5sum of the target file will
# be checked against this value and redownload/abort if the check failed.
function(fcitx_download tgt_name url output)
  set(options)
  set(one_value_args MD5SUM)
  set(multi_value_args)
  fcitx_parse_arguments(FCITX_DOWNLOAD
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  get_filename_component(output "${output}" ABSOLUTE)
  get_filename_component(dir "${output}" PATH)
  if(FCITX_DOWNLOAD_MD5SUM)
    add_custom_target("${tgt_name}" ALL
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
      COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
      "${FCITX_CMAKE_HELPER_SCRIPT}"
      --download "${url}" "${output}" "${FCITX_DOWNLOAD_MD5SUM}"
      COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
      "${FCITX_CMAKE_HELPER_SCRIPT}"
      --check-md5sum "${output}" "${FCITX_DOWNLOAD_MD5SUM}" 1
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    add_custom_target("${tgt_name}" ALL
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
      COMMAND env ${__FCITX_CMAKE_HELPER_EXPORT}
      "${FCITX_CMAKE_HELPER_SCRIPT}" --download "${url}" "${output}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  # This is the rule to create the target file, it is depending of the target
  # that does the real download so any files/targets that is depending on this
  # file will be run after the download finished.
  #
  # Since this rule doesn't have any command or file dependencies, cmake
  # won't notice any change in the rule and therefore it won't remove the
  # target file (and therefore triggers an unwilling redownload) if the real
  # rule (which is in the target defined above) has changed.
  #
  # This behavior is designed to be friendly for a build from cache with all
  # necessary files already downloaded so that a change in the
  # build options/url/checksum will not cause cmake to remove the target file
  # if it has already been updated correctly.
  add_custom_command(OUTPUT "${output}" DEPENDS "${tgt_name}")
endfunction()

function(fcitx_extract tgt_name ifile)
  set(options)
  set(one_value_args)
  set(multi_value_args OUTPUT DEPENDS)
  fcitx_parse_arguments(FCITX_EXTRACT
    "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  set(STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/.${tgt_name}.stamp")
  get_filename_component(ifile "${ifile}" ABSOLUTE)
  add_custom_command(OUTPUT "${STAMP_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E tar x "${ifile}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${STAMP_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E touch_nocreate ${FCITX_EXTRACT_OUTPUT}
    DEPENDS ${FCITX_EXTRACT_DEPENDS} "${ifile}")
  add_custom_target("${tgt_name}" ALL DEPENDS "${STAMP_FILE}")
  add_custom_command(OUTPUT ${FCITX_EXTRACT_OUTPUT}
    DEPENDS "${tgt_name}")
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
    COMMAND LC_ALL=C ${INTLTOOL_MERGE} -d -u ${PROJECT_SOURCE_DIR}/po
    ${infile} ${outfile}
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
    COMMAND "${_FCITX_GETDESCPO_PATH}" "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}")
ENDMACRO(EXTRACT_FCITX_DESC_FILE_POSTRING)


MACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)
  intltool_merge_translation("${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${conffilename}")
  __FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME(fcitx_addon_conf targetname)
  add_custom_target(${targetname} ALL DEPENDS ${conffilename})
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${conffilename}"
    DESTINATION "${FCITX4_ADDON_CONFIG_INSTALL_DIR}")
ENDMACRO(FCITX_ADD_ADDON_CONF_FILE conffilename)

MACRO(FCITX_ADD_INPUTMETHOD_CONF_FILE conffilename)
  intltool_merge_translation("${CMAKE_CURRENT_SOURCE_DIR}/${conffilename}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${conffilename}")
  __FCITX_CONF_FILE_GET_UNIQUE_TARGET_NAME(fcitx_inputmethod_conf targetname)
  add_custom_target(${targetname} ALL DEPENDS ${conffilename})
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${conffilename}"
    DESTINATION "${FCITX4_INPUTMETHOD_CONFIG_INSTALL_DIR}")
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
      add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/tmp/${_EXTRAFILENAME}.h"
        COMMAND ${INTLTOOL_EXTRACT} --srcdir=${PROJECT_BINARY_DIR}
        --local --type=gettext/ini ${EXTRACTFILE}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
      list(APPEND TEMPHEADER
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
    DESTINATION
    "${FCITX4_FCITX_INCLUDEDIR}/${FCITX4_PACKAGE_NAME}/module/${subdir}")
endfunction()
