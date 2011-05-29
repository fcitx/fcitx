set(FCITX_UTILS_SOURCES
    utf8.c
    utils.c
    keys.c
    cutils.c
)

add_library(fcitx-utils SHARED ${FCITX_UTILS_SOURCES})
set_target_properties(fcitx-utils PROPERTIES VERSION 0.1.0 SOVERSION 0.1)

install(TARGETS fcitx-utils LIBRARY DESTINATION "${libdir}")