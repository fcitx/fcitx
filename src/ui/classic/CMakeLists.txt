include_directories(${fcitx_SOURCE_DIR}/src)

set(fcitx_classic_ui_SOURCES
    classicui.c
    classicuiconfig.c
    InputWindow.c
    MainWindow.c
    skinconfig.c
    skin.c
)

set(CLASSICUI_DESC_FILES
    fcitx-classic-ui.desc
    skin.desc
)

add_library(fcitx-classic-ui MODULE ${fcitx_classic_ui_SOURCES})
install(TARGETS fcitx-classic-ui DESTINATION ${addondir})
set_target_properties( fcitx-classic-ui PROPERTIES PREFIX "")
target_link_libraries( fcitx-classic-ui ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES} fcitx-config )

install(FILES fcitx-classic-ui.conf DESTINATION ${pkgdatadir}/data/addon)
install(FILES ${CLASSICUI_DESC_FILES} DESTINATION ${pkgdatadir}/data)