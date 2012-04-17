# - Try to find the LUA libraries
# Once done this will define
#
# LUA_FOUND - system has LUA
# LUA_INCLUDE_DIR - the LUA include directory
# LUA_LIBRARIES - LUA library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
    # Already in cache, be silent
    set(LUA_FIND_QUIETLY TRUE)
endif(LUA_INCLUDE_DIR AND LUA_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_LIBLUA QUIET lua)

find_path(LUA_MAIN_INCLUDE_DIR
          NAMES lua.h
          HINTS ${PC_LIBLUA_INCLUDEDIR})

find_library(LUA_LIBRARIES
             NAMES lua
             HINTS ${PC_LIBLUA_LIBDIR})

set(LUA_INCLUDE_DIR "${LUA_MAIN_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LUA DEFAULT_MSG LUA_LIBRARIES LUA_MAIN_INCLUDE_DIR)

mark_as_advanced(LUA_INCLUDE_DIR LUA_LIBRARIES)