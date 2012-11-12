#!/bin/sh

qdbusxml2cpp -N -p fcitxinputcontextproxy -c FcitxInputContextProxy interfaces/org.fcitx.Fcitx.InputContext.xml -i fcitxformattedpreedit.h -i fcitxqt_export.h
qdbusxml2cpp -N -p fcitxinputmethodproxy -c FcitxInputMethodProxy interfaces/org.fcitx.Fcitx.InputMethod.xml -i fcitxqt_export.h
