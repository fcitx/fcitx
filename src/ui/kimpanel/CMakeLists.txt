include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

set(fcitx_kimpanel_ui_SOURCES
    kimpanel.c
)

add_library(fcitx-kimpanel-ui MODULE ${fcitx_kimpanel_ui_SOURCES})
install(TARGETS fcitx-kimpanel-ui DESTINATION ${addondir})
set_target_properties( fcitx-kimpanel-ui PROPERTIES PREFIX "")
target_link_libraries( fcitx-kimpanel-ui ${DBUS_LIBRARIES} fcitx-core)

install(FILES fcitx-kimpanel-ui.conf DESTINATION ${addonconfigdir})