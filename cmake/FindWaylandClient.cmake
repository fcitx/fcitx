# Locate Wayland Client library
# This module defines
#  WAYLAND_CLIENT_FOUND, if false, do not try to link to wayland-client
#  WAYLAND_CLIENT_LIBRARIES
#  WAYLAND_CLIENT_INCLUDE_DIRS, where to find wayland-client.h
#  WAYLAND_CLIENT_VERSION, the version of wayland_client found

# Copyright (c) 2012 Yichao Yu <yyc1992@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_package(PkgConfig)
unset(WAYLAND_CLIENT_LIBRARIES CACHE)
unset(__pkg_config_checked_WAYLAND_CLIENT CACHE)
pkg_check_modules(WAYLAND_CLIENT wayland-client)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set WAYLAND_CLIENT_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(WaylandClient
  REQUIRED_VARS WAYLAND_CLIENT_LIBRARIES
  VERSION_VAR WAYLAND_CLIENT_VERSION)
mark_as_advanced(WAYLAND_CLIENT_INCLUDE_DIRS WAYLAND_CLIENT_LIBRARIES
  WAYLAND_CLIENT_VERSION)
