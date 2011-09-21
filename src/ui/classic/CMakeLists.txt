include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CAIRO_XLIB_INCLUDE_DIRS})
link_directories(${CAIRO_XLIB_LIBRARY_DIRS})

if (_ENABLE_PANGO)
    include_directories ( ${PANGOCAIRO_INCLUDE_DIRS} )
    link_directories(${PANGOCAIRO_LIBRARY_DIRS})
else (_ENABLE_PANGO)
    include_directories ( ${FONTCONFIG_INCLUDE_DIRS} )
    link_directories(${FONTCONFIG_LIBRARY_DIRS})
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

fcitx_add_addon(fcitx-classic-ui ${fcitx_classic_ui_SOURCES})
target_link_libraries( fcitx-classic-ui ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES} )

if (_ENABLE_PANGO)
    target_link_libraries( fcitx-classic-ui ${PANGOCAIRO_LIBRARIES} )
else (_ENABLE_PANGO)
    target_link_libraries( fcitx-classic-ui ${FONTCONFIG_LIBRARIES} )
endif (_ENABLE_PANGO)
fcitx_add_addon_header(classicui classicuiinterface.h)
fcitx_add_addon_conf_file(fcitx-classic-ui.conf)
fcitx_add_configdesc_file(${CLASSICUI_DESC_FILES})

add_custom_target(installer-desktopfile ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fcitx-skin-installer.desktop)

install(FILES fcitx-skin-installer.sh RENAME fcitx-skin-installer DESTINATION ${bindir} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
intltool_merge_translation(${CMAKE_CURRENT_SOURCE_DIR}/fcitx-skin-installer.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-skin-installer.desktop )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-skin-installer.desktop DESTINATION share/applications/)
install(FILES x-fskin.xml DESTINATION share/mime/packages/ )