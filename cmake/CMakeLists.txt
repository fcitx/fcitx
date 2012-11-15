install(FILES FcitxConfig.cmake FcitxMacro.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/FcitxConfigVersion.cmake"
  DESTINATION "${prefix}/share/cmake/fcitx/")
install(FILES
  fcitx-merge-config.sh
  fcitx-cmake-helper.sh
  fcitx-extract-gettext.sh
  fcitx-extract-confdesc.sh
  fcitx-extract-desktop.sh
  fcitx-extract-po.sh
  fcitx-extract-qt.sh
  fcitx-extract-kde.sh
  DESTINATION "${prefix}/share/cmake/fcitx/"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES
  fcitx-parse-po.sh
  fcitx-write-po.sh
  DESTINATION "${prefix}/share/cmake/fcitx/"
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
