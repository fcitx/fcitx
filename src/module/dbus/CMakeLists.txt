include_directories(${fcitx_SOURCE_DIR}/src)
include_directories(${DBUS_INCLUDE_DIRS})

set(FCITX_DBUS_SOURCES
	dbus.c
)

add_library(fcitx-dbus MODULE ${FCITX_DBUS_SOURCES})
install(TARGETS fcitx-dbus DESTINATION ${addondir})
set_target_properties( fcitx-dbus PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")
target_link_libraries( fcitx-dbus ${DBUS_LIBRARIES} )

install(FILES fcitx-dbus.conf DESTINATION ${pkgdatadir}/data/addon)

