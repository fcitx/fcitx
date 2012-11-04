set(FCITX_VK_LINK_LIBS)
if(_ENABLE_CAIRO)
  include_directories(${CAIRO_XLIB_INCLUDE_DIRS})
  link_directories(${CAIRO_XLIB_LIBRARY_DIRS})
  set(FCITX_VK_LINK_LIBS ${FCITX_VK_LINK_LIBS}
    ${X11_LIBRARIES} ${CAIRO_XLIB_LIBRARIES})
  if(_ENABLE_PANGO)
    include_directories(${PANGOCAIRO_INCLUDE_DIRS})
    link_directories(${PANGOCAIRO_LIBRARY_DIRS})
    set(FCITX_VK_LINK_LIBS ${FCITX_VK_LINK_LIBS} ${PANGOCAIRO_LIBRARIES})
  else()
    include_directories(${FONTCONFIG_INCLUDE_DIRS})
    link_directories(${FONTCONFIG_LIBRARY_DIRS})
    set(FCITX_VK_LINK_LIBS ${FCITX_VK_LINK_LIBS} ${FONTCONFIG_LIBRARIES})
  endif()
  set(vk_noinstall)
else()
  set(vk_noinstall NO_INSTALL)
endif()

set(FCITX_VK_SOURCES
  vk.c
  ../../ui/cairostuff/cairostuff.c
  ../../ui/cairostuff/font.c
  )

fcitx_add_addon_full(vk ${vk_noinstall}
  SOURCES ${FCITX_VK_SOURCES}
  LINK_LIBS ${FCITX_VK_LINK_LIBS})
