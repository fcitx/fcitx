if(ENABLE_GTK2_IM_MODULE)

PKG_CHECK_MODULES(GLIB2 "glib-2.0" )
PKG_CHECK_MODULES(GTK2 "gtk+-2.0" )
PKG_CHECK_MODULES(DBUS_GLIB "dbus-glib-1")

_pkgconfig_invoke("gtk+-2.0" GTK2 BINARY_VERSION "" "--variable=gtk_binary_version")
_pkgconfig_invoke("glib-2.0" GLIB2 GLIB_GENMARSHAL "" "--variable=glib_genmarshal")

FIND_PROGRAM(GLIB_GENMARSHAL ${GLIB2_GLIB_GENMARSHAL})

add_custom_command(OUTPUT marshall.c
                   COMMAND ${GLIB_GENMARSHAL} --body --prefix=fcitx_marshall ${CMAKE_CURRENT_SOURCE_DIR}/marshal.list > marshall.c
)

set(GTK2_IM_MODULEDIR
 "${libdir}/gtk-2.0/${GTK2_BINARY_VERSION}/immodules"
)

include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${DBUS_GLIB_INCLUDE_DIRS})

set(FCITX_GTK2_IM_MODULE_SOURCES
    fcitxim.c
    fcitximcontext.c
    client.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.c
)

add_library(im-fcitx MODULE ${FCITX_GTK2_IM_MODULE_SOURCES})
set_target_properties( im-fcitx PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( im-fcitx ${GTK2_LIBRARIES} ${DBUS_GLIB_LIBRARIES} fcitx-utils)

install(TARGETS im-fcitx DESTINATION ${GTK2_IM_MODULEDIR})

endif(ENABLE_GTK2_IM_MODULE)
