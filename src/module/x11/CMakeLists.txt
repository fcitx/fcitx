include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${X11_X11_INCLUDE_PATH})

set(FCITX_X11_SOURCES
    xerrorhandler.c
    x11stuff.c
)

fcitx_add_addon(fcitx-x11 ${FCITX_X11_SOURCES})
target_link_libraries( fcitx-x11 ${X11_LIBRARIES} ${X11_Xrender_LIB})

if (HAVE_XINERAMA)
    target_link_libraries( fcitx-x11 ${X11_Xinerama_LIB} )
endif (HAVE_XINERAMA)

fcitx_add_addon_header(x11 x11stuff.h)
fcitx_add_addon_conf_file(fcitx-x11.conf)