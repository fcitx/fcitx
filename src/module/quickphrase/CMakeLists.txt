include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_QUICKPHRASE_SOURCES
    QuickPhrase.c
)

add_library(fcitx-quickphrase MODULE ${FCITX_QUICKPHRASE_SOURCES})
install(TARGETS fcitx-quickphrase DESTINATION ${addondir})
set_target_properties( fcitx-quickphrase PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
install(FILES fcitx-quickphrase.conf DESTINATION ${addonconfigdir})