include_directories(${PROJECT_SOURCE_DIR}/src)

if (_ENABLE_OPENCC)
    include_directories(${OPENCC_INCLUDE_DIRS})
    link_directories(${OPENCC_LIBRARY_DIRS})
endif (_ENABLE_OPENCC)

set(FCITX_CHTTRANS_SOURCES
    chttrans.c
)

fcitx_add_addon(fcitx-chttrans ${FCITX_CHTTRANS_SOURCES})

if (_ENABLE_OPENCC)
    target_link_libraries( fcitx-chttrans ${OPENCC_LIBRARIES} )
    include_directories(${OPENCC_INCLUDE_DIRS})
endif (_ENABLE_OPENCC)

fcitx_add_configdesc_file(fcitx-chttrans.desc)
fcitx_add_addon_conf_file(fcitx-chttrans.conf)
