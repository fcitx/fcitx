
set(FCITX_TABLE_SOURCES
    table.c
    tableconfig.c
)

add_library(fcitx-table MODULE ${FCITX_TABLE_SOURCES})
set_target_properties( fcitx-table PROPERTIES PREFIX "")

install(TARGETS fcitx-table DESTINATION ${libdir}/fcitx/)
install(FILES fcitx-table.conf DESTINATION share/fcitx/data/addon)
install(FILES table.desc DESTINATION share/fcitx/data/addon)
