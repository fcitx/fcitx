if(ENABLE_LUA)
  find_package(Lua 5.1 REQUIRED)
  include_directories(${LUA_INCLUDE_DIRS})
  set(lua_noinstall)
else()
  set(lua_noinstall NO_INSTALL)
endif()

set(FCITX_LUA_SOURCES
  lua.c
  luawrap.c)

fcitx_add_addon_full(lua SCAN SCAN_PRIV ${lua_noinstall}
  SOURCES ${FCITX_LUA_SOURCES}
  HEADERS luamod.h
  LINK_LIBS ${LUA_LIBRARIES}
  EXTRA_PO luawrap.h)
