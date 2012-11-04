install(FILES FcitxConfig.cmake FcitxMacro.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/FcitxConfigVersion.cmake"
  fcitx-filter-po.sh
  fcitx-merge-config.sh
  fcitx-scan-pot.sh
  DESTINATION "${prefix}/share/cmake/fcitx/")
