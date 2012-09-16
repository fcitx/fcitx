# - Try to find the XKBFILE libraries
# Once done this will define
#
#  XKBFILE_FOUND - system has XKBFILE
#  XKBFILE_INCLUDE_DIR - the XKBFILE include directory
#  XKBFILE_LIBRARIES - XKBFILE library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(XKBFILE_INCLUDE_DIR AND XKBFILE_LIBRARIES)
    # Already in cache, be silent
    set(XKBFILE_FIND_QUIETLY TRUE)
endif(XKBFILE_INCLUDE_DIR AND XKBFILE_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_LIBXKBFILE xkbfile)

find_path(XKBFILE_MAIN_INCLUDE_DIR
          NAMES XKBfile.h
          HINTS ${PC_LIBXKBFILE_INCLUDE_DIRS}
          PATH_SUFFIXES "X11/extensions")

find_library(XKBFILE_LIBRARIES
             NAMES xkbfile
             HINTS ${PC_LIBXKBFILE_LIBRARY_DIRS})

set(XKBFILE_INCLUDE_DIR "${XKBFILE_MAIN_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XkbFile  DEFAULT_MSG  XKBFILE_LIBRARIES XKBFILE_MAIN_INCLUDE_DIR)

mark_as_advanced(XKBFILE_INCLUDE_DIR XKBFILE_LIBRARIES)
