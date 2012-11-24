set(fcitx_qt_SOURCES
    fcitxconnection.cpp
    fcitxformattedpreedit.cpp
    fcitxinputcontextproxy.cpp
    fcitxinputmethodproxy.cpp
    fcitxconfiguiplugin.cpp
    fcitxconfiguiwidget.cpp
    fcitxconfiguifactory.cpp
    )

set(fcitx_qt_HEADERS
    fcitxconnection.h
    fcitxqt_export.h
    fcitxformattedpreedit.h
    fcitxinputcontextproxy.h
    fcitxinputmethodproxy.h
    fcitxconfiguiplugin.h
    fcitxconfiguiwidget.h
    fcitxconfiguifactory.h
)

fcitx_translate_add_sources(${fcitx_qt_HEADERS} ${fcitx_qt_SOURCES})

if(NOT (ENABLE_QT_IM_MODULE AND ENABLE_QT))
  return()
endif()

include_directories(
    ${QT_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${X11_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

QT4_WRAP_CPP(
    fcitx_qt_MOC_SRCS
    fcitxconnection.h
    fcitxconnection_p.h
    fcitxformattedpreedit.h
    fcitxinputcontextproxy.h
    fcitxinputmethodproxy.h
    fcitxconfiguiplugin.h
    fcitxconfiguiwidget.h
    fcitxconfiguifactory.h
)

if (ENABLE_X11)
    include_directories(${X11_INCLUDE_DIRS})
    link_directories(${X11_LIBRARY_DIRS})
endif (ENABLE_X11)

add_library(fcitx-qt SHARED ${fcitx_qt_SOURCES} ${fcitx_qt_MOC_SRCS})
set_target_properties(fcitx-qt
                      PROPERTIES VERSION 0.1
                      SOVERSION 0
                      COMPILE_FLAGS "-fvisibility=hidden"
                      LINK_FLAGS "-Wl,--no-undefined"
)

target_link_libraries(
    fcitx-qt
    debug ${QT_QTCORE_LIBRARY_DEBUG}
    debug ${QT_QTDBUS_LIBRARY_DEBUG}
    debug ${QT_QTGUI_LIBRARY_DEBUG}
    optimized ${QT_QTCORE_LIBRARY_RELEASE}
    optimized ${QT_QTDBUS_LIBRARY_RELEASE}
    optimized ${QT_QTGUI_LIBRARY_RELEASE}
    fcitx-config
    fcitx-utils
)

if (ENABLE_X11)
    target_link_libraries(fcitx-qt ${X11_LIBRARIES})
endif (ENABLE_X11)

install(TARGETS fcitx-qt LIBRARY DESTINATION "${libdir}")

install(FILES ${fcitx_qt_HEADERS} DESTINATION "${includedir}/fcitx-qt")

add_subdirectory(test)
