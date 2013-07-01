set(DICT_COMP_SRC
  comp_spell_dict.c
  )

add_executable(comp-spell-dict ${DICT_COMP_SRC})
target_link_libraries(comp-spell-dict fcitx-utils)
install(TARGETS comp-spell-dict
  DESTINATION "${addondir}/libexec")

set(SPELL_EN_DICT_VER 20121020)
set(SPELL_EN_DICT "${CMAKE_CURRENT_BINARY_DIR}/en_dict.fscd")
set(SPELL_EN_DICT_SRC "${CMAKE_CURRENT_BINARY_DIR}/en_dict.txt")
set(SPELL_EN_DICT_TAR "en_dict-${SPELL_EN_DICT_VER}.tar.gz")
set(SPELL_EN_DICT_URL
  "http://download.fcitx-im.org/data/en_dict-${SPELL_EN_DICT_VER}.tar.gz")

fcitx_download(spell-en-download "${SPELL_EN_DICT_URL}" "${SPELL_EN_DICT_TAR}"
  MD5SUM 8315f85331e0545c256a46e0cb00f10f)
fcitx_extract(spell-en-extract "${SPELL_EN_DICT_TAR}" DEPENDS spell-en-download
  OUTPUT ${SPELL_EN_DICT_SRC})

set(COMP_SPELL_DICT
  "${PROJECT_BINARY_DIR}/src/module/spell/dict/comp-spell-dict")
add_custom_command(
  OUTPUT "${SPELL_EN_DICT}"
  DEPENDS "${SPELL_EN_DICT_SRC}" "${COMP_SPELL_DICT}" spell-en-extract
  COMMAND "${COMP_SPELL_DICT}" --comp-dict
  "${SPELL_EN_DICT_SRC}" "${SPELL_EN_DICT}")
add_custom_target(spell_en_dict ALL DEPENDS "${SPELL_EN_DICT}")
install(FILES "${SPELL_EN_DICT}" DESTINATION "${pkgdatadir}/spell")
