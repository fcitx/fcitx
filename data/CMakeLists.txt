if(ENABLE_TABLE)
  add_subdirectory(table)
endif()
fcitx_translate_add_sources(
  table/qxm.conf.in
  table/wbx.conf.in
  table/wanfeng.conf.in
  table/db.conf.in
  table/wbpy.conf.in
  table/erbi.conf.in
  table/zrm.conf.in
  table/cangjie.conf.in
  )

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

set(FCITX_PY_FILES
  pySym.mb
  sp.dat
  )

set(FCITX_DSEKTOP_FILES
  fcitx.desktop
  fcitx-configtool.desktop
  fcitx-autostart.desktop
  )

set(PY_ORGDATA pyPhrase.org)
set(PY_DATA "${CMAKE_CURRENT_BINARY_DIR}/pybase.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb")
set(PY_TAR pinyin.tar.gz)
set(PY_URL "http://fcitx.googlecode.com/files/pinyin.tar.gz")

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

add_custom_target(pinyin_data ALL DEPENDS ${PY_DATA})

fcitx_download(py-data-download "${PY_URL}" "${PY_TAR}"
  MD5SUM 34dcb1b5209c28baa4e87f6a2773bfd0)
fcitx_extract(py-data-extract "${PY_TAR}" DEPENDS py-data-download
  OUTPUT ${PY_ORGDATA})
set(CREATE_PYMB_BIN "${PROJECT_BINARY_DIR}/tools/cli/createPYMB")
add_custom_command(OUTPUT ${PY_DATA}
  DEPENDS ${PY_ORGDATA} "${CREATE_PYMB_BIN}" py-data-extract
  COMMAND "${CREATE_PYMB_BIN}" "${CMAKE_CURRENT_SOURCE_DIR}/gbkpy.org"
  "${CMAKE_CURRENT_BINARY_DIR}/${PY_ORGDATA}")

add_custom_target(desktopfile ALL DEPENDS ${FCITX_DSEKTOP_FILES})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx.desktop"
  DESTINATION share/applications/)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx-configtool.desktop"
  DESTINATION share/applications/)

install(FILES ${FCITX_DATA_FILES} DESTINATION "${pkgdatadir}/data")
fcitx_install_addon_desc(${FCITX_DESC_FILES})
install(FILES ${FCITX_PY_FILES} "${CMAKE_CURRENT_BINARY_DIR}/pybase.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb" DESTINATION "${pkgdatadir}/pinyin")

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${CMAKE_CURRENT_BINARY_DIR}/pyERROR;${CMAKE_CURRENT_BINARY_DIR}/pyPhrase.ok")

if(_ENABLE_DBUS AND ENABLE_XDGAUTOSTART)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx-autostart.desktop"
    DESTINATION /etc/xdg/autostart)
endif()

if(_ENABLE_DBUS)
  install(FILES daemon.conf DESTINATION share/fcitx/dbus/)
endif()

set(PY_STROKE_VER 20121124)
set(PY_STROKE_TGT "${CMAKE_CURRENT_BINARY_DIR}/py_stroke.mb")
set(PY_STROKE_TAR "py_stroke-${PY_STROKE_VER}.tar.gz")
set(PY_STROKE_URL
  "http://fcitx.googlecode.com/files/py_stroke-${PY_STROKE_VER}.tar.gz")
fcitx_download(py-stroke-download "${PY_STROKE_URL}" "${PY_STROKE_TAR}"
  MD5SUM 2559d025c5bbb50fa450a02429f92762)
fcitx_extract(py-stroke-extract "${PY_STROKE_TAR}" DEPENDS py-stroke-download
  OUTPUT ${PY_STROKE_TGT})
install(FILES "${PY_STROKE_TGT}" DESTINATION "${pkgdatadir}/py-enhance")

set(PY_TABLE_VER 20121124)
set(PY_TABLE_TGT "${CMAKE_CURRENT_BINARY_DIR}/py_table.mb")
set(PY_TABLE_TAR "py_table-${PY_TABLE_VER}.tar.gz")
set(PY_TABLE_URL
  "http://fcitx.googlecode.com/files/py_table-${PY_TABLE_VER}.tar.gz")
fcitx_download(py-table-download "${PY_TABLE_URL}" "${PY_TABLE_TAR}"
  MD5SUM a72e275fe1916d67d01a2f038ca5d920)
fcitx_extract(py-table-extract "${PY_TABLE_TAR}" DEPENDS py-table-download
  OUTPUT ${PY_TABLE_TGT})
install(FILES "${PY_TABLE_TGT}" DESTINATION "${pkgdatadir}/py-enhance")

set(SPELL_EN_DICT_VER 20121020)
set(SPELL_EN_DICT "${CMAKE_CURRENT_BINARY_DIR}/en_dict.fscd")
set(SPELL_EN_DICT_SRC "${CMAKE_CURRENT_BINARY_DIR}/en_dict.txt")
set(SPELL_EN_DICT_TAR "en_dict-${SPELL_EN_DICT_VER}.tar.gz")
set(SPELL_EN_DICT_URL
  "http://fcitx.googlecode.com/files/en_dict-${SPELL_EN_DICT_VER}.tar.gz")

fcitx_download(spell-en-download "${SPELL_EN_DICT_URL}" "${SPELL_EN_DICT_TAR}"
  MD5SUM 8315f85331e0545c256a46e0cb00f10f)
fcitx_extract(spell-en-extract "${SPELL_EN_DICT_TAR}" DEPENDS spell-en-download
  OUTPUT ${SPELL_EN_DICT_SRC})

set(COMP_SPELL_DICT
  "${PROJECT_BINARY_DIR}/src/module/spell/dict/comp-spell-dict")
add_custom_command(
  OUTPUT "${SPELL_EN_DICT}"
  DEPENDS "${SPELL_EN_DICT_SRC}" "${COMP_SPELL_DICT}" spell-en-extract
  COMMAND "${COMP_SPELL_DICT}" "${SPELL_EN_DICT_SRC}" "${SPELL_EN_DICT}")
add_custom_target(spell_en_dict ALL DEPENDS "${SPELL_EN_DICT}")
install(FILES "${SPELL_EN_DICT}" DESTINATION "${pkgdatadir}/spell")
