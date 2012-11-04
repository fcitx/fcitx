install(FILES FcitxConfig.cmake FcitxMacro.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/FcitxConfigVersion.cmake"
  DESTINATION "${prefix}/share/cmake/fcitx/")
install(FILES
  fcitx-filter-po.sh
  fcitx-merge-config.sh
  fcitx-scan-pot.sh
  fcitx-extract-cpp.sh
  fcitx-extract-confdesc.sh
  fcitx-extract-desktop.sh
  DESTINATION "${prefix}/share/cmake/fcitx/"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
