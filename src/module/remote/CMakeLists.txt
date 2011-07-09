include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_REMOTE_MODULE_SOURCES
    remote.c
)

add_library(fcitx-remote-module MODULE ${FCITX_REMOTE_MODULE_SOURCES})
install(TARGETS fcitx-remote-module DESTINATION ${addondir})
set_target_properties( fcitx-remote-module PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-remote-module fcitx-core)

install(FILES fcitx-remote-module.conf DESTINATION ${addonconfigdir})


