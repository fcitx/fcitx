set(LIBFCITX_SOURCES
    configfile.c
    profile.c
    addon.c
    frontend.c
    hook.c
    ime.c
    ui.c
    instance.c
    module.c
    keys.c
)

set(FCITX_HEADERS
    ime.h
    ui.h
    hook.h
    configfile.h
    profile.h
    module.h
    instance.h
    addon.h
    frontend.h
    fcitx.h
    keys.h
)

add_library(fcitx-core SHARED ${LIBFCITX_SOURCES})
set_target_properties(fcitx-core PROPERTIES VERSION 0.1.0 SOVERSION 0.1 COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries(fcitx-core fcitx-config)
configure_file(fcitx.pc.in fcitx.pc)

target_link_libraries(fcitx-core fcitx-config fcitx-utils dl pthread)
install(TARGETS fcitx-core LIBRARY DESTINATION "${libdir}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx.pc DESTINATION "${libdir}/pkgconfig")
install(FILES ${FCITX_HEADERS} DESTINATION "${includedir}/fcitx")

if (ENABLE_STATIC)

add_library(fcitx-core-static STATIC ${LIBFCITX_SOURCES})
set_target_properties(fcitx-core-static PROPERTIES
    OUTPUT_NAME "fcitx-core"
    PREFIX "lib"
)

install(TARGETS fcitx-core-static
    LIBRARY DESTINATION ${libdir}
    ARCHIVE DESTINATION ${libdir}
)
endif (ENABLE_STATIC)