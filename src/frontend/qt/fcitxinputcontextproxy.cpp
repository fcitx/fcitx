/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
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

#include "fcitxinputcontextproxy.h"
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusConnectionInterface>
#include <fcitx-utils/utils.h>
#include "fcitxwatcher.h"
#include <QFileInfo>
#include <QCoreApplication>
#include <QTimer>

FcitxInputContextProxy::FcitxInputContextProxy(FcitxWatcher *watcher, QObject *parent) : QObject(parent), m_fcitxWatcher(watcher), m_portal(false)
{
    FcitxFormattedPreedit::registerMetaType();
    FcitxInputContextArgument::registerMetaType();
    connect(m_fcitxWatcher, SIGNAL(availibilityChanged(bool)), this, SLOT(availabilityChanged(bool)));
    availabilityChanged(m_fcitxWatcher->availability());
    m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(&m_watcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(serviceUnregistered()));
}

FcitxInputContextProxy::~FcitxInputContextProxy()
{
    if (isValid()) {
        if (m_portal) {
            m_ic1proxy->DestroyIC();
        } else {
            m_icproxy->DestroyIC();
        }
    }
}


void FcitxInputContextProxy::setDisplay(const QString& display)
{
    m_display = display;
}


void FcitxInputContextProxy::availabilityChanged(bool avail) {
    QTimer::singleShot(100, this, SLOT(recheck()));
}

void FcitxInputContextProxy::recheck()
{
    if (!isValid() && m_fcitxWatcher->availability()) {
        createInputContext();
    }
    if (!m_fcitxWatcher->availability()) {
        cleanUp();
    }
}


void FcitxInputContextProxy::cleanUp() {
    auto services = m_watcher.watchedServices();
    for (const auto &service: services) {
        m_watcher.removeWatchedService(service);
    }

    delete m_improxy;
    m_improxy = nullptr;
    delete m_im1proxy;
    m_im1proxy = nullptr;
    delete m_icproxy;
    m_icproxy = nullptr;
    delete m_ic1proxy;
    m_ic1proxy = nullptr;
    delete m_createInputContextWatcher;
    m_createInputContextWatcher = nullptr;
}

void FcitxInputContextProxy::createInputContext()
{
    if (!m_fcitxWatcher->availability()) {
        return;
    }

    cleanUp();

    auto service = m_fcitxWatcher->service();
    auto connection = m_fcitxWatcher->connection();

    auto owner =  connection.interface()->serviceOwner(service);
    if (!owner.isValid()) {
        return;
    }

    m_watcher.setConnection(connection);
    m_watcher.setWatchedServices(QStringList() << owner);
    // Avoid race, query again.
    if (!connection.interface()->isServiceRegistered(owner)) {
        cleanUp();
        return;
    }

    QFileInfo info(QCoreApplication::applicationFilePath());
    if (service == "org.freedesktop.portal.Fcitx") {
        m_portal = true;
        m_im1proxy = new org::fcitx::Fcitx::InputMethod1(owner, "/inputmethod", connection, this);
        FcitxInputContextArgumentList list;
        FcitxInputContextArgument arg;
        arg.setName("program");
        arg.setValue(info.fileName());
        list << arg;
        if (!m_display.isEmpty()) {
            FcitxInputContextArgument arg2;
            arg2.setName("display");
            arg2.setValue(m_display);
            list << arg2;
        }

        auto result = m_im1proxy->CreateInputContext(list);
        m_createInputContextWatcher = new QDBusPendingCallWatcher(result);
        connect(m_createInputContextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(createInputContextFinished()));
    } else {
        m_portal = false;
        m_improxy = new org::fcitx::Fcitx::InputMethod(owner, "/inputmethod", connection, this);
        auto result = m_improxy->CreateICv3(info.fileName(), getpid());
        m_createInputContextWatcher = new QDBusPendingCallWatcher(result);
        connect(m_createInputContextWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(createInputContextFinished()));
    }
}

