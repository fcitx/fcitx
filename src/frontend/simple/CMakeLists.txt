set(fcitx_simple_frontend_SOURCES
    simple.c
    simple-module.c
)

fcitx_add_addon(fcitx-simple ${fcitx_simple_frontend_SOURCES})

fcitx_add_addon_conf_file(fcitx-simple-frontend.conf)
fcitx_add_addon_conf_file(fcitx-simple-module.conf)
