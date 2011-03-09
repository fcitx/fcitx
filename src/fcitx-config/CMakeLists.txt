set(fcitx_config_SOURCES
	cutils.c
	fcitx-config.c
	hotkey.c
	sprintf.c
	xdg.c
)

add_library(fcitx-config SHARED ${fcitx_config_SOURCES})
set_target_properties(fcitx-config PROPERTIES VERSION 4.1.0 SOVERSION 4.1)

install(TARGETS fcitx-config DESTINATION ${libdir})