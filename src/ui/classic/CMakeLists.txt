set(FCITX_CLASSIC_UI_LINK_LIBS)
if(_ENABLE_CAIRO)
  include_directories(${CAIRO_XLIB_INCLUDE_DIRS})
  link_directories(${CAIRO_XLIB_LIBRARY_DIRS})
  set(FCITX_CLASSIC_UI_LINK_LIBS ${FCITX_CLASSIC_UI_LINK_LIBS}
    ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES})
  if(_ENABLE_PANGO)
    include_directories(${PANGOCAIRO_INCLUDE_DIRS})
    link_directories(${PANGOCAIRO_LIBRARY_DIRS})
    set(FCITX_CLASSIC_UI_LINK_LIBS ${FCITX_CLASSIC_UI_LINK_LIBS}
      ${PANGOCAIRO_LIBRARIES})
  else()
    include_directories(${FONTCONFIG_INCLUDE_DIRS})
    link_directories(${FONTCONFIG_LIBRARY_DIRS})
    set(FCITX_CLASSIC_UI_LINK_LIBS ${FCITX_CLASSIC_UI_LINK_LIBS}
      ${FONTCONFIG_LIBRARIES})
  endif()
  add_custom_target(installer-desktopfile ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/fcitx-skin-installer.desktop")

  install(FILES fcitx-skin-installer.sh RENAME fcitx-skin-installer
    DESTINATION "${bindir}"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  fcitx_translate_add_apply_source(
    "fcitx-skin-installer.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fcitx-skin-installer.desktop")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx-skin-installer.desktop"
    DESTINATION share/applications/)
  install(FILES x-fskin.xml DESTINATION share/mime/packages/)
  set(classicui_noinstall)
else()
  set(classicui_noinstall NO_INSTALL)
endif()

set(FCITX_CLASSIC_UI_SOURCES
  classicui.c
  classicuiconfig.c
  InputWindow.c
  MainWindow.c
  MenuWindow.c
  MessageWindow.c
  TrayWindow.c
  tray.c
  skinconfig.c
  skin.c
  ../cairostuff/font.c
  ../cairostuff/cairostuff.c
  )

fcitx_add_addon_full(classic-ui DESC SCAN SCAN_PRIV ${classicui_noinstall}
  HEADER_DIR classicui
  FXADDON_SRC fcitx-classicui.fxaddon
  FXADDON_GEN fcitx-classicui.h
  SOURCES ${FCITX_CLASSIC_UI_SOURCES}
  HEADERS classicuiinterface.h
  LINK_LIBS ${FCITX_CLASSIC_UI_LINK_LIBS}
  EXTRA_DESC skin.desc
  EXTRA_PO fcitx-skin-installer.desktop.in fcitx-skin-installer.sh)
