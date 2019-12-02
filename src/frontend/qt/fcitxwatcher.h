/***************************************************************************
 *   Copyright (C) 2017~2017 by CSSlayer                                   *
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

#ifndef FCITXWATCHER_H_
#define FCITXWATCHER_H_

#include <QObject>

class QDBusConnection;
class QFileSystemWatcher;
class QDBusServiceWatcher;

// A FcitxQtConnection replacement, to implement compatibility with fcitx 5.
// Since we have three thing to monitor, the situation becomes much more
// complexer.
class FcitxWatcher : public QObject {
    Q_OBJECT
public:
    explicit FcitxWatcher(QObject *parent = nullptr);
    ~FcitxWatcher();
    void watch();
    void unwatch();

    bool availability() const;

    QDBusConnection connection() const;
    QString service() const;

signals:
    void availabilityChanged(bool);

private slots:
    void dbusDisconnected();
    void socketFileChanged();
    void imChanged(const QString &service, const QString &oldOwner,
                   const QString &newOwner);

private:
    QString address();
    void watchSocketFile();
    void unwatchSocketFile();
    void createConnection();
    void cleanUpConnection();
    void setAvailability(bool availability);
    void updateAvailability();

    QFileSystemWatcher *m_fsWatcher;
    QDBusServiceWatcher *m_serviceWatcher;
    QDBusConnection *m_connection;
    QString m_socketFile;
    QString m_serviceName;
    bool m_availability = false;
    bool m_mainPresent = false;
    bool m_portalPresent = false;
    bool m_watched = false;
};

#endif // FCITXWATCHER_H_
