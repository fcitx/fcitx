# - Try to find the Enchant libraries
# Once done this will define
#
#  ENCHANT_FOUND - system has ENCHANT
#  ENCHANT_INCLUDE_DIR - the ENCHANT include directory
#  ENCHANT_LIBRARIES - ENCHANT library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(ENCHANT_INCLUDE_DIR AND ENCHANT_LIBRARIES)
    # Already in cache, be silent
    set(ENCHANT_FIND_QUIETLY TRUE)
endif(ENCHANT_INCLUDE_DIR AND ENCHANT_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_LIBENCHANT QUIET enchant)

find_path(ENCHANT_MAIN_INCLUDE_DIR
          NAMES enchant.h
          HINTS ${PC_LIBENCHANT_INCLUDEDIR}
          PATH_SUFFIXES "enchant")

find_library(ENCHANT_LIBRARIES
             NAMES enchant
             HINTS ${PC_LIBENCHANT_LIBDIR})

set(ENCHANT_INCLUDE_DIR "${ENCHANT_MAIN_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ENCHANT  DEFAULT_MSG  ENCHANT_LIBRARIES ENCHANT_MAIN_INCLUDE_DIR)

mark_as_advanced(ENCHANT_INCLUDE_DIR ENCHANT_LIBRARIES)
