#include <QDBusConnection>
#include <QDebug>

#include "fcitx-bus.h"
#include "fcitx-utils/utils.h"
#include "module/dbus/dbusstuff.h"

QString FcitxBus::address()
{
    return QString("%1-%2").arg(FCITX_DBUS_SERVER_ADDRESS).arg(fcitx_utils_get_display_number());
}

FcitxBus::FcitxBus(QObject* parent): QObject(parent)
    ,m_connection(QDBusConnection::connectToBus(FcitxBus::address(), "fcitx-bus"))
{
    m_connection.connect();
    qDebug() << FcitxBus::address();
}

FcitxBus::~FcitxBus()
{
}

QDBusConnection FcitxBus::connection()
{
    return m_connection;
}
