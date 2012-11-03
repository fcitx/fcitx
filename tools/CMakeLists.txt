
include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/lib/)
include_directories(${LIBICONV_INCLUDE_DIR})

add_subdirectory(gui)
add_subdirectory(cli)
add_subdirectory(dev)
