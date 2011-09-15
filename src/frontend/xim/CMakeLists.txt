include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/lib)
include_directories(${PROJECT_SOURCE_DIR}/src/frontend/xim/lib)
include_directories(${X11_X11_INCLUDE_PATH})

set(fcitx_xim_SOURCES
    IC.c
    xim.c
    ximhandler.c
    ximconfig.c
)

set(Ximd_SOURCES
    lib/FrameMgr.c
    lib/i18nAttr.c
    lib/i18nClbk.c
    lib/i18nIc.c
    lib/i18nIMProto.c
    lib/i18nMethod.c
    lib/i18nPtHdr.c
    lib/i18nUtil.c
    lib/i18nX.c
    lib/IMConn.c
    lib/IMMethod.c
    lib/IMValues.c
)


fcitx_add_addon(fcitx-xim ${fcitx_xim_SOURCES} ${Ximd_SOURCES})
target_link_libraries( fcitx-xim ${X11_LIBRARIES})

fcitx_add_addon_conf_file(fcitx-xim.conf)
fcitx_add_configdesc_file(fcitx-xim.desc)
