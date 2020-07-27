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

#include "fcitxwatcher.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFileSystemWatcher>
#include <signal.h>

// utils function in fcitx-utils and fcitx-config
bool _pid_exists(pid_t pid) {
    if (pid <= 0)
        return 0;
    return !(kill(pid, 0) && (errno == ESRCH));
}

int displayNumber() {
    QByteArray display(qgetenv("DISPLAY"));
    QByteArray displayNumber("0");
    int pos = display.indexOf(':');

    if (pos >= 0) {
        ++pos;
        int pos2 = display.indexOf('.', pos);
        if (pos2 > 0) {
            displayNumber = display.mid(pos, pos2 - pos);
        } else {
            displayNumber = display.mid(pos);
        }
    }

    bool ok;
    int d = displayNumber.toInt(&ok);
    if (ok) {
        return d;
    }
    return 0;
}

QString socketFile() {
    QString filename =
        QString("%1-%2")
            .arg(QString::fromLatin1(QDBusConnection::localMachineId()))
            .arg(displayNumber());

    QString home = QString::fromLocal8Bit(qgetenv("XDG_CONFIG_HOME"));
    if (home.isEmpty()) {
        home = QDir::homePath().append(QLatin1String("/.config"));
    }
    return QString("%1/fcitx/dbus/%2").arg(home).arg(filename);
}

FcitxWatcher::FcitxWatcher(QObject *parent)
    : QObject(parent), m_fsWatcher(new QFileSystemWatcher(this)),
      m_serviceWatcher(new QDBusServiceWatcher(this)), m_connection(nullptr),
      m_socketFile(socketFile()),
      m_serviceName(QString("org.fcitx.Fcitx-%2").arg(displayNumber())),
      m_availability(false) {}

FcitxWatcher::~FcitxWatcher() {
    cleanUpConnection();
    delete m_fsWatcher;
    m_fsWatcher = nullptr;
}

bool FcitxWatcher::availability() const { return m_availability; }

QDBusConnection FcitxWatcher::connection() const {
    if (m_connection) {
        return *m_connection;
    }
    return QDBusConnection::sessionBus();
}

QString FcitxWatcher::service() const {
    if (m_connection) {
        return m_serviceName;
    }
    if (m_mainPresent) {
        return m_serviceName;
    }
    if (m_portalPresent) {
        return "org.freedesktop.portal.Fcitx";
    }
    return QString();
}

void FcitxWatcher::setAvailability(bool availability) {
    if (m_availability != availability) {
        m_availability = availability;
        emit availabilityChanged(m_availability);
    }
}

void FcitxWatcher::watch() {
    if (m_watched) {
        return;
    }

    connect(m_serviceWatcher,
            SIGNAL(serviceOwnerChanged(QString, QString, QString)), this,
            SLOT(imChanged(QString, QString, QString)));
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->addWatchedService(m_serviceName);
    m_serviceWatcher->addWatchedService("org.freedesktop.portal.Fcitx");

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(
            m_serviceName)) {
        m_mainPresent = true;
    }
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(
            "org.freedesktop.portal.Fcitx")) {
        m_portalPresent = true;
    }

    watchSocketFile();
    createConnection();
    m_watched = true;
}

void FcitxWatcher::unwatch() {
    if (!m_watched) {
        return;
    }
    disconnect(m_serviceWatcher,
               SIGNAL(serviceOwnerChanged(QString, QString, QString)), this,
               SLOT(imChanged(QString, QString, QString)));
    unwatchSocketFile();
    cleanUpConnection();
    m_mainPresent = false;
    m_portalPresent =false;
    m_watched = false;
    updateAvailability();
}

QString FcitxWatcher::address() {
    QString addr;
    QByteArray addrVar = qgetenv("FCITX_DBUS_ADDRESS");
    if (!addrVar.isNull())
        return QString::fromLocal8Bit(addrVar);

    QFile file(m_socketFile);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const int BUFSIZE = 1024;

    char buffer[BUFSIZE];
    size_t sz = file.read(buffer, BUFSIZE);
    file.close();
    if (sz == 0)
        return QString();
    char *p = buffer;
    while (*p)
        p++;
    size_t addrlen = p - buffer;
    if (sz != addrlen + 2 * sizeof(pid_t) + 1)
        return QString();

    /* skip '\0' */
    p++;
    pid_t *ppid = (pid_t *)p;
    pid_t daemonpid = ppid[0];
    pid_t fcitxpid = ppid[1];

    if (!_pid_exists(daemonpid) || !_pid_exists(fcitxpid))
        return QString();

    addr = QLatin1String(buffer);

    return addr;
}

void FcitxWatcher::cleanUpConnection() {
    QDBusConnection::disconnectFromBus("fcitx");
    delete m_connection;
    m_connection = nullptr;
}

void FcitxWatcher::socketFileChanged() {
    cleanUpConnection();
    createConnection();
}

void FcitxWatcher::createConnection() {
    QString addr = address();
    // qDebug() << addr;
    if (!addr.isNull()) {
        QDBusConnection connection(
            QDBusConnection::connectToBus(addr, "fcitx"));
        if (connection.isConnected()) {
            // qDebug() << "create private";
            m_connection = new QDBusConnection(connection);
        } else {
            QDBusConnection::disconnectFromBus("fcitx");
        }
    }

    if (m_connection) {
        m_connection->connect("org.freedesktop.DBus.Local",
                              "/org/freedesktop/DBus/Local",
                              "org.freedesktop.DBus.Local", "Disconnected",
                              this, SLOT(dbusDisconnected()));
        unwatchSocketFile();
    }
    updateAvailability();
}

void FcitxWatcher::dbusDisconnected() {
    cleanUpConnection();
    watchSocketFile();
    // Try recreation immediately to avoid race.
    createConnection();
}

void FcitxWatcher::watchSocketFile() {
    if (m_socketFile.isEmpty()) {
        return;
    }
    QFileInfo info(m_socketFile);
    QDir dir(info.path());
    if (!dir.exists()) {
        QDir rt(QDir::root());
        rt.mkpath(info.path());
    }
    m_fsWatcher->addPath(info.path());
    if (info.exists()) {
        m_fsWatcher->addPath(info.filePath());
    }

    connect(m_fsWatcher, SIGNAL(fileChanged(QString)), this,
            SLOT(socketFileChanged()));
    connect(m_fsWatcher, SIGNAL(directoryChanged(QString)), this,
            SLOT(socketFileChanged()));
}

void FcitxWatcher::unwatchSocketFile() {
    m_fsWatcher->removePaths(m_fsWatcher->files());
    m_fsWatcher->removePaths(m_fsWatcher->directories());
    m_fsWatcher->disconnect(SIGNAL(fileChanged(QString)));
    m_fsWatcher->disconnect(SIGNAL(directoryChanged(QString)));
}

void FcitxWatcher::imChanged(const QString &service, const QString &oldOwner,
                             const QString &newOwner) {
    if (service == m_serviceName) {
        if (!newOwner.isEmpty()) {
            m_mainPresent = true;
        } else {
            m_mainPresent = false;
        }
    } else if (service == "org.freedesktop.portal.Fcitx") {
        if (!newOwner.isEmpty()) {
            m_portalPresent = true;
        } else {
            m_portalPresent = false;
        }
    }

    updateAvailability();
}

void FcitxWatcher::updateAvailability() {
    setAvailability(m_mainPresent || m_portalPresent || m_connection);
}
