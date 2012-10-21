include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_CHTTRANS_SOURCES
    chttrans.c
)
if(ENABLE_OPENCC)
  set(FCITX_CHTTRANS_SOURCES ${FCITX_CHTTRANS_SOURCES} chttrans-opencc.c)
endif(ENABLE_OPENCC)

fcitx_add_addon(fcitx-chttrans ${FCITX_CHTTRANS_SOURCES})

fcitx_add_configdesc_file(fcitx-chttrans.desc)
fcitx_add_addon_conf_file(fcitx-chttrans.conf)
fcitx_add_addon_header(chttrans chttrans.h)
fcitx_scan_addon(chttrans)
