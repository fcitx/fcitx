include_directories(${fcitx_SOURCE_DIR}/src)

set(FCITX_X11_SOURCES
    xerrorhandler.c
    x11stuff.c
)

add_library(fcitx-x11 MODULE ${FCITX_X11_SOURCES})
install(TARGETS fcitx-x11 DESTINATION ${libdir}/fcitx/)
set_target_properties( fcitx-x11 PROPERTIES PREFIX "")
target_link_libraries( fcitx-x11 ${X11_LIBRARIES} )

install(FILES fcitx-x11.conf DESTINATION share/fcitx/data/addon)

