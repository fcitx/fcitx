# Locate Lua library
# This module defines
#  LUA_FOUND, if false, do not try to link to Lua
#  LUA_LIBRARIES
#  LUA_INCLUDE_DIRS, where to find lua.h
#  LUA_VERSION_STRING, the version of Lua found
#
# Note that the expected include convention is
#  #include "lua.h"
# and not
#  #include <lua/lua.h>
# This is because, the lua location is not standardized and may exist
# in locations other than lua/

# Copyright (c) 2012 Yichao Yu <yyc1992@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# This file incorporates work covered by the following copyright and
# permission notice:
#    =========================================================================
#     Copyright 2007-2009 Kitware, Inc.
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

find_package(PkgConfig)
unset(LUA_LIBRARIES CACHE)
unset(__pkg_config_checked_LUA CACHE)
if("${LUA_MODULE_NAME}" STREQUAL "")
  pkg_check_modules(LUA lua5.2)
  # try best to find lua52
  if(NOT LUA_FOUND)
    unset(__pkg_config_checked_LUA CACHE)
    pkg_check_modules(LUA lua)
  endif()
  # Too lazy to increase indentation...
  if(NOT LUA_FOUND)
    unset(__pkg_config_checked_LUA CACHE)
    pkg_check_modules(LUA lua5.1)
  endif()
else()
  pkg_check_modules(LUA "${LUA_MODULE_NAME}")
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUA_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(Lua
  REQUIRED_VARS LUA_LIBRARIES
  VERSION_VAR LUA_VERSION)
mark_as_advanced(LUA_INCLUDE_DIRS LUA_LIBRARIES LUA_VERSION)
