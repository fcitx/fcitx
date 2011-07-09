include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_AUTOENG_SOURCES
    AutoEng.c
)

add_library(fcitx-autoeng MODULE ${FCITX_AUTOENG_SOURCES})
install(TARGETS fcitx-autoeng DESTINATION ${addondir})
set_target_properties( fcitx-autoeng PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-autoeng fcitx-config fcitx-core)

install(FILES fcitx-autoeng.conf DESTINATION ${addonconfigdir})
