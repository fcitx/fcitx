
set(FCITX_TABLE_SOURCES
    table.c
    tableconfig.c
)

fcitx_add_addon(fcitx-table ${FCITX_TABLE_SOURCES})
target_link_libraries( fcitx-table fcitx-config fcitx-core)
fcitx_add_addon_conf_file(fcitx-table.conf)
fcitx_add_configdesc_file(table.desc)