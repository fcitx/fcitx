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
    X11SelectionNotifyCallback cb;
    FcitxDestroyNotify destroy;
} X11SelectionNotify;

static void X11ClipboardSelectionNotify(void *owner, Atom selection,
                                        int subtype, void *data);
static void X11SelectionNotifyFreeFunc(void *obj);

void
X11InitSelection(FcitxX11 *x11priv)
{
#ifdef HAVE_XFIXES
    if (x11priv->hasXfixes) {
        x11priv->selectionNotify = fcitx_handler_table_new(
            sizeof(X11SelectionNotify), X11SelectionNotifyFreeFunc);
        Atom select_atoms[] = {
            x11priv->primaryAtom,
            x11priv->clipboardAtom
        };
        int i;
        for (i = 0;i < sizeof(select_atoms) / sizeof(Atom);i++) {
            X11SelectionNotifyRegister(x11priv, select_atoms[i], x11priv,
                                       X11ClipboardSelectionNotify,
                                       NULL, NULL);
        }
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
        notify->cb(notify->owner, notify_event->selection,
                   notify_event->subtype, notify->data);
    }
#endif
}

#ifdef HAVE_XFIXES
static void
X11ClipboardSelectionNotify(void *owner, Atom selection, int subtype, void *data)
{
    /* FcitxX11 *x11priv = owner; */
    /* char *name = XGetAtomName(x11priv->dpy, selection); */
    /* printf("%s, selection name, %s\n", __func__, name); */
    /* XFree(name); */
    /* switch (subtype) { */
    /* case XFixesSetSelectionOwnerNotify: */
    /* case XFixesSelectionWindowDestroyNotify: */
    /* case XFixesSelectionClientCloseNotify: */
    /*     break; */
    /* } */
}
#endif

unsigned int
X11SelectionNotifyRegister(FcitxX11 *x11priv, Atom selection, void *owner,
                           X11SelectionNotifyCallback cb, void *data,
                           FcitxDestroyNotify destroy)
{
#ifdef HAVE_XFIXES
    if (!(x11priv->hasXfixes && cb))
        return INVALID_ID;
    //TODO catch bad atom
    XFixesSelectSelectionInput(x11priv->dpy, x11priv->eventWindow,
                               selection,
                               XFixesSetSelectionOwnerNotifyMask |
                               XFixesSelectionWindowDestroyNotifyMask |
                               XFixesSelectionClientCloseNotifyMask);
    X11SelectionNotify notify = {
        .owner = owner,
        .data = data,
        .cb = cb,
        .destroy = destroy
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
