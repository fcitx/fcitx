set(FCITX_QUWEI_SOURCES
  qw.c
  )

fcitx_add_addon_full(qw
  SOURCES ${FCITX_QUWEI_SOURCES}
  IM_CONFIG qw.conf)
