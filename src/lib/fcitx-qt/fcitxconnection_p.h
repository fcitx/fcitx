#ifndef FCITXCONNECTION_P_H
#define FCITXCONNECTION_P_H

#include "fcitxconnection.h"
#include <QWeakPointer>
#include <QFileSystemWatcher>

class QDBusConnection;
class QDBusServiceWatcher;

class FcitxConnectionPrivate : public QObject {
    Q_OBJECT
public:
    FcitxConnectionPrivate(FcitxConnection* conn);
    virtual ~FcitxConnectionPrivate();
    FcitxConnection * const q_ptr;
    Q_DECLARE_PUBLIC(FcitxConnection);

private Q_SLOTS:
    void imChanged(const QString& service, const QString& oldowner, const QString& newowner);
    void dbusDisconnected();
    void cleanUp();
    void newServiceAppear();
    void socketFileChanged();

private:
    bool isConnected();

    static QByteArray localMachineId();
    const QString& socketFile();
    void createConnection();
    QString address();
    int displayNumber();
    void initialize();
    void finalize();

    int m_displayNumber;
    QString m_serviceName;
    QDBusConnection* m_connection;
    QDBusServiceWatcher* m_serviceWatcher;
    QWeakPointer<QFileSystemWatcher> m_watcher;
    QString m_socketFile;
    bool m_autoReconnect;
    bool m_connectedOnce;
    bool m_initialized;
};


#endif // FCITXCONNECTION_P_H