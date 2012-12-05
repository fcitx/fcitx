

QT4_WRAP_CPP(
    testqconnection_MOC_SRCS
    testconnectionapp.h
)

add_executable(testqconnection testconnection.cpp ${testqconnection_MOC_SRCS})

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
