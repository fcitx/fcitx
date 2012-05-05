#ifndef FCITX_BUS_H
#define FCITX_BUS_H

#include <QObject>
#include <QDBusConnection>

class FcitxBus : public QObject {
    Q_OBJECT
public:
    explicit FcitxBus(QObject* parent = 0);
    virtual ~FcitxBus();
    static QString address();

    QDBusConnection connection();
public slots:

private:
    QDBusConnection m_connection;
};

#endif //FCITX_BUS_H