set(LIBFCITX_SOURCES
  instance.c
  candidate.c
  configfile.c
  profile.c
  addon.c
  frontend.c
  hook.c
  ime.c
  ui.c
  module.c
  keys.c
  context.c
  )

set(FCITX_HEADERS
  candidate.h
  ime.h
  ui.h
  hook.h
  configfile.h
  profile.h
  module.h
  instance.h
  addon.h
  frontend.h
  fcitx.h
  keys.h
  context.h
  )

set(FCITX_INTERNAL_HEADERS
  addon-internal.h
  context-internal.h
  fcitx-internal.h
  hook-internal.h
  ime-internal.h
  instance-internal.h
  module-internal.h
  ui-internal.h
  )

fcitx_translate_add_sources(${LIBFCITX_SOURCES} ${FCITX_HEADERS}
  ${FCITX_INTERNAL_HEADERS})

link_directories(${DL_LIBRARY_DIRS} ${PTHREAD_LIBRARY_DIRS})

add_library(fcitx-core SHARED ${LIBFCITX_SOURCES})
set_target_properties(fcitx-core
  PROPERTIES VERSION 0.3
  SOVERSION 0
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined")

configure_file(fcitx.pc.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx.pc)

target_link_libraries(fcitx-core fcitx-config fcitx-utils
  ${DL_LIBRARIES} ${PTHREAD_LIBRARIES})
install(TARGETS fcitx-core LIBRARY DESTINATION "${libdir}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx.pc
  DESTINATION "${pkgconfdir}")
install(FILES ${FCITX_HEADERS} DESTINATION "${includedir}/fcitx")

if(ENABLE_STATIC)
  add_library(fcitx-core-static STATIC ${LIBFCITX_SOURCES})
  set_target_properties(fcitx-core-static PROPERTIES
    OUTPUT_NAME "fcitx-core"
    PREFIX "lib")

  install(TARGETS fcitx-core-static
    LIBRARY DESTINATION "${libdir}"
    ARCHIVE DESTINATION "${libdir}")
endif()
