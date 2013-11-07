set(QUICKPHRASE_EDITOR_SRCS 
  main.cpp
  model.cpp
  editor.cpp
  editordialog.cpp
  batchdialog.cpp
  filelistmodel.cpp
  )
fcitx_translate_add_sources(
  ${QUICKPHRASE_EDITOR_SRCS})

if(NOT (ENABLE_QT AND ENABLE_QT_GUI))
  return()
endif()

include_directories(
  ${QT_QTCORE_INCLUDE_DIR}
  ${QT_QTGUI_INCLUDE_DIR}
  ${LIBINTL_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
qt4_wrap_ui(QUICKPHRASE_EDITOR_SRCS editordialog.ui batchdialog.ui editor.ui)
add_library(fcitx-quickphrase-editor
  MODULE ${QUICKPHRASE_EDITOR_SRCS})

set_target_properties(fcitx-quickphrase-editor PROPERTIES
                      LINK_FLAGS "-Wl,--no-undefined"
                      AUTOMOC TRUE
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
