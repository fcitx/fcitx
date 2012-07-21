include_directories (
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src/module/xkb
    ${LIBINTL_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBICONV_INCLUDE_DIR}
    ${ICU_INCLUDE_DIRS}
    )

set(fcitx_keyboard_SOURCES
    keyboard.c
    isocodes.c
    keyboardconfig.c
)

fcitx_add_addon(fcitx-keyboard ${fcitx_keyboard_SOURCES})
target_link_libraries(fcitx-keyboard ${LIBXML2_LIBRARIES}
  ${LIBICONV_LIBRARIES} ${ICU_LIBRARIES})

fcitx_add_addon_conf_file(fcitx-keyboard.conf)
fcitx_add_configdesc_file(fcitx-keyboard.desc)
