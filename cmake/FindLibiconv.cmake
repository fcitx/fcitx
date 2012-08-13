# Try to find Libiconv functionality
# Once done this will define
#
#  LIBICONV_FOUND - system has Libiconv
#  LIBICONV_INCLUDE_DIR - Libiconv include directory
#  LIBICONV_LIBRARIES - Libraries needed to use Libiconv
#  LIBICONV_SECOND_ARGUMENT_IS_CONST - iconv second argument is const
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

if(LIBICONV_INCLUDE_DIR AND LIBICONV_LIB_FOUND)
  set(Libiconv_FIND_QUIETLY TRUE)
endif(LIBICONV_INCLUDE_DIR AND LIBICONV_LIB_FOUND)

include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

find_path(LIBICONV_INCLUDE_DIR iconv.h)

set(LIBICONV_LIB_FOUND FALSE)

if(LIBICONV_INCLUDE_DIR)
  include(CheckFunctionExists)
  check_function_exists(iconv_open LIBICONV_LIBC_HAS_ICONV_OPEN)

  if (LIBICONV_LIBC_HAS_ICONV_OPEN)
    set(LIBICONV_LIBRARIES)
    set(LIBICONV_LIB_FOUND TRUE)
  else (LIBICONV_LIBC_HAS_ICONV_OPEN)
    find_library(LIBICONV_LIBRARIES NAMES iconv)
    if(LIBICONV_LIBRARIES)
      set(LIBICONV_LIB_FOUND TRUE)
    endif(LIBICONV_LIBRARIES)
  endif (LIBICONV_LIBC_HAS_ICONV_OPEN)

  if (LIBICONV_LIB_FOUND)
    check_c_compiler_flag("-Werror" ICONV_HAVE_WERROR)
    set (CMAKE_C_FLAGS_BACKUP "${CMAKE_C_FLAGS}")
    if(ICONV_HAVE_WERROR)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    endif(ICONV_HAVE_WERROR)
    set(CMAKE_REQUIRED_INCLUDES "${LIBICONV_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${LIBICONV_LIBRARIES}")
    check_c_source_compiles("
    #include <iconv.h>
    int main(){
        iconv_t conv = 0;
        const char* in = 0;
        size_t ilen = 0;
        char* out = 0;
        size_t olen = 0;
        iconv(conv, &in, &ilen, &out, &olen);
        return 0;
    }
    " LIBICONV_SECOND_ARGUMENT_IS_CONST )
    set(CMAKE_REQUIRED_INCLUDES)
    set(CMAKE_REQUIRED_LIBRARIES)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS_BACKUP}")
  endif(LIBICONV_LIB_FOUND)

endif(LIBICONV_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libiconv  DEFAULT_MSG  LIBICONV_INCLUDE_DIR  LIBICONV_LIB_FOUND)

mark_as_advanced(LIBICONV_INCLUDE_DIR  LIBICONV_LIBRARIES  LIBICONV_LIBC_HAS_ICONV_OPEN  LIBICONV_LIB_FOUND)
