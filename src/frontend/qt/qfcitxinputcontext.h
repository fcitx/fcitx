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
#include <QWeakPointer>

#include "fcitx-qt/fcitxqtinputcontextproxy.h"
#include "fcitx-qt/fcitxqtinputmethodproxy.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/ime.h"

#if defined(Q_WS_X11)
#include <X11/Xlib.h>
#include "fcitx/frontend.h"

struct FcitxQtICData {
    FcitxQtICData() : capacity(0), proxy(0), surroundingAnchor(-1), surroundingCursor(-1) {}
    ~FcitxQtICData() {
        if (proxy && proxy->isValid()) {
            proxy->DestroyIC();
            delete proxy;
        }
    }
    QFlags<FcitxCapacityFlags> capacity;
    QPointer<FcitxQtInputContextProxy> proxy;
    QRect rect;
    QString surroundingText;
    int surroundingAnchor;
    int surroundingCursor;
};

class FcitxQtConnection;

class ProcessKeyWatcher : public QDBusPendingCallWatcher
{
    Q_OBJECT
public:
    ProcessKeyWatcher(XEvent* e, KeySym s, const QDBusPendingCall &call, QObject *parent = 0) : QDBusPendingCallWatcher(call, parent) {
        event = static_cast<XEvent*>(malloc(sizeof(XEvent)));
        *event = *e;
        sym = s;
    }

    virtual ~ProcessKeyWatcher() {
        free(event);
    }

public slots:
    void processEvent() {
        qApp->x11ProcessEvent(event);
        deleteLater();
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

private Q_SLOTS:
    void connected();
    void createInputContext(WId w);
    void cleanUp();
    void commitString(const QString& str);
    void updateFormattedPreedit(const FcitxQtFormattedPreeditList& preeditList, int cursorPos);
    void forwardKey(uint keyval, uint state, int type);
    void deleteSurroundingText(int offset, uint nchar);
    void createInputContextFinished(QDBusPendingCallWatcher* watcher);
    void updateCursor();
#if defined(Q_WS_X11) && defined(ENABLE_X11)
    void x11ProcessKeyEventCallback(QDBusPendingCallWatcher* watcher);
#endif
private:
    QWidget* validFocusWidget();
    bool processCompose(uint keyval, uint state, FcitxKeyEventType event);
    bool checkAlgorithmically();
    bool checkCompactTable(const struct _FcitxComposeTableCompact *table);
#if defined(Q_WS_X11) && defined(ENABLE_X11)
    bool x11FilterEventFallback(XEvent *event, KeySym sym);
    XEvent* createXEvent(Display* dpy, WId wid, uint keyval, uint state, int type);
#endif // Q_WS_X11
    QKeyEvent* createKeyEvent(uint keyval, uint state, int type);
    bool isValid();
    FcitxQtInputContextProxy* validIC();
    FcitxQtInputContextProxy* validICByWidget(QWidget* w);

    void addCapacity(FcitxQtICData* data, QFlags<FcitxCapacityFlags> capacity, bool forceUpdate = false)
    {
        QFlags< FcitxCapacityFlags > newcaps = data->capacity | capacity;
        if (data->capacity != newcaps || forceUpdate) {
            data->capacity = newcaps;
            updateCapacity(data);
        }
    }

    void removeCapacity(FcitxQtICData* data, QFlags<FcitxCapacityFlags> capacity, bool forceUpdate = false)
    {
        QFlags< FcitxCapacityFlags > newcaps = data->capacity & (~capacity);
        if (data->capacity != newcaps || forceUpdate) {
            data->capacity = newcaps;
            updateCapacity(data);
        }
    }

    void updateCapacity(FcitxQtICData* data);
    void commitPreedit();
    void createICData(QWidget* w);

    FcitxQtInputMethodProxy* m_improxy;
    uint m_compose_buffer[FCITX_MAX_COMPOSE_LEN + 1];
    int m_n_compose;
    QString m_preedit;
    QString m_commitPreedit;
    FcitxQtFormattedPreeditList m_preeditList;
    int m_cursorPos;
    bool m_useSurroundingText;
    bool m_syncMode;
    FcitxQtConnection* m_connection;
    QHash<WId, FcitxQtICData*> m_icMap;
};

#endif //__FCITX_INPUT_CONTEXT_H_

// kate: indent-mode cstyle; space-indent on; indent-width 0;
