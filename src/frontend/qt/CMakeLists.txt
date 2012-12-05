set(qtim_fcitx_SRCS
  qfcitxinputcontextplugin.cpp
  qfcitxinputcontext.cpp
  )

fcitx_translate_add_sources(${qtim_fcitx_SRCS}
  keyserver_x11.h
  qfcitxinputcontext.h
  )

if(NOT (ENABLE_QT_IM_MODULE AND ENABLE_QT))
  return()
endif()

include_directories(
  ${QT_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${QT_QTDBUS_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

if(ENABLE_X11)
  include_directories(${X11_INCLUDE_DIRS})
  link_directories(${X11_LIBRARY_DIRS})
endif()

set(qtim_fcitx_MOC_HDRS
  qfcitxinputcontext.h
  )

QT4_AUTOMOC(${qtim_fcitx_SRCS})
QT4_WRAP_CPP(qtim_fcitx_MOC_SRCS
  ${qtim_fcitx_MOC_HDRS}
  )

add_library(
  qtim-fcitx MODULE
  ${qtim_fcitx_SRCS}
  ${qtim_fcitx_MOC_SRCS}
  )

set_target_properties(qtim-fcitx PROPERTIES
  PREFIX ""
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined")

target_link_libraries(
  qtim-fcitx
  debug ${QT_QTCORE_LIBRARY_DEBUG}
  debug ${QT_QTDBUS_LIBRARY_DEBUG}
  debug ${QT_QTGUI_LIBRARY_DEBUG}
  optimized ${QT_QTCORE_LIBRARY_RELEASE}
  optimized ${QT_QTDBUS_LIBRARY_RELEASE}
  optimized ${QT_QTGUI_LIBRARY_RELEASE}
  fcitx-config
  fcitx-qt
  )

if(ENABLE_X11)
  target_link_libraries(qtim-fcitx ${X11_LIBRARIES})
endif()

install(
  TARGETS qtim-fcitx
  LIBRARY DESTINATION
  "${QT_PLUGINS_DIR}/inputmethods")
