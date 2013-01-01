# - Try to find the Opencc libraries
# Once done this will define
#
#  OPENCC_FOUND - system has OPENCC
#  OPENCC_INCLUDE_DIR - the OPENCC include directory
#  OPENCC_LIBRARIES - OPENCC library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(OPENCC_INCLUDE_DIR AND OPENCC_LIBRARIES)
  # Already in cache, be silent
  set(OPENCC_FIND_QUIETLY TRUE)
endif(OPENCC_INCLUDE_DIR AND OPENCC_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_OPENCC opencc)

find_path(OPENCC_INCLUDE_DIR
  NAMES opencc.h
  HINTS ${PC_OPENCC_INCLUDE_DIRS}
)

find_library(OPENCC_LIBRARIES
  NAMES opencc
  HINTS ${PC_OPENCC_LIBDIR})

if(OPENCC_INCLUDE_DIR AND OPENCC_LIBRARIES)
  check_c_compiler_flag("-Werror" OPENCC_HAVE_WERROR)
  set(CMAKE_C_FLAGS_BACKUP "${CMAKE_C_FLAGS}")
  if(OPENCC_HAVE_WERROR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  endif(OPENCC_HAVE_WERROR)
  set(CMAKE_REQUIRED_INCLUDES "${OPENCC_INCLUDE_DIR}")
  set(CMAKE_REQUIRED_LIBRARIES "${OPENCC_LIBRARIES}")
  check_c_source_compiles("
  #include <opencc.h>

  opencc_t opencc_open(const char* config_file);
  char* opencc_convert_utf8(opencc_t opencc, const char* inbuf, size_t length);

  int main()
  {
      return 0;
  }
  " OPENCC_API_COMPATIBLE)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_BACKUP}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenCC DEFAULT_MSG OPENCC_LIBRARIES
  OPENCC_INCLUDE_DIR OPENCC_API_COMPATIBLE)

mark_as_advanced(OPENCC_INCLUDE_DIR OPENCC_LIBRARIES PC_OPENCC_INCLUDEDIR PC_OPENCC_LIBDIR)
