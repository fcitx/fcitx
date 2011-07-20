include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

set(fcitx_ipc_SOURCES
    ipc.c
)

add_library(fcitx-ipc MODULE ${fcitx_ipc_SOURCES})
install(TARGETS fcitx-ipc DESTINATION ${addondir})
set_target_properties( fcitx-ipc PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-ipc fcitx-config fcitx-core ${DBUS_LIBRARIES} )

install(FILES fcitx-ipc.conf DESTINATION ${addonconfigdir})
install(FILES ipc.h DESTINATION ${includedir}/fcitx/module/ipc/)