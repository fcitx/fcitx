
set(FCITX_TABLE_SOURCES
    tabledict.c
    tablepinyinwrapper.c
    table.c
    tableconfig.c
)

fcitx_add_addon(fcitx-table ${FCITX_TABLE_SOURCES})
fcitx_add_addon_conf_file(fcitx-table.conf)
fcitx_add_configdesc_file(table.desc)