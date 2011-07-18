#include <QtDebug>
#include <QInputMethodEvent>
#include <QTextCharFormat>
#include <QApplication>
#include "fcitx-input-context.h"

#ifdef Q_WS_X11
# include <QX11Info>
# include <X11/Xlib.h>
# include <X11/keysym.h>
# include <X11/Xutil.h>
# ifdef HAVE_X11_XKBLIB_H
# define HAVE_XKB
# include <X11/XKBlib.h>
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <unicode/unorm.h>

typedef QInputMethodEvent::Attribute QAttribute;

FcitxInputContext::FcitxInputContext()
{

}

FcitxInputContext::~FcitxInputContext()
{

}

QString FcitxInputContext::identifierName()
{

}

QString FcitxInputContext::language()
{

}

void FcitxInputContext::reset()
{

}

bool FcitxInputContext::isComposing() const
{
    return false;
}
