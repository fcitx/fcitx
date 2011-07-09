include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/backend/xim/lib)
include_directories(${X11_X11_INCLUDE_PATH})

set(fcitx_xim_SOURCES
    IC.c
    xim.c
    ximhandler.c
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


add_library(fcitx-xim MODULE ${fcitx_xim_SOURCES} ${Ximd_SOURCES})
install(TARGETS fcitx-xim DESTINATION ${addondir})
set_target_properties( fcitx-xim PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-xim ${X11_LIBRARIES} fcitx-config fcitx-core)

install(FILES fcitx-xim.conf DESTINATION ${addonconfigdir})
