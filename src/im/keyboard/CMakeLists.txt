include_directories (
  ${LIBINTL_INCLUDE_DIR}
  ${LIBICONV_INCLUDE_DIR}
  )
set(FCITX_KEYBOARD_LINK_LIBS ${LIBICONV_LIBRARIES})
if(ENABLE_LIBXML2)
  include_directories(${LIBXML2_INCLUDE_DIR})
  set(FCITX_KEYBOARD_LINK_LIBS ${FCITX_KEYBOARD_LINK_LIBS} ${LIBXML2_LIBRARIES})
endif()

set(FCITX_KEYBOARD_SOURCES
  keyboard.c
  keyboardconfig.c
  )

if(ENABLE_LIBXML2)
  set(FCITX_KEYBOARD_SOURCES ${FCITX_KEYBOARD_SOURCES} isocodes.c)
endif()

fcitx_add_addon_full(keyboard DESC
  SOURCES ${FCITX_KEYBOARD_SOURCES}
  LINK_LIBS ${FCITX_KEYBOARD_LINK_LIBS} XKBCommon::XKBCommon
  EXTRA_PO isocodes.c isocodes.h keyboard.h)
