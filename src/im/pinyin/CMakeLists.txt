
set(FCITX_PINYIN_SOURCES
    py.c
    PYFA.c
    pyMapTable.c
    pyParser.c
    sp.c
    pyconfig.c
)

add_library(fcitx-pinyin MODULE ${FCITX_PINYIN_SOURCES})
set_target_properties( fcitx-pinyin PROPERTIES PREFIX "")

install(TARGETS fcitx-pinyin DESTINATION ${libdir}/fcitx/)
install(FILES fcitx-pinyin.conf DESTINATION share/fcitx/data/addon)
install(FILES fcitx-pinyin.desc DESTINATION share/fcitx/data/addon)
