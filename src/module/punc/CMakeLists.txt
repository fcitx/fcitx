include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_PUNC_SOURCES
    punc.c
)

add_library(fcitx-punc MODULE ${FCITX_PUNC_SOURCES})
install(TARGETS fcitx-punc DESTINATION ${addondir})
set_target_properties( fcitx-punc PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")

install(FILES fcitx-punc.conf DESTINATION ${addonconfigdir})
install(FILES punc.h DESTINATION ${includedir}/fcitx/module/punc/)