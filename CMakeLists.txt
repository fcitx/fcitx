cmake_minimum_required (VERSION 2.6)

project(fcitx)

set(version 4.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(ENABLE_CAIRO "Enable Cairo" ON)
option(ENABLE_DBUS "Enable DBus Support" ON)
option(ENABLE_PANGO "Enable Pango Support" ON)
option(ENABLE_DEBUG "Enable Debug" ON)
option(ENABLE_TABLE "Enable Table IM" ON)
option(ENABLE_GTK2_IM_MODULE "Enable GTK2 IM Module" OFF)
option(ENABLE_GTK3_IM_MODULE "Enable GTK3 IM Module" OFF)
option(ENABLE_OPENCC "Enable OpenCC for Chinese Transform" ON)
option(ENABLE_STATIC "Build Static Library" OFF)

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

set(_ENABLE_CAIRO 0)
set(_ENABLE_PANGO 0)
if (ENABLE_CAIRO)
    PKG_CHECK_MODULES(CAIRO_XLIB REQUIRED "cairo-xlib >= 1.0" )
    if (CAIRO_XLIB_FOUND)
        set(_ENABLE_CAIRO 1)
    endif (CAIRO_XLIB_FOUND)
endif (ENABLE_CAIRO)

set(_ENABLE_PANGO 0)
if (_ENABLE_CAIRO)
    if (ENABLE_PANGO)
        PKG_CHECK_MODULES(PANGOCAIRO "pangocairo >= 1.0" )
        if (PANGOCAIRO_FOUND)
            set(_ENABLE_PANGO 1)
        endif (PANGOCAIRO_FOUND)
    endif (ENABLE_PANGO)
    if (NOT _ENABLE_PANGO)
        PKG_CHECK_MODULES(FONTCONFIG REQUIRED "fontconfig")
    endif (NOT _ENABLE_PANGO)
endif(_ENABLE_CAIRO)

set(_ENABLE_DBUS 0)
if (ENABLE_DBUS)
    PKG_CHECK_MODULES(DBUS "dbus-1 >= 1.4.0" )
    if (DBUS_FOUND)
        set(_ENABLE_DBUS 1)
    endif (DBUS_FOUND)
endif (ENABLE_DBUS)

set(_ENABLE_OPENCC 0)
if (ENABLE_OPENCC)
    PKG_CHECK_MODULES(OPENCC "opencc >= 0.1.1" )
    if (OPENCC_FOUND)
        set(_ENABLE_OPENCC 1)
    endif (OPENCC_FOUND)
endif (ENABLE_OPENCC)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if(ENABLE_DEBUG)
    set(_DEBUG)
endif(ENABLE_DEBUG)

list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${LIB_INSTALL_DIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
    set(CMAKE_INSTALL_RPATH "${LIB_INSTALL_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif("${isSystemDir}" STREQUAL "-1")

set(package     fcitx)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(bindir ${prefix}/bin)
set(libdir ${LIB_INSTALL_DIR})
set(addondir ${libdir}/${package} )
set(pkgdatadir ${CMAKE_INSTALL_PREFIX}/share/${package})
set(configdescdir ${pkgdatadir}/configdesc)
set(addonconfigdir ${pkgdatadir}/addon)
set(localedir ${CMAKE_INSTALL_PREFIX}/share/locale)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(CMAKE_C_FLAGS "-Wall -fvisibility=hidden")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined,--as-needed")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined,--as-needed")
add_definitions("-D_GNU_SOURCE -DFCITX_HAVE_CONFIG_H")

configure_file(${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h)

include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(skin)
add_subdirectory(tools)
add_subdirectory(data)
add_subdirectory(po)
