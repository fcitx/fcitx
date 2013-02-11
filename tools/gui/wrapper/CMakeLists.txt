set(QT_WRAPPER_SRCS
  main.cpp
  wrapperapp.cpp
  mainwindow.cpp
  )
set(QT_WRAPPER_HDRS
  wrapperapp.h
  mainwindow.h
  )
fcitx_translate_add_sources(
  ${QT_WRAPPER_SRCS}
  ${QT_WRAPPER_HDRS})

if(NOT (ENABLE_QT AND ENABLE_QT_GUI))
  return()
endif()

include_directories(
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

qt4_wrap_ui(QT_WRAPPER_SRCS mainwindow.ui)
qt4_wrap_cpp(QT_WRAPPER_MOCS ${QT_WRAPPER_HDRS})
add_executable(fcitx-qt-gui-wrapper ${QT_WRAPPER_SRCS} ${QT_WRAPPER_MOCS})
target_link_libraries(fcitx-qt-gui-wrapper
  debug ${QT_QTCORE_LIBRARY_DEBUG}
  debug ${QT_QTDBUS_LIBRARY_DEBUG}
  debug ${QT_QTGUI_LIBRARY_DEBUG}
  optimized ${QT_QTCORE_LIBRARY_RELEASE}
  optimized ${QT_QTDBUS_LIBRARY_RELEASE}
  optimized ${QT_QTGUI_LIBRARY_RELEASE}
  fcitx-utils
  fcitx-config
  fcitx-qt
  )

install(TARGETS fcitx-qt-gui-wrapper DESTINATION ${addondir}/libexec)
