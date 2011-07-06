set(FCITX_DEFAULT_SKIN_FILES
chttrans_active.png chttrans_inactive.png
fullwidth_active.png fullwidth_inactive.png
punc_active.png punc_inactive.png
active.png  bingchan.png erbi.png  fcitx_skin.conf 
input.png  remind_active.png remind_inactive.png  quwei.png vk_active.png vk_inactive.png
wubi.png wbpy.png ziranma.png
bar.png cangjie.png  cn.png en.png  inactive.png logo.png
dianbaoma.png shuangpin.png wanfeng.png pinyin.png prev.png next.png keyboard.png menu.png
)

install(FILES ${FCITX_DEFAULT_SKIN_FILES} DESTINATION ${pkgdatadir}/skin/default)