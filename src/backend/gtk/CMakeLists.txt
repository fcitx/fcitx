if(ENABLE_GTK2_IM_MODULE)

PKG_CHECK_MODULES(GTK2 "gtk+-2.0" )

include_directories(${GTK2_INCLUDE_DIRS})

set(FCITX_GTK2_IM_MODULE_SOURCES
    fcitxim.c
    fcitximcontext.c
)

add_library(im-fcitx MODULE ${FCITX_GTK2_IM_MODULE_SOURCES})
set_target_properties( im-fcitx PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden")

install(TARGETS im-fcitx DESTINATION ${addondir})

endif(ENABLE_GTK2_IM_MODULE)
