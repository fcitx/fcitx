add_subdirectory(pinyin)
add_subdirectory(keyboard)

if (ENABLE_TABLE)
    add_subdirectory(table)
endif (ENABLE_TABLE)

add_subdirectory(qw)