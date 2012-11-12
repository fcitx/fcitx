if(NOT ENABLE_GTK3_IM_MODULE)
  return()
endif()
PKG_CHECK_MODULES(GTK3 REQUIRED "gtk+-3.0" )

_pkgconfig_invoke("gtk+-3.0" GTK3 BINARY_VERSION
  "" "--variable=gtk_binary_version")
_pkgconfig_invoke("gtk+-3.0" GTK3 LIBDIR "" "--variable=libdir")
set(GTK3_IM_MODULEDIR
  "${GTK3_LIBDIR}/gtk-3.0/${GTK3_BINARY_VERSION}/immodules")

include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GTK3_LIBRARY_DIRS})

if(GIO2_FOUND)
  include_directories(${GIO2_INCLUDE_DIRS})
  link_directories(${GIO2_LIBRARY_DIRS})

  set(FCITX_GTK3_IM_MODULE_SOURCES
    fcitxim.c
    fcitximcontext-gio.c)
else()
  include_directories(${DBUS_GLIB_INCLUDE_DIRS})
  link_directories(${DBUS_GLIB_LIBRARY_DIRS})

  add_custom_command(OUTPUT marshall.c
    COMMAND ${GLIB_GENMARSHAL} --body --prefix=fcitx_marshall
    ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.c
    DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list)
  add_custom_command(OUTPUT marshall.h
    COMMAND ${GLIB_GENMARSHAL} --header --prefix=fcitx_marshall
    ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.h
    DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list)

  set(FCITX_GTK3_IM_MODULE_SOURCES
    fcitxim.c
    fcitximcontext.c
    client.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.h
    )
endif()

add_library(im-fcitx-gtk3 MODULE ${FCITX_GTK3_IM_MODULE_SOURCES})
set_target_properties( im-fcitx-gtk3 PROPERTIES PREFIX ""
  COMPILE_FLAGS "-fvisibility=hidden" OUTPUT_NAME "im-fcitx"
  LINK_FLAGS "-Wl,--no-undefined")

if(GIO2_FOUND)
  target_link_libraries(im-fcitx-gtk3 ${GTK3_LIBRARIES} ${GIO2_LIBRARIES}
    fcitx-gclient fcitx-utils)
else()
  target_link_libraries(im-fcitx-gtk3 ${GTK3_LIBRARIES}
    ${DBUS_GLIB_LIBRARIES} fcitx-utils)
endif()
install(TARGETS im-fcitx-gtk3 DESTINATION "${GTK3_IM_MODULEDIR}")
