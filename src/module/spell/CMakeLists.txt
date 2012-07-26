include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_SPELL_SOURCES
    spell.c
    spell-custom.c
    spellconfig.c
)

if(ENCHANT_FOUND)
  include_directories(${ENCHANT_INCLUDE_DIR})
  set(FCITX_SPELL_SOURCES ${FCITX_SPELL_SOURCES} spell-enchant.c)
endif(ENCHANT_FOUND)
if(PRESAGE_FOUND)
  include_directories(${PRESAGE_INCLUDE_DIR})
  set(FCITX_SPELL_SOURCES ${FCITX_SPELL_SOURCES} spell-presage.c)
endif(PRESAGE_FOUND)

fcitx_add_addon(fcitx-spell ${FCITX_SPELL_SOURCES})
if(ENCHANT_FOUND)
  target_link_libraries(fcitx-spell ${ENCHANT_LIBRARIES})
endif(ENCHANT_FOUND)
if(PRESAGE_FOUND)
  target_link_libraries(fcitx-spell ${PRESAGE_LIBRARIES})
endif(PRESAGE_FOUND)

fcitx_add_addon_conf_file(fcitx-spell.conf)
fcitx_add_configdesc_file(fcitx-spell.desc)
fcitx_add_addon_header(spell spell.h)
