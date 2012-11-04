set(FCITX_CLIPBOARD_SOURCES
  clipboard.c
  clipboardconfig.c
  )

if(ENABLE_X11)
  set(FCITX_CLIPBOARD_SOURCES ${FCITX_CLIPBOARD_SOURCES} clipboard-x11.c)
endif()

fcitx_add_addon_full(clipboard SCAN DESC
  SOURCES ${FCITX_CLIPBOARD_SOURCES}
  HEADERS clipboard.h
  EXTRA_PO clipboard-x11.c)
