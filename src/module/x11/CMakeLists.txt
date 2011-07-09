include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${X11_X11_INCLUDE_PATH})

set(FCITX_X11_SOURCES
    xerrorhandler.c
    x11stuff.c
)

add_library(fcitx-x11 MODULE ${FCITX_X11_SOURCES})
install(TARGETS fcitx-x11 DESTINATION ${addondir})
set_target_properties( fcitx-x11 PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-x11 ${X11_LIBRARIES} ${X11_Xrender_LIB} fcitx-core )

install(FILES fcitx-x11.conf DESTINATION ${addonconfigdir})
install(FILES x11stuff.h DESTINATION ${includedir}/fcitx/module/x11/)
