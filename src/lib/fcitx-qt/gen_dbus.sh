#!/bin/sh

qdbusxml2cpp -N -p fcitxqtinputcontextproxy -c FcitxQtInputContextProxy interfaces/org.fcitx.Fcitx.InputContext.xml -i fcitxqtformattedpreedit.h -i fcitxqt_export.h
qdbusxml2cpp -N -p fcitxqtinputmethodproxy -c FcitxQtInputMethodProxy interfaces/org.fcitx.Fcitx.InputMethod.xml -i fcitxqt_export.h
