include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_FULLWIDTH_CHAR_SOURCES
    fullwidthchar.c
)

add_library(fcitx-fullwidth-char MODULE ${FCITX_FULLWIDTH_CHAR_SOURCES})
install(TARGETS fcitx-fullwidth-char DESTINATION ${addondir})
set_target_properties( fcitx-fullwidth-char PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-fullwidth-char fcitx-core)

install(FILES fcitx-fullwidth-char.conf DESTINATION ${addonconfigdir})
