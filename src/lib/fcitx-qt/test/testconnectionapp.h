#include <QCoreApplication>
#include <QDebug>
#include "fcitxqtconnection.h"

class TestQConnectionApp : public QCoreApplication
{
    Q_OBJECT
public:
    TestQConnectionApp(int& argc, char** argv) : QCoreApplication(argc, argv) {
        conn = new FcitxQtConnection(this);

        connect(conn, SIGNAL(connected()), SLOT(connected()));
        connect(conn, SIGNAL(disconnected()), SLOT(disconnected()));
        conn->setAutoReconnect(false);
        conn->startConnection();
    }

private:
    FcitxQtConnection* conn;
public slots:
    void connected() {
        qDebug() << "connected";
    }
    void disconnected() {
        qDebug() << "disconnected";
    }
};
