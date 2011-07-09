
set(FCITX_TABLE_SOURCES
    table.c
    tableconfig.c
)

add_library(fcitx-table MODULE ${FCITX_TABLE_SOURCES})
set_target_properties( fcitx-table PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-table fcitx-config fcitx-core)

install(TARGETS fcitx-table DESTINATION ${addondir})
install(FILES fcitx-table.conf DESTINATION ${addonconfigdir})
install(FILES table.desc DESTINATION ${configdescdir})
