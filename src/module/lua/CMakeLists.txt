find_package(Lua51 REQUIRED)

include_directories(${LUA_INCLUDE_DIR})

set(FCITX_LUA_SOURCES
    lua.c
    luawrap.c
)

fcitx_add_addon(fcitx-lua ${FCITX_LUA_SOURCES})
fcitx_add_addon_conf_file(fcitx-lua.conf)

target_link_libraries(fcitx-lua ${LUA_LIBRARIES})
