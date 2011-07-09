if(FALSE)

include_directories(${PROJECT_SOURCE_DIR}/src)

set(fcitx_ipc_SOURCES
    
)

install(TARGETS fcitx-ipc DESTINATION ${addondir})
set_target_properties( fcitx-ipc PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-ipc ${X11_LIBRARIES} fcitx-config )

install(FILES fcitx-ipc.conf DESTINATION ${addonconfigdir})

endif()