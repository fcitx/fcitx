include_directories (
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src/module/xkb
    ${LIBINTL_INCLUDE_DIR}
    ${LIBICONV_INCLUDE_DIR}
    ${ICU_INCLUDE_DIRS}
    )

if (ENABLE_LIBXML2)
    include_directories(${LIBXML2_INCLUDE_DIR})
endif (ENABLE_LIBXML2)

set(fcitx_keyboard_SOURCES
    keyboard.c
    keyboardconfig.c
)

if (ENABLE_LIBXML2)
    set(fcitx_keyboard_SOURCES ${fcitx_keyboard_SOURCES} isocodes.c)
endif (ENABLE_LIBXML2)

fcitx_add_addon(fcitx-keyboard ${fcitx_keyboard_SOURCES})
target_link_libraries(fcitx-keyboard ${LIBICONV_LIBRARIES} ${ICU_LIBRARIES})
if(ENABLE_LIBXML2)
  target_link_libraries(fcitx-keyboard ${LIBXML2_LIBRARIES})
endif(ENABLE_LIBXML2)

fcitx_add_addon_conf_file(fcitx-keyboard.conf)
fcitx_add_configdesc_file(fcitx-keyboard.desc)
