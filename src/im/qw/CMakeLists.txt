
set(FCITX_QUWEI_SOURCES
    qw.c
)

add_library(fcitx-qw MODULE ${FCITX_QUWEI_SOURCES})
set_target_properties( fcitx-qw PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-qw fcitx-core)

install(TARGETS fcitx-qw DESTINATION ${addondir})
install(FILES fcitx-qw.conf DESTINATION ${addonconfigdir})