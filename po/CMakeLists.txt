file(GLOB PO_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.po)

foreach(po_file ${PO_FILES})
  string(REPLACE ".po" "" po_lang "${po_file}")
  fcitx_translate_add_po_file("${po_lang}" "${po_file}")
endforeach()
fcitx_translate_set_pot_target(pot fcitx fcitx.pot)

install(FILES getdescpo DESTINATION share/cmake/fcitx
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
