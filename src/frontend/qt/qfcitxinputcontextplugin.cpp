/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <QInputContextPlugin>
#include <QDBusConnection>
#include "qfcitxinputcontext.h"

/* The class Definition */
class QFcitxInputContextPlugin: public QInputContextPlugin
{

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
    : QInputContextPlugin(parent)
{
}


QFcitxInputContextPlugin::~QFcitxInputContextPlugin()
{
}

QStringList
QFcitxInputContextPlugin::keys() const
{
    QStringList identifiers;
    identifiers.push_back(FCITX_IDENTIFIER_NAME);
    return identifiers;
}


QStringList
QFcitxInputContextPlugin::languages(const QString & key)
{
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


QString
QFcitxInputContextPlugin::description(const QString &key)
{
    if (key.toLower() != FCITX_IDENTIFIER_NAME) {
        return QString("");
    }

    return QString::fromUtf8("Qt immodule plugin for Fcitx");
}


QInputContext *
QFcitxInputContextPlugin::create(const QString &key)
{
    if (key.toLower() != FCITX_IDENTIFIER_NAME) {
        return NULL;
    }

    return static_cast<QInputContext *>(new QFcitxInputContext());
}


QString QFcitxInputContextPlugin::displayName(const QString &key)
{
    return key;
}

Q_EXPORT_PLUGIN2(QFcitxInputContextPlugin, QFcitxInputContextPlugin)

// kate: indent-mode cstyle; space-indent on; indent-width 0;
