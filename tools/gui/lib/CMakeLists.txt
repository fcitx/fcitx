include_directories(
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
set(EDITOR_LIB_UIS
  editor.ui
  dialog.ui
  )

fcitx_translate_add_sources(${EDITOR_LIB_HDRS} ${EDITOR_LIB_UIS}
  ${EDITOR_LIB_SRCS})

qt4_wrap_cpp(EDITOR_LIB_MOCS ${EDITOR_LIB_HDRS})
qt4_wrap_ui(EDITOR_LIB_SRCS ${EDITOR_LIB_UIS})
add_library(editor STATIC ${EDITOR_LIB_SRCS} ${EDITOR_LIB_MOCS})

# necessary??
set_target_properties(editor
  PROPERTIES COMPILE_FLAGS "-fPIC"
  )
