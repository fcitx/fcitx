# - Try to find the PRESAGE libraries
# Once done this will define
#
#  PRESAGE_FOUND - system has PRESAGE
#  PRESAGE_INCLUDE_DIR - the PRESAGE include directory
#  PRESAGE_LIBRARIES - PRESAGE library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(PRESAGE_INCLUDE_DIR AND PRESAGE_LIBRARIES)
    # Already in cache, be silent
    set(PRESAGE_FIND_QUIETLY TRUE)
endif(PRESAGE_INCLUDE_DIR AND PRESAGE_LIBRARIES)

find_path(PRESAGE_INCLUDE_DIR
          NAMES presage.h)

find_library(PRESAGE_LIBRARIES
             NAMES presage)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Presage  DEFAULT_MSG  PRESAGE_LIBRARIES PRESAGE_INCLUDE_DIR)

mark_as_advanced(PRESAGE_INCLUDE_DIR PRESAGE_LIBRARIES)