void FcitxInputContextProxy::createInputContextFinished() {
    if (m_createInputContextWatcher->isError()) {
        cleanUp();
        return;
    }

    if (m_portal) {
        QDBusPendingReply<QDBusObjectPath, QByteArray> reply(*m_createInputContextWatcher);
        m_ic1proxy = new org::fcitx::Fcitx::InputContext1(m_im1proxy->service(), reply.value().path(), m_im1proxy->connection(), this);
        connect(m_ic1proxy, SIGNAL(CommitString(QString)), this, SIGNAL(commitString(QString)));
        connect(m_ic1proxy, SIGNAL(CurrentIM(QString,QString,QString)), this, SIGNAL(currentIM(QString,QString,QString)));
        connect(m_ic1proxy, SIGNAL(DeleteSurroundingText(int,uint)), this, SIGNAL(deleteSurroundingText(int,uint)));
        connect(m_ic1proxy, SIGNAL(ForwardKey(uint, uint, bool)), this, SIGNAL(forwardKey(uint, uint, bool)));
        connect(m_ic1proxy, SIGNAL(UpdateFormattedPreedit(FcitxFormattedPreeditList,int)), this, SIGNAL(updateFormattedPreedit(FcitxFormattedPreeditList,int)));
    } else {
        QDBusPendingReply<int, bool, uint, uint, uint, uint> reply(*m_createInputContextWatcher);
        QString path = QString("/inputcontext_%1").arg(reply.value());
        m_icproxy = new org::fcitx::Fcitx::InputContext(m_improxy->service(), path, m_improxy->connection(), this);
        connect(m_icproxy, SIGNAL(CommitString(QString)), this, SIGNAL(commitString(QString)));
        connect(m_icproxy, SIGNAL(CurrentIM(QString,QString,QString)), this, SIGNAL(currentIM(QString,QString,QString)));
        connect(m_icproxy, SIGNAL(DeleteSurroundingText(int,uint)), this, SIGNAL(deleteSurroundingText(int,uint)));
        connect(m_icproxy, SIGNAL(ForwardKey(uint, uint, int)), this, SLOT(forwardKeyWrapper(uint, uint, int)));
        connect(m_icproxy, SIGNAL(UpdateFormattedPreedit(FcitxFormattedPreeditList,int)), this, SLOT(updateFormattedPreeditWrapper(FcitxFormattedPreeditList,int)));
    }

    delete m_createInputContextWatcher;
    m_createInputContextWatcher = nullptr;
    emit inputContextCreated();
}

bool FcitxInputContextProxy::isValid() const
{
    return (m_icproxy && m_icproxy->isValid()) || (m_ic1proxy && m_ic1proxy->isValid());
}

void FcitxInputContextProxy::serviceUnregistered() {
    cleanUp();
}

void FcitxInputContextProxy::forwardKeyWrapper(uint keyval, uint state, int type) {
    emit forwardKey(keyval, state, type == 1);
}

void FcitxInputContextProxy::updateFormattedPreeditWrapper(const FcitxFormattedPreeditList &list, int cursorpos) {
    auto newList = list;
    for (auto item : newList) {
        const qint32 underlineBit = (1 << 3);
        // revert non underline and "underline"
        item.setFormat(item.format() ^ underlineBit);
    }

    emit updateFormattedPreedit(list, cursorpos);
}

QDBusPendingReply<> FcitxInputContextProxy::focusIn()
{
    if (m_portal) {
        return m_ic1proxy->FocusIn();
    } else {
        return m_icproxy->FocusIn();
    }
}

QDBusPendingReply<> FcitxInputContextProxy::focusOut()
{
    if (m_portal) {
        return m_ic1proxy->FocusOut();
    } else {
        return m_icproxy->FocusOut();
    }
}

QDBusPendingCall FcitxInputContextProxy::processKeyEvent(uint keyval, uint keycode, uint state, bool type, uint time)
{
    if (m_portal) {
        return m_ic1proxy->ProcessKeyEvent(keyval, keycode, state, type, time);
    } else {
        return m_icproxy->ProcessKeyEvent(keyval, keycode, state, type ? 1 : 0, time);
    }
}

QDBusPendingReply<> FcitxInputContextProxy::reset()
{
    if (m_portal) {
        return m_ic1proxy->Reset();
    } else {
        return m_icproxy->Reset();
    }
}

QDBusPendingReply<> FcitxInputContextProxy::setCapability(qulonglong caps)
{
    if (m_portal) {
        return m_ic1proxy->SetCapability(caps);
    } else {
        return m_icproxy->SetCapacity(static_cast<uint>(caps));
    }
}

QDBusPendingReply<> FcitxInputContextProxy::setCursorRect(int x, int y, int w, int h)
{
    if (m_portal) {
        return m_ic1proxy->SetCursorRect(x, y, w, h);
    } else {
        return m_icproxy->SetCursorRect(x, y, w, h);
    }
}

QDBusPendingReply<> FcitxInputContextProxy::setSurroundingText(const QString &text, uint cursor, uint anchor)
{
    if (m_portal) {
        return m_ic1proxy->SetSurroundingText(text, cursor, anchor);
    } else {
        return m_icproxy->SetSurroundingText(text, cursor, anchor);
    }
}

QDBusPendingReply<> FcitxInputContextProxy::setSurroundingTextPosition(uint cursor, uint anchor)
{
    if (m_portal) {
        return m_ic1proxy->SetSurroundingTextPosition(cursor, anchor);
    } else {
        return m_icproxy->SetSurroundingTextPosition(cursor, anchor);
    }
}


bool FcitxInputContextProxy::processKeyEventResult(const QDBusPendingCall &call) {
    if (call.isError()) {
        return false;
    }
    if (m_portal) {
        QDBusPendingReply<bool> reply = call;
        return reply.value();
    } else {
        QDBusPendingReply<int> reply = call;
        return reply.value() > 0;
    }
}
