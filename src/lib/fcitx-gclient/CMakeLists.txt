set(FCITX_GCLIENT_SOURCES
  fcitxclient.c
  fcitxinputmethod.c
  fcitxkbd.c
  fcitxconnection.c
  #fcitxdbusproxy.c
  )

set(FCITX_GCLIENT_BUILT_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/marshall.c
  )

set(FCITX_GCLIENT_HEADERS
  fcitxclient.h
  fcitxinputmethod.h
  fcitxkbd.h
  #fcitxdbusproxy.h
  fcitxconnection.h
  )

set(FCITX_GCLIENT_BUILT_HEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/marshall.h
  )

fcitx_translate_add_sources(${FCITX_GCLIENT_SOURCES}
  ${FCITX_GCLIENT_HEADERS})

if(NOT (_ENABLE_DBUS AND ENABLE_GLIB2 AND GIO2_FOUND))
  return()
endif()

_pkgconfig_invoke("glib-2.0" GLIB2 GLIB_GENMARSHAL
  "" "--variable=glib_genmarshal")
FIND_PROGRAM(GLIB_GENMARSHAL ${GLIB2_GLIB_GENMARSHAL})

include_directories(${GIO2_INCLUDE_DIRS})
include_directories(${DBUS_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GIO2_LIBRARY_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

add_custom_command(OUTPUT marshall.c
  COMMAND ${GLIB_GENMARSHAL} --body --prefix=fcitx_marshall
  ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.c
  DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list)
add_custom_command(OUTPUT marshall.h
  COMMAND ${GLIB_GENMARSHAL} --header --prefix=fcitx_marshall
  ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.h
  DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list)

add_library(fcitx-gclient SHARED ${FCITX_GCLIENT_SOURCES}
  ${FCITX_GCLIENT_BUILT_SOURCES} ${FCITX_GCLIENT_BUILT_HEADERS})
set_target_properties(fcitx-gclient
  PROPERTIES VERSION 0.1
  SOVERSION 0
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined")
target_link_libraries(fcitx-gclient ${GIO2_LIBRARIES}
  ${DBUS_LIBRARIES} fcitx-utils)
configure_file(fcitx-gclient.pc.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-gclient.pc)

install(TARGETS fcitx-gclient LIBRARY DESTINATION "${libdir}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-gclient.pc
  DESTINATION "${pkgconfdir}")
install(FILES ${FCITX_GCLIENT_HEADERS}
  DESTINATION "${includedir}/fcitx-gclient")

if(ENABLE_GIR)
  include(GObjectIntrospection)
  gobject_introspection(
    FILENAME Fcitx-1.0.gir
    NSVERSION 1.0
    INCLUDE Gio-2.0 GObject-2.0 GLib-2.0
    PACKAGE_EXPORT fcitx-gclient
    LIBRARY fcitx-gclient
    SCANNER_ARGS --warn-all --add-include-path=${CMAKE_CURRENT_SOURCE_DIR}
    COMPILER_ARGS "--includedir=${CMAKE_CURRENT_SOURCE_DIR}"
    SYMBOL_PREFIXES fcitx
    SOURCES ${FCITX_GCLIENT_SOURCES} ${FCITX_GCLIENT_HEADERS}
    QUIET
    )
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Fcitx-1.0.gir"
    DESTINATION "${GOBJECT_INTROSPECTION_GIRDIR}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/Fcitx-1.0.typelib"
    DESTINATION "${GOBJECT_INTROSPECTION_TYPELIBDIR}")
endif()

if(ENABLE_STATIC)
  add_library(fcitx-gclient-static STATIC ${FCITX_GCLIENT_SOURCES})
  set_target_properties(fcitx-gclient-static PROPERTIES
    OUTPUT_NAME "fcitx-gclient"
    PREFIX "lib")
  install(TARGETS fcitx-gclient-static
    LIBRARY DESTINATION "${libdir}"
    ARCHIVE DESTINATION "${libdir}")
endif()

add_subdirectory(test)
