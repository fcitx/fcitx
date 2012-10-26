# Try to find Pthread functionality
# Once done this will define
#
#  PTHREAD_FOUND - system has Pthread
#  PTHREAD_INCLUDE_DIR - Pthread include directory
#  PTHREAD_LIBRARIES - Libraries needed to use Pthread
#
# TODO: This will enable translations only if Gettext functionality is
# present in libc. Must have more robust system for release, where Gettext
# functionality can also reside in standalone Gettext library, or the one
# embedded within kdelibs (cf. gettext.m4 from Gettext source).

# Copyright (c) 2006, Chusslove Illich, <caslav.ilic@gmx.net>
# Copyright (c) 2007, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(PTHREAD_INCLUDE_DIR AND PTHREAD_LIB_FOUND)
  set(Pthread_FIND_QUIETLY TRUE)
endif(PTHREAD_INCLUDE_DIR AND PTHREAD_LIB_FOUND)

find_path(PTHREAD_INCLUDE_DIR pthread.h)

set(PTHREAD_LIB_FOUND FALSE)

if(PTHREAD_INCLUDE_DIR)
  include(CheckFunctionExists)
  check_function_exists(pthread_create PTHREAD_LIBC_HAS_PTHREAD_CREATE)

  if (PTHREAD_LIBC_HAS_PTHREAD_CREATE)
    set(PTHREAD_LIBRARIES)
    set(PTHREAD_LIB_FOUND TRUE)
  else (PTHREAD_LIBC_HAS_PTHREAD_CREATE)
    find_library(PTHREAD_LIBRARIES NAMES pthread libpthread )
    if(PTHREAD_LIBRARIES)
      set(PTHREAD_LIB_FOUND TRUE)
    endif(PTHREAD_LIBRARIES)
  endif (PTHREAD_LIBC_HAS_PTHREAD_CREATE)

endif(PTHREAD_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pthread  DEFAULT_MSG  PTHREAD_INCLUDE_DIR  PTHREAD_LIB_FOUND)

mark_as_advanced(PTHREAD_INCLUDE_DIR  PTHREAD_LIBRARIES  PTHREAD_LIBC_HAS_PTHREAD_CREATE  PTHREAD_LIB_FOUND)
