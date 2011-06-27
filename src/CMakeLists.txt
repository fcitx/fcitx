include_directories(${fcitx_SOURCE_DIR}/src)
include_directories(${fcitx_SOURCE_DIR}/src/lib)

add_subdirectory(lib)
add_subdirectory(core)
add_subdirectory(module)
add_subdirectory(im)
add_subdirectory(backend)
add_subdirectory(ui)