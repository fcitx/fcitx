/*
 * Copyright (C) 2017~2020 by CSSlayer
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
