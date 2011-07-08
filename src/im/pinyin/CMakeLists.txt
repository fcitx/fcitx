
set(FCITX_PINYIN_SOURCES
    py.c
    PYFA.c
    pyMapTable.c
    pyParser.c
    sp.c
    pyconfig.c
)

add_library(fcitx-pinyin MODULE ${FCITX_PINYIN_SOURCES})
set_target_properties( fcitx-pinyin PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")

install(TARGETS fcitx-pinyin DESTINATION ${addondir})
install(FILES fcitx-pinyin.conf DESTINATION ${addonconfigdir})
install(FILES fcitx-pinyin.desc DESTINATION ${configdescdir})
