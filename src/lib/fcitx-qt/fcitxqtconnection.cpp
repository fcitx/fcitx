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

#include "fcitxqtconnection_p.h"
#include "fcitx-config/xdg.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusReply>
#include <QDBusConnectionInterface>

#include "module/dbus/dbusstuff.h"
#include "fcitx-utils/utils.h"
#include <QX11Info>
#include <QFile>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <X11/Xlib.h>

FcitxQtConnection::FcitxQtConnection(QObject* parent): QObject(parent)
    ,d_ptr(new FcitxQtConnectionPrivate(this))
{
}

void FcitxQtConnection::startConnection()
{
    Q_D(FcitxQtConnection);
    if (!d->m_initialized) {
        d->initialize();
        d->createConnection();
    }
}

void FcitxQtConnection::endConnection()
{
    Q_D(FcitxQtConnection);
    d->cleanUp();
    d->finalize();
    d->m_connectedOnce = false;
}

bool FcitxQtConnection::autoReconnect()
{
    Q_D(FcitxQtConnection);
    return d->m_autoReconnect;
}

void FcitxQtConnection::setAutoReconnect(bool a)
{
    Q_D(FcitxQtConnection);
    d->m_autoReconnect = a;
}

QDBusConnection* FcitxQtConnection::connection()
{
    Q_D(FcitxQtConnection);
    return d->m_connection;
}

const QString& FcitxQtConnection::serviceName()
{
    Q_D(FcitxQtConnection);
    return d->m_serviceName;
}

bool FcitxQtConnection::isConnected()
{
    Q_D(FcitxQtConnection);
    return d->isConnected();
}



FcitxQtConnection::~FcitxQtConnection()
{
}

FcitxQtConnectionPrivate::FcitxQtConnectionPrivate(FcitxQtConnection* conn) : QObject(conn)
    ,q_ptr(conn)
    ,m_displayNumber(-1)
    ,m_serviceName(QString("%1-%2").arg(FCITX_DBUS_SERVICE).arg(displayNumber()))
    ,m_connection(0)
    ,m_serviceWatcher(new QDBusServiceWatcher(conn))
    ,m_watcher(new QFileSystemWatcher(this))
    ,m_autoReconnect(true)
    ,m_connectedOnce(false)
    ,m_initialized(false)
{
#if QT_VERSION < QT_VERSION_CHECK(4, 8, 0)
    connect(qApp, SIGNAL(aboutToQuit()), m_watcher.data(), SLOT(deleteLater()));
#endif
}

FcitxQtConnectionPrivate::~FcitxQtConnectionPrivate()
{
    if (!m_watcher.isNull())
        delete m_watcher.data();
    if (m_connection)
        delete m_connection;
}

void FcitxQtConnectionPrivate::initialize() {
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->addWatchedService(m_serviceName);

    QFileInfo info(socketFile());
    QDir dir(info.path());
    if (!dir.exists()) {
        QDir rt(QDir::root());
        rt.mkpath(info.path());
    }
    m_watcher.data()->addPath(info.path());
    if (info.exists()) {
        m_watcher.data()->addPath(info.filePath());
    }

    connect(m_watcher.data(), SIGNAL(fileChanged(QString)), this, SLOT(socketFileChanged()));
    connect(m_watcher.data(), SIGNAL(directoryChanged(QString)), this, SLOT(socketFileChanged()));
    m_initialized = true;
}

void FcitxQtConnectionPrivate::finalize() {
    m_serviceWatcher->removeWatchedService(m_serviceName);
    m_watcher.data()->removePaths(m_watcher.data()->files());
    m_watcher.data()->removePaths(m_watcher.data()->directories());
    m_watcher.data()->disconnect(SIGNAL(fileChanged(QString)));
    m_watcher.data()->disconnect(SIGNAL(directoryChanged(QString)));
    m_initialized = false;
}

void FcitxQtConnectionPrivate::socketFileChanged() {
    if (m_watcher.isNull())
        return;

    QFileInfo info(socketFile());
    if (info.exists()) {
        if (m_watcher.data()->files().indexOf(info.filePath()) == -1)
            m_watcher.data()->addPath(info.filePath());
    }

    QString addr = address();
    if (addr.isNull())
        return;

    cleanUp();
    createConnection();
}

QByteArray FcitxQtConnectionPrivate::localMachineId()
{
    return QDBusConnection::localMachineId();
}

