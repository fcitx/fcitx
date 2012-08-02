add_subdirectory(man)

set(FCITX_DOC_FILES
	cjkvinput.txt wb_fh.htm pinyin.txt
	Develop_Readme API.txt
)

install(FILES ${FCITX_DOC_FILES} DESTINATION share/doc/${package} )
