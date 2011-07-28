set(FCITX_SOURCES
    fcitx.c
    errorhandler.c
)

add_executable(fcitx ${FCITX_SOURCES})
install(TARGETS fcitx RUNTIME DESTINATION ${bindir})
target_link_libraries(fcitx fcitx-core ${LIBEXECINFO_LIBRARIES})