set(fcitx_config_SOURCES
	cutils.c
	fcitx-config.c
	hotkey.c
	sprintf.c
	xdg.c
)

add_library(fcitx-config SHARED ${fcitx_config_SOURCES})
install(TARGETS fcitx-config DESTINATION ${libdir})