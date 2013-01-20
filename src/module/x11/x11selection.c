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
#include "fcitx-utils/handler-table.h"
#include "fcitx-utils/objpool.h"
#include "x11selection.h"

#define FCITX_X11_SEL "FCITX_X11_SEL_"

#ifdef HAVE_XFIXES
static void X11SelectionNotifyFreeFunc(void *obj);
#endif
static void X11ConvertSelectionFreeFunc(void *obj);
static int X11RequestConvertSelectionInternal(
    FcitxX11 *x11priv, const char *sel_str, Atom selection, Atom target,
    void *owner, X11ConvertSelectionInternalCallback cb, void *data,
    FcitxDestroyNotify destroy, FcitxCallBack func);

void
X11InitSelection(FcitxX11 *x11priv)
{
#ifdef HAVE_XFIXES
    if (x11priv->hasXfixes) {
        x11priv->selectionNotify = fcitx_handler_table_new(
            sizeof(X11SelectionNotify), X11SelectionNotifyFreeFunc);
    }
#endif
    x11priv->convertSelection = fcitx_handler_table_new(
        sizeof(X11ConvertSelection), X11ConvertSelectionFreeFunc);
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

static void
X11ConvertSelectionFreeFunc(void *obj)
{
    X11ConvertSelection *convert = obj;
    if (convert->destroy)
        convert->destroy(convert->data);
}

#ifdef HAVE_XFIXES
void
X11ProcessXFixesSelectionNotifyEvent(FcitxX11 *x11priv,
                                     XFixesSelectionNotifyEvent *notify_event)
{
    X11SelectionNotify *notify;
    notify = fcitx_handler_table_first(x11priv->selectionNotify, sizeof(Atom),
                                       &notify_event->selection);
    for (;notify;notify = fcitx_handler_table_next(x11priv->selectionNotify,
                                                   notify)) {
        notify->cb(x11priv, notify_event->selection,
                   notify_event->subtype, notify);
    }
}
#endif

void
X11SelectionNotifyHelper(FcitxX11 *x11priv, Atom selection, int subtype,
                         X11SelectionNotify *notify)
{
#ifdef HAVE_XFIXES
    X11SelectionNotifyCallback cb;
    cb = (X11SelectionNotifyCallback)notify->func;
    char *name = XGetAtomName(x11priv->dpy, selection);
    cb(notify->owner, name, subtype, notify->data);
    XFree(name);
#endif
}

int
X11SelectionNotifyRegister(
    FcitxX11 *x11priv, const char *sel_str, void *owner,
    X11SelectionNotifyCallback cb, void *data, FcitxDestroyNotify destroy)
{
#ifdef HAVE_XFIXES
    if (!cb)
        return FCITX_OBJECT_POOL_INVALID_ID;
    return X11SelectionNotifyRegisterInternal(
        x11priv, XInternAtom(x11priv->dpy, sel_str, False), owner,
        X11SelectionNotifyHelper, data, destroy, (FcitxCallBack)cb);
#else
    return FCITX_OBJECT_POOL_INVALID_ID;
#endif
}

int
X11SelectionNotifyRegisterInternal(
    FcitxX11 *x11priv, Atom selection, void *owner,
    X11SelectionNotifyInternalCallback cb, void *data,
    FcitxDestroyNotify destroy, FcitxCallBack func)
{
#ifdef HAVE_XFIXES
    if (!(x11priv->hasXfixes && cb))
        return FCITX_OBJECT_POOL_INVALID_ID;
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
    return FCITX_OBJECT_POOL_INVALID_ID;
#endif
}

void
X11SelectionNotifyRemove(FcitxX11 *x11priv, int id)
{
#ifdef HAVE_XFIXES
    if (!(x11priv->hasXfixes && id >= 0))
        return;
    fcitx_handler_table_remove_by_id(x11priv->selectionNotify, id);
#endif
}

static boolean
X11ConvertSelectionHelper(
    FcitxX11 *x11priv, Atom selection, Atom target, int format,
    size_t nitems, const void *buff, X11ConvertSelection *convert)
{
    X11ConvertSelectionCallback cb;
    cb = (X11ConvertSelectionCallback)convert->func;
    char *sel_str = XGetAtomName(x11priv->dpy, selection);
    char *tgt_str = XGetAtomName(x11priv->dpy, target);
    cb(convert->owner, sel_str, tgt_str, format, nitems, buff, convert->data);
    XFree(sel_str);
    XFree(tgt_str);
    return True;
}

static boolean
X11TextConvertSelectionHelper(
    FcitxX11 *x11priv, Atom selection, Atom target, int format,
    size_t nitems, const void *buff, X11ConvertSelection *convert)
{
    boolean res = true;
    char *sel_str = XGetAtomName(x11priv->dpy, selection);
    if (!buff) {
        Atom new_tgt;
        if (target == x11priv->utf8Atom) {
            new_tgt = x11priv->compTextAtom;
        } else if (target == x11priv->compTextAtom) {
            new_tgt = x11priv->stringAtom;
        } else {
            new_tgt = None;
        }
        if (new_tgt != None) {
            fcitx_utils_local_cat_str(prop_str, 256, FCITX_X11_SEL, sel_str);
            Atom prop = XInternAtom(x11priv->dpy, prop_str, False);
            XConvertSelection(x11priv->dpy, selection, new_tgt, prop,
                              x11priv->eventWindow, CurrentTime);
            res = false;
            goto out;
        }
    }
    X11ConvertSelectionCallback cb;
    cb = (X11ConvertSelectionCallback)convert->func;
    char *tgt_str = XGetAtomName(x11priv->dpy, target);
    /* compound text also looks fine here, need more info.. */
    cb(convert->owner, sel_str, tgt_str, format, nitems, buff, convert->data);
    XFree(tgt_str);
out:
    XFree(sel_str);
    return res;
}

static int
X11RequestConvertSelectionInternal(
    FcitxX11 *x11priv, const char *sel_str, Atom selection, Atom target,
    void *owner, X11ConvertSelectionInternalCallback cb, void *data,
    FcitxDestroyNotify destroy, FcitxCallBack func)
{
    fcitx_utils_local_cat_str(prop_str, 256, FCITX_X11_SEL, sel_str);
    Atom prop = XInternAtom(x11priv->dpy, prop_str, False);
    //TODO catch bad atom?
    XDeleteProperty(x11priv->dpy, x11priv->eventWindow, prop);
    XConvertSelection(x11priv->dpy, selection, target, prop,
                      x11priv->eventWindow, CurrentTime);
    X11ConvertSelection convert = {
        .owner = owner,
        .data = data,
        .target = target,
        .cb = cb,
        .destroy = destroy,
        .func = func
    };
    return fcitx_handler_table_prepend(x11priv->convertSelection,
                                       sizeof(Atom), &selection, &convert);
}

int
X11RequestConvertSelection(
    FcitxX11 *x11priv, const char *sel_str, const char *tgt_str,
    void *owner, X11ConvertSelectionCallback cb, void *data,
    FcitxDestroyNotify destroy)
{
    if (!cb)
        return FCITX_OBJECT_POOL_INVALID_ID;
    X11ConvertSelectionInternalCallback intern_cb;
    Atom tgt_atom;
    if (tgt_str && *tgt_str) {
        tgt_atom = XInternAtom(x11priv->dpy, tgt_str, False);
        intern_cb = X11ConvertSelectionHelper;
    } else {
        tgt_atom = x11priv->utf8Atom;
        intern_cb = X11TextConvertSelectionHelper;
    }
    return X11RequestConvertSelectionInternal(
        x11priv, sel_str, XInternAtom(x11priv->dpy, sel_str, False),
        tgt_atom, owner, intern_cb, data, destroy, (FcitxCallBack)cb);
}

static unsigned char*
X11GetWindowProperty(FcitxX11 *x11priv, Window win, Atom prop, Atom *ret_type,
                     int *ret_format, unsigned long *nitems)
{
    unsigned char *buff = NULL;
    int res;
    unsigned long bytes_left = 0;
    if (prop == None)
        goto fail;
    res = XGetWindowProperty(x11priv->dpy, win, prop, 0, 0x6400, // 100k / 4
                             False, AnyPropertyType, ret_type,
                             ret_format, nitems, &bytes_left, &buff);
    if (res != Success || *ret_type == None || !buff)
        goto fail;
    switch (*ret_format) {
    case 8:
    case 16:
    case 32:
        break;
    default:
        goto fail;
    }
    if (bytes_left)
        FcitxLog(WARNING, "Selection is too long.");
    return buff;
fail:
    if (buff)
        XFree(buff);
    *nitems = 0;
    *ret_format = 0;
    *ret_type = None;
    return NULL;
}

void
X11ProcessSelectionNotifyEvent(FcitxX11 *x11priv,
                               XSelectionEvent *selection_event)
{
    X11ConvertSelection *convert;
    int id;
    int next_id;
    FcitxHandlerTable *table = x11priv->convertSelection;
    id = fcitx_handler_table_first_id(table, sizeof(Atom),
                                      &selection_event->selection);
    if (id == FCITX_OBJECT_POOL_INVALID_ID)
        return;
    unsigned long nitems;
    int ret_format;
    Atom ret_type = None;
    unsigned char *buff;
    buff = X11GetWindowProperty(x11priv, x11priv->eventWindow,
                                selection_event->property, &ret_type,
                                &ret_format, &nitems);

    for (;(convert = fcitx_handler_table_get_by_id(table, id));id = next_id) {
        next_id = fcitx_handler_table_next_id(table, convert);
        if (convert->cb(x11priv, selection_event->selection,
                        selection_event->target, ret_format,
                        nitems, buff, convert)) {
            fcitx_handler_table_remove_by_id(table, id);
        }
    }
    if (buff) {
        XFree(buff);
        buff = NULL;
    }
}
