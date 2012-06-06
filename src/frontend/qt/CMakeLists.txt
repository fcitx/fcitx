if (ENABLE_QT_IM_MODULE)
find_package(Qt4 4.5 COMPONENTS QtCore QtGui QtDBus REQUIRED)

# use newer macro from Qt4Macro.cmake
MACRO(FCITX_QT4_ADD_DBUS_INTERFACE _sources _interface _basename)
  GET_FILENAME_COMPONENT(_infile ${_interface} ABSOLUTE)
  SET(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
  SET(_impl   ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp)
  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc)

  GET_SOURCE_FILE_PROPERTY(_nonamespace ${_interface} NO_NAMESPACE)
  IF(_nonamespace)
    SET(_params -N -m)
  ELSE(_nonamespace)
    SET(_params -m)
  ENDIF(_nonamespace)

  GET_SOURCE_FILE_PROPERTY(_classname ${_interface} CLASSNAME)
  IF(_classname)
    SET(_params ${_params} -c ${_classname})
  ENDIF(_classname)

  GET_SOURCE_FILE_PROPERTY(_include ${_interface} INCLUDE)
  IF(_include)
    SET(_params ${_params} -i ${_include})
  ENDIF(_include)

  ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
      COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} ${_params} -p ${_basename} ${_infile}
      DEPENDS ${_infile} VERBATIM)

  SET_SOURCE_FILES_PROPERTIES(${_impl} PROPERTIES SKIP_AUTOMOC TRUE)

  QT4_GENERATE_MOC(${_header} ${_moc})

  SET(${_sources} ${${_sources}} ${_impl} ${_header} ${_moc})
  MACRO_ADD_FILE_DEPENDENCIES(${_impl} ${_moc})

ENDMACRO(FCITX_QT4_ADD_DBUS_INTERFACE)

include_directories(
    ${QT_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTGUI_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${X11_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

link_directories(
    ${X11_LIBRARY_DIRS}
    ${DBUS_LIBRARY_DIRS}
)

set(
    qtim_fcitx_SRCS
    fcitx-plugin.cpp
    fcitx-input-context.cpp
    fcitx-formatted-preedit.cpp
)

set_property(
    SOURCE org.fcitx.Fcitx.InputContext.xml
    PROPERTY INCLUDE fcitx-formatted-preedit.h
)

FCITX_QT4_ADD_DBUS_INTERFACE(qtim_fcitx_SRCS
    org.fcitx.Fcitx.InputContext.xml
    org.fcitx.Fcitx.InputContext
)

FCITX_QT4_ADD_DBUS_INTERFACE(qtim_fcitx_SRCS
    org.fcitx.Fcitx.InputMethod.xml
    org.fcitx.Fcitx.InputMethod
)

FCITX_QT4_ADD_DBUS_INTERFACE(qtim_fcitx_SRCS
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
