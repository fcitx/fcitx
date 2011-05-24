set(FCITX_UTILS_SOURCES
    configfile.c
    profile.c
    utf8.c
    utils.c
    keys.c
)

add_library(fcitx-utils SHARED ${FCITX_UTILS_SOURCES})
set_target_properties(fcitx-utils PROPERTIES VERSION 0.1.0 SOVERSION 0.1)
target_link_libraries(fcitx-utils fcitx-config)
