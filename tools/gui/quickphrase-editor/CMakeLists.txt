set(QUICKPHRASE_EDITOR_SRCS
  main.cpp
  model.cpp
  editor.cpp
  editordialog.cpp
  batchdialog.cpp
  )
set(QUICKPHRASE_EDITOR_HDRS
  main.h
  model.h
  editor.h
  editordialog.h
  batchdialog.h
  )
fcitx_translate_add_sources(
  ${QUICKPHRASE_EDITOR_HDRS}
  ${QUICKPHRASE_EDITOR_SRCS})

if(NOT (ENABLE_QT AND ENABLE_QT_GUI))
  return()
endif()

include_directories(
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
qt4_wrap_ui(QUICKPHRASE_EDITOR_SRCS editordialog.ui batchdialog.ui editor.ui)
qt4_wrap_cpp(QUICKPHRASE_EDITOR_MOCS ${QUICKPHRASE_EDITOR_HDRS})
add_library(fcitx-quickphrase-editor
  MODULE ${QUICKPHRASE_EDITOR_SRCS} ${QUICKPHRASE_EDITOR_MOCS})

set_target_properties(fcitx-quickphrase-editor PROPERTIES
                      LINK_FLAGS "-Wl,--no-undefined"
)
target_link_libraries(fcitx-quickphrase-editor
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

install(TARGETS fcitx-quickphrase-editor DESTINATION ${libdir}/fcitx/qt)
