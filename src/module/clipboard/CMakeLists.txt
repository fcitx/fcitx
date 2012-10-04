include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_CLIPBOARD_SOURCES
  clipboard.c
  clipboardconfig.c
)

if(ENABLE_X11)
  set(FCITX_CLIPBOARD_SOURCES ${FCITX_CLIPBOARD_SOURCES} clipboard-x11.c)
endif()

fcitx_add_addon(fcitx-clipboard ${FCITX_CLIPBOARD_SOURCES})
fcitx_add_addon_conf_file(fcitx-clipboard.conf)
fcitx_add_configdesc_file(fcitx-clipboard.desc)
fcitx_add_addon_header(clipboard clipboard.h)
