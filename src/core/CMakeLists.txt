set(FCITX_SOURCES
  fcitx.c
  errorhandler.c
  )

add_executable(fcitx ${FCITX_SOURCES})
install(TARGETS fcitx RUNTIME DESTINATION "${bindir}")
target_link_libraries(fcitx fcitx-core ${LIBEXECINFO_LIBRARIES})

fcitx_translate_add_sources(${FCITX_SOURCES} errorhandler.h)
