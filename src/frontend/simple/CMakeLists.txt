include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(fcitx_simple_frontend_SOURCES
    simple-frontend.c
)

set(fcitx_simple_module_SOURCES
    simple-module.c
)

fcitx_add_addon_full(simple-frontend SCAN
  SOURCES ${fcitx_simple_frontend_SOURCES}
)
fcitx_add_addon_full(simple-module SCAN
  SOURCES ${fcitx_simple_module_SOURCES}
  HEADERS simple-api.h
)

