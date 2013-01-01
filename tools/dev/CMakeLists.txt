set(fcitx_scanner_SOURCES
  fcitx-scanner.c
  )

add_executable(fcitx-scanner ${fcitx_scanner_SOURCES})
target_link_libraries(fcitx-scanner fcitx-utils)

set(fcitx_po_parser_SOURCES
  fcitx-po-parser.c
  )

if(NOT _FCITX_DISABLE_GETTEXT)
  include_directories(${GETTEXTPO_INCLUDE_DIR})
endif()

add_executable(fcitx-po-parser ${fcitx_po_parser_SOURCES})

if(NOT _FCITX_DISABLE_GETTEXT)
  target_link_libraries(fcitx-po-parser fcitx-utils ${GETTEXTPO_LIBRARIES})
endif()

install(TARGETS fcitx-scanner fcitx-po-parser DESTINATION ${bindir})
