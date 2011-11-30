
set(FCITX_QUWEI_SOURCES
    qw.c
)

fcitx_add_addon(fcitx-qw ${FCITX_QUWEI_SOURCES})
fcitx_add_addon_conf_file(fcitx-qw.conf)
fcitx_add_inputmethod_conf_file(qw.conf)