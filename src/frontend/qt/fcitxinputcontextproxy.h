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
#ifndef FCITXINPUTCONTEXTPROXY_H_
#define FCITXINPUTCONTEXTPROXY_H_

#include "inputcontext1proxy.h"
#include "inputcontextproxy.h"
#include "inputmethod1proxy.h"
#include "inputmethodproxy.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QObject>

class QDBusPendingCallWatcher;
class FcitxWatcher;

class FcitxInputContextProxy : public QObject {
    Q_OBJECT
public:
    FcitxInputContextProxy(FcitxWatcher *watcher, QObject *parent);
    ~FcitxInputContextProxy();

    bool isValid() const;

    QDBusPendingReply<> focusIn();
    QDBusPendingReply<> focusOut();
    QDBusPendingCall processKeyEvent(uint keyval, uint keycode, uint state,
                                     bool type, uint time);
    bool processKeyEventResult(const QDBusPendingCall &call);
    QDBusPendingReply<> reset();
    QDBusPendingReply<> setCapability(qulonglong caps);
    QDBusPendingReply<> setCursorRect(int x, int y, int w, int h);
    QDBusPendingReply<> setSurroundingText(const QString &text, uint cursor,
                                           uint anchor);
    QDBusPendingReply<> setSurroundingTextPosition(uint cursor, uint anchor);
    void setDisplay(const QString &display);

signals:
    void commitString(const QString &str);
    void currentIM(const QString &name, const QString &uniqueName,
                   const QString &langCode);
    void deleteSurroundingText(int offset, uint nchar);
    void forwardKey(uint keyval, uint state, bool isRelease);
    void updateFormattedPreedit(const FcitxFormattedPreeditList &str,
                                int cursorpos);
    void inputContextCreated();

private slots:
    void availabilityChanged();
    void createInputContext();
    void createInputContextFinished();
    void serviceUnregistered();
    void recheck();
    void forwardKeyWrapper(uint keyval, uint state, int type);
    void updateFormattedPreeditWrapper(const FcitxFormattedPreeditList &str,
                                       int cursorpos);

private:
    void cleanUp();

    QDBusServiceWatcher m_watcher;
    FcitxWatcher *m_fcitxWatcher;
    org::fcitx::Fcitx::InputMethod *m_improxy = nullptr;
    org::fcitx::Fcitx::InputMethod1 *m_im1proxy = nullptr;
    org::fcitx::Fcitx::InputContext *m_icproxy = nullptr;
    org::fcitx::Fcitx::InputContext1 *m_ic1proxy = nullptr;
    QDBusPendingCallWatcher *m_createInputContextWatcher = nullptr;
    QString m_display;
    bool m_portal;
};

#endif // FCITXINPUTCONTEXTPROXY_H_
