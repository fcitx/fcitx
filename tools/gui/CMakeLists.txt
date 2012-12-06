include_directories(
  ${QT_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )


add_subdirectory(quickphrase-editor)
add_subdirectory(wrapper)
