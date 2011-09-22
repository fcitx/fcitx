#ifndef __FCITX_INPUT_CONTEXT_H_
#define __FCITX_INPUT_CONTEXT_H_

#include <QInputContext>
#include <QList>
#include <QDBusConnection>
#include <QDir>
#include "org.freedesktop.DBus.h"
#include "org.fcitx.Fcitx.InputMethod.h"
#include "org.fcitx.Fcitx.InputContext.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/ime.h"

#if defined(Q_WS_X11)
#include <X11/Xlib.h>
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

#if defined(Q_WS_X11)
    virtual bool x11FilterEvent(QWidget *keywidget, XEvent *event);
#endif // Q_WS_X11
    virtual bool filterEvent(const QEvent* event);

private Q_SLOTS:
    void imChanged(const QString& service, const QString& oldowner, const QString& newowner);
    void closeIM();
    void enableIM();
    void commitString(const QString& str);
    void updatePreedit(const QString& str, int cursorPos);
    void forwardKey(uint keyval, uint state, int type);
    void destroySlaveContext();
    void createInputContextFinished(QDBusPendingCallWatcher* watcher);
private:
    void createInputContext();
    bool processCompose(uint keyval, uint state, FcitxKeyEventType event);
    bool checkAlgorithmically();
    bool checkCompactTable(const struct _FcitxComposeTableCompact *table);
#if defined(Q_WS_X11)
    bool x11FilterEventFallback(QWidget *keywidget, XEvent *event , KeySym sym);
    XEvent* createXEvent(Display* dpy, WId wid, uint keyval, uint state, int type);
#endif // Q_WS_X11
    QKeyEvent* createKeyEvent(uint keyval, uint state, int type);
    bool isValid();

    QDBusConnection m_connection;
    org::freedesktop::DBus* m_dbusproxy;
    org::fcitx::Fcitx::InputMethod* m_improxy;
    org::fcitx::Fcitx::InputContext* m_icproxy;
    int m_id;
    QString m_path;
    bool m_enable;
    bool m_has_focus;
    HOTKEYS m_triggerKey[2];
    QInputContext* m_slave;
    uint m_compose_buffer[FCITX_MAX_COMPOSE_LEN + 1];
    int m_n_compose;
    QString m_serviceName;
};

#endif //__FCITX_INPUT_CONTEXT_H_

// kate: indent-mode cstyle; space-indent on; indent-width 0;
