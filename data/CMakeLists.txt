add_subdirectory(script)
add_subdirectory(icon)

set(FCITX_DESC_FILES
  addon.desc
  config.desc
  profile.desc
  inputmethod.desc
  )

set(FCITX_DATA_FILES
  AutoEng.dat
  gbks2t.tab
  punc.mb.zh_CN
  punc.mb.zh_HK
  punc.mb.zh_TW
  vk.conf
  )

set(FCITX_DSEKTOP_FILES
  fcitx.desktop
  fcitx-configtool.desktop
  fcitx-autostart.desktop
  )

fcitx_translate_add_sources(
  fcitx-configtool.desktop.in
  fcitx.desktop.in
  fcitx-autostart.desktop.in)
fcitx_translate_add_apply_source(
  fcitx-configtool.desktop.in
  "${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop")
fcitx_translate_add_apply_source(
  fcitx.desktop.in
  "${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop")
fcitx_translate_add_apply_source(
  fcitx-autostart.desktop.in
  "${CMAKE_CURRENT_BINARY_DIR}/fcitx-autostart.desktop")

add_custom_target(desktopfile ALL DEPENDS ${FCITX_DSEKTOP_FILES})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop"
  DESTINATION share/applications/)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop"
  DESTINATION share/applications/)

install(FILES ${FCITX_DATA_FILES} DESTINATION "${pkgdatadir}/data")
fcitx_install_addon_desc(${FCITX_DESC_FILES})

if(_ENABLE_DBUS AND ENABLE_XDGAUTOSTART)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx-autostart.desktop"
    DESTINATION "${SYSCONFDIR}/xdg/autostart")
endif()

if(_ENABLE_DBUS)
  install(FILES daemon.conf DESTINATION share/fcitx/dbus/)
endif()

install(FILES env_setup.sh DESTINATION share/fcitx/data/)
