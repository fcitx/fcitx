include_directories(../src/lib)

if (ENABLE_QT)
add_subdirectory(gui)
endif (ENABLE_QT)
add_subdirectory(cli)
add_subdirectory(dev)
