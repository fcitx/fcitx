
set(PY_STROKE_VER 20121124)
set(PY_STROKE_TGT "${CMAKE_CURRENT_BINARY_DIR}/py_stroke.mb")
set(PY_STROKE_TAR "py_stroke-${PY_STROKE_VER}.tar.gz")
set(PY_STROKE_URL
  "http://download.fcitx-im.org/data/py_stroke-${PY_STROKE_VER}.tar.gz")
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
