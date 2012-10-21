include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${X11_X11_INCLUDE_PATH})

set(FCITX_X11_SOURCES
    xerrorhandler.c
    x11stuff.c
    x11selection.c
)

fcitx_add_addon(fcitx-x11 ${FCITX_X11_SOURCES})
target_link_libraries( fcitx-x11 ${X11_LIBRARIES} ${X11_Xrender_LIB})

if (HAVE_XINERAMA)
    target_link_libraries( fcitx-x11 ${X11_Xinerama_LIB} )
endif (HAVE_XINERAMA)

if (HAVE_XFIXES)
    target_link_libraries( fcitx-x11 ${X11_Xfixes_LIB} )
endif (HAVE_XFIXES)

fcitx_scanner_addon(x11 fcitx-x11)
fcitx_add_addon_header(x11 x11stuff.h)
fcitx_add_addon_conf_file(fcitx-x11.conf)
