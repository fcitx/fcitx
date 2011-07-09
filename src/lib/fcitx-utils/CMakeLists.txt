set(FCITX_UTILS_SOURCES
    utf8.c
    utils.c
    log.c
)

set(FCITX_UTILS_HEADERS
    log.h
    utarray.h
    utf8.h
    utils.h
    uthash.h
)

add_library(fcitx-utils SHARED ${FCITX_UTILS_SOURCES})
set_target_properties(fcitx-utils PROPERTIES VERSION 0.1.0 SOVERSION 0.1 COMPILE_FLAGS "-fvisibility=hidden")

install(TARGETS fcitx-utils LIBRARY DESTINATION "${libdir}")
install(FILES ${FCITX_UTILS_HEADERS} DESTINATION "${includedir}/fcitx-utils")