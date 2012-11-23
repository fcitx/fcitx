include_directories(lib)
include_directories(
  ${QT_INCLUDE_DIR}
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  )

add_subdirectory(lib)
add_subdirectory(quickphrase-editor)
add_subdirectory(wrapper)