int FcitxQtConnectionPrivate::displayNumber() {
    if (m_displayNumber >= 0)
        return m_displayNumber;
    Display * dpy = QX11Info::display();
    int displayNumber = 0;
    if (dpy) {
        char* display = XDisplayString(dpy);
        if (display) {
            char* strDisplayNumber = NULL;
            display = strdup(display);
            char* p = display;
            for (; *p != ':' && *p != '\0'; p++);

            if (*p == ':') {
                *p = '\0';
                p++;
                strDisplayNumber = p;
            }
            for (; *p != '.' && *p != '\0'; p++);

            if (*p == '.') {
                *p = '\0';
            }

            if (strDisplayNumber) {
                displayNumber = atoi(strDisplayNumber);
            }

            free(display);
        }
    }
    else
        displayNumber = fcitx_utils_get_display_number();

    m_displayNumber = displayNumber;

    return displayNumber;
}

const QString& FcitxQtConnectionPrivate::socketFile()
{
    if (!m_socketFile.isEmpty())
        return m_socketFile;

    char* addressFile = NULL;

    asprintf(&addressFile, "%s-%d", localMachineId().data(), displayNumber());

    char* file = NULL;

    FcitxXDGGetFileUserWithPrefix("dbus", addressFile, NULL, &file);

    QString path = QString::fromUtf8(file);
    free(file);
    free(addressFile);

    m_socketFile = path;

    return m_socketFile;
}

QString FcitxQtConnectionPrivate::address()
{
    QString addr;
    QByteArray addrVar = qgetenv("FCITX_DBUS_ADDRESS");
    if (!addrVar.isNull())
        return QString::fromLocal8Bit(addrVar);

    QFile file(socketFile());
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const int BUFSIZE = 1024;

    char buffer[BUFSIZE];
    size_t sz = file.read(buffer, BUFSIZE);
    file.close();
    if (sz == 0)
        return QString();
    char* p = buffer;
    while(*p)
        p++;
    size_t addrlen = p - buffer;
    if (sz != addrlen + 2 * sizeof(pid_t) + 1)
        return QString();

    /* skip '\0' */
    p++;
    pid_t *ppid = (pid_t*) p;
    pid_t daemonpid = ppid[0];
    pid_t fcitxpid = ppid[1];

    if (!fcitx_utils_pid_exists(daemonpid)
        || !fcitx_utils_pid_exists(fcitxpid))
        return QString();

    addr = QLatin1String(buffer);

    return addr;
}

void FcitxQtConnectionPrivate::createConnection() {
    if (m_connectedOnce && !m_autoReconnect) {
        return;
    }

    m_serviceWatcher->disconnect(SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QString addr = address();
    // qDebug() << addr;
    if (!addr.isNull()) {
        QDBusConnection connection(QDBusConnection::connectToBus(addr, "fcitx"));
        if (connection.isConnected()) {
            // qDebug() << "create private";
            m_connection = new QDBusConnection(connection);
        }
        else
            QDBusConnection::disconnectFromBus("fcitx");
    }

    if (!m_connection) {
        QDBusConnection* connection = new QDBusConnection(QDBusConnection::sessionBus());
        connect(m_serviceWatcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)), this, SLOT(imChanged(QString,QString,QString)));
        QDBusReply<bool> registered = connection->interface()->isServiceRegistered(m_serviceName);
        if (!registered.isValid() || !registered.value()) {
            delete connection;
        }
        else {
            m_connection = connection;
        }
    }

    Q_Q(FcitxQtConnection);
    if (m_connection) {

        m_connection->connect ("org.freedesktop.DBus.Local",
                            "/org/freedesktop/DBus/Local",
                            "org.freedesktop.DBus.Local",
                            "Disconnected",
                            this,
                            SLOT (dbusDisconnected ()));
        m_connectedOnce = true;
        Q_EMIT q->connected();
    }
}


void FcitxQtConnectionPrivate::dbusDisconnected()
{
    cleanUp();

    createConnection();
}

void FcitxQtConnectionPrivate::imChanged(const QString& service, const QString& oldowner, const QString& newowner)
{
    if (service == m_serviceName) {
        /* old die */
        if (oldowner.length() > 0 || newowner.length() > 0)
            cleanUp();

        /* new rise */
        if (newowner.length() > 0) {
            QTimer::singleShot(100, this, SLOT(newServiceAppear()));
        }
    }
}

void FcitxQtConnectionPrivate::cleanUp()
{
    Q_Q(FcitxQtConnection);
    bool doemit = false;
    QDBusConnection::disconnectFromBus("fcitx");
    if (m_connection) {
        delete m_connection;
        m_connection = 0;
        doemit = true;
    }

    if (!m_autoReconnect && m_connectedOnce)
        finalize();

    /* we want m_connection and finalize being called before the signal
     * thus isConnected will return false in slot
     * and startConnection can be called in slot
     */
    if (doemit)
        Q_EMIT q->disconnected();
}

bool FcitxQtConnectionPrivate::isConnected()
{
    return m_connection && m_connection->isConnected();
}

void FcitxQtConnectionPrivate::newServiceAppear() {
    if (!isConnected()) {
        cleanUp();

        createConnection();
    }
}
