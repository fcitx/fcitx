/*
 * Copyright (C) 2011~2020 by CSSlayer
 * wengxt@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above Copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above Copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */
#include <QApplication>
#include <QInputContextFactory>
#include <QTextCharFormat>

#include <sys/time.h>
#include <xkbcommon/xkbcommon-compose.h>

#include "fcitx-config/hotkey.h"
#include "fcitx-utils/utils.h"
#include "fcitx/frontend.h"
#include "fcitx/ime.h"
#include "fcitx/ui.h"

#include "frontend/ipc/ipc.h"
#include "module/dbus/dbusstuff.h"

#include "fcitxwatcher.h"
#include "qfcitxinputcontext.h"
#include "qtkey.h"

#include <QX11Info>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
static const int XKeyPress = KeyPress;
static const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut

#ifndef Q_LIKELY
#define Q_LIKELY(x) (x)
#endif

#ifndef Q_UNLIKELY
#define Q_UNLIKELY(x) (x)
#endif

static inline const char *get_locale() {
    const char *locale = getenv("LC_ALL");
    if (!locale)
        locale = getenv("LC_CTYPE");
    if (!locale)
        locale = getenv("LANG");
    if (!locale)
        locale = "C";

    return locale;
}

struct xkb_context *_xkb_context_new_helper() {
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (context) {
        xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    }

    return context;
}

static bool get_boolean_env(const char *name, bool defval) {
    const char *value = getenv(name);

    if (value == nullptr)
        return defval;

    if (strcmp(value, "") == 0 || strcmp(value, "0") == 0 ||
        strcmp(value, "false") == 0 || strcmp(value, "False") == 0 ||
        strcmp(value, "FALSE") == 0)
        return false;

    return true;
}


typedef QInputMethodEvent::Attribute QAttribute;

QFcitxInputContext::QFcitxInputContext()
    : m_cursorPos(0), m_useSurroundingText(false), m_syncMode(true),
      m_watcher(new FcitxWatcher(this)),
      m_xkbContext(_xkb_context_new_helper()),
      m_xkbComposeTable(m_xkbContext ? xkb_compose_table_new_from_locale(
                                           m_xkbContext.data(), get_locale(),
                                           XKB_COMPOSE_COMPILE_NO_FLAGS)
                                     : 0),
      m_xkbComposeState(m_xkbComposeTable
                            ? xkb_compose_state_new(m_xkbComposeTable.data(),
                                                    XKB_COMPOSE_STATE_NO_FLAGS)
                            : 0) {

    if (m_xkbContext) {
        xkb_context_set_log_level(m_xkbContext.data(), XKB_LOG_LEVEL_CRITICAL);
    }
    /*
     * event loop will cause some problem, so we tries to use async way.
     */
    m_syncMode = get_boolean_env("FCITX_QT_USE_SYNC", false);
    m_watcher->watch();
}

QFcitxInputContext::~QFcitxInputContext() {
    m_watcher->unwatch();
    cleanUp();
    delete m_watcher;
}

void QFcitxInputContext::cleanUp() {

    for (QHash<WId, FcitxQtICData *>::const_iterator i = m_icMap.constBegin(),
                                                     e = m_icMap.constEnd();
         i != e; ++i) {
        FcitxQtICData *data = i.value();

        if (data->proxy)
            delete data->proxy;
    }

    m_icMap.clear();

    reset();
}

QString QFcitxInputContext::identifierName() { return FCITX_IDENTIFIER_NAME; }

QString QFcitxInputContext::language() { return ""; }

void QFcitxInputContext::commitPreedit() {
    if (m_preeditList.length() > 0) {
        QInputMethodEvent e;
        if (m_commitPreedit.length() > 0) {
            e.setCommitString(m_commitPreedit);
            m_commitPreedit.clear();
        }
        sendEvent(e);
        m_preeditList.clear();
    }
}

bool checkUtf8(const QByteArray &byteArray) {
    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    const QString text =
        codec->toUnicode(byteArray.constData(), byteArray.size(), &state);
    return state.invalidChars == 0;
}

void QFcitxInputContext::reset() {
    commitPreedit();
    if (auto proxy = validIC()) {
        proxy->reset();
    }

    if (m_xkbComposeState) {
        xkb_compose_state_reset(m_xkbComposeState.data());
    }
}

void QFcitxInputContext::update() {
    QWidget *widget = validFocusWidget();
    FcitxInputContextProxy *proxy = validICByWidget(widget);
    if (!proxy) {
        return;
    }

    FcitxQtICData *data = m_icMap.value(widget->effectiveWinId());

#define CHECK_HINTS(_HINTS, _CAPACITY)                                         \
    if (widget->inputMethodHints() & _HINTS)                                   \
        addCapacity(data, _CAPACITY);                                          \
    else                                                                       \
        removeCapacity(data, _CAPACITY);

    CHECK_HINTS(Qt::ImhNoAutoUppercase, CAPACITY_NOAUTOUPPERCASE)
    CHECK_HINTS(Qt::ImhPreferNumbers, CAPACITY_NUMBER)
    CHECK_HINTS(Qt::ImhPreferUppercase, CAPACITY_UPPERCASE)
    CHECK_HINTS(Qt::ImhPreferLowercase, CAPACITY_LOWERCASE)
    CHECK_HINTS(Qt::ImhNoPredictiveText, CAPACITY_NO_SPELLCHECK)
    CHECK_HINTS(Qt::ImhDigitsOnly, CAPACITY_DIGIT)
    CHECK_HINTS(Qt::ImhFormattedNumbersOnly, CAPACITY_NUMBER)
    CHECK_HINTS(Qt::ImhUppercaseOnly, CAPACITY_UPPERCASE)
    CHECK_HINTS(Qt::ImhLowercaseOnly, CAPACITY_LOWERCASE)
    CHECK_HINTS(Qt::ImhDialableCharactersOnly, CAPACITY_DIALABLE)
    CHECK_HINTS(Qt::ImhEmailCharactersOnly, CAPACITY_EMAIL)

    bool setSurrounding = false;
    if (m_useSurroundingText) {
        QVariant var = widget->inputMethodQuery(Qt::ImSurroundingText);
        QVariant var1 = widget->inputMethodQuery(Qt::ImCursorPosition);
        QVariant var2 = widget->inputMethodQuery(Qt::ImAnchorPosition);
        if (var.isValid() && var1.isValid() &&
            !data->capacity.testFlag(CAPACITY_PASSWORD)) {
            QString text = var.toString();
/* we don't want to waste too much memory here */
#define SURROUNDING_THRESHOLD 4096
            if (text.length() < SURROUNDING_THRESHOLD) {
                if (checkUtf8(text.toUtf8())) {
                    addCapacity(data, CAPACITY_SURROUNDING_TEXT);

                    int cursor = var1.toInt();
                    int anchor;
                    if (var2.isValid())
                        anchor = var2.toInt();
                    else
                        anchor = cursor;

                    // adjust it to real character size
                    // QTBUG-25536;
                    QVector<uint> tempUCS4 = text.leftRef(cursor).toUcs4();
                    while (!tempUCS4.empty() && tempUCS4.last() == 0) {
                        tempUCS4.pop_back();
                    }
                    cursor = tempUCS4.size();
                    tempUCS4 = text.leftRef(anchor).toUcs4();
                    while (!tempUCS4.empty() && tempUCS4.last() == 0) {
                        tempUCS4.pop_back();
                    }
                    anchor = tempUCS4.size();

                    if (data->surroundingText != text) {
                        data->surroundingText = text;
                        proxy->setSurroundingText(text, cursor, anchor);
                    } else {
                        if (data->surroundingAnchor != anchor ||
                            data->surroundingCursor != cursor)
                            proxy->setSurroundingTextPosition(cursor, anchor);
                    }
                    data->surroundingCursor = cursor;
                    data->surroundingAnchor = anchor;
                    setSurrounding = true;
                }
            }
        }
        if (!setSurrounding) {
            data->surroundingAnchor = -1;
            data->surroundingCursor = -1;
            data->surroundingText = QString::null;
            removeCapacity(data, CAPACITY_SURROUNDING_TEXT);
        }
    }

    QMetaObject::invokeMethod(this, "updateCursor", Qt::QueuedConnection);
}

void QFcitxInputContext::updateCursor() {
    QWidget *widget = validFocusWidget();
    FcitxInputContextProxy *proxy = validICByWidget(widget);
    if (!proxy)
        return;

    FcitxQtICData *data = m_icMap.value(widget->effectiveWinId());

    QRect rect = widget->inputMethodQuery(Qt::ImMicroFocus).toRect();

    QPoint topleft = widget->mapToGlobal(QPoint(0, 0));
    rect.translate(topleft);

    if (data->rect != rect) {
        data->rect = rect;
        proxy->setCursorRect(rect.x(), rect.y(), rect.width(), rect.height());
    }
}

bool QFcitxInputContext::isComposing() const { return false; }

void QFcitxInputContext::mouseHandler(int x, QMouseEvent *event) {
    if ((event->type() == QEvent::MouseButtonPress ||
         event->type() == QEvent::MouseButtonRelease) &&
        (x <= 0 || x >= m_preedit.length())) {
        commitPreedit();
        if (auto *proxy = validIC()) {
            proxy->reset();
        }
    }
}

void QFcitxInputContext::createICData(QWidget *w) {
    FcitxQtICData *data = m_icMap.value(w->effectiveWinId());
    if (!data) {
        m_icMap[w->effectiveWinId()] = data = new FcitxQtICData(m_watcher);
        data->proxy->setDisplay("x11:");
        data->proxy->setProperty("wid", (qulonglong)w);
        data->proxy->setProperty("icData",
                                 qVariantFromValue(static_cast<void *>(data)));
        connect(data->proxy, SIGNAL(inputContextCreated()), this,
                SLOT(createInputContextFinished()));
        connect(data->proxy, SIGNAL(commitString(QString)), this,
                SLOT(commitString(QString)));
        connect(data->proxy, SIGNAL(forwardKey(uint, uint, bool)), this,
                SLOT(forwardKey(uint, uint, bool)));
        connect(data->proxy,
                SIGNAL(updateFormattedPreedit(FcitxFormattedPreeditList, int)),
                this,
                SLOT(updateFormattedPreedit(FcitxFormattedPreeditList, int)));
        connect(data->proxy, SIGNAL(deleteSurroundingText(int, uint)), this,
                SLOT(deleteSurroundingText(int, uint)));
    }
}

void QFcitxInputContext::setFocusWidget(QWidget *w) {
    QWidget *oldFocus = validFocusWidget();

    if (oldFocus == w) {
        return;
    }

    if (oldFocus) {
        FcitxInputContextProxy *proxy = validICByWidget(oldFocus);
        if (proxy)
            proxy->focusOut();
    }

    QInputContext::setFocusWidget(w);

    if (!w)
        return;

    FcitxInputContextProxy *newproxy = validICByWidget(w);

    if (newproxy) {
        newproxy->focusIn();
    } else {
        // This will also create IC if IC Data exists
        createICData(w);
    }
}

void QFcitxInputContext::widgetDestroyed(QWidget *w) {
    QInputContext::widgetDestroyed(w);

    FcitxQtICData *data = m_icMap.take(w->effectiveWinId());
    if (!data)
        return;

    delete data;
}

bool QFcitxInputContext::x11FilterEvent(QWidget *keywidget, XEvent *event) {
    if (!keywidget || !keywidget->testAttribute(Qt::WA_WState_Created))
        return false;

    FcitxQtICData *data = m_icMap.value(keywidget->effectiveWinId());

    // if (keywidget != focusWidget())
    //    return false;

    if (data) {
        if (keywidget->inputMethodHints() &
            (Qt::ImhExclusiveInputMask | Qt::ImhHiddenText)) {
            addCapacity(data, CAPACITY_PASSWORD);
        } else {
            removeCapacity(data, CAPACITY_PASSWORD);
        }
    }

    if (Q_UNLIKELY(event->xkey.state & FcitxKeyState_IgnoredMask))
        return false;

    if (Q_UNLIKELY(event->type != XKeyRelease && event->type != XKeyPress))
        return false;

    KeySym sym = 0;
    char strbuf[64];
    memset(strbuf, 0, 64);
    XLookupString(&event->xkey, strbuf, 64, &sym, 0);

    FcitxInputContextProxy *proxy = validICByWidget(keywidget);

    if (Q_UNLIKELY(!proxy)) {
        return x11FilterEventFallback(event, sym);
    }

    auto result =
        proxy->processKeyEvent(sym, event->xkey.keycode, event->xkey.state,
                               event->type == XKeyRelease, event->xkey.time);
    if (Q_UNLIKELY(m_syncMode)) {
        do {
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
        } while (QCoreApplication::hasPendingEvents() || !result.isFinished());

        auto filtered = proxy->processKeyEventResult(result);
        if (!filtered) {
            return x11FilterEventFallback(event, sym);
        } else {
            update();
            return true;
        }
        return false;
    } else {
        ProcessKeyWatcher *watcher =
            new ProcessKeyWatcher(event, sym, result, proxy);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)),
                SLOT(x11ProcessKeyEventCallback(QDBusPendingCallWatcher *)));
        return true;
    }
}

void QFcitxInputContext::x11ProcessKeyEventCallback(
    QDBusPendingCallWatcher *watcher) {
    ProcessKeyWatcher *pkwatcher = static_cast<ProcessKeyWatcher *>(watcher);
    auto proxy = qobject_cast<FcitxInputContextProxy *>(pkwatcher->parent());
    auto filtered = proxy->processKeyEventResult(*watcher);
    bool r = false;
    if (!filtered) {
        r = x11FilterEventFallback(pkwatcher->event, pkwatcher->sym);
    } else {
        r = true;
    }

    if (!watcher->isError()) {
        update();
    }

    if (r)
        delete pkwatcher;
    else {
        pkwatcher->event->xkey.state |= FcitxKeyState_IgnoredMask;
        QMetaObject::invokeMethod(pkwatcher, "processEvent",
                                  Qt::QueuedConnection);
    }
}

bool QFcitxInputContext::x11FilterEventFallback(XEvent *event, KeySym sym) {
    if (event->type == XKeyPress || event->type == XKeyRelease) {
        if (processCompose(sym, event->xkey.state,
                           (event->type == XKeyPress) ? FCITX_PRESS_KEY
                                                      : FCITX_RELEASE_KEY)) {
            return true;
        }
    }
    return false;
}

void QFcitxInputContext::createInputContextFinished() {
    QInputMethodEvent event;

    FcitxInputContextProxy *proxy =
        qobject_cast<FcitxInputContextProxy *>(sender());
    if (!proxy) {
        return;
    }
    WId w = proxy->property("wid").toULongLong();
    FcitxQtICData *data =
        static_cast<FcitxQtICData *>(proxy->property("icData").value<void *>());
    data->rect = QRect();
    if (proxy->isValid()) {
        QWidget *widget = validFocusWidget();
        if (widget && widget->effectiveWinId() == w) {
            proxy->focusIn();
            updateCursor();
        }
    }

    QFlags<FcitxCapacityFlags> flag;
    flag |= CAPACITY_PREEDIT;
    flag |= CAPACITY_FORMATTED_PREEDIT;
    flag |= CAPACITY_CLIENT_UNFOCUS_COMMIT;
    m_useSurroundingText =
        get_boolean_env("FCITX_QT_ENABLE_SURROUNDING_TEXT", true);
    if (m_useSurroundingText)
        flag |= CAPACITY_SURROUNDING_TEXT;

    addCapacity(data, flag, true);
}

void QFcitxInputContext::updateCapacity(FcitxQtICData *data) {
    if (!data->proxy || !data->proxy->isValid())
        return;

    QDBusPendingReply<void> result =
        data->proxy->setCapability((uint)data->capacity);
}

void QFcitxInputContext::commitString(const QString &str) {
    m_cursorPos = 0;
    m_preeditList.clear();
    m_commitPreedit.clear();
    QInputMethodEvent event;
    event.setCommitString(str);
    sendEvent(event);
}

void QFcitxInputContext::updateFormattedPreedit(
    const FcitxFormattedPreeditList &preeditList, int cursorPos) {
    if (cursorPos == m_cursorPos && preeditList == m_preeditList)
        return;
    m_preeditList = preeditList;
    m_cursorPos = cursorPos;
    QString str, commitStr;
    int pos = 0;
    QList<QAttribute> attrList;

    // Fcitx 5's flags support.
    enum TextFormatFlag : int {
        TextFormatFlag_Underline = (1 << 3), /**< underline is a flag */
        TextFormatFlag_HighLight = (1 << 4), /**< highlight the preedit */
        TextFormatFlag_DontCommit = (1 << 5),
        TextFormatFlag_Bold = (1 << 6),
        TextFormatFlag_Strike = (1 << 7),
        TextFormatFlag_Italic = (1 << 8),
    };

    Q_FOREACH (const FcitxFormattedPreedit &preedit, preeditList) {
        str += preedit.string();
        if (!(preedit.format() & TextFormatFlag_DontCommit))
            commitStr += preedit.string();
        QTextCharFormat format;
        if (preedit.format() & TextFormatFlag_Underline) {
            format.setUnderlineStyle(QTextCharFormat::DashUnderline);
        }
        if (preedit.format() & TextFormatFlag_Strike) {
            format.setFontStrikeOut(true);
        }
        if (preedit.format() & TextFormatFlag_Bold) {
            format.setFontWeight(QFont::Bold);
        }
        if (preedit.format() & TextFormatFlag_Italic) {
            format.setFontItalic(true);
        }
        if (preedit.format() & TextFormatFlag_HighLight) {
            QBrush brush;
            QPalette palette;
            if (validFocusWidget())
                palette = validFocusWidget()->palette();
            else
                palette = QApplication::palette();
            format.setBackground(QBrush(
                QColor(palette.color(QPalette::Active, QPalette::Highlight))));
            format.setForeground(QBrush(QColor(
                palette.color(QPalette::Active, QPalette::HighlightedText))));
        }
        attrList.append(QAttribute(QInputMethodEvent::TextFormat, pos,
                                   preedit.string().length(), format));
        pos += preedit.string().length();
    }

    if (cursorPos < 0) {
        cursorPos = 0;
    }
    QByteArray array = str.toUtf8();
    array.truncate(cursorPos);
    cursorPos = QString::fromUtf8(array).length();

    attrList.append(QAttribute(QInputMethodEvent::Cursor, cursorPos, 1, 0));
    QInputMethodEvent event(str, attrList);
    m_preedit = str;
    m_commitPreedit = commitStr;
    sendEvent(event);
}

void QFcitxInputContext::deleteSurroundingText(int offset, uint _nchar) {
    QInputMethodEvent event;

    FcitxInputContextProxy *proxy =
        qobject_cast<FcitxInputContextProxy *>(sender());
    if (!proxy) {
        return;
    }

    FcitxQtICData *data =
        static_cast<FcitxQtICData *>(proxy->property("icData").value<void *>());
    QVector<uint> ucsText = data->surroundingText.toUcs4();

    // QTBUG-25536
    while (!ucsText.empty() && ucsText.last() == 0) {
        ucsText.pop_back();
    }

    int cursor = data->surroundingCursor;
    // make nchar signed so we are safer
    int nchar = _nchar;
    // Qt's reconvert semantics is different from gtk's. It doesn't count the
    // current
    // selection. Discard selection from nchar.
    if (data->surroundingAnchor < data->surroundingCursor) {
        nchar -= data->surroundingCursor - data->surroundingAnchor;
        offset += data->surroundingCursor - data->surroundingAnchor;
        cursor = data->surroundingAnchor;
    } else if (data->surroundingAnchor > data->surroundingCursor) {
        nchar -= data->surroundingAnchor - data->surroundingCursor;
        cursor = data->surroundingCursor;
    }

    // validates
    if (nchar >= 0 && cursor + offset >= 0 &&
        cursor + offset + nchar < ucsText.size()) {
        // order matters
        QVector<uint> replacedChars = ucsText.mid(cursor + offset, nchar);
        nchar = QString::fromUcs4(replacedChars.data(), replacedChars.size())
                    .size();

        int start, len;
        if (offset >= 0) {
            start = cursor;
            len = offset;
        } else {
            start = cursor;
            len = -offset;
        }

        QVector<uint> prefixedChars = ucsText.mid(start, len);
        offset = QString::fromUcs4(prefixedChars.data(), prefixedChars.size())
                     .size() *
                 (offset >= 0 ? 1 : -1);
        event.setCommitString("", offset, nchar);
        sendEvent(event);
    }
}

void QFcitxInputContext::forwardKey(uint keyval, uint state, bool isRelease) {
    QWidget *widget = focusWidget();
    if (Q_LIKELY(widget != 0)) {
        const WId window_id = widget->winId();
        Display *x11_display = QX11Info::display();

        XEvent *xevent =
            createXEvent(x11_display, window_id, keyval,
                         state | FcitxKeyState_IgnoredMask, isRelease);
        qApp->x11ProcessEvent(xevent);
        free(xevent);
    }
}

XEvent *QFcitxInputContext::createXEvent(Display *dpy, WId wid, uint keyval,
                                         uint state, bool isRelease) {
    XEvent *xevent = static_cast<XEvent *>(malloc(sizeof(XEvent)));
    XKeyEvent *xkeyevent = &xevent->xkey;

    xkeyevent->type = isRelease ? XKeyRelease : XKeyPress;
    xkeyevent->display = dpy;
    xkeyevent->window = wid;
    xkeyevent->subwindow = wid;
    xkeyevent->serial = 0; /* hope it's ok */
    xkeyevent->send_event = FALSE;
    xkeyevent->same_screen = FALSE;

    struct timeval current_time;
    gettimeofday(&current_time, 0);
    xkeyevent->time =
        (current_time.tv_sec * 1000) + (current_time.tv_usec / 1000);

    if (dpy != 0) {
        xkeyevent->root = DefaultRootWindow(dpy);
        xkeyevent->keycode = XKeysymToKeycode(dpy, (KeySym)keyval);
    } else {
        xkeyevent->root = None;
        xkeyevent->keycode = 0;
    }

    xkeyevent->state = state;

    return xevent;
}

bool QFcitxInputContext::isValid() { return validIC() != 0; }

FcitxInputContextProxy *QFcitxInputContext::validICByWidget(QWidget *w) {
    if (!w)
        return 0;

    FcitxQtICData *icData = m_icMap.value(w->effectiveWinId());
    if (!icData)
        return 0;
    if (!icData->proxy || !icData->proxy->isValid()) {
        return 0;
    }
    return icData->proxy;
}

FcitxInputContextProxy *QFcitxInputContext::validIC() {
    QWidget *w = validFocusWidget();
    return validICByWidget(w);
}

bool QFcitxInputContext::processCompose(uint keyval, uint state,
                                        FcitxKeyEventType event) {
    Q_UNUSED(state);

    if (!m_xkbComposeState || event == FCITX_RELEASE_KEY)
        return false;

    struct xkb_compose_state *xkbComposeState = m_xkbComposeState.data();

    enum xkb_compose_feed_result result =
        xkb_compose_state_feed(xkbComposeState, keyval);
    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return false;
    }

    enum xkb_compose_status status =
        xkb_compose_state_get_status(xkbComposeState);
    if (status == XKB_COMPOSE_NOTHING) {
        return false;
    } else if (status == XKB_COMPOSE_COMPOSED) {
        char buffer[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        int length =
            xkb_compose_state_get_utf8(xkbComposeState, buffer, sizeof(buffer));
        xkb_compose_state_reset(xkbComposeState);
        if (length != 0) {
            commitString(QString::fromUtf8(buffer));
        }

    } else if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(xkbComposeState);
    }

    return true;
}

QWidget *QFcitxInputContext::validFocusWidget() {
    QWidget *widget = focusWidget();
    if (widget && !widget->testAttribute(Qt::WA_WState_Created))
        widget = 0;
    return widget;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
