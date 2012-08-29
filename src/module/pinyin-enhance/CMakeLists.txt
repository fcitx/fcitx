include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_PYENHANCE_SOURCES
  pinyin-enhance.c
  pinyin-enhance-spell.c
)

fcitx_add_addon(fcitx-pinyin-enhance ${FCITX_PYENHANCE_SOURCES})
fcitx_add_configdesc_file(fcitx-pinyin-enhance.desc)
fcitx_add_addon_conf_file(fcitx-pinyin-enhance.conf)
