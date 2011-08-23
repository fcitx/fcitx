set(FCITX_PNG_32_FILES
fcitx-fullwidth-active.png
fcitx-fullwidth-inactive.png
fcitx-punc-active.png
fcitx-punc-inactive.png
fcitx-vk-active.png
fcitx-vk-inactive.png
)

set(FCITX_PNG_48_FILES
fcitx-cangjie.png
fcitx-chn.png
fcitx-chttrans-active.png
fcitx-chttrans-inactive.png
fcitx-eng.png
fcitx-erbi.png
fcitx-remind-active.png
fcitx-remind-inactive.png
fcitx-pinyin.png
fcitx-wbpy.png
fcitx-wubi.png
fcitx-ziranma.png
fcitx-shuangpin.png
)

foreach(size 16 22 24 32 48 128)
    install(FILES fcitx-${size}.png RENAME fcitx.png DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach(size 16 22 24 32 48 128)

install(FILES ${FCITX_PNG_32_FILES} DESTINATION share/icons/hicolor/32x32/status )
install(FILES ${FCITX_PNG_48_FILES} DESTINATION share/icons/hicolor/48x48/status )
