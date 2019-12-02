set(FCITX_GTK2_IM_MODULE_GIO_SOURCES
  fcitxim.c
  fcitximcontext.c
  )

fcitx_translate_add_sources(
  ${FCITX_GTK2_IM_MODULE_GIO_SOURCES})

if(NOT ENABLE_GTK2_IM_MODULE)
  return()
endif()

PKG_CHECK_MODULES(GTK2 REQUIRED "gtk+-2.0" )
_pkgconfig_invoke("gtk+-2.0" GTK2 BINARY_VERSION
  "" "--variable=gtk_binary_version")

if (NOT DEFINED GTK2_IM_MODULEDIR)
    set(GTK2_IM_MODULEDIR "${LIB_INSTALL_DIR}/gtk-2.0/${GTK2_BINARY_VERSION}/immodules" CACHE PATH "Gtk2 im module directory")
endif()

include_directories(${GTK2_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GTK2_LIBRARY_DIRS})

include_directories(${GIO2_INCLUDE_DIRS})
link_directories(${GIO2_LIBRARY_DIRS})

set(FCITX_GTK2_IM_MODULE_SOURCES ${FCITX_GTK2_IM_MODULE_GIO_SOURCES})

add_library(im-fcitx MODULE ${FCITX_GTK2_IM_MODULE_SOURCES})
set_target_properties( im-fcitx PROPERTIES PREFIX ""
  COMPILE_FLAGS "-fvisibility=hidden" LINK_FLAGS "-Wl,--no-undefined")
target_link_libraries(im-fcitx XKBCommon::XKBCommon ${GTK2_LIBRARIES} ${GIO2_LIBRARIES}
    fcitx-gclient fcitx-utils)
install(TARGETS im-fcitx DESTINATION "${GTK2_IM_MODULEDIR}")
