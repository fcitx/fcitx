set(fcitx_config_SOURCES
	fcitx-config.c
	hotkey.c
	sprintf.c
	xdg.c
)

set(FCITX_CONFIG_HEADERS
    fcitx-config.h
    hotkey.h
    uthash.h
    xdg.h
)

add_library(fcitx-config SHARED ${fcitx_config_SOURCES})
target_link_libraries(fcitx-config fcitx-utils)
set_target_properties(fcitx-config PROPERTIES VERSION 4.1.0 SOVERSION 4.1)
configure_file(fcitx-config.pc.in fcitx-config.pc)

install(TARGETS fcitx-config DESTINATION ${libdir})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-config.pc DESTINATION "${libdir}/pkgconfig")
install(FILES ${FCITX_CONFIG_HEADERS} DESTINATION "${includedir}/fcitx-config")