cmake_minimum_required (VERSION 2.6)

project(fcitx)

set(version 4.1.2)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(ENABLE_CAIRO "Enable Cairo" ON)
option(ENABLE_DBUS "Enable DBus Support" ON)
option(ENABLE_PANGO "Enable Pango Support" ON)
option(ENABLE_DEBUG "Enable Debug" OFF)
option(ENABLE_TABLE "Enable Table IM" ON)
option(ENABLE_GTK2_IM_MODULE "Enable GTK2 IM Module" ON)
option(ENABLE_GTK3_IM_MODULE "Enable GTK3 IM Module" OFF)
option(ENABLE_QT_IM_MODULE "Enable Qt IM Module" OFF)
option(ENABLE_OPENCC "Enable OpenCC for Chinese Transform" ON)
option(ENABLE_STATIC "Build Static Library" OFF)
option(ENABLE_TEST "Test build" OFF)

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
include(FcitxMacro)

check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(malloc.h HAVE_MALLOC_H)
check_include_files(stdbool.h HAVE_STDBOOL_H)
check_function_exists(asprintf HAVE_ASPRINTF)

FIND_PACKAGE(Gettext REQUIRED)
FIND_PACKAGE(Libintl REQUIRED)
FIND_PACKAGE(Libiconv REQUIRED)
FIND_PACKAGE(Pthread REQUIRED)
FIND_PACKAGE(Libexecinfo REQUIRED)
FIND_PACKAGE(Libkvm)
FIND_PACKAGE(DL REQUIRED)
FIND_PROGRAM(WGET wget)
FIND_PROGRAM(TAR tar)

FIND_PACKAGE(X11 REQUIRED)

if (ENABLE_TEST)
    ENABLE_TESTING()
endif (ENABLE_TEST)

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
    PKG_CHECK_MODULES(DBUS "dbus-1 >= 1.1.0" )
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

if(ENABLE_DEBUG)
    set(_DEBUG)
endif(ENABLE_DEBUG)

set(package     fcitx)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(bindir ${exec_prefix}/bin)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR ${exec_prefix}/lib)
endif()

set(libdir ${LIB_INSTALL_DIR})
set(addondir ${libdir}/${package} )
set(pkgdatadir ${prefix}/share/${package})
set(configdescdir ${pkgdatadir}/configdesc)
set(addonconfigdir ${pkgdatadir}/addon)
set(localedir ${CMAKE_INSTALL_PREFIX}/share/locale)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)

list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${LIB_INSTALL_DIR}" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_SKIP_BUILD_RPATH  FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${LIB_INSTALL_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif("${isSystemDir}" STREQUAL "-1")

# For FcitxMacro
set(FCITX4_ADDON_INSTALL_DIR ${addondir})
set(FCITX4_CONFIGDESC_INSTALL_DIR ${configdescdir})
set(FCITX4_ADDON_CONFIG_INSTALL_DIR ${addonconfigdir})
set(FCITX4_PACKAGE_NAME ${package})

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -fvisibility=hidden ${CMAKE_CXX_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--as-needed ${CMAKE_MODULE_LINKER_FLAGS}")
add_definitions("-D_GNU_SOURCE -DFCITX_HAVE_CONFIG_H")

configure_file(${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h)
configure_file("${PROJECT_SOURCE_DIR}/cmake/FcitxConfigVersion.cmake.in"
               "${PROJECT_BINARY_DIR}/cmake/FcitxConfigVersion.cmake"
                IMMEDIATE @ONLY
)

include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(skin)
add_subdirectory(tools)
add_subdirectory(data)
add_subdirectory(po)
add_subdirectory(cmake)

if (ENABLE_TEST)
    add_subdirectory(test)
endif (ENABLE_TEST)
