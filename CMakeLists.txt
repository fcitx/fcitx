cmake_minimum_required (VERSION 2.6)

project(fcitx)

set(version 4.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(FindPkgConfig)
include(UseDoxygen OPTIONAL)

check_include_files(malloc.h HAVE_MALLOC_H)
check_include_files(stdbool.h HAVE_STDBOOL_H)
check_function_exists(asprintf HAVE_ASPRINTF)

FIND_PACKAGE(Gettext REQUIRED)
FIND_PROGRAM(INTLTOOL_EXTRACT intltool-extract)
FIND_PROGRAM(INTLTOOL_UPDATE intltool-update)
FIND_PROGRAM(INTLTOOL_MERGE intltool-merge)
FIND_PROGRAM(WGET wget)
FIND_PROGRAM(TAR tar) 

FIND_PACKAGE(X11 REQUIRED)

PKG_CHECK_MODULES(CAIRO_XLIB REQUIRED "cairo-xlib >= 1.0" )
PKG_CHECK_MODULES(PANGOCAIRO "pangocairo >= 1.0" )
PKG_CHECK_MODULES(OPENCC "opencc >= 0.1.1" )
if (${PANGOCAIRO_FOUND})
    set(_ENABLE_PANGO 1)
endif(${PANGOCAIRO_FOUND})

if (${OPENCC_FOUND})
    set(HAVE_OPENCC 1)
endif (${OPENCC_FOUND})

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

set(HAVE_CONFIG_H 1)

set(package     fcitx4)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(bindir ${prefix}/bin)
set(libdir ${LIB_INSTALL_DIR})
set(addondir ${libdir}/${package} )
set(pkgdatadir ${CMAKE_INSTALL_PREFIX}/share/${package})
set(localedir ${CMAKE_INSTALL_PREFIX}/share/locale)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
add_definitions("-D_GNU_SOURCE")

configure_file(${fcitx_SOURCE_DIR}/config.h.in ${fcitx_BINARY_DIR}/config.h)

include_directories(${fcitx_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(skin)
add_subdirectory(tools)
add_subdirectory(data)
add_subdirectory(po)