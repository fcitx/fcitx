include_directories(${PROJECT_SOURCE_DIR}/src)

set(FCITX_SPELL_SOURCES
    spell.c
    spellconfig.c
)

fcitx_add_addon(fcitx-spell ${FCITX_SPELL_SOURCES})
fcitx_add_addon_conf_file(fcitx-spell.conf)
fcitx_add_configdesc_file(fcitx-spell.desc)
fcitx_add_addon_header(spell spell.h)
