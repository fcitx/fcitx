include_directories(${fcitx_SOURCE_DIR}/src/)
include_directories(${fcitx_SOURCE_DIR}/src/lib/)

set(fcitx_fbterm_SOURCES
    imapi.c
    fcitx-fbterm.c
)

add_executable(fcitx-fbterm ${fcitx_fbterm_SOURCES})

install(TARGETS fcitx-fbterm DESTINATION ${bindir})
