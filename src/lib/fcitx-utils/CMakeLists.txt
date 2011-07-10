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

configure_file(fcitx-utils.pc.in fcitx-utils.pc)

install(TARGETS fcitx-utils LIBRARY DESTINATION "${libdir}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-utils.pc DESTINATION "${libdir}/pkgconfig")
install(FILES ${FCITX_UTILS_HEADERS} DESTINATION "${includedir}/fcitx-utils")

if (ENABLE_STATIC)

add_library(fcitx-utils-static STATIC ${FCITX_UTILS_SOURCES})
set_target_properties(fcitx-utils-static PROPERTIES
    OUTPUT_NAME "fcitx-utils"
    PREFIX "lib"
)

install(TARGETS fcitx-utils-static
    LIBRARY DESTINATION ${libdir}
    ARCHIVE DESTINATION ${libdir}
)
endif (ENABLE_STATIC)