include_directories(${GIO2_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GIO2_LIBRARY_DIRS})

add_custom_command(OUTPUT marshall.c
                   COMMAND ${GLIB_GENMARSHAL} --body --prefix=fcitx_marshall ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.c
                   DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list
)
add_custom_command(OUTPUT marshall.h
                   COMMAND ${GLIB_GENMARSHAL} --header --prefix=fcitx_marshall ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list > marshall.h
                   DEPENDS ${PROJECT_SOURCE_DIR}/src/frontend/gtk-common/marshall.list
)

set(FCITX_GCLIENT_SOURCES
    fcitxclient.c
    fcitxinputmethod.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.c
    ${CMAKE_CURRENT_BINARY_DIR}/marshall.h
)

set(FCITX_GCLIENT_HEADERS
    fcitxclient.h
    fcitxinputmethod.h
)

add_library(fcitx-gclient SHARED ${FCITX_GCLIENT_SOURCES})
set_target_properties(fcitx-gclient
                      PROPERTIES VERSION 0.1
                      SOVERSION 0
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
)
target_link_libraries(fcitx-gclient ${GIO2_LIBRARIES} fcitx-utils)

configure_file(fcitx-gclient.pc.in ${CMAKE_CURRENT_BINARY_DIR}/fcitx-gclient.pc)

install(TARGETS fcitx-gclient LIBRARY DESTINATION "${libdir}")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fcitx-gclient.pc DESTINATION "${libdir}/pkgconfig")
install(FILES ${FCITX_GCLIENT_HEADERS} DESTINATION "${includedir}/fcitx-gclient")

if (ENABLE_STATIC)

add_library(fcitx-gclient-static STATIC ${FCITX_GCLIENT_SOURCES})
set_target_properties(fcitx-gclient-static PROPERTIES
    OUTPUT_NAME "fcitx-gclient"
    PREFIX "lib"
)

install(TARGETS fcitx-gclient-static
    LIBRARY DESTINATION ${libdir}
    ARCHIVE DESTINATION ${libdir}
)
endif (ENABLE_STATIC)
