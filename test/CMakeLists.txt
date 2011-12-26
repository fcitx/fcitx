add_executable(testconfig testconfig.c)
target_link_libraries(testconfig fcitx-config)

add_executable(testmessage testmessage.c)
target_link_libraries(testmessage fcitx-core)

add_executable(teststring teststring.c)
target_link_libraries(teststring fcitx-utils)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/lib)

add_test(NAME testconfig
         COMMAND testconfig ${CMAKE_CURRENT_SOURCE_DIR}/test.desc ${CMAKE_CURRENT_SOURCE_DIR}/test.conf ${CMAKE_CURRENT_BINARY_DIR}/test.conf)

add_test(NAME testmessage
         COMMAND testmessage)
         
add_test(NAME teststring
         COMMAND teststring)
         