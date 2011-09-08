if(ENABLE_GTK2_IM_MODULE)

PKG_CHECK_MODULES(GTK2 REQUIRED "gtk+-2.0" )

_pkgconfig_invoke("gtk+-2.0" GTK2 BINARY_VERSION "" "--variable=gtk_binary_version")
set(GTK2_IM_MODULEDIR
 "${libdir}/gtk-2.0/${GTK2_BINARY_VERSION}/immodules"
)

include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${DBUS_GLIB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GTK2_LIBRARY_DIRS} ${DBUS_GLIB_LIBRARY_DIRS})

add_custom_command(OUTPUT marshall.c
                   COMMAND ${GLIB_GENMARSHAL} --body --prefix=fcitx_marshall ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.c
)
add_custom_command(OUTPUT marshall.h
                   COMMAND ${GLIB_GENMARSHAL} --header --prefix=fcitx_marshall ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.h
)

set(FCITX_GTK2_IM_MODULE_SOURCES
    fcitxim.c
    fcitximcontext.c
    client.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.h
)

add_library(im-fcitx MODULE ${FCITX_GTK2_IM_MODULE_SOURCES})
set_target_properties( im-fcitx PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden" LINK_FLAGS "-Wl,--no-undefined")
target_link_libraries( im-fcitx ${GTK2_LIBRARIES} ${DBUS_GLIB_LIBRARIES} fcitx-utils)

install(TARGETS im-fcitx DESTINATION ${GTK2_IM_MODULEDIR})

endif(ENABLE_GTK2_IM_MODULE)
