if(ENABLE_LUA)
  find_package(Lua51 REQUIRED)
  include_directories(${LUA_INCLUDE_DIR})
  set(lua_noinstall)
else()
  set(lua_noinstall NO_INSTALL)
endif()

set(FCITX_LUA_SOURCES
  lua.c
  luawrap.c)

fcitx_add_addon_full(lua SCAN ${lua_noinstall}
  SOURCES ${FCITX_LUA_SOURCES}
  HEADERS luamod.h
  LINK_LIBS ${LUA_LIBRARIES}
  EXTRA_PO luawrap.h)
