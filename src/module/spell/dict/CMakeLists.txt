set(DICT_COMP_SRC
  comp_spell_dict.c
  )

add_executable(comp-spell-dict ${DICT_COMP_SRC})
target_link_libraries(comp-spell-dict fcitx-utils)
install(TARGETS comp-spell-dict
  DESTINATION "${prefix}/share/cmake/fcitx/")
