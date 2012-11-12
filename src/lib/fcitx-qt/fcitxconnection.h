#ifndef FCITXCONNECTION_H
#define FCITXCONNECTION_H


#include "fcitxqt_export.h"

#include <QtCore/QObject>
#include <QtDBus/QDBusConnection>

class QDBusConnection;

class FcitxConnectionPrivate;


class FCITX_QT_EXPORT_API FcitxConnection : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect)
    Q_PROPERTY(bool connected READ isConnected)
    Q_PROPERTY(QDBusConnection* connection READ connection)
    Q_PROPERTY(QString serviceName READ serviceName)
public:
    explicit FcitxConnection(QObject* parent = 0);
    virtual ~FcitxConnection();
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
    FcitxConnectionPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(FcitxConnection);
};

#endif // FCITXCONNECTION_H
