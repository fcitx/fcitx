
set(FCITX_PINYIN_SOURCES
    py.c
    PYFA.c
    pyMapTable.c
    pyParser.c
    sp.c
    pyconfig.c
)

fcitx_add_addon(fcitx-pinyin ${FCITX_PINYIN_SOURCES})
target_link_libraries( fcitx-pinyin fcitx-config fcitx-core)

fcitx_add_addon_header(pinyin pydef.h)
fcitx_add_configdesc_file(fcitx-pinyin.desc)
fcitx_add_addon_conf_file(fcitx-pinyin.conf)
