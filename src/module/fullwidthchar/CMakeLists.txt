include_directories(${fcitx_SOURCE_DIR}/src)

set(FCITX_FULLWIDTH_CHAR_SOURCES
    fullwidthchar.c
)

add_library(fcitx-fullwidth-char MODULE ${FCITX_FULLWIDTH_CHAR_SOURCES})
install(TARGETS fcitx-fullwidth-char DESTINATION ${addondir})
set_target_properties( fcitx-fullwidth-char PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")

install(FILES fcitx-fullwidth-char.conf DESTINATION ${addonconfigdir})
