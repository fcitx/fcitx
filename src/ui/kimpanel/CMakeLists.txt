include_directories(${fcitx_SOURCE_DIR}/src)

set(fcitx_kimpanel_ui_SOURCES
    DBus.c
)

add_library(fcitx-kimpanel-ui MODULE ${fcitx_kimpanel_ui_SOURCES})
install(TARGETS fcitx-kimpanel-ui DESTINATION ${libdir}/fcitx/)
set_target_properties( fcitx-kimpanel-ui PROPERTIES PREFIX "")

install(FILES fcitx-kimpanel-ui.conf DESTINATION share/fcitx/data/addon)

