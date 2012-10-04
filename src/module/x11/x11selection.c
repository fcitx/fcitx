/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#include "config.h"
#include "fcitx/fcitx.h"
#include <X11/Xatom.h>

#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include "x11stuff-internal.h"
#include "x11handlertable.h"
#include "x11selection.h"

typedef struct {
    void *owner;
    void *data;
    X11SelectionNotifyInternalCallback cb;
    FcitxDestroyNotify destroy;
    FcitxCallBack func;
} X11SelectionNotify;

/* typedef struct { */
/*     void *owner; */
/*     void *data; */
/*     X11ConvertSelectionInternalCallback cb; */
/*     FcitxDestroyNotify destroy; */
/*     FcitxCallBack func; */
/* } X11ConvertSelection; */

static void X11SelectionNotifyFreeFunc(void *obj);

void
X11InitSelection(FcitxX11 *x11priv)
{
#ifdef HAVE_XFIXES
    if (x11priv->hasXfixes) {
        x11priv->selectionNotify = fcitx_handler_table_new(
            sizeof(X11SelectionNotify), X11SelectionNotifyFreeFunc);
    }
#endif
}

#ifdef HAVE_XFIXES
static void
X11SelectionNotifyFreeFunc(void *obj)
{
    X11SelectionNotify *notify = obj;
    if (notify->destroy)
        notify->destroy(notify->data);
}
#endif

void
X11ProcessXFixesSelectionNotifyEvent(FcitxX11 *x11priv,
                                     XFixesSelectionNotifyEvent *notify_event)
{
#ifdef HAVE_XFIXES
    X11SelectionNotify *notify;
    notify = fcitx_handler_table_first(x11priv->selectionNotify, sizeof(Atom),
                                       &notify_event->selection);
    for (;notify;notify = fcitx_handler_table_next(x11priv->selectionNotify,
                                                   notify)) {
        notify->cb(x11priv, notify->owner, notify_event->selection,
                   notify_event->subtype, notify->data, notify->func);
    }
#endif
}

void
X11SelectionNotifyHelper(FcitxX11 *x11priv, void *owner, Atom selection,
                         int subtype, void *data, FcitxCallBack func)
{
#ifdef HAVE_XFIXES
    X11SelectionNotifyCallback cb = (X11SelectionNotifyCallback)func;
    char *name = XGetAtomName(x11priv->dpy, selection);
    cb(owner, name, subtype, data);
    XFree(name);
#endif
}

unsigned int
X11SelectionNotifyRegister(
    FcitxX11 *x11priv, const char *sel_str, void *owner,
    X11SelectionNotifyCallback cb, void *data, FcitxDestroyNotify destroy)
{
#ifdef HAVE_XFIXES
    if (!cb)
        return INVALID_ID;
    return X11SelectionNotifyRegisterInternal(
        x11priv, XInternAtom(x11priv->dpy, sel_str, False), owner,
        X11SelectionNotifyHelper, data, destroy, (FcitxCallBack)cb);
#else
    return INVALID_ID;
#endif
}

unsigned int
X11SelectionNotifyRegisterInternal(
    FcitxX11 *x11priv, Atom selection, void *owner,
    X11SelectionNotifyInternalCallback cb, void *data,
    FcitxDestroyNotify destroy, FcitxCallBack func)
{
#ifdef HAVE_XFIXES
    if (!(x11priv->hasXfixes && cb))
        return INVALID_ID;
    //TODO catch bad atom?
    XFixesSelectSelectionInput(x11priv->dpy, x11priv->eventWindow,
                               selection,
                               XFixesSetSelectionOwnerNotifyMask |
                               XFixesSelectionWindowDestroyNotifyMask |
                               XFixesSelectionClientCloseNotifyMask);
    X11SelectionNotify notify = {
        .owner = owner,
        .data = data,
        .cb = cb,
        .destroy = destroy,
        .func = func
    };
    return fcitx_handler_table_append(x11priv->selectionNotify,
                                      sizeof(Atom), &selection, &notify);
#else
    return INVALID_ID;
#endif
}

void
X11SelectionNotifyRemove(FcitxX11 *x11priv, unsigned int id)
{
#ifdef HAVE_XFIXES
    if (!(x11priv->hasXfixes && id != INVALID_ID))
        return;
    fcitx_handler_table_remove_by_id(x11priv->selectionNotify, id);
#endif
}
