/*
 * Copyright (C) 2011~2020 by CSSlayer
 * wengxt@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above Copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above Copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */
#include "qfcitxinputcontext.h"
#include <QDBusConnection>
#include <QInputContextPlugin>

/* The class Definition */
class QFcitxInputContextPlugin : public QInputContextPlugin {

private:
    /**
     * The language list for Fcitx.
     */
    static QStringList fcitx_languages;

public:
    QFcitxInputContextPlugin(QObject *parent = 0);

    ~QFcitxInputContextPlugin();

    QStringList keys() const;

    QStringList languages(const QString &key);

    QString description(const QString &key);

    QInputContext *create(const QString &key);

    QString displayName(const QString &key);

private:
};

/* Implementations */
QStringList QFcitxInputContextPlugin::fcitx_languages;

QFcitxInputContextPlugin::QFcitxInputContextPlugin(QObject *parent)
    : QInputContextPlugin(parent) {}

QFcitxInputContextPlugin::~QFcitxInputContextPlugin() {}

QStringList QFcitxInputContextPlugin::keys() const {
    QStringList identifiers;
    identifiers.push_back(FCITX_IDENTIFIER_NAME);
    return identifiers;
}

QStringList QFcitxInputContextPlugin::languages(const QString &key) {
    if (key.toLower() != FCITX_IDENTIFIER_NAME) {
        return QStringList();
    }

    if (fcitx_languages.empty()) {
        fcitx_languages.push_back("zh");
        fcitx_languages.push_back("ja");
        fcitx_languages.push_back("ko");
    }
    return fcitx_languages;
}

QString QFcitxInputContextPlugin::description(const QString &key) {
    if (key.toLower() != FCITX_IDENTIFIER_NAME) {
        return QString("");
    }

    return QString::fromUtf8("Qt immodule plugin for Fcitx");
}

QInputContext *QFcitxInputContextPlugin::create(const QString &key) {
    if (key.toLower() != FCITX_IDENTIFIER_NAME) {
        return NULL;
    }

    return static_cast<QInputContext *>(new QFcitxInputContext());
}

QString QFcitxInputContextPlugin::displayName(const QString &key) {
    return key;
}

Q_EXPORT_PLUGIN2(QFcitxInputContextPlugin, QFcitxInputContextPlugin)

// kate: indent-mode cstyle; space-indent on; indent-width 0;
