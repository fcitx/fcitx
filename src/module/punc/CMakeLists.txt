include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_PUNC_SOURCES
    punc.c
)

fcitx_add_addon(fcitx-punc ${FCITX_PUNC_SOURCES})
target_link_libraries( fcitx-punc fcitx-config fcitx-core)
fcitx_add_addon_header(punc punc.h)
fcitx_add_addon_conf_file(fcitx-punc.conf)