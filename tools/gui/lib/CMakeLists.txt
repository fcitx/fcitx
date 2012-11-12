include_directories(
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

set(EDITOR_LIB_SRCS
    editor.cpp
    dialog.cpp
    abstractmodel.cpp
    )
set(EDITOR_LIB_HDRS
    editor.h
    dialog.h
    abstractmodel.h
    )

qt4_wrap_cpp(EDITOR_LIB_MOCS ${EDITOR_LIB_HDRS})
qt4_wrap_ui(EDITOR_LIB_SRCS editor.ui dialog.ui)
add_library(editor STATIC ${EDITOR_LIB_SRCS} ${EDITOR_LIB_MOCS})

set_target_properties(editor
                      PROPERTIES COMPILE_FLAGS "-fPIC"
)