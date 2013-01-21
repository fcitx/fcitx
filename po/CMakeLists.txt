set(PO_LANGS de es ja zh_CN zh_TW)
foreach(lang ${PO_LANGS})
  fcitx_translate_add_po_file("${lang}" "${lang}.po")
endforeach()
fcitx_translate_set_pot_target(pot fcitx fcitx.pot)

install(FILES getdescpo DESTINATION share/cmake/fcitx
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
