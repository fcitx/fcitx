include_directories("${PROJECT_SOURCE_DIR}/src/frontend/simple"
"${PROJECT_BINARY_DIR}/src/frontend/simple")

set(FCITX_TESTBED_SOURCES
    main.c
)

add_executable(fcitx-testbed ${FCITX_TESTBED_SOURCES})
install(TARGETS fcitx-testbed RUNTIME DESTINATION ${bindir})
target_link_libraries(fcitx-testbed fcitx-core ${LIBEXECINFO_LIBRARIES})
