include_directories (
    ${PROJECT_BINARY_DIR}
    ${FCITX4_FCITX_UTILS_INCLUDE_DIRS}
    ${FCITX4_FCITX_CONFIG_INCLUDE_DIRS}
    ${FCITX4_FCITX_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
    ${X11_Xkb_INCLUDE_PATH}
    ${X11_Xkblib_INCLUDE_PATH}
    ${LIBINTL_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
    )

set(fcitx_xkb_SOURCES
    xkb.c
    rules.c
    xkbconfig.c
)

fcitx_add_addon(fcitx-xkb ${fcitx_xkb_SOURCES})
target_link_libraries(fcitx-xkb ${X11_X11_LIB} ${XKBFILE_LIBRARIES} ${LIBXML2_LIBRARIES})

fcitx_add_addon_conf_file(fcitx-xkb.conf)
fcitx_add_configdesc_file(fcitx-xkb.desc)