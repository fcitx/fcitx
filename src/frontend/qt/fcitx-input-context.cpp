#include <QApplication>
#include <QEventLoop>
#include <QInputContextFactory>
#include <QTextCharFormat>

#include <sys/time.h>
#include <unicode/unorm.h>

#include "fcitx/ime.h"
#include "fcitx-utils/utils.h"
#include "fcitx/frontend.h"
#include "fcitx-config/hotkey.h"
#include "module/dbus/dbusstuff.h"
#include "frontend/ipc/ipc.h"
#include "fcitx-compose-data.h"
#include "fcitx-input-context.h"
#include "keyserver_x11.h"

#if defined(Q_WS_X11)
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
static const int XKeyPress = KeyPress;
static const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#endif

typedef struct _FcitxComposeTableCompact FcitxComposeTableCompact;
struct _FcitxComposeTableCompact {
    const quint32 *data;
    int max_seq_len;
    int n_index_size;
    int n_index_stride;
};

static const FcitxComposeTableCompact ibus_compose_table_compact = {
    fcitx_compose_seqs_compact,
    5,
    23,
    6
};

static int
compare_seq_index (const void *key, const void *value) {
    const uint *keysyms = (const uint *)key;
    const quint32 *seq = (const quint32 *)value;

    if (keysyms[0] < seq[0])
        return -1;
    else if (keysyms[0] > seq[0])
        return 1;
    return 0;
}

static int
compare_seq (const void *key, const void *value) {
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
    Key_Shift_L,
    Key_Shift_R,
    Key_Control_L,
    Key_Control_R,
    Key_Caps_Lock,
    Key_Shift_Lock,
    Key_Meta_L,
    Key_Meta_R,
    Key_Alt_L,
    Key_Alt_R,
    Key_Super_L,
    Key_Super_R,
    Key_Hyper_L,
    Key_Hyper_R,
    Key_Mode_switch,
    Key_ISO_Level3_Shift,
    Key_VoidSymbol
};

typedef QInputMethodEvent::Attribute QAttribute;

static bool key_filtered = false;

static boolean FcitxIsHotKey(FcitxKeySym sym, int state, HOTKEYS * hotkey);

boolean FcitxIsHotKey(FcitxKeySym sym, int state, HOTKEYS * hotkey)
{
    state &= KEY_CTRL_ALT_SHIFT_COMP;
    if (hotkey[0].sym && sym == hotkey[0].sym && (hotkey[0].state == state) )
        return true;
    if (hotkey[1].sym && sym == hotkey[1].sym && (hotkey[1].state == state) )
        return true;
    return false;
}

QFcitxInputContext::QFcitxInputContext()
        : m_connection(QDBusConnection::sessionBus()),
        m_dbusproxy(0),
        m_improxy(0),
        m_icproxy(0),
        m_id(0),
        m_path(""),
        m_enable(false),
        m_has_focus(false),
        m_slave(0),
        m_n_compose(0)
{
#if defined(Q_WS_X11)
    /* slave has too much limitation, ibus compose by hand is better, so m_slave will be NULL then */
#if 0
    m_slave = QInputContextFactory::create("xims", 0);
#endif
#endif
    memset(m_compose_buffer, 0, sizeof(uint)* (FCITX_MAX_COMPOSE_LEN + 1));

    if (m_slave)
    {
        qDebug() << "slave created";
        m_slave->setParent(this);
        connect(m_slave, SIGNAL(destroyed(QObject*)), this, SLOT(destroySlaveContext()));
    }

    m_dbusproxy = new org::freedesktop::DBus("org.freedesktop.DBus", "/org/freedesktop/DBus", m_connection, this);
    connect(m_dbusproxy, SIGNAL(NameOwnerChanged(QString,QString,QString)), this, SLOT(imChanged(QString,QString,QString)));

    m_triggerKey[0].sym = m_triggerKey[1].sym = (FcitxKeySym) 0;
    m_triggerKey[0].state = m_triggerKey[1].state = 0;

    m_serviceName = QString("%1-%2").arg(FCITX_DBUS_SERVICE).arg(FcitxGetDisplayNumber());

    createInputContext();
}

QFcitxInputContext::~QFcitxInputContext()
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

QString QFcitxInputContext::identifierName()
{
    return FCITX_IDENTIFIER_NAME;
}

QString QFcitxInputContext::language()
{
    return "";
}

void QFcitxInputContext::reset()
{
    if (isValid())
        m_icproxy->Reset();
    if (m_slave)
        m_slave->reset();
}

void QFcitxInputContext::update()
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


bool QFcitxInputContext::isComposing() const
{
    return false;
}

bool QFcitxInputContext::filterEvent(const QEvent* event)
{
#ifndef Q_WS_X11
    QWidget* keywidget = focusWidget();

    if (key_filtered)
        return false;

    if (!keywidget->testAttribute(Qt::WA_WState_Created))
        return false;

    if (!keywidget || keywidget->inputMethodHints() & (Qt::ImhExclusiveInputMask | Qt::ImhHiddenText))
        return false;

    if (!isValid() || (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease))
    {
        return QInputContext::filterEvent(event);
    }

    const QKeyEvent *key_event = static_cast<const QKeyEvent*> (event);
    if (!m_enable)
    {
        FcitxKeySym fcitxsym;
        uint fcitxstate;
        GetKey((FcitxKeySym) key_event->nativeVirtualKey(), key_event->nativeModifiers(), &fcitxsym, &fcitxstate);
        if (!FcitxIsHotKey(fcitxsym, fcitxstate, m_triggerKey))
        {
            if (m_slave && m_slave->filterEvent(event ))
                return true;
            else
                return QInputContext::filterEvent(event);
        }

    }
    m_icproxy->FocusIn();

    struct timeval current_time;
    gettimeofday (&current_time, NULL);
    uint time = (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);

    QDBusPendingReply< int > result =  this->m_icproxy->ProcessKeyEvent(key_event->nativeVirtualKey(),
                                       key_event->nativeScanCode(),
                                       key_event->nativeModifiers(),
                                       (event->type() == QEvent::KeyPress)?FCITX_PRESS_KEY:FCITX_RELEASE_KEY,
                                       time
                                      );
    {
        QEventLoop loop;
        QDBusPendingCallWatcher watcher (result);
        loop.connect(&watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(quit()));
        loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);
    }

    if (result.isError() || result.value() <= 0)
        return QInputContext::filterEvent(event);
    else
    {
        update();
        return true;
    }
#else
    Q_UNUSED(event);
    return false;
#endif

}

QKeyEvent* QFcitxInputContext::createKeyEvent(uint keyval, uint state, int type)
{
    Q_UNUSED(keyval);
    Q_UNUSED(state);

    Qt::KeyboardModifiers qstate = Qt::NoModifier;

    int count = 1;
    if(state & KEY_ALT_COMP)
    {
        qstate |= Qt::AltModifier;
        count ++;
    }

    if(state & KEY_SHIFT_COMP)
    {
        qstate |= Qt::ShiftModifier;
        count ++;
    }

    if(state & KEY_CTRL_COMP)
    {
        qstate |= Qt::ControlModifier;
        count ++;
    }

    int key;
    symToKeyQt(keyval, key);

    QKeyEvent* keyevent = new QKeyEvent(
        (type == FCITX_PRESS_KEY)? (QEvent::KeyPress) : (QEvent::KeyRelease),
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
    if (m_slave)
    {
        m_slave->setFocusWidget(w);
    }

    if (!w || w->inputMethodHints() & (Qt::ImhExclusiveInputMask | Qt::ImhHiddenText))
        return;

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

void QFcitxInputContext::widgetDestroyed(QWidget* w)
{
    QInputContext::widgetDestroyed(w);
    if (m_slave)
        m_slave->widgetDestroyed(w);
    if (!isValid())
        return;
    if (w == focusWidget())
        m_icproxy->FocusOut();
    update();
}


#if defined(Q_WS_X11)

bool QFcitxInputContext::x11FilterEvent(QWidget* keywidget, XEvent* event)
{
    if (key_filtered)
        return false;

    if (!keywidget->testAttribute(Qt::WA_WState_Created))
        return false;

    if (keywidget != focusWidget())
        return false;

    if (!keywidget || keywidget->inputMethodHints() & (Qt::ImhExclusiveInputMask | Qt::ImhHiddenText))
        return false;

    if (!isValid() || (event->type != XKeyRelease && event->type != XKeyPress))
    {
        return x11FilterEventFallback(keywidget, event, 0);
    }

    KeySym sym = 0;
    char strbuf[64];
    memset(strbuf, 0, 64);
    XLookupString(&event->xkey, strbuf, 64, &sym, NULL);


    if (!m_enable)
    {
        FcitxKeySym fcitxsym;
        uint fcitxstate;
        GetKey((FcitxKeySym) sym, event->xkey.state, &fcitxsym, &fcitxstate);
        if (!FcitxIsHotKey(fcitxsym, fcitxstate, m_triggerKey))
        {
            return x11FilterEventFallback(keywidget, event, sym);
        }
    }

    m_icproxy->FocusIn();

    QDBusPendingReply< int > result = this->m_icproxy->ProcessKeyEvent(
                                          sym,
                                          event->xkey.keycode,
                                          event->xkey.state,
                                          (event->type == XKeyPress)?FCITX_PRESS_KEY:FCITX_RELEASE_KEY,
                                          event->xkey.time
                                      );
    {
        QEventLoop loop;
        QDBusPendingCallWatcher watcher (result);
        loop.connect(&watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(quit()));
        loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);
    }

    if (result.isError() || result.value() <= 0)
    {
        return x11FilterEventFallback(keywidget, event, sym);
    }
    else
    {
        update();
        return true;
    }
    return false;
}

bool QFcitxInputContext::x11FilterEventFallback(QWidget *keywidget, XEvent *event, KeySym sym)
{
    if (m_slave && m_slave->x11FilterEvent(keywidget, event))
        return true;
    else
    {
        if (event->type == XKeyPress || event->type == XKeyRelease)
        {
            if (processCompose(sym, event->xkey.state, (event->type == XKeyPress)?FCITX_PRESS_KEY:FCITX_RELEASE_KEY))
            {
                return true;
            }
        }
        return QInputContext::x11FilterEvent(keywidget, event);
    }
}

#endif // Q_WS_X11


void QFcitxInputContext::imChanged(const QString& service, const QString& oldowner, const QString& newowner)
{
    if (service == m_serviceName)
    {
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
            m_triggerKey[0].sym = m_triggerKey[1].sym = (FcitxKeySym) 0;
            m_triggerKey[0].state = m_triggerKey[1].state = 0;
        }

        /* new rise */
        if (newowner.length() > 0)
            createInputContext();
    }
}

void QFcitxInputContext::createInputContext()
{
    m_improxy = new org::fcitx::Fcitx::InputMethod(m_serviceName, FCITX_IM_DBUS_PATH, m_connection, this);

    if (!m_improxy->isValid())
        return;

    char* name = fcitx_get_process_name();
    QDBusPendingReply< int, bool, uint, uint, uint, uint > result = m_improxy->CreateICv2(name);
    free(name);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(result);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(createInputContextFinished(QDBusPendingCallWatcher*)));
}

void QFcitxInputContext::createInputContextFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply< int, bool, uint, uint, uint, uint > result = *watcher;
    if (result.isError())
        qWarning() << result.error();
    else
    {
        this->m_id = qdbus_cast<int>(result.argumentAt(0));
        this->m_enable = qdbus_cast<bool>(result.argumentAt(1));
        m_triggerKey[0].sym = (FcitxKeySym) qdbus_cast<uint>(result.argumentAt(2));
        m_triggerKey[0].state = qdbus_cast<uint>(result.argumentAt(3));
        m_triggerKey[1].sym = (FcitxKeySym) qdbus_cast<uint>(result.argumentAt(4));
        m_triggerKey[1].state = qdbus_cast<uint>(result.argumentAt(5));
        this->m_path = QString(FCITX_IC_DBUS_PATH_QSTRING).arg(m_id);
        m_icproxy = new org::fcitx::Fcitx::InputContext(m_serviceName, m_path, m_connection, this);
        connect(m_icproxy, SIGNAL(CloseIM()), this, SLOT(closeIM()));
        connect(m_icproxy, SIGNAL(CommitString(QString)), this, SLOT(commitString(QString)));
        connect(m_icproxy, SIGNAL(EnableIM()), this, SLOT(enableIM()));
        connect(m_icproxy, SIGNAL(ForwardKey(uint, uint, int)), this, SLOT(forwardKey(uint, uint, int)));
        connect(m_icproxy, SIGNAL(UpdatePreedit(QString,int)), this, SLOT(updatePreedit(QString, int)));

        if (m_icproxy->isValid() && focusWidget() != NULL)
            m_icproxy->FocusIn();

        if (m_icproxy)
            m_icproxy->SetCapacity(CAPACITY_PREEDIT);
    }
    delete watcher;
}

void QFcitxInputContext::closeIM()
{
    this->m_enable = false;
}

void QFcitxInputContext::enableIM()
{
    this->m_enable = true;
}

void QFcitxInputContext::commitString(const QString& str)
{
    QInputMethodEvent event;
    event.setCommitString(str);
    sendEvent(event);
    update();
}

void QFcitxInputContext::updatePreedit(const QString& str, int cursorPos)
{
    QByteArray array = str.toUtf8();
    array.truncate(cursorPos);
    cursorPos = QString::fromUtf8(array).length();

    QList<QAttribute> attrList;
    QTextCharFormat format;
    format.setUnderlineStyle(QTextCharFormat::DashUnderline);
    attrList.append(QAttribute(QInputMethodEvent::Cursor, cursorPos, 1, 0));
    attrList.append(QAttribute(QInputMethodEvent::TextFormat, 0, str.length(), format));
    QInputMethodEvent event(str, attrList);
    sendEvent(event);
    update();
}

void QFcitxInputContext::forwardKey(uint keyval, uint state, int type)
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
        QKeyEvent *keyevent = createKeyEvent(keyval, state, type);
        QApplication::sendEvent(widget, keyevent);
        delete keyevent;
#endif
        key_filtered = false;
    }
}

#ifdef Q_WS_X11
XEvent* QFcitxInputContext::createXEvent(Display* dpy, WId wid, uint keyval, uint state, int type)
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

bool QFcitxInputContext::isValid()
{
    return m_icproxy != NULL && m_icproxy->isValid();
}

void QFcitxInputContext::destroySlaveContext()
{
    if ( m_slave )
    {
        m_slave->deleteLater();
        m_slave = 0;
    }
}

bool
QFcitxInputContext::processCompose (uint keyval, uint state, FcitxKeyEventType event)
{
    Q_UNUSED(state);
    int i;

    if (event == FCITX_RELEASE_KEY)
        return false;

    for (i = 0; fcitx_compose_ignore[i] != Key_VoidSymbol; i++) {
        if (keyval == fcitx_compose_ignore[i])
            return false;
    }

    m_compose_buffer[m_n_compose ++] = keyval;
    m_compose_buffer[m_n_compose] = 0;

    if (checkCompactTable (&ibus_compose_table_compact)) {
        // qDebug () << "checkCompactTable ->true";
        return true;
    }

    if (checkAlgorithmically ()) {
        // qDebug () << "checkAlgorithmically ->true";
        return true;
    }

    if (m_n_compose > 1) {
        QApplication::beep ();
        m_compose_buffer[0] = 0;
        m_n_compose = 0;
        return true;
    }
    else {
        m_compose_buffer[0] = 0;
        m_n_compose = 0;
        return false;
    }
}


#define IS_DEAD_KEY(k) \
((k) >= Key_dead_grave && (k) <= (Key_dead_dasia+1))
quint32 fcitx_keyval_to_unicode (uint keyval);

bool
QFcitxInputContext::checkAlgorithmically ()
{
    int i;
    UChar combination_buffer[FCITX_MAX_COMPOSE_LEN];

    if (m_n_compose >= FCITX_MAX_COMPOSE_LEN)
        return false;

    for (i = 0; i < m_n_compose && IS_DEAD_KEY (m_compose_buffer[i]); i++);
    if (i == m_n_compose)
        return true;

    if (i > 0 && i == m_n_compose - 1) {
        combination_buffer[0] = fcitx_keyval_to_unicode (m_compose_buffer[i]);
        combination_buffer[m_n_compose] = 0;
        i--;
        while (i >= 0) {
            switch (m_compose_buffer[i]) {
#define CASE(keysym, unicode) \
case Key_dead_##keysym: combination_buffer[i + 1] = unicode; break
                CASE (grave, 0x0300);
                CASE (acute, 0x0301);
                CASE (circumflex, 0x0302);
                CASE (tilde, 0x0303); /* Also used with perispomeni, 0x342. */
                CASE (macron, 0x0304);
                CASE (breve, 0x0306);
                CASE (abovedot, 0x0307);
                CASE (diaeresis, 0x0308);
                CASE (hook, 0x0309);
                CASE (abovering, 0x030A);
                CASE (doubleacute, 0x030B);
                CASE (caron, 0x030C);
                CASE (abovecomma, 0x0313); /* Equivalent to psili */
                CASE (abovereversedcomma, 0x0314); /* Equivalent to dasia */
                CASE (horn, 0x031B); /* Legacy use for psili, 0x313 (or 0x343). */
                CASE (belowdot, 0x0323);
                CASE (cedilla, 0x0327);
                CASE (ogonek, 0x0328); /* Legacy use for dasia, 0x314.*/
                CASE (iota, 0x0345);
                CASE (voiced_sound, 0x3099); /* Per Markus Kuhn keysyms.txt file. */
                CASE (semivoiced_sound, 0x309A); /* Per Markus Kuhn keysyms.txt file. */
                /* The following cases are to be removed once xkeyboard-config,
                * xorg are fully updated.
                **/
                /* Workaround for typo in 1.4.x xserver-xorg */
            case 0xfe66:
                combination_buffer[i + 1] = 0x314;
                break;
                /* CASE (dasia, 0x314); */
                /* CASE (perispomeni, 0x342); */
                /* CASE (psili, 0x343); */
#undef CASE
            default:
                combination_buffer[i + 1] = fcitx_keyval_to_unicode (m_compose_buffer[i]);
            }
            i--;
        }

        /* If the buffer normalizes to a single character,
        * then modify the order of combination_buffer accordingly, if necessary,
        * and return TRUE.
        **/
#if 0
        if (check_normalize_nfc (combination_buffer, m_n_compose))
        {
            gunichar value;
            combination_utf8 = g_ucs4_to_utf8 (combination_buffer, -1, NULL, NULL, NULL);
            nfc = g_utf8_normalize (combination_utf8, -1, G_NORMALIZE_NFC);

            value = g_utf8_get_char (nfc);
            gtk_im_context_simple_commit_char (GTK_IM_CONTEXT (context_simple), value);
            context_simple->compose_buffer[0] = 0;

            g_free (combination_utf8);
            g_free (nfc);

            return TRUE;
        }
#endif
        UErrorCode state = U_ZERO_ERROR;
        UChar result[FCITX_MAX_COMPOSE_LEN + 1];
        i = unorm_normalize (combination_buffer, m_n_compose, UNORM_NFC, 0, result, FCITX_MAX_COMPOSE_LEN + 1, &state);

        // qDebug () << "combination_buffer = " << QString::fromUtf16(combination_buffer) << "m_n_compose" << m_n_compose;
        // qDebug () << "result = " << QString::fromUtf16(result) << "i = " << i << state;

        if (i == 1) {
            commitString (QString (QChar (result[0])));
            m_compose_buffer[0] = 0;
            m_n_compose = 0;
            return true;
        }
    }
    return false;
}


bool
QFcitxInputContext::checkCompactTable (const FcitxComposeTableCompact *table)
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

    seq_index = (const quint32 *)bsearch (m_compose_buffer,
                                          table->data, table->n_index_size,
                                          sizeof (quint32) * table->n_index_stride,
                                          compare_seq_index);

    if (!seq_index) {
        return false;
    }

    if (seq_index && m_n_compose == 1) {
        return true;
    }

    seq = NULL;
    for (i = m_n_compose-1; i < table->max_seq_len; i++) {
        row_stride = i + 1;

        if (seq_index[i+1] - seq_index[i] > 0) {
            seq = (const quint32 *) bsearch (m_compose_buffer + 1,
                                             table->data + seq_index[i], (seq_index[i+1] - seq_index[i]) / row_stride,
                                             sizeof (quint32) * row_stride,
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
    }
    else
    {
        uint value;
        value = seq[row_stride - 1];
        commitString (QString (QChar (value)));
        m_compose_buffer[0] = 0;
        m_n_compose = 0;
        return true;
    }
    return false;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
