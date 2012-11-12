#include <QCoreApplication>
#include <QDebug>
#include "fcitxconnection.h"

class TestQConnectionApp : public QCoreApplication
{
    Q_OBJECT
public:
    TestQConnectionApp(int& argc, char** argv) : QCoreApplication(argc, argv) {
        conn = new FcitxConnection(this);

        connect(conn, SIGNAL(fcitxConnected()), SLOT(connected()));
        connect(conn, SIGNAL(fcitxDisconnected()), SLOT(disconnected()));
        conn->setAutoReconnect(false);
        conn->startConnection();
    }

private:
    FcitxConnection* conn;
public slots:
    void connected() {
        qDebug() << "connected";
    }
    void disconnected() {
        qDebug() << "disconnected";
    }
};