if (ENABLE_QT_IM_MODULE)
find_package(Qt4 4.5 COMPONENTS QtCore QtGui QtDBus REQUIRED)
find_package(ICU REQUIRED)

include_directories(
    ${QT_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${X11_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

link_directories(
    ${X11_LIBRARY_DIRS}
    ${DBUS_LIBRARY_DIRS}
)

set(
    qtim_fcitx_SRCS
    fcitx-plugin.cpp
    fcitx-input-context.cpp
)

QT4_ADD_DBUS_INTERFACE(qtim_fcitx_SRCS
    org.fcitx.Fcitx.InputContext.xml
    org.fcitx.Fcitx.InputContext
)

QT4_ADD_DBUS_INTERFACE(qtim_fcitx_SRCS
    org.fcitx.Fcitx.InputMethod.xml
    org.fcitx.Fcitx.InputMethod
)

QT4_ADD_DBUS_INTERFACE(qtim_fcitx_SRCS
    org.freedesktop.DBus.xml
    org.freedesktop.DBus
)


set(
    qtim_fcitx_MOC_HDRS
    fcitx-input-context.h
)

QT4_AUTOMOC(${qtim_fcitx_SRCS})
QT4_WRAP_CPP(
    qtim_fcitx_MOC_SRCS
    ${qtim_fcitx_MOC_HDRS}
)

add_library(
    qtim-fcitx MODULE
    ${qtim_fcitx_SRCS}
    ${qtim_fcitx_MOC_SRCS}
)

set_target_properties( qtim-fcitx PROPERTIES PREFIX "" COMPILE_FLAGS "-fvisibility=hidden" LINK_FLAGS "-Wl,--no-undefined")

target_link_libraries(
    qtim-fcitx
    debug ${QT_QTCORE_LIBRARY_DEBUG}
    debug ${QT_QTDBUS_LIBRARY_DEBUG}
    debug ${QT_QTGUI_LIBRARY_DEBUG}
    optimized ${QT_QTCORE_LIBRARY_RELEASE}
    optimized ${QT_QTDBUS_LIBRARY_RELEASE}
    optimized ${QT_QTGUI_LIBRARY_RELEASE}
    ${X11_LIBRARIES}
    ${ICU_LIBRARIES}
    fcitx-config
)

install(
    TARGETS qtim-fcitx
    LIBRARY DESTINATION
    ${QT_PLUGINS_DIR}/inputmethods)

endif (ENABLE_QT_IM_MODULE)