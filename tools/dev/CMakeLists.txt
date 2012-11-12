set(fcitx_scanner_SOURCES
  fcitx-scanner.c
  )

add_executable(fcitx-scanner ${fcitx_scanner_SOURCES})
target_link_libraries(fcitx-scanner fcitx-utils)
install(TARGETS fcitx-scanner DESTINATION ${bindir})
