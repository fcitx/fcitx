set(FCITX_UTILS_SOURCES
    configfile.c
    profile.c
    utf8.c
    utils.c
)

add_library(utils STATIC ${FCITX_UTILS_SOURCES})