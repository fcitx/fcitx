/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#ifndef FCITXQTCONFIGPLUGIN_H
#define FCITXQTCONFIGPLUGIN_H

#include "fcitxqt_export.h"
#include <QString>
#include <QObject>
#include <QStringList>

class FcitxQtConfigUIWidget;
struct FCITX_QT_EXPORT_API FcitxQtConfigUIFactoryInterface
{
    virtual QString name() = 0;
    virtual FcitxQtConfigUIWidget *create( const QString &key ) = 0;
    virtual QStringList files() = 0;
    virtual QString domain() = 0;

};

#define FcitxQtConfigUIFactoryInterface_iid "org.fcitx.Fcitx.FcitxQtConfigUIFactoryInterface"
Q_DECLARE_INTERFACE(FcitxQtConfigUIFactoryInterface, FcitxQtConfigUIFactoryInterface_iid)

class FCITX_QT_EXPORT_API FcitxQtConfigUIPlugin : public QObject, public FcitxQtConfigUIFactoryInterface {
    Q_OBJECT
    Q_INTERFACES(FcitxQtConfigUIFactoryInterface)
public:
    explicit FcitxQtConfigUIPlugin(QObject* parent = 0);
    virtual ~FcitxQtConfigUIPlugin();
};


#endif // FCITXCONFIGPLUGIN_H
