include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/lib/)

set(fcitx_fbterm_SOURCES
    imapi.c
    fcitx-fbterm.c
)

add_executable(fcitx-fbterm ${fcitx_fbterm_SOURCES})

install(TARGETS fcitx-fbterm DESTINATION ${bindir})
