#include <QApplication>

#include <fcitx/ime.h>
#include <sys/time.h>
#include "module/dbus/dbusstuff.h"
#include "backend/ipc/ipc.h"
#include "fcitx-input-context.h"

#if defined(Q_WS_X11)
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
static const int XKeyPress = KeyPress;
static const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#endif

typedef QInputMethodEvent::Attribute QAttribute;

static bool key_filtered = false;

FcitxInputContext::FcitxInputContext()
    : m_connection(QDBusConnection::sessionBus()),
    m_dbusproxy(0),
    m_improxy(0),
    m_icproxy(0),
    m_id(0),
    m_path(""),
    m_enable(false),
    m_has_focus(false)
{
    m_dbusproxy = new org::freedesktop::DBus(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, m_connection, this);    
    connect(m_dbusproxy, SIGNAL(NameOwnerChanged(QString,QString,QString)), this, SLOT(imChanged(QString,QString,QString)));
    
    createInputContext();
}

FcitxInputContext::~FcitxInputContext()
{
    delete m_dbusproxy;
    if (m_improxy)
        delete m_improxy;
    if (m_icproxy)
    {
        if (m_icproxy->isValid())
        {
            m_icproxy->DestroyIC();
        }
        
        delete m_icproxy;
    }
}

QString FcitxInputContext::identifierName()
{
    return FCITX_IDENTIFIER_NAME;
}

QString FcitxInputContext::language()
{
    return "";
}

void FcitxInputContext::reset()
{
    if (isValid())
        m_icproxy->Reset();
}

void FcitxInputContext::update()
{
    QWidget* widget = focusWidget();
    if (widget == NULL || !isValid())
    {
        return;
    }
    
    QRect rect = widget->inputMethodQuery(Qt::ImMicroFocus).toRect ();

    QPoint topleft = widget->mapToGlobal(QPoint(0,0));
    rect.translate (topleft);

    m_icproxy->SetCursorLocation(rect.x (), rect.y () + rect.height());
}


bool FcitxInputContext::isComposing() const
{
    return false;
}

bool FcitxInputContext::filterEvent(const QEvent* event)
{
//#ifndef Q_WS_X11

    if (key_filtered)
        return true;
    if (!isValid())
        return QInputContext::filterEvent(event);

    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)
        return QInputContext::filterEvent(event);
    
    struct timeval current_time;
    gettimeofday (&current_time, NULL);
    uint time = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);

    
    const QKeyEvent *key_event = static_cast<const QKeyEvent*> (event);
    QDBusPendingReply< int > result =  this->m_icproxy->ProcessKeyEvent(key_event->nativeVirtualKey(),
                                              key_event->nativeScanCode(),
                                              key_event->nativeModifiers(),
                                              (event->type() == QEvent::KeyPress)?FCITX_PRESS_KEY:FCITX_RELEASE_KEY,
                                              time
                                              );
    result.waitForFinished();
    if (result.isError() || result.value() <= 0)
        return QInputContext::filterEvent(event);
    else
        return true;
//#endif

}

QKeyEvent* FcitxInputContext::createKeyEvent(uint keyval, uint state, int type)
{
    QKeyEvent* keyevent = new QKeyEvent(
        (type == FCITX_PRESS_KEY)? (QEvent::KeyPress) : (QEvent::KeyRelease),
        0,
        Qt::NoModifier
    );
    
    return keyevent;
}

void FcitxInputContext::setFocusWidget(QWidget* w)
{
    qDebug() << "setFocusWidget";
    QInputContext::setFocusWidget(w);
    
    bool has_focus = (w != NULL);

    if (!isValid())
        return;

    if (has_focus) {
        m_icproxy->FocusIn();
    }
    else {
        m_icproxy->FocusOut();
    }
    update ();
}

void FcitxInputContext::widgetDestroyed(QWidget* w)
{
    QInputContext::widgetDestroyed(w);
    if (!isValid())
        return;
    if (w == focusWidget())
        m_icproxy->FocusOut();
    update();
}


#if defined(Q_WS_X11)

bool FcitxInputContext::x11FilterEvent(QWidget* keywidget, XEvent* event)
{
    if (key_filtered)
        return true;
    if (!isValid())
        return QInputContext::x11FilterEvent(keywidget, event);

    if (event->type != XKeyRelease && event->type != XKeyPress)
        return QInputContext::x11FilterEvent(keywidget, event);

    m_icproxy->FocusIn();

    KeySym sym;
    char strbuf[64];
    memset(strbuf, 0, 64);
    XLookupString(&event->xkey, strbuf, 64, &sym, NULL);
    QDBusPendingReply< int > result =  this->m_icproxy->ProcessKeyEvent(sym,
                                              event->xkey.keycode,
                                              event->xkey.state,
                                              (event->type == XKeyPress)?FCITX_PRESS_KEY:FCITX_RELEASE_KEY,
                                              event->xkey.time
                                              );
    result.waitForFinished();
    qDebug() << "keysym: " << sym << "keycode: " << event->xkey.keycode << " keystate: " << event->xkey.state << " result: " << result.value();
    if (result.isError() || result.value() <= 0)
        return QInputContext::x11FilterEvent(keywidget, event);
    else
    {
        update();
        return true;
    }
    return false;
}
#endif // Q_WS_X11


void FcitxInputContext::imChanged(const QString& service, const QString& oldowner, const QString& newowner)
{
    if (service == FCITX_DBUS_SERVICE)
    {
        qDebug() << "Service: " << service << " oldowner: " << oldowner << " newowner: " << newowner << endl;
        
        /* old die */
        if (oldowner.length() > 0 || newowner.length() > 0)
        {
            if (m_improxy)
            {
                delete m_improxy;
                m_improxy = NULL;
            }

            if (m_icproxy)
            {
                delete m_icproxy;
                m_icproxy = NULL;
            }
            m_enable = false;
        }
        
        /* new rise */
        if (newowner.length() > 0)
            createInputContext();
    }
}

void FcitxInputContext::createInputContext()
{
    m_improxy = new org::fcitx::Fcitx::InputMethod(FCITX_DBUS_SERVICE, FCITX_IM_DBUS_PATH, m_connection, this);
    
    if (!m_improxy->isValid())
        return;
    
    QDBusPendingReply< int > result = m_improxy->CreateIC();
    result.waitForFinished();
    if (result.isError())
        qDebug() << result.error();
    else
    {
        this->m_id = result.value();
        this->m_path = QString(FCITX_IC_DBUS_PATH_QSTRING).arg(m_id);
        qDebug() << this->m_path;
        m_icproxy = new org::fcitx::Fcitx::InputContext(FCITX_DBUS_SERVICE, m_path, m_connection, this);
        connect(m_icproxy, SIGNAL(CloseIM()), this, SLOT(closeIM()));
        connect(m_icproxy, SIGNAL(CommitString(QString)), this, SLOT(commitString(QString)));
        connect(m_icproxy, SIGNAL(EnableIM()), this, SLOT(enableIM()));
        connect(m_icproxy, SIGNAL(ForwardKey(uint, uint, int)), this, SLOT(forwardKey(uint, uint, int)));
        
        if (m_icproxy->isValid() && focusWidget() != NULL)
            m_icproxy->FocusIn();
    }
}

void FcitxInputContext::closeIM()
{
    qDebug() << "Close IM";
    this->m_enable = false;
}

void FcitxInputContext::enableIM()
{
    qDebug() << "Enable IM";
    this->m_enable = true;
}

void FcitxInputContext::commitString(const QString& str)
{
    QInputMethodEvent event;
    event.setCommitString(str);
    sendEvent(event);
    update();
}

void FcitxInputContext::forwardKey(uint keyval, uint state, int type)
{
    QWidget* widget = focusWidget();
    if (widget != NULL)
    {
        key_filtered = true;
#ifdef Q_WS_X11
        const WId window_id = widget->winId();
        Display* x11_display = QX11Info::display();

        XEvent* xevent = createXEvent(x11_display, window_id, keyval, state, type);
        qApp->x11ProcessEvent(xevent);
        free(xevent);
#else
        QKeyEvent *keyevent = createKeyEvent(keyval, state, typedef);
        QApplication::sendEvent(widget, keyevent);
        delete keyevent;
#endif
        key_filtered = false;
    }
}

#ifdef Q_WS_X11
XEvent* FcitxInputContext::createXEvent(Display* dpy, WId wid, uint keyval, uint state, int type)
{
    XEvent* xevent = static_cast<XEvent*> (malloc(sizeof(XEvent)));
    XKeyEvent *xkeyevent = &xevent->xkey;
    
    xkeyevent->type = type == FCITX_PRESS_KEY? XKeyPress : XKeyRelease;
    xkeyevent->display = dpy;
    xkeyevent->window = wid;
    xkeyevent->subwindow = wid;
    xkeyevent->serial = 0; /* hope it's ok */
    xkeyevent->send_event = FALSE;
    xkeyevent->same_screen = FALSE;

    struct timeval current_time;
    gettimeofday (&current_time, NULL);
    xkeyevent->time = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);
    
    if (dpy != NULL) {
        xkeyevent->root = DefaultRootWindow (dpy);
        xkeyevent->keycode = XKeysymToKeycode (dpy, (KeySym) keyval);
    } else {
        xkeyevent->root = None;
        xkeyevent->keycode = 0;
    }

    xkeyevent->state = state;
    
    return xevent;
}
#endif

bool FcitxInputContext::isValid()
{
    return m_icproxy != NULL && m_icproxy->isValid();
}
