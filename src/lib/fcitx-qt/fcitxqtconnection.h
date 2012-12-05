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

#ifndef FCITXCONNECTION_H
#define FCITXCONNECTION_H


#include "fcitxqt_export.h"

#include <QtCore/QObject>
#include <QtDBus/QDBusConnection>

class QDBusConnection;

class FcitxQtConnectionPrivate;


class FCITX_QT_EXPORT_API FcitxQtConnection : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect)
    Q_PROPERTY(bool connected READ isConnected)
    Q_PROPERTY(QDBusConnection* connection READ connection)
    Q_PROPERTY(QString serviceName READ serviceName)
public:
    explicit FcitxQtConnection(QObject* parent = 0);
    virtual ~FcitxQtConnection();
    void startConnection();
    void endConnection();
    void setAutoReconnect(bool a);
    bool autoReconnect();

    QDBusConnection* connection();
    const QString& serviceName();
    bool isConnected();

Q_SIGNALS:
    void connected();
    void disconnected();

private:
    FcitxQtConnectionPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(FcitxQtConnection);
};

#endif // FCITXCONNECTION_H
