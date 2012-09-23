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

#ifndef __FCITX_INPUT_CONTEXT_H_
#define __FCITX_INPUT_CONTEXT_H_

#include "config.h"

#include <QInputContext>
#include <QList>
#include <QDBusConnection>
#include <QDir>
#include <QApplication>
#include "org.fcitx.Fcitx.InputMethod.h"
#include "org.fcitx.Fcitx.InputContext.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/ime.h"

#if defined(Q_WS_X11)
#include <X11/Xlib.h>
#include "fcitx/frontend.h"

class ProcessKeyWatcher : public QDBusPendingCallWatcher
{
    Q_OBJECT
public:
    ProcessKeyWatcher(XEvent* e, KeySym s, const QDBusPendingCall &call, QObject *parent = 0) : QDBusPendingCallWatcher(call, parent) {
        event = static_cast<XEvent*>(malloc(sizeof(XEvent)));
        *event = *e;
        sym = s;
    }

public slots:
    void processEvent() {
        qApp->x11ProcessEvent(event);
        this->deleteLater();
    }
public:
    XEvent* event;
    KeySym sym;
};
#endif


#define FCITX_IDENTIFIER_NAME "fcitx"
#define FCITX_MAX_COMPOSE_LEN 7

class QFcitxInputContext : public QInputContext
{
    Q_OBJECT
public:
    QFcitxInputContext();
    ~QFcitxInputContext();

    virtual QString identifierName();
    virtual QString language();
    virtual void reset();
    virtual bool isComposing() const;
    virtual void update();
    virtual void setFocusWidget(QWidget *w);

    virtual void widgetDestroyed(QWidget *w);

#if defined(Q_WS_X11) && defined(ENABLE_X11)
    virtual bool x11FilterEvent(QWidget *keywidget, XEvent *event);
#endif // Q_WS_X11
    virtual bool filterEvent(const QEvent* event);
    virtual void mouseHandler(int x, QMouseEvent* event);

Q_SIGNALS:
    void dbusDisconnected();

private Q_SLOTS:
    void socketFileChanged();
    void dbusDisconnect();
    void imChanged(const QString& service, const QString& oldowner, const QString& newowner);
    void commitString(const QString& str);
    void updateFormattedPreedit(const FcitxFormattedPreeditList& preeditList, int cursorPos);
    void forwardKey(uint keyval, uint state, int type);
    void deleteSurroundingText(int offset, uint nchar);
    void createInputContextFinished(QDBusPendingCallWatcher* watcher);
    void updateIM();
#if defined(Q_WS_X11) && defined(ENABLE_X11)
    void x11ProcessKeyEventCallback(QDBusPendingCallWatcher* watcher);
    void newServiceAppear();
    void updateCursor();
#endif
private:
    static QByteArray localMachineId();
    static QString socketFile();
    static QString address();
    void cleanUp();
    void createConnection();
    void createInputContext();
    bool processCompose(uint keyval, uint state, FcitxKeyEventType event);
    bool checkAlgorithmically();
    bool checkCompactTable(const struct _FcitxComposeTableCompact *table);
#if defined(Q_WS_X11) && defined(ENABLE_X11)
    bool x11FilterEventFallback(XEvent *event, KeySym sym);
    XEvent* createXEvent(Display* dpy, WId wid, uint keyval, uint state, int type);
#endif // Q_WS_X11
    QKeyEvent* createKeyEvent(uint keyval, uint state, int type);
    bool isValid();
    bool isConnected();

    void addCapacity(QFlags<FcitxCapacityFlags> capacity, bool forceUpdage = false)
    {
        QFlags< FcitxCapacityFlags > newcaps = m_capacity | capacity;
        if (m_capacity != newcaps || forceUpdage) {
            m_capacity = newcaps;
            updateCapacity();
        }
    }

    void removeCapacity(QFlags<FcitxCapacityFlags> capacity, bool forceUpdage = false)
    {
        QFlags< FcitxCapacityFlags > newcaps = m_capacity & (~capacity);
        if (m_capacity != newcaps || forceUpdage) {
            m_capacity = newcaps;
            updateCapacity();
        }
    }

    void updateCapacity();
    void commitPreedit();

    QFileSystemWatcher m_watcher;
    QDBusServiceWatcher m_serviceWatcher;
    QDBusConnection* m_connection;
    org::fcitx::Fcitx::InputMethod* m_improxy;
    org::fcitx::Fcitx::InputContext* m_icproxy;
    QFlags<FcitxCapacityFlags> m_capacity;
    int m_id;
    QString m_path;
    bool m_has_focus;
    uint m_compose_buffer[FCITX_MAX_COMPOSE_LEN + 1];
    int m_n_compose;
    QString m_serviceName;
    QString m_preedit;
    QString m_commitPreedit;
    FcitxFormattedPreeditList m_preeditList;
    int m_cursorPos;
    boolean m_useSurroundingText;
    boolean m_syncMode;
    QRect m_rect;
};

#endif //__FCITX_INPUT_CONTEXT_H_

// kate: indent-mode cstyle; space-indent on; indent-width 0;
