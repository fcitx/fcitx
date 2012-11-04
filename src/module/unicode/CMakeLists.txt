set(FCITX_UNICODE_SOURCES
  unicode.c
  charselectdata.c)

fcitx_add_addon_full(unicode DESC
  SOURCES ${FCITX_UNICODE_SOURCES})

install(FILES charselectdata DESTINATION ${pkgdatadir}/data/)
