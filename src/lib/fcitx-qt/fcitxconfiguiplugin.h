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

#ifndef FCITXCONFIGPLUGIN_H
#define FCITXCONFIGPLUGIN_H

#include "fcitxqt_export.h"
#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QStringList>

class FcitxConfigUIWidget;
struct FCITX_QT_EXPORT_API FcitxConfigUIFactoryInterface
{
    virtual QString name() = 0;
    virtual FcitxConfigUIWidget *create( const QString &key ) = 0;
    virtual QStringList files() = 0;

};

#define FcitxConfigUIFactoryInterface_iid "org.fcitx.Fcitx.FcitxConfigUIFactoryInterface"
Q_DECLARE_INTERFACE(FcitxConfigUIFactoryInterface, FcitxConfigUIFactoryInterface_iid)

class FCITX_QT_EXPORT_API FcitxConfigUIPlugin : public QObject, public FcitxConfigUIFactoryInterface {
    Q_OBJECT
    Q_INTERFACES(FcitxConfigUIFactoryInterface)
public:
    explicit FcitxConfigUIPlugin(QObject* parent = 0);
    virtual ~FcitxConfigUIPlugin();
};


#endif // FCITXCONFIGPLUGIN_H
