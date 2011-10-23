
set(FCITX_PINYIN_SOURCES
    py.c
    PYFA.c
    pyMapTable.c
    pyParser.c
    sp.c
    pyconfig.c
)

fcitx_add_addon(fcitx-pinyin ${FCITX_PINYIN_SOURCES})

fcitx_add_addon_header(pinyin pydef.h)
fcitx_add_configdesc_file(fcitx-pinyin.desc)
fcitx_add_addon_conf_file(fcitx-pinyin.conf)
fcitx_add_inputmethod_conf_file(pinyin.conf)
fcitx_add_inputmethod_conf_file(shuangpin.conf)
