set(FCITX_QUICKPHRASE_SOURCES
  quickphrase.c)

fcitx_add_addon_full(quickphrase SCAN SCAN_PRIV DESC
  SOURCES ${FCITX_QUICKPHRASE_SOURCES}
  HEADERS quickphrase.h)
  
install(DIRECTORY quickphrase.d DESTINATION "${pkgdatadir}/data"
         PATTERN ".*" EXCLUDE
         PATTERN "*~" EXCLUDE)
