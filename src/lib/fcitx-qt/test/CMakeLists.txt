include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(testkeysequencewidget testkeysequencewidget.cpp)

target_link_libraries(testkeysequencewidget
    debug ${QT_QTCORE_LIBRARY_DEBUG}
    debug ${QT_QTDBUS_LIBRARY_DEBUG}
    debug ${QT_QTGUI_LIBRARY_DEBUG}
    optimized ${QT_QTCORE_LIBRARY_RELEASE}
    optimized ${QT_QTDBUS_LIBRARY_RELEASE}
    optimized ${QT_QTGUI_LIBRARY_RELEASE}
    fcitx-utils
    fcitx-config
    fcitx-qt
    )

add_executable(testqconnection testconnection.cpp)
set_target_properties(testqconnection PROPERTIES AUTOMOC TRUE)

target_link_libraries(testqconnection
    debug ${QT_QTCORE_LIBRARY_DEBUG}
    debug ${QT_QTDBUS_LIBRARY_DEBUG}
    debug ${QT_QTGUI_LIBRARY_DEBUG}
    optimized ${QT_QTCORE_LIBRARY_RELEASE}
    optimized ${QT_QTDBUS_LIBRARY_RELEASE}
    optimized ${QT_QTGUI_LIBRARY_RELEASE}
    fcitx-utils
    fcitx-config
    fcitx-qt
    )
