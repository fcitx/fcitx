
set(FCITX_QUWEI_SOURCES
    qw.c
)

fcitx_add_addon(fcitx-qw ${FCITX_QUWEI_SOURCES})
target_link_libraries(fcitx-qw fcitx-core)
fcitx_add_addon_conf_file(fcitx-qw.conf)