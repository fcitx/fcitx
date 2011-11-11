set(FCITX_CONFIG_SOURCES
    fcitx-config.c
    hotkey.c
    sprintf.c
    xdg.c
)

set(FCITX_CONFIG_HEADERS
    fcitx-config.h
    hotkey.h
    xdg.h
)

add_library(fcitx-config SHARED ${FCITX_CONFIG_SOURCES})
target_link_libraries(fcitx-config fcitx-utils)
set_target_properties(fcitx-config
                      PROPERTIES VERSION 4.1
                      SOVERSION 4
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
)

configure_file(fcitx-config.pc.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-config.pc)

install(TARGETS fcitx-config DESTINATION ${libdir})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-config.pc DESTINATION "${libdir}/pkgconfig")
install(FILES ${FCITX_CONFIG_HEADERS} DESTINATION "${includedir}/fcitx-config")

if (ENABLE_STATIC)

add_library(fcitx-config-static STATIC ${FCITX_CONFIG_SOURCES})
set_target_properties(fcitx-config-static PROPERTIES
    OUTPUT_NAME "fcitx-config"
    PREFIX "lib"
)

install(TARGETS fcitx-config-static
    LIBRARY DESTINATION ${libdir}
    ARCHIVE DESTINATION ${libdir}
)
endif (ENABLE_STATIC)
