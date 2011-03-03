
set(FCITX_QUWEI_SOURCES
    qw.c
)

add_library(fcitx-qw MODULE ${FCITX_QUWEI_SOURCES})
set_target_properties( fcitx-qw PROPERTIES PREFIX "")

install(TARGETS fcitx-qw DESTINATION ${libdir}/fcitx/)
install(FILES fcitx-qw.conf DESTINATION share/fcitx/data/addon)