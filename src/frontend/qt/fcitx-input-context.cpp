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

#include <QApplication>
#include <QEventLoop>
#include <QInputContextFactory>
#include <QTextCharFormat>

#include <sys/time.h>

#include "fcitx/ui.h"
#include "fcitx/ime.h"
#include "fcitx-utils/utils.h"
#include "fcitx/frontend.h"
#include "fcitx-config/hotkey.h"
#include "module/dbus/dbusstuff.h"
#include "frontend/ipc/ipc.h"
#include "im/keyboard/fcitx-compose-data.h"
#include "fcitx-input-context.h"
#include "keyserver_x11.h"

#if defined(Q_WS_X11)
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <unistd.h>
static const int XKeyPress = KeyPress;
static const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#endif

#ifndef Q_LIKELY
#define Q_LIKELY(x) (x)
#endif

#ifndef Q_UNLIKELY
#define Q_UNLIKELY(x) (x)
#endif

static int
compare_seq_index(const void *key, const void *value)
{
    const uint *keysyms = (const uint *)key;
    const quint32 *seq = (const quint32 *)value;

    if (keysyms[0] < seq[0])
        return -1;
    else if (keysyms[0] > seq[0])
        return 1;
    return 0;
}

static int
compare_seq(const void *key, const void *value)
{
    int i = 0;
    const uint *keysyms = (const uint *)key;
    const quint32 *seq = (const quint32 *)value;

    while (keysyms[i]) {
        if (keysyms[i] < seq[i])
            return -1;
        else if (keysyms[i] > seq[i])
            return 1;
        i++;
    }

    return 0;
}

static const uint fcitx_compose_ignore[] = {
    FcitxKey_Shift_L,
    FcitxKey_Shift_R,
    FcitxKey_Control_L,
    FcitxKey_Control_R,
    FcitxKey_Caps_Lock,
    FcitxKey_Shift_Lock,
    FcitxKey_Meta_L,
    FcitxKey_Meta_R,
    FcitxKey_Alt_L,
    FcitxKey_Alt_R,
    FcitxKey_Super_L,
    FcitxKey_Super_R,
    FcitxKey_Hyper_L,
    FcitxKey_Hyper_R,
    FcitxKey_Mode_switch,
    FcitxKey_ISO_Level3_Shift,
    FcitxKey_VoidSymbol
};

typedef QInputMethodEvent::Attribute QAttribute;

static bool key_filtered = false;

QString
QFcitxInputContext::socketFile()
{
    char* addressFile = NULL;
    asprintf(&addressFile, "%s-%d", QDBusConnection::localMachineId().data(), fcitx_utils_get_display_number());

    char* file = NULL;

    FcitxXDGGetFileUserWithPrefix("dbus", addressFile, NULL, &file);

    QString path = QString::fromUtf8(file);
    free(file);
    free(addressFile);

    return path;

}

QString
QFcitxInputContext::address()
{
    QString addr;
    QByteArray addrVar = qgetenv("FCITX_DBUS_ADDRESS");
    if (!addrVar.isNull())
        return QString::fromLocal8Bit(addrVar);

    QFile file(socketFile());
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const int BUFSIZE = 1024;

    char buffer[BUFSIZE];
    size_t sz = file.read(buffer, BUFSIZE);
    file.close();
    if (sz == 0)
        return QString();
    char* p = buffer;
    while(*p)
        p++;
    size_t addrlen = p - buffer;
    if (sz != addrlen + 2 * sizeof(pid_t) + 1)
        return NULL;

    /* skip '\0' */
    p++;
    pid_t *ppid = (pid_t*) p;
    pid_t daemonpid = ppid[0];
    pid_t fcitxpid = ppid[1];

    if (!fcitx_utils_pid_exists(daemonpid)
        || !fcitx_utils_pid_exists(fcitxpid))
        return NULL;

    addr = QLatin1String(buffer);

    return addr;

}

QFcitxInputContext::QFcitxInputContext()
    : m_watcher(this),
      m_connection(0),
      m_improxy(0),
      m_icproxy(0),
      m_capacity(0),
      m_id(0),
      m_path(""),
      m_has_focus(false),
      m_n_compose(0),
      m_cursorPos(0),
      m_useSurroundingText(false),
      m_syncMode(true)
{
    FcitxFormattedPreedit::registerMetaType();

    memset(m_compose_buffer, 0, sizeof(uint) * (FCITX_MAX_COMPOSE_LEN + 1));

    m_serviceName = QString("%1-%2").arg(FCITX_DBUS_SERVICE).arg(fcitx_utils_get_display_number());
    m_serviceWatcher.setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher.addWatchedService(m_serviceName);

    QFileInfo info(socketFile());
    QDir dir(info.path());
    if (!dir.exists()) {
        QDir rt(QDir::root());
        rt.mkpath(info.path());
    }
    m_watcher.addPath(info.path());
    if (info.exists()) {
        m_watcher.addPath(info.filePath());
    }

    connect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(socketFileChanged()));
    connect(&m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(socketFileChanged()));

    createConnection();
}

QFcitxInputContext::~QFcitxInputContext()
{
    if (m_icproxy && m_icproxy->isValid()) {
        m_icproxy->DestroyIC();
    }

    cleanUp();
}

void QFcitxInputContext::socketFileChanged()
{
    QFileInfo info(socketFile());
    if (info.exists()) {
        if (m_watcher.files().indexOf(info.filePath()) == -1)
            m_watcher.addPath(info.filePath());
    }

    QString addr = address();
    if (addr.isNull())
        return;

    cleanUp();
    createConnection();
}

void QFcitxInputContext::cleanUp()
{
    if (m_connection) {
        if (m_connection->name() == "fcitx") {
            QDBusConnection::disconnectFromBus("fcitx");
        }
        delete m_connection;
        m_connection = NULL;
    }

    if (m_improxy) {
        delete m_improxy;
        m_improxy = NULL;
    }

    if (m_icproxy) {
        delete m_icproxy;
        m_icproxy = NULL;
    }

    reset();
}

void QFcitxInputContext::createConnection()
{
    m_serviceWatcher.disconnect(SIGNAL(serviceOwnerChanged(QString,QString,QString)));
    QString addr = address();
    if (!addr.isNull()) {
        QDBusConnection connection(QDBusConnection::connectToBus(addr, "fcitx"));
        if (connection.isConnected()) {
            m_connection = new QDBusConnection(connection);
        }
    }

    if (!m_connection) {
        m_connection = new QDBusConnection(QDBusConnection::sessionBus());
        connect(&m_serviceWatcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)), this, SLOT(imChanged(QString,QString,QString)));
    }

    m_connection->connect ("org.freedesktop.DBus.Local",
                           "/org/freedesktop/DBus/Local",
                           "org.freedesktop.DBus.Local",
                           "Disconnected",
                           this,
                           SLOT (dbusDisconnect ()));

    createInputContext();
}

void QFcitxInputContext::dbusDisconnect()
{
    cleanUp();
    emit dbusDisconnected();
    createConnection();
}

QString QFcitxInputContext::identifierName()
{
    return FCITX_IDENTIFIER_NAME;
}

QString QFcitxInputContext::language()
{
    return "";
}

void QFcitxInputContext::commitPreedit()
{
    if (m_commitPreedit.length() > 0) {
        QInputMethodEvent e;
        e.setCommitString(m_commitPreedit);
        m_commitPreedit.clear();
        sendEvent(e);
    }
}

void QFcitxInputContext::reset()
{
    commitPreedit();
    if (isValid())
        m_icproxy->Reset();
}

void QFcitxInputContext::update()
{
    QWidget* widget = focusWidget();
    if (widget == NULL || !isValid()) {
        return;
    }

    if (m_useSurroundingText) {
        QVariant var = widget->inputMethodQuery(Qt::ImSurroundingText);
        QVariant var1 = widget->inputMethodQuery(Qt::ImCursorPosition);
        QVariant var2 = widget->inputMethodQuery(Qt::ImAnchorPosition);
        if (var.isValid() && var1.isValid() && !m_capacity.testFlag(CAPACITY_PASSWORD) ) {
            addCapacity(CAPACITY_SURROUNDING_TEXT);
            QString text = var.toString();
            int cursor = var1.toInt();
            int anchor;
            if (var2.isValid())
                anchor = var2.toInt();
            else
                anchor = cursor;
            m_icproxy->SetSurroundingText(text, cursor, anchor);
        }
        else {
            removeCapacity(CAPACITY_SURROUNDING_TEXT);
        }
    }

    QRect rect = widget->inputMethodQuery(Qt::ImMicroFocus).toRect();

    QPoint topleft = widget->mapToGlobal(QPoint(0, 0));
    rect.translate(topleft);

    if (m_rect != rect) {
        m_rect = rect;
        m_icproxy->SetCursorRect(rect.x(), rect.y(), rect.width(), rect.height());
    }
}


bool QFcitxInputContext::isComposing() const
{
    return false;
}

bool QFcitxInputContext::filterEvent(const QEvent* event)
{
#if not (defined(Q_WS_X11) && defined(ENABLE_X11))
    QWidget* keywidget = focusWidget();

    if (key_filtered)
        return false;

    if (!keywidget || !keywidget->testAttribute(Qt::WA_WState_Created))
        return false;

    if (keywidget->inputMethodHints() & (Qt::ImhExclusiveInputMask | Qt::ImhHiddenText))
        addCapacity(CAPACITY_PASSWORD);
    else
        removeCapacity(CAPACITY_PASSWORD);

    if (!isValid() || (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)) {
        return QInputContext::filterEvent(event);
    }

    const QKeyEvent *key_event = static_cast<const QKeyEvent*>(event);
    m_icproxy->FocusIn();

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    uint time = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);

    QDBusPendingReply< int > result =  this->m_icproxy->ProcessKeyEvent(key_event->nativeVirtualKey(),
                                       key_event->nativeScanCode(),
                                       key_event->nativeModifiers(),
                                       (event->type() == QEvent::KeyPress) ? FCITX_PRESS_KEY : FCITX_RELEASE_KEY,
                                       time);
    {
        QEventLoop loop;
        QDBusPendingCallWatcher watcher(result);
        loop.connect(&watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(quit()));
        loop.connect(this, SIGNAL(dbusDisconnected()), SLOT(quit()));
        loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);
    }

    if (!m_connection || !result.isFinished() || result.isError() || result.value() <= 0)
        return QInputContext::filterEvent(event);
    else {
        update();
        return true;
    }
#else
    Q_UNUSED(event);
    return false;
#endif

}

void QFcitxInputContext::mouseHandler(int x, QMouseEvent* event)
{
    if ((event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonRelease)
        && (x <= 0 || x >= m_preedit.length())
    )
    {
        commitPreedit();
        if (isValid())
            m_icproxy->Reset();
    }
}


QKeyEvent* QFcitxInputContext::createKeyEvent(uint keyval, uint state, int type)
{
    Q_UNUSED(keyval);
    Q_UNUSED(state);

    Qt::KeyboardModifiers qstate = Qt::NoModifier;

    int count = 1;
    if (state & FcitxKeyState_Alt) {
        qstate |= Qt::AltModifier;
        count ++;
    }

    if (state & FcitxKeyState_Shift) {
        qstate |= Qt::ShiftModifier;
        count ++;
    }

    if (state & FcitxKeyState_Ctrl) {
        qstate |= Qt::ControlModifier;
        count ++;
    }

    int key;
    symToKeyQt(keyval, key);

    QKeyEvent* keyevent = new QKeyEvent(
        (type == FCITX_PRESS_KEY) ? (QEvent::KeyPress) : (QEvent::KeyRelease),
        key,
        qstate,
        QString(),
        false,
        count
    );

    return keyevent;
}

void QFcitxInputContext::setFocusWidget(QWidget* w)
{
    QWidget *oldFocus = focusWidget();

    if (oldFocus == w)
        return;

    if (oldFocus && isValid()) {
        m_icproxy->FocusOut();
    }

    QInputContext::setFocusWidget(w);

    bool has_focus = (w != NULL);

    if (!isValid())
        return;

    if (has_focus) {
        m_icproxy->FocusIn();
    } else {
        m_icproxy->FocusOut();
    }
    update();
}

void QFcitxInputContext::widgetDestroyed(QWidget* w)
{
    if (isValid()) {
        if (w == focusWidget())
            m_icproxy->FocusOut();
        update();
    }

    QInputContext::widgetDestroyed(w);
}


#if defined(Q_WS_X11) && defined(ENABLE_X11)

