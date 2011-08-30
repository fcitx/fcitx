include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/lib/)
include_directories(${LIBICONV_INCLUDE_DIR})

set(createPYMB_SOURCES
    createPYMB.c
    ../src/im/pinyin/pyParser.c
    ../src/im/pinyin/pyMapTable.c
    ../src/im/pinyin/PYFA.c
    ../src/im/pinyin/sp.c
)

set(mb2org_SOURCES
    mb2org.c
    pyTools.c
    ../src/im/pinyin/pyParser.c
    ../src/im/pinyin/pyMapTable.c
    ../src/im/pinyin/PYFA.c
    ../src/im/pinyin/sp.c
)

set(readPYBase_SOURCES
    readPYBase.c
    pyTools.c
)

set(readPYMB_SOURCES
    readPYMB.c
    pyTools.c
)

set(mb2txt_SOURCES
    mb2txt.c
)

set(txt2mb_SOURCES
    txt2mb.c
)

set(scel2org_SOURCES
    scel2org.c
)

add_executable(createPYMB ${createPYMB_SOURCES})
add_executable(readPYBase ${readPYBase_SOURCES})
add_executable(readPYMB ${readPYMB_SOURCES})
add_executable(fcitx-remote fcitx-remote.c)
add_executable(mb2org ${mb2org_SOURCES})
add_executable(mb2txt ${mb2txt_SOURCES})
add_executable(txt2mb ${txt2mb_SOURCES})
add_executable(scel2org ${scel2org_SOURCES})

target_link_libraries(createPYMB fcitx-config fcitx-utils)
target_link_libraries(readPYBase fcitx-config)
target_link_libraries(readPYMB fcitx-config)
target_link_libraries(mb2org fcitx-config)
target_link_libraries(mb2txt fcitx-config fcitx-utils)
target_link_libraries(txt2mb fcitx-config fcitx-utils)
target_link_libraries(fcitx-remote fcitx-utils)
target_link_libraries(scel2org ${LIBICONV_LIBRARIES})

install(TARGETS createPYMB readPYBase readPYMB fcitx-remote mb2org mb2txt txt2mb scel2org DESTINATION ${bindir})
