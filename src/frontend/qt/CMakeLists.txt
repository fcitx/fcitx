set(qtim_fcitx_SRCS
  qfcitxinputcontextplugin.cpp
  qfcitxinputcontext.cpp
  qtkey.cpp
  fcitxwatcher.cpp
  fcitxinputcontextproxy.cpp
  fcitxqtdbustypes.cpp
  )

fcitx_translate_add_sources(${qtim_fcitx_SRCS}
  qfcitxinputcontext.h
  )

if(NOT (ENABLE_QT_IM_MODULE AND ENABLE_QT))
  return()
endif()

set_source_files_properties(org.fcitx.Fcitx.InputContext.xml PROPERTIES
INCLUDE fcitxqtdbustypes.h)
set_source_files_properties(org.fcitx.Fcitx.InputContext1.xml PROPERTIES
INCLUDE fcitxqtdbustypes.h)
set_source_files_properties(org.fcitx.Fcitx.InputMethod.xml PROPERTIES
INCLUDE fcitxqtdbustypes.h)
set_source_files_properties(org.fcitx.Fcitx.InputMethod1.xml PROPERTIES
INCLUDE fcitxqtdbustypes.h)

qt4_add_dbus_interface(qtim_fcitx_SRCS org.fcitx.Fcitx.InputContext.xml inputcontextproxy)
qt4_add_dbus_interface(qtim_fcitx_SRCS org.fcitx.Fcitx.InputContext1.xml inputcontext1proxy)
qt4_add_dbus_interface(qtim_fcitx_SRCS org.fcitx.Fcitx.InputMethod.xml inputmethodproxy)
qt4_add_dbus_interface(qtim_fcitx_SRCS org.fcitx.Fcitx.InputMethod1.xml inputmethod1proxy)

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

add_library(
  qtim-fcitx MODULE
  ${qtim_fcitx_SRCS}
  )

set_target_properties(qtim-fcitx PROPERTIES
  PREFIX ""
  COMPILE_FLAGS "-fvisibility=hidden"
  LINK_FLAGS "-Wl,--no-undefined"
  CXX_STANDARD 11
  AUTOMOC TRUE)

target_link_libraries(
  qtim-fcitx
  debug ${QT_QTCORE_LIBRARY_DEBUG}
  debug ${QT_QTDBUS_LIBRARY_DEBUG}
  debug ${QT_QTGUI_LIBRARY_DEBUG}
  optimized ${QT_QTCORE_LIBRARY_RELEASE}
  optimized ${QT_QTDBUS_LIBRARY_RELEASE}
  optimized ${QT_QTGUI_LIBRARY_RELEASE}
  fcitx-config
  XKBCommon::XKBCommon
  )

if(ENABLE_X11)
  target_link_libraries(qtim-fcitx ${X11_LIBRARIES})
endif()

install(
  TARGETS qtim-fcitx
  LIBRARY DESTINATION
  "${QT_PLUGINS_DIR}/inputmethods")
