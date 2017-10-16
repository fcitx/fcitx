if(NOT ENABLE_GTK3_IM_MODULE)
  return()
endif()
PKG_CHECK_MODULES(GTK3 REQUIRED "gtk+-3.0" )

_pkgconfig_invoke("gtk+-3.0" GTK3 BINARY_VERSION
  "" "--variable=gtk_binary_version")

if (NOT DEFINED GTK3_IM_MODULEDIR)
    set(GTK3_IM_MODULEDIR "${LIB_INSTALL_DIR}/gtk-3.0/${GTK3_BINARY_VERSION}/immodules" CACHE PATH "Gtk3 im module directory")
endif()

include_directories(${GTK3_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GTK3_LIBRARY_DIRS})

include_directories(${GIO2_INCLUDE_DIRS})
link_directories(${GIO2_LIBRARY_DIRS})

set(FCITX_GTK3_IM_MODULE_SOURCES
  fcitxim.c
  fcitximcontext.c)

add_library(im-fcitx-gtk3 MODULE ${FCITX_GTK3_IM_MODULE_SOURCES})
set_target_properties( im-fcitx-gtk3 PROPERTIES PREFIX ""
  COMPILE_FLAGS "-fvisibility=hidden" OUTPUT_NAME "im-fcitx"
  LINK_FLAGS "-Wl,--no-undefined")

target_link_libraries(im-fcitx-gtk3 XKBCommon::XKBCommon ${GTK3_LIBRARIES} ${GIO2_LIBRARIES}
    fcitx-gclient fcitx-utils)
install(TARGETS im-fcitx-gtk3 DESTINATION "${GTK3_IM_MODULEDIR}")
