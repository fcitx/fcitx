set(FCITX_GTK2_IM_MODULE_GIO_SOURCES
  fcitxim.c
  fcitximcontext-gio.c
  )
set(FCITX_GTK2_IM_MODULE_NO_GIO_SOURCES
  fcitxim.c
  fcitximcontext.c
  client.c
  )

fcitx_translate_add_sources(${FCITX_GTK2_IM_MODULE_NO_GIO_SOURCES}
  ${FCITX_GTK2_IM_MODULE_GIO_SOURCES})

set(FCITX_GTK2_IM_MODULE_NO_GIO_SOURCES
  ${FCITX_GTK2_IM_MODULE_NO_GIO_SOURCES}
  "${CMAKE_CURRENT_BINARY_DIR}/marshall.c"
  "${CMAKE_CURRENT_BINARY_DIR}/marshall.h"
  )

if(NOT ENABLE_GTK2_IM_MODULE)
  return()
endif()

PKG_CHECK_MODULES(GTK2 REQUIRED "gtk+-2.0" )
_pkgconfig_invoke("gtk+-2.0" GTK2 BINARY_VERSION
  "" "--variable=gtk_binary_version")
_pkgconfig_invoke("gtk+-2.0" GTK2 LIBDIR "" "--variable=libdir")
set(GTK2_IM_MODULEDIR
  "${GTK2_LIBDIR}/gtk-2.0/${GTK2_BINARY_VERSION}/immodules")

include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GTK2_LIBRARY_DIRS})

add_custom_command(OUTPUT marshall.c
  COMMAND ${GLIB_GENMARSHAL} --body --prefix=fcitx_marshall
  ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.c
  DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list)
add_custom_command(OUTPUT marshall.h
  COMMAND ${GLIB_GENMARSHAL} --header --prefix=fcitx_marshall
  ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.h
  DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list)

if(GIO2_FOUND)
  include_directories(${GIO2_INCLUDE_DIRS})
  link_directories(${GIO2_LIBRARY_DIRS})

  set(FCITX_GTK2_IM_MODULE_SOURCES ${FCITX_GTK2_IM_MODULE_GIO_SOURCES})
else()
  include_directories(${DBUS_GLIB_INCLUDE_DIRS})
  link_directories(${DBUS_GLIB_LIBRARY_DIRS})

  set(FCITX_GTK2_IM_MODULE_SOURCES ${FCITX_GTK2_IM_MODULE_NO_GIO_SOURCES})
endif()

add_library(im-fcitx MODULE ${FCITX_GTK2_IM_MODULE_SOURCES})
set_target_properties( im-fcitx PROPERTIES PREFIX ""
  COMPILE_FLAGS "-fvisibility=hidden" LINK_FLAGS "-Wl,--no-undefined")
target_link_libraries(im-fcitx ${GTK2_LIBRARIES} fcitx-utils)
if(GIO2_FOUND)
  target_link_libraries(im-fcitx ${GTK2_LIBRARIES} ${GIO2_LIBRARIES}
    fcitx-gclient fcitx-utils)
else()
  target_link_libraries(im-fcitx ${GTK2_LIBRARIES} ${DBUS_GLIB_LIBRARIES}
    fcitx-utils)
endif()
install(TARGETS im-fcitx DESTINATION "${GTK2_IM_MODULEDIR}")
