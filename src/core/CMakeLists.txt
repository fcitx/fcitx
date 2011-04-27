set(FCITX_SOURCES
    fcitx.c
    addon.c
    backend.c
    keys.c
    errorhandler.c
    module.c
    ime.c
    ui.c
    hook.c
)

add_executable(fcitx ${FCITX_SOURCES})
install(TARGETS fcitx DESTINATION ${bindir})
configure_file(fcitx.pc.in fcitx.pc)

target_link_libraries(fcitx fcitx-config)
target_link_libraries(fcitx utils)
target_link_libraries(fcitx dl)
target_link_libraries(fcitx pthread)