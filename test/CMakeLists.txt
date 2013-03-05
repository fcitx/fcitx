include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/lib)
include_directories(${PROJECT_SOURCE_DIR}/src/im/pinyin)
include_directories(${PROJECT_SOURCE_DIR}/src/module/unicode)
include_directories(${PROJECT_SOURCE_DIR}/src/module/dbus)

add_executable(testbacktrace testbacktrace.c)
target_link_libraries(testbacktrace fcitx-utils)

add_executable(testconfig testconfig.c)
target_link_libraries(testconfig fcitx-config)

add_executable(testmessage testmessage.c)
target_link_libraries(testmessage fcitx-core)

add_executable(teststring teststring.c)
target_link_libraries(teststring fcitx-utils)

add_executable(testarray testarray.c)
target_link_libraries(testarray fcitx-utils)

add_executable(testutf8 testutf8.c)
target_link_libraries(testutf8 fcitx-utils)

add_executable(testxdg testxdg.c)
target_link_libraries(testxdg fcitx-config fcitx-utils)

add_executable(testunicode testunicode.c ../src/module/unicode/charselectdata.c)
target_link_libraries(testunicode fcitx-utils fcitx-config)

add_executable(testobjpool testobjpool.c)
target_link_libraries(testobjpool fcitx-utils)

add_executable(testcast testcast.c)
target_link_libraries(testcast fcitx-utils)

add_executable(testhandlertable testhandlertable.c)
target_link_libraries(testhandlertable fcitx-utils)

add_executable(testhotkey testhotkey.c)
target_link_libraries(testhotkey fcitx-config)

add_executable(testpinyin
    ../src/im/pinyin/pyParser.c
    ../src/im/pinyin/pyMapTable.c
    ../src/im/pinyin/PYFA.c
    ../src/im/pinyin/sp.c
    testpinyin.c
)

target_link_libraries(testpinyin fcitx-config)

add_executable(testdbuslaunch testdbuslaunch.c
               ../src/module/dbus/dbuslauncher.c
               )

if(_ENABLE_DBUS)
  include_directories(${DBUS_INCLUDE_DIRS})
  link_directories(${DBUS_LIBRARY_DIRS})
  add_executable(testdbussocket testdbussocket.c
                 ../src/module/dbus/dbussocket.c)
  target_link_libraries(testdbussocket ${DBUS_LIBRARIES} fcitx-utils)
  add_test(NAME testdbussocket COMMAND testdbussocket)
endif()


target_link_libraries(testdbuslaunch fcitx-utils)

add_test(NAME testconfig
         COMMAND testconfig ${CMAKE_CURRENT_SOURCE_DIR}/test.desc ${CMAKE_CURRENT_SOURCE_DIR}/test.conf ${CMAKE_CURRENT_BINARY_DIR}/test.conf)

add_test(NAME testmessage
         COMMAND testmessage)

add_test(NAME testbacktrace
         COMMAND testbacktrace)

add_test(NAME testpinyin
         COMMAND testpinyin)

add_test(NAME teststring
         COMMAND teststring)

add_test(NAME testarray
         COMMAND testarray)

add_test(NAME testobjpool
         COMMAND testobjpool)

add_test(NAME testcast
         COMMAND testcast)

add_test(NAME testhandlertable
         COMMAND testhandlertable)

add_test(NAME testutf8
         COMMAND testutf8)

add_test(NAME testxdg
         COMMAND testxdg)

add_test(NAME testunicode
         COMMAND testunicode ${PROJECT_SOURCE_DIR}/src/module/unicode/charselectdata)

add_test(NAME testdbuslaunch
         COMMAND testdbuslaunch)

add_test(NAME testhotkey
         COMMAND testhotkey)
