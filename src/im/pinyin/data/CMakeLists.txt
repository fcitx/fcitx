
set(PY_ORGDATA pyPhrase.org)
set(PY_DATA "${CMAKE_CURRENT_BINARY_DIR}/pybase.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb")
set(PY_TAR pinyin.tar.gz)
set(PY_URL "http://fcitx.googlecode.com/files/pinyin.tar.gz")

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

install(FILES ${FCITX_PY_FILES} "${CMAKE_CURRENT_BINARY_DIR}/pybase.mb"
  "${CMAKE_CURRENT_BINARY_DIR}/pyphrase.mb" DESTINATION "${pkgdatadir}/pinyin")

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
  "${CMAKE_CURRENT_BINARY_DIR}/pyERROR;${CMAKE_CURRENT_BINARY_DIR}/pyPhrase.ok")