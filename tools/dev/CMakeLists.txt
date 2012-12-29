set(fcitx_scanner_SOURCES
  fcitx-scanner.c
  )

add_executable(fcitx-scanner ${fcitx_scanner_SOURCES})
target_link_libraries(fcitx-scanner fcitx-utils)

find_package(GettextPo REQUIRED)
set(fcitx_po_parser_SOURCES
  fcitx-po-parser.c
  )

include_directories(${GETTEXTPO_INCLUDE_DIR})
add_executable(fcitx-po-parser ${fcitx_po_parser_SOURCES})
target_link_libraries(fcitx-po-parser fcitx-utils ${GETTEXTPO_LIBRARIES})
install(TARGETS fcitx-scanner fcitx-po-parser DESTINATION ${bindir})

add_subdirectory(testbed)
