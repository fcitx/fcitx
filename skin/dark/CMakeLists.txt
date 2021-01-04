file(GLOB SKIN_FILES *.png)

fcitx_translate_add_sources(
  fcitx_skin.conf.in)
fcitx_translate_add_apply_source(
  fcitx_skin.conf.in
  "${CMAKE_CURRENT_BINARY_DIR}/fcitx_skin.conf")

install(FILES ${SKIN_FILES} "${CMAKE_CURRENT_BINARY_DIR}/fcitx_skin.conf" DESTINATION ${pkgdatadir}/skin/dark)
