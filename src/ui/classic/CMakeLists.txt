include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CAIRO_XLIB_INCLUDE_DIRS})

if (_ENABLE_PANGO)
    include_directories ( ${PANGOCAIRO_INCLUDE_DIRS} )
else (_ENABLE_PANGO)
    include_directories ( ${FONTCONFIG_INCLUDE_DIRS} )
endif (_ENABLE_PANGO)

set(fcitx_classic_ui_SOURCES
    classicui.c
    classicuiconfig.c
    InputWindow.c
    MainWindow.c
    MenuWindow.c
    MessageWindow.c
    AboutWindow.c
    TrayWindow.c
    tray.c
    skinconfig.c
    skin.c
    ../cairostuff/font.c
    ../cairostuff/cairostuff.c
)

set(CLASSICUI_DESC_FILES
    fcitx-classic-ui.desc
    skin.desc
)

add_library(fcitx-classic-ui MODULE ${fcitx_classic_ui_SOURCES})
install(TARGETS fcitx-classic-ui DESTINATION ${addondir})
set_target_properties( fcitx-classic-ui PROPERTIES PREFIX "")
target_link_libraries( fcitx-classic-ui ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES} fcitx-config fcitx-core )

if (_ENABLE_PANGO)
    target_link_libraries( fcitx-classic-ui ${PANGOCAIRO_LIBRARIES} )
else (_ENABLE_PANGO)
    target_link_libraries( fcitx-classic-ui ${FONTCONFIG_LIBRARIES} )
endif (_ENABLE_PANGO)
install(FILES fcitx-classic-ui.conf DESTINATION ${addonconfigdir})
install(FILES ${CLASSICUI_DESC_FILES} DESTINATION ${configdescdir})
install(FILES classicuiinterface.h DESTINATION ${includedir}/fcitx/module/classicui/)