include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_UNICODE_SOURCES
    unicode.c
    charselectdata.c
)

fcitx_add_addon(fcitx-unicode ${FCITX_UNICODE_SOURCES})
fcitx_add_configdesc_file(fcitx-unicode.desc)
fcitx_add_addon_conf_file(fcitx-unicode.conf)

install(FILES charselectdata DESTINATION ${pkgdatadir}/data/)
