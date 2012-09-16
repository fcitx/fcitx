# - Try to find the Enchant libraries
# Once done this will define
#
#  ENCHANT_FOUND - system has ENCHANT
#  ENCHANT_INCLUDE_DIR - the ENCHANT include directory
#  ENCHANT_LIBRARIES - ENCHANT library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(ENCHANT_INCLUDE_DIR AND ENCHANT_LIBRARIES)
  # Already in cache, be silent
  set(ENCHANT_FIND_QUIETLY TRUE)
endif(ENCHANT_INCLUDE_DIR AND ENCHANT_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_ENCHANT enchant)

find_path(ENCHANT_INCLUDE_DIR
  NAMES enchant.h
  HINTS ${PC_ENCHANT_INCLUDE_DIRS}
  PATH_SUFFIXES "enchant")

find_library(ENCHANT_LIBRARIES
  NAMES enchant
  HINTS ${PC_ENCHANT_LIBRARY_DIRS})

if(ENCHANT_INCLUDE_DIR AND ENCHANT_LIBRARIES)
  check_c_compiler_flag("-Werror" ENCHANT_HAVE_WERROR)
  set(CMAKE_C_FLAGS_BACKUP "${CMAKE_C_FLAGS}")
  if(ENCHANT_HAVE_WERROR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  endif(ENCHANT_HAVE_WERROR)
  set(CMAKE_REQUIRED_INCLUDES "${ENCHANT_INCLUDE_DIR}")
  set(CMAKE_REQUIRED_LIBRARIES "${ENCHANT_LIBRARIES}")
  check_c_source_compiles("
  #include <stdlib.h>
  #include <stddef.h>
  #include <string.h>
  #include <enchant/enchant.h>

  EnchantBroker *enchant_broker_init();
  char **enchant_dict_suggest(EnchantDict *dict, const char *str,
                              ssize_t len, size_t *out_n);
  void enchant_dict_free_string_list(EnchantDict *dict, char **str_list);
  void enchant_broker_free_dict(EnchantBroker *broker, EnchantDict *dict);
  void enchant_broker_free(EnchantBroker *broker);
  EnchantDict *enchant_broker_request_dict(EnchantBroker *broker,
                                           const char *const tag);
  void enchant_broker_set_ordering(EnchantBroker *broker,
                                   const char *const tag,
                                   const char *const ordering);
  void enchant_dict_add_to_personal(EnchantDict *dict, const char *const word,
                                    ssize_t len);

  int main()
  {
      void *enchant;
      void *dict;
      char **suggestions = NULL;
      size_t number;
      enchant = enchant_broker_init();
      enchant_broker_set_ordering(enchant, \"*\", \"aspell,myspell,ispell\");
      dict = enchant_broker_request_dict(enchant, \"en\");
      enchant_dict_add_to_personal(dict, \"Fcitx\", strlen(\"Fcitx\"));
      suggestions = enchant_dict_suggest(dict, \"Fcitx\", strlen(\"Fcitx\"),
                                         &number);
      enchant_dict_free_string_list(dict, suggestions);
      enchant_broker_free_dict(enchant, dict);
      enchant_broker_free(enchant);
      return 0;
  }
  " ENCHANT_API_COMPATIBLE)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_BACKUP}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Enchant DEFAULT_MSG ENCHANT_LIBRARIES
  ENCHANT_INCLUDE_DIR ENCHANT_API_COMPATIBLE)

mark_as_advanced(ENCHANT_INCLUDE_DIR ENCHANT_LIBRARIES PC_ENCHANT_INCLUDEDIR PC_ENCHANT_LIBDIR)
