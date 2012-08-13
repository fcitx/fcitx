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

# so here is the api test for presage since fcitx-spell uses dlopen to load
# presage at runtime which would have the risk of api non-compatible.
# not really sure if this is the right place to put this test (since only a
# small fraction of api are tested) but I don't feel like this should be
# put in CMakeLists.txt of fcitx-spell either
if(PRESAGE_INCLUDE_DIR AND PRESAGE_LIBRARIES)
  check_c_compiler_flag("-Werror" PRESAGE_HAVE_WERROR)
  set(CMAKE_C_FLAGS_BACKUP "${CMAKE_C_FLAGS}")
  if(ICONV_HAVE_WERROR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  endif(ICONV_HAVE_WERROR)
  set(CMAKE_REQUIRED_INCLUDES "${PRESAGE_INCLUDE_DIR}")
  set(CMAKE_REQUIRED_LIBRARIES "${PRESAGE_LIBRARIES}")
  check_c_source_compiles("
  #include <stdlib.h>
  #include <presage.h>
  const char *get_stream(void *arg)
  {
      return \"a\";
  }
  int main()
  {
      presage_t presage;
      void *p;
      char **suggestions = NULL;
      char *result = NULL;
      presage_new(get_stream, NULL, get_stream, NULL, &presage);
      p = presage;
      presage_config_set(p, \"Presage.Selector.SUGGESTIONS\", \"10\");
      presage_predict(p, &suggestions);
      presage_completion(p, *suggestions, &result);
      presage_free_string(result);
      presage_free_string_array(suggestions);
      presage_free(p);
      return 0;
  }
  " PRESAGE_API_COMPATIBLE)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_BACKUP}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Presage DEFAULT_MSG PRESAGE_LIBRARIES
  PRESAGE_INCLUDE_DIR PRESAGE_API_COMPATIBLE)

mark_as_advanced(PRESAGE_INCLUDE_DIR PRESAGE_LIBRARIES)