bool QFcitxInputContext::x11FilterEvent(QWidget* keywidget, XEvent* event)
{
    if (key_filtered)
        return false;

    if (!keywidget || !keywidget->testAttribute(Qt::WA_WState_Created))
        return false;

    if (keywidget != focusWidget())
        return false;

    if (keywidget->inputMethodHints() & (Qt::ImhExclusiveInputMask | Qt::ImhHiddenText))
        addCapacity(CAPACITY_PASSWORD);
    else
        removeCapacity(CAPACITY_PASSWORD);

    if (Q_UNLIKELY(event->xkey.state & FcitxKeyState_IgnoredMask))
        return false;

    if (Q_UNLIKELY(event->type != XKeyRelease && event->type != XKeyPress))
        return false;

    KeySym sym = 0;
    char strbuf[64];
    memset(strbuf, 0, 64);
    XLookupString(&event->xkey, strbuf, 64, &sym, NULL);

    if (Q_UNLIKELY(!isValid())) {
        return x11FilterEventFallback(event, sym);
    }

    QDBusPendingReply< int > result = this->m_icproxy->ProcessKeyEvent(
                                          sym,
                                          event->xkey.keycode,
                                          event->xkey.state,
                                          (event->type == XKeyPress) ? FCITX_PRESS_KEY : FCITX_RELEASE_KEY,
                                          event->xkey.time
                                      );
    if (Q_LIKELY(m_syncMode)) {
        QEventLoop loop;
        QDBusPendingCallWatcher watcher(result);
        loop.connect(&watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(quit()));
        loop.connect(this, SIGNAL(dbusDisconnected()), SLOT(quit()));
        loop.exec(QEventLoop::ExcludeUserInputEvents);

        if (!m_connection || !result.isFinished() || result.isError() || result.value() <= 0) {
            QTimer::singleShot(0, this, SLOT(updateIM()));
            return x11FilterEventFallback(event, sym);
        } else {
            update();
            return true;
        }
        return false;
    }
    else {
        ProcessKeyWatcher* watcher = new ProcessKeyWatcher(event, sym, result);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(x11ProcessKeyEventCallback(QDBusPendingCallWatcher*)));
        return true;
    }
}

void QFcitxInputContext::x11ProcessKeyEventCallback(QDBusPendingCallWatcher* watcher)
{
    ProcessKeyWatcher* pkwatcher = static_cast<ProcessKeyWatcher*>(watcher);
    QDBusPendingReply< int > result(*watcher);
    bool r = false;
    if (result.isError() || result.value() <= 0) {
        QTimer::singleShot(0, this, SLOT(updateIM()));
        r = x11FilterEventFallback(pkwatcher->event, pkwatcher->sym);
    } else {
        update();
        r = true;
    }
    if (r) {
        free(pkwatcher->event);
        delete pkwatcher;
    }
    else {
        pkwatcher->event->xkey.state |= FcitxKeyState_IgnoredMask;
        QTimer::singleShot(0, pkwatcher, SLOT(processEvent()));
    }
}

bool QFcitxInputContext::x11FilterEventFallback(XEvent *event, KeySym sym)
{
    if (event->type == XKeyPress || event->type == XKeyRelease) {
        if (processCompose(sym, event->xkey.state, (event->type == XKeyPress) ? FCITX_PRESS_KEY : FCITX_RELEASE_KEY)) {
            return true;
        }
    }
    return false;
}

#endif // Q_WS_X11


void QFcitxInputContext::imChanged(const QString& service, const QString& oldowner, const QString& newowner)
{
    if (service == m_serviceName) {
        /* old die */
        if (oldowner.length() > 0 || newowner.length() > 0)
            cleanUp();

        /* new rise */
        if (newowner.length() > 0) {
            cleanUp();
            createConnection();
        }
    }
}

void QFcitxInputContext::createInputContext()
{
    if (!m_connection)
        return;

    m_rect = QRect();
    if (m_improxy) {
        delete m_improxy;
        m_improxy = NULL;
    }
    m_improxy = new org::fcitx::Fcitx::InputMethod(m_serviceName, FCITX_IM_DBUS_PATH, *m_connection, this);

    if (!m_improxy->isValid())
        return;

    char* name = fcitx_utils_get_process_name();
    QDBusPendingReply< int, bool, uint, uint, uint, uint > result = m_improxy->CreateICv3(name, getpid());
    free(name);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(result);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(createInputContextFinished(QDBusPendingCallWatcher*)));
}

void QFcitxInputContext::createInputContextFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply< int, bool, uint, uint, uint, uint > result = *watcher;

    do {
        if (result.isError()) {
            break;
        }

        if (!m_connection)
            break;

        this->m_id = qdbus_cast<int>(result.argumentAt(0));
        this->m_path = QString(FCITX_IC_DBUS_PATH_QSTRING).arg(m_id);
        if (m_icproxy) {
            delete m_icproxy;
            m_icproxy = NULL;
        }
        m_icproxy = new org::fcitx::Fcitx::InputContext(m_serviceName, m_path, *m_connection, this);
        connect(m_icproxy, SIGNAL(CommitString(QString)), this, SLOT(commitString(QString)));
        connect(m_icproxy, SIGNAL(ForwardKey(uint, uint, int)), this, SLOT(forwardKey(uint, uint, int)));
        connect(m_icproxy, SIGNAL(UpdateFormattedPreedit(FcitxFormattedPreeditList,int)), this, SLOT(updateFormattedPreedit(FcitxFormattedPreeditList,int)));
        connect(m_icproxy, SIGNAL(DeleteSurroundingText(int,uint)), this, SLOT(deleteSurroundingText(int,uint)));

        if (m_icproxy->isValid() && focusWidget() != NULL)
            m_icproxy->FocusIn();

        QFlags<FcitxCapacityFlags> flag;
        flag |= CAPACITY_PREEDIT;
        flag |= CAPACITY_FORMATTED_PREEDIT;
        flag |= CAPACITY_CLIENT_UNFOCUS_COMMIT;
        m_useSurroundingText = fcitx_utils_get_boolean_env("FCITX_QT_ENABLE_SURROUNDING_TEXT", false);
        if (m_useSurroundingText)
            flag |= CAPACITY_SURROUNDING_TEXT;

        m_syncMode = fcitx_utils_get_boolean_env("FCITX_QT_USE_SYNC", true);

        addCapacity(flag, true);
    } while(0);
    delete watcher;
}

void QFcitxInputContext::updateCapacity()
{
    if (!m_icproxy || !m_icproxy->isValid())
        return;

    QDBusPendingReply< void > result = m_icproxy->SetCapacity((uint) m_capacity);
}

void QFcitxInputContext::commitString(const QString& str)
{
    QInputMethodEvent event;
    event.setCommitString(str);
    sendEvent(event);
    update();
}

void QFcitxInputContext::updateFormattedPreedit(const FcitxFormattedPreeditList& preeditList, int cursorPos)
{
    m_preeditList = preeditList;
    m_cursorPos = cursorPos;
    QString str, commitStr;
    int pos = 0;
    QList<QAttribute> attrList;
    Q_FOREACH(const FcitxFormattedPreedit& preedit, preeditList)
    {
        str += preedit.string();
        if (!(preedit.format() & MSG_DONOT_COMMIT_WHEN_UNFOCUS))
            commitStr += preedit.string();
        QTextCharFormat format;
        if ((preedit.format() & MSG_NOUNDERLINE) == 0) {
            format.setUnderlineStyle(QTextCharFormat::DashUnderline);
        }
        if (preedit.format() & MSG_HIGHLIGHT) {
            QBrush brush;
            QPalette palette;
            if (focusWidget())
                palette = focusWidget()->palette();
            else
                palette = QApplication::palette();
            format.setBackground(QBrush(QColor(palette.color(QPalette::Active, QPalette::Highlight))));
            format.setForeground(QBrush(QColor(palette.color(QPalette::Active, QPalette::HighlightedText))));
        }
        attrList.append(QAttribute(QInputMethodEvent::TextFormat, pos, preedit.string().length(), format));
        pos += preedit.string().length();
    }

    QByteArray array = str.toUtf8();
    array.truncate(cursorPos);
    cursorPos = QString::fromUtf8(array).length();

    attrList.append(QAttribute(QInputMethodEvent::Cursor, cursorPos, 1, 0));
    QInputMethodEvent event(str, attrList);
    m_preedit = str;
    m_commitPreedit = commitStr;
    sendEvent(event);
    update();
}

void QFcitxInputContext::deleteSurroundingText(int offset, uint nchar)
{
    QInputMethodEvent event;
    event.setCommitString("", offset, nchar);
    sendEvent(event);
    update();
}

void QFcitxInputContext::forwardKey(uint keyval, uint state, int type)
{
    QWidget* widget = focusWidget();
    if (Q_LIKELY(widget != NULL)) {
        key_filtered = true;
#if defined(Q_WS_X11) && defined(ENABLE_X11)
        const WId window_id = widget->winId();
        Display* x11_display = QX11Info::display();

        XEvent* xevent = createXEvent(x11_display, window_id, keyval, state | FcitxKeyState_IgnoredMask, type);
        qApp->x11ProcessEvent(xevent);
        free(xevent);
#else
        QKeyEvent *keyevent = createKeyEvent(keyval, state, type);
        QApplication::sendEvent(widget, keyevent);
        delete keyevent;
#endif
        key_filtered = false;
    }
}

#if defined(Q_WS_X11) && defined(ENABLE_X11)
XEvent* QFcitxInputContext::createXEvent(Display* dpy, WId wid, uint keyval, uint state, int type)
{
    XEvent* xevent = static_cast<XEvent*>(malloc(sizeof(XEvent)));
    XKeyEvent *xkeyevent = &xevent->xkey;

    xkeyevent->type = type == FCITX_PRESS_KEY ? XKeyPress : XKeyRelease;
    xkeyevent->display = dpy;
    xkeyevent->window = wid;
    xkeyevent->subwindow = wid;
    xkeyevent->serial = 0; /* hope it's ok */
    xkeyevent->send_event = FALSE;
    xkeyevent->same_screen = FALSE;

    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    xkeyevent->time = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);

    if (dpy != NULL) {
        xkeyevent->root = DefaultRootWindow(dpy);
        xkeyevent->keycode = XKeysymToKeycode(dpy, (KeySym) keyval);
    } else {
        xkeyevent->root = None;
        xkeyevent->keycode = 0;
    }

    xkeyevent->state = state;

    return xevent;
}
#endif

bool QFcitxInputContext::isValid()
{
    return m_icproxy != NULL && m_icproxy->isValid();
}

bool
QFcitxInputContext::processCompose(uint keyval, uint state, FcitxKeyEventType event)
{
    Q_UNUSED(state);
    int i;

    if (event == FCITX_RELEASE_KEY)
        return false;

    for (i = 0; fcitx_compose_ignore[i] != FcitxKey_VoidSymbol; i++) {
        if (keyval == fcitx_compose_ignore[i])
            return false;
    }

    m_compose_buffer[m_n_compose ++] = keyval;
    m_compose_buffer[m_n_compose] = 0;

    if (checkCompactTable(&fcitx_compose_table_compact)) {
        // qDebug () << "checkCompactTable ->true";
        return true;
    }

    if (checkAlgorithmically()) {
        // qDebug () << "checkAlgorithmically ->true";
        return true;
    }

    if (m_n_compose > 1) {
        QApplication::beep();
        m_compose_buffer[0] = 0;
        m_n_compose = 0;
        return true;
    } else {
        m_compose_buffer[0] = 0;
        m_n_compose = 0;
        return false;
    }
}


#define IS_DEAD_KEY(k) \
    ((k) >= FcitxKey_dead_grave && (k) <= (FcitxKey_dead_dasia+1))

bool
QFcitxInputContext::checkAlgorithmically()
{
    int i;
    uint32_t combination_buffer[FCITX_MAX_COMPOSE_LEN];

    if (m_n_compose >= FCITX_MAX_COMPOSE_LEN)
        return false;

    for (i = 0; i < m_n_compose && IS_DEAD_KEY(m_compose_buffer[i]); i++);
    if (i == m_n_compose)
        return true;

    if (i > 0 && i == m_n_compose - 1) {
        combination_buffer[0] = FcitxKeySymToUnicode((FcitxKeySym) m_compose_buffer[i]);
        combination_buffer[m_n_compose] = 0;
        i--;
        while (i >= 0) {
            switch (m_compose_buffer[i]) {
#define CASE(keysym, unicode) \
case FcitxKey_dead_##keysym: combination_buffer[i + 1] = unicode; break
                CASE(grave, 0x0300);
                CASE(acute, 0x0301);
                CASE(circumflex, 0x0302);
                CASE(tilde, 0x0303);   /* Also used with perispomeni, 0x342. */
                CASE(macron, 0x0304);
                CASE(breve, 0x0306);
                CASE(abovedot, 0x0307);
                CASE(diaeresis, 0x0308);
                CASE(hook, 0x0309);
                CASE(abovering, 0x030A);
                CASE(doubleacute, 0x030B);
                CASE(caron, 0x030C);
                CASE(abovecomma, 0x0313);         /* Equivalent to psili */
                CASE(abovereversedcomma, 0x0314); /* Equivalent to dasia */
                CASE(horn, 0x031B);    /* Legacy use for psili, 0x313 (or 0x343). */
                CASE(belowdot, 0x0323);
                CASE(cedilla, 0x0327);
                CASE(ogonek, 0x0328);  /* Legacy use for dasia, 0x314.*/
                CASE(iota, 0x0345);
                CASE(voiced_sound, 0x3099);    /* Per Markus Kuhn keysyms.txt file. */
                CASE(semivoiced_sound, 0x309A);    /* Per Markus Kuhn keysyms.txt file. */

                /* The following cases are to be removed once xkeyboard-config,
                * xorg are fully updated.
                */
                    /* Workaround for typo in 1.4.x xserver-xorg */
                case 0xfe66: combination_buffer[i+1] = 0x314; break;
                /* CASE(dasia, 0x314); */
                /* CASE(perispomeni, 0x342); */
                /* CASE(psili, 0x343); */
#undef CASE
            default:
                combination_buffer[i + 1] = FcitxKeySymToUnicode((FcitxKeySym) m_compose_buffer[i]);
            }
            i--;
        }

        /* If the buffer normalizes to a single character,
        * then modify the order of combination_buffer accordingly, if necessary,
        * and return TRUE.
        **/
#if 0
        if (check_normalize_nfc(combination_buffer, m_n_compose)) {
            gunichar value;
            combination_utf8 = g_ucs4_to_utf8(combination_buffer, -1, NULL, NULL, NULL);
            nfc = g_utf8_normalize(combination_utf8, -1, G_NORMALIZE_NFC);

            value = g_utf8_get_char(nfc);
            gtk_im_context_simple_commit_char(GTK_IM_CONTEXT(context_simple), value);
            context_simple->compose_buffer[0] = 0;

            g_free(combination_utf8);
            g_free(nfc);

            return TRUE;
        }
#endif
        QString s(QString::fromUcs4(combination_buffer, m_n_compose));
        s = s.normalized(QString::NormalizationForm_C);

        // qDebug () << "combination_buffer = " << QString::fromUcs4(combination_buffer, m_n_compose) << "m_n_compose" << m_n_compose;
        // qDebug () << "result = " << s << "i = " << s.length();

        if (s.length() == 1) {
            commitString(QString(s[0]));
            m_compose_buffer[0] = 0;
            m_n_compose = 0;
            return true;
        }
    }
    return false;
}


bool
QFcitxInputContext::checkCompactTable(const FcitxComposeTableCompact *table)
{
    int row_stride;
    const quint32 *seq_index;
    const quint32 *seq;
    int i;

    /* Will never match, if the sequence in the compose buffer is longer
    * than the sequences in the table. Further, compare_seq (key, val)
    * will overrun val if key is longer than val. */
    if (m_n_compose > table->max_seq_len)
        return false;

    seq_index = (const quint32 *)bsearch(m_compose_buffer,
                                         table->data, table->n_index_size,
                                         sizeof(quint32) * table->n_index_stride,
                                         compare_seq_index);

    if (!seq_index) {
        return false;
    }

    if (seq_index && m_n_compose == 1) {
        return true;
    }

    seq = NULL;
    for (i = m_n_compose - 1; i < table->max_seq_len; i++) {
        row_stride = i + 1;

        if (seq_index[i + 1] - seq_index[i] > 0) {
            seq = (const quint32 *) bsearch(m_compose_buffer + 1,
                                            table->data + seq_index[i], (seq_index[i + 1] - seq_index[i]) / row_stride,
                                            sizeof(quint32) * row_stride,
                                            compare_seq);
            if (seq) {
                if (i == m_n_compose - 1)
                    break;
                else {
                    return true;
                }
            }
        }
    }

    if (!seq) {
        return false;
    } else {
        uint value;
        value = seq[row_stride - 1];
        commitString(QString(QChar(value)));
        m_compose_buffer[0] = 0;
        m_n_compose = 0;
        return true;
    }
    return false;
}

void QFcitxInputContext::updateIM()
{
    update();
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
