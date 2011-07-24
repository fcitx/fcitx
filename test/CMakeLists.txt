

add_executable(testconfig testconfig.c)
target_link_libraries(testconfig fcitx-config)

add_test(NAME testconfig
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
         COMMAND testconfig ${CMAKE_CURRENT_SOURCE_DIR}/test.desc ${CMAKE_CURRENT_SOURCE_DIR}/test.conf ${CMAKE_CURRENT_BINARY_DIR}/test.conf)