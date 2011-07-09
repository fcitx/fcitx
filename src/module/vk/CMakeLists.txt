include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${CAIRO_XLIB_INCLUDE_DIRS})
if (_ENABLE_PANGO)
    include_directories ( ${PANGOCAIRO_INCLUDE_DIRS} )
else (_ENABLE_PANGO)
    include_directories ( ${FONTCONFIG_INCLUDE_DIRS} )
endif (_ENABLE_PANGO)

set(FCITX_VK_SOURCES
    vk.c
    ../../ui/cairostuff/cairostuff.c
    ../../ui/cairostuff/font.c
)

add_library(fcitx-vk MODULE ${FCITX_VK_SOURCES})
install(TARGETS fcitx-vk DESTINATION ${addondir})
set_target_properties( fcitx-vk PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-vk ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES} fcitx-config fcitx-core )
if (_ENABLE_PANGO)
    target_link_libraries( fcitx-vk ${PANGOCAIRO_LIBRARIES} )
else (_ENABLE_PANGO)
    target_link_libraries( fcitx-vk ${FONTCONFIG_LIBRARIES} )
endif (_ENABLE_PANGO)

install(FILES fcitx-vk.conf DESTINATION ${addonconfigdir})