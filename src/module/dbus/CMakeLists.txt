include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

set(FCITX_DBUS_SOURCES
	dbusstuff.c
)

add_library(fcitx-dbus MODULE ${FCITX_DBUS_SOURCES})
install(TARGETS fcitx-dbus DESTINATION ${addondir})
set_target_properties( fcitx-dbus PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-dbus ${DBUS_LIBRARIES} fcitx-core)

install(FILES fcitx-dbus.conf DESTINATION ${addonconfigdir})
install(FILES dbusstuff.h DESTINATION ${includedir}/fcitx/module/dbus/)

