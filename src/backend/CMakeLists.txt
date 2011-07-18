add_subdirectory(xim)

if (_ENABLE_DBUS)
    #add_subdirectory(fbterm)
    add_subdirectory(ipc)

    if (ENABLE_GTK2_IM_MODULE OR ENABLE_GTK3_IM_MODULE)

    PKG_CHECK_MODULES(GLIB2 REQUIRED "glib-2.0" )
    PKG_CHECK_MODULES(DBUS_GLIB REQUIRED "dbus-glib-1")

    _pkgconfig_invoke("glib-2.0" GLIB2 GLIB_GENMARSHAL "" "--variable=glib_genmarshal")

    FIND_PROGRAM(GLIB_GENMARSHAL ${GLIB2_GLIB_GENMARSHAL})

    endif (ENABLE_GTK2_IM_MODULE OR ENABLE_GTK3_IM_MODULE)

    add_subdirectory(gtk2)
    add_subdirectory(gtk3)
    add_subdirectory(qt)
endif (_ENABLE_DBUS)