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

set(FCITX_VK_SOURCES
    vk.c
    ../../ui/cairostuff/cairostuff.c
    ../../ui/cairostuff/font.c
)

fcitx_add_addon(fcitx-vk ${FCITX_VK_SOURCES})
target_link_libraries( fcitx-vk ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES})
if (_ENABLE_PANGO)
    target_link_libraries( fcitx-vk ${PANGOCAIRO_LIBRARIES} )
else (_ENABLE_PANGO)
    target_link_libraries( fcitx-vk ${FONTCONFIG_LIBRARIES} )
endif (_ENABLE_PANGO)

fcitx_add_addon_conf_file(fcitx-vk.conf)