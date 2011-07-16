add_subdirectory(xim)

if (ENABLE_DBUS)
    #add_subdirectory(fbterm)
    add_subdirectory(ipc)
    add_subdirectory(gtk2)
endif (ENABLE_DBUS)