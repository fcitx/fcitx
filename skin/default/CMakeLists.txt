set(FCITX_DEFAULT_SKIN_FILES
chttrans_active.png chttrans_inactive.png
active.png  bingchan.png chnpunc.png  engpunc.png  erbi.png  fcitx_skin.conf  half.png
input.png  legend.png  nolegend.png  quwei.png  vk.png wubi.png wbpy.png ziranma.png
bar.png cangjie.png  cn.png en.png  full.png inactive.png logo.png
dianbaoma.png novk.png shuangpin.png wanfeng.png pinyin.png prev.png next.png keyboard.png menu.png
)

install(FILES ${FCITX_DEFAULT_SKIN_FILES} DESTINATION ${pkgdatadir}/skin/default)