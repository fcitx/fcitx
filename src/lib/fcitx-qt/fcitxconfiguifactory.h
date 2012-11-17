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

#ifndef FCITX_CONFIG_UI_FACTORY_H
#define FCITX_CONFIG_UI_FACTORY_H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QStringList>

#include "fcitxqt_export.h"
#include "fcitxconfiguiwidget.h"
#include "fcitxconfiguiplugin.h"

class FcitxConfigUIFactoryPrivate;
class FCITX_QT_EXPORT_API FcitxConfigUIFactory : public QObject
{
    Q_OBJECT
public:
    explicit FcitxConfigUIFactory(QObject* parent = 0);
    virtual ~FcitxConfigUIFactory();
    FcitxConfigUIWidget* create(const QString& file);
private:
    FcitxConfigUIFactoryPrivate* d_ptr;
    QMap<QString, FcitxConfigUIFactoryInterface*> plugins;
    Q_DECLARE_PRIVATE(FcitxConfigUIFactory);
    void scan();
};

#endif // FCITX_CONFIG_UI_FACTORY_H
