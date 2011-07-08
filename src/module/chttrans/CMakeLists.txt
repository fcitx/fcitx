include_directories(${fcitx_SOURCE_DIR}/src)

set(FCITX_CHTTRANS_SOURCES
    chttrans.c
)

add_library(fcitx-chttrans MODULE ${FCITX_CHTTRANS_SOURCES})
install(TARGETS fcitx-chttrans DESTINATION ${addondir})
set_target_properties( fcitx-chttrans PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")

if (${HAVE_OPENCC})
    target_link_libraries( fcitx-chttrans ${OPENCC_LIBRARIES} )
    include_directories(${OPENCC_INCLUDE_DIRS})
endif (${HAVE_OPENCC})

install(FILES fcitx-chttrans.conf DESTINATION ${addonconfigdir})
install(FILES fcitx-chttrans.desc DESTINATION ${configdescdir})
