set(FCITX_CLIPBOARD_SOURCES
  clipboard.c
  clipboardconfig.c
  )

# do not disable clipboard-x11 even if x11 is diabled, since fcitx-scanner and
# fcitx_add_addon_full is capable for doing this.
if(1)
  set(FCITX_CLIPBOARD_SOURCES ${FCITX_CLIPBOARD_SOURCES} clipboard-x11.c)
endif()

fcitx_add_addon_full(clipboard SCAN SCAN_PRIV DESC
  SOURCES ${FCITX_CLIPBOARD_SOURCES}
  HEADERS clipboard.h
  EXTRA_PO clipboard-x11.c)
