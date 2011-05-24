set(LIBFCITX_SOURCES
    addon.c
    backend.c
    hook.c
    ime.c
    ui.c
    instance.c
    module.c
)

add_library(fcitx-core SHARED ${LIBFCITX_SOURCES})
set_target_properties(fcitx-core PROPERTIES VERSION 0.1.0 SOVERSION 0.1)
target_link_libraries(fcitx-core fcitx-config)
configure_file(fcitx.pc.in fcitx.pc)

target_link_libraries(fcitx-core fcitx-config fcitx-utils dl pthread)
install(TARGETS fcitx-core LIBRARY DESTINATION "${libdir}")