if (_ENABLE_CAIRO)
    add_subdirectory(classic)
endif (_ENABLE_CAIRO)

if (_ENABLE_DBUS)
    add_subdirectory(kimpanel)
endif (_ENABLE_DBUS)