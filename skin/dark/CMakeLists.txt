set(FCITX_DARK_SKIN_FILES
active.png  bingchan.png erbi.png  fcitx_skin.conf 
input.png quwei.png wubi.png wbpy.png ziranma.png
bar.png cangjie.png  cn.png en.png  inactive.png logo.png
dianbaoma.png shuangpin.png wanfeng.png pinyin.png prev.png next.png menu.png keyboard.png
remind_active.png remind_inactive.png vk_active.png vk_inactive.png
punc_active.png punc_inactive.png fullwidth_active.png fullwidth_inactive.png
chttrans_active.png chttrans_inactive.png
)

install(FILES ${FCITX_DARK_SKIN_FILES} DESTINATION ${pkgdatadir}/skin/dark)