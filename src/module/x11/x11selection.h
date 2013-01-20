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

#ifndef X11SELECTION_H
#define X11SELECTION_H

#include "x11stuff-internal.h"
void X11InitSelection(FcitxX11 *x11priv);
#ifdef HAVE_XFIXES
void X11ProcessXFixesSelectionNotifyEvent(
    FcitxX11 *x11priv, XFixesSelectionNotifyEvent *notify_event);
#endif
void X11ProcessSelectionNotifyEvent(
    FcitxX11 *x11priv, XSelectionEvent *selection_event);
int X11SelectionNotifyRegisterInternal(
    FcitxX11 *x11priv, Atom selection, void *owner,
    X11SelectionNotifyInternalCallback cb, void *data,
    FcitxDestroyNotify destroy, FcitxCallBack func);
int X11SelectionNotifyRegister(
    FcitxX11 *x11priv, const char *sel_str, void *owner,
    X11SelectionNotifyCallback cb, void *data,
    FcitxDestroyNotify destroy);
void X11SelectionNotifyRemove(FcitxX11 *x11priv, int id);
int X11RequestConvertSelection(
    FcitxX11 *x11priv, const char *sel_str, const char *tgt_str,
    void *owner, X11ConvertSelectionCallback cb, void *data,
    FcitxDestroyNotify destroy);

#endif
