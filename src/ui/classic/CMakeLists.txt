include_directories(${fcitx_SOURCE_DIR}/src)

set(fcitx_classic_ui_SOURCES
    AboutWindow.c
    font.c
    InputWindow.c
    MainWindow.c
    MenuWindow.c
    MessageWindow.c
    skin.c
    skinconfig.c
    tray.c
    TrayWindow.c
    ui.c
)

add_library(fcitx-classic-ui MODULE ${fcitx_classic_ui_SOURCES})
install(TARGETS fcitx-classic-ui DESTINATION ${libdir}/fcitx/)
set_target_properties( fcitx-classic-ui PROPERTIES PREFIX "")
target_link_libraries( fcitx-classic-ui ${X11_LIBRARIES} fcitx-config )

install(FILES fcitx-classic-ui.conf DESTINATION share/fcitx/data/addon)

