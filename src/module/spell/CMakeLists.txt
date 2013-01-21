set(FCITX_SPELL_SOURCES
  spell.c
  spell-custom.c
  spell-custom-dict.c
  spellconfig.c
  )

if(ENABLE_ENCHANT)
  set(FCITX_SPELL_SOURCES ${FCITX_SPELL_SOURCES} spell-enchant.c)
endif()
if(ENABLE_PRESAGE)
  set(FCITX_SPELL_SOURCES ${FCITX_SPELL_SOURCES} spell-presage.c)
endif()

link_directories(${DL_LIBRARY_DIRS})

fcitx_add_addon_full(spell SCAN SCAN_PRIV DESC
  SOURCES ${FCITX_SPELL_SOURCES}
  HEADERS spell.h
  LINK_LIBS ${DL_LIBRARIES}
  EXTRA_PO spell-enchant.c spell-presage.c)

add_subdirectory(dict)
