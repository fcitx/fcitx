# - Try to find the GETTEXT-PO libraries
# Once done this will define
#
#  GETTEXTPO_FOUND - system has GETTEXT-PO
#  GETTEXTPO_INCLUDE_DIR - the GETTEXT-PO include directory
#  GETTEXTPO_LIBRARIES - GETTEXT-PO library
#
# Copyright (c) 2012 Yichao Yu <yyc1992@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(GETTEXTPO_INCLUDE_DIR AND GETTEXTPO_LIBRARIES)
  # Already in cache, be silent
  set(GETTEXTPO_FIND_QUIETLY TRUE)
endif()

find_path(GETTEXTPO_INCLUDE_DIR
  NAMES gettext-po.h)

find_library(GETTEXTPO_LIBRARIES
  NAMES gettextpo)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GettextPo DEFAULT_MSG GETTEXTPO_LIBRARIES
  GETTEXTPO_INCLUDE_DIR)

mark_as_advanced(GETTEXTPO_INCLUDE_DIR GETTEXTPO_LIBRARIES)
