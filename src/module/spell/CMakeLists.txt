include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_SPELL_SOURCES
    spell.c
    spell-custom.c
    spellconfig.c
)

if(ENABLE_ENCHANT)
  set(FCITX_SPELL_SOURCES ${FCITX_SPELL_SOURCES} spell-enchant.c)
endif()
if(ENABLE_PRESAGE)
  set(FCITX_SPELL_SOURCES ${FCITX_SPELL_SOURCES} spell-presage.c)
endif()

link_directories(${DL_LIBRARY_DIRS})

fcitx_add_addon(fcitx-spell ${FCITX_SPELL_SOURCES})
target_link_libraries(fcitx-spell ${DL_LIBRARIES})

fcitx_add_addon_conf_file(fcitx-spell.conf)
fcitx_add_configdesc_file(fcitx-spell.desc)
fcitx_add_addon_header(spell spell.h)

install(FILES en_dict.txt DESTINATION "${pkgdatadir}/data")
