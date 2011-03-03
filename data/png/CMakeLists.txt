set(FCITX_PNG_FILES
    fcitx.png
    fcitx-cangjie.png
    fcitx-chn.png
    fcitx-eng.png
    fcitx-erbi.png
    fcitx-full-letter.png
    fcitx-full-punct.png
    fcitx-vkon.png
    fcitx-vkoff.png
    fcitx-half-letter.png
    fcitx-half-punct.png
    fcitx-legend.png
    fcitx-nolegend.png
    fcitx-pinyin.png
    fcitx-simp.png
    fcitx-trad.png
    fcitx-wubi.png
    fcitx-ziranma.png
)

install(FILES ${FCITX_PNG_FILES} DESTINATION ${prefix}/share/pixmaps )
