/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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

#ifndef FCITX_MODULE_FCITX_X11_H
#define FCITX_MODULE_FCITX_X11_H

#include <stdint.h>
#include <fcitx/addon.h>
#include <fcitx/module.h>
#include "x11stuff.h"

static inline FcitxAddon*
FcitxX11GetAddon(FcitxInstance *instance)
{
    static FcitxAddon *addon = NULL;
    if (!addon) {
        addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance),
                                          FCITX_X11_NAME);
    }
    return addon;
}

static inline Display*
FcitxX11GetDisplay(FcitxInstance *instance)
{
    return (Display*)(intptr_t)FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_GETDISPLAY);
}

static inline void
FcitxX11AddXEventHandler(FcitxInstance *instance,
                         FcitxX11XEventHandler arg0, void *arg1)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_ADDXEVENTHANDLER,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1);
}

static inline void
FcitxX11RemoveXEventHandler(FcitxInstance *instance,
                            void *arg0)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_REMOVEXEVENTHANDLER,
        (void*)(intptr_t)arg0);
}

static inline Visual*
FcitxX11FindARGBVisual(FcitxInstance *instance)
{
    return (Visual*)(intptr_t)FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_FINDARGBVISUAL);
}

static inline void
FcitxX11InitWindowAttribute(FcitxInstance *instance,
                            Visual **arg0, Colormap *arg1,
                            XSetWindowAttributes *arg2,
                            unsigned long *arg3, int *arg4)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_INITWINDOWATTR,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1, (void*)(intptr_t)arg2,
        (void*)(intptr_t)arg3, (void*)(intptr_t)arg4);
}

static inline void
FcitxX11SetWindowProp(FcitxInstance *instance,
                      Window *arg0, FcitxXWindowType *arg1, char *arg2)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_SETWINDOWPROP,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1, (void*)(intptr_t)arg2);
}

static inline void
FcitxX11GetScreenSize(FcitxInstance *instance,
                      int *arg0, int *arg1)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_GETSCREENSIZE,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1);
}

static inline void
FcitxX11MouseClick(FcitxInstance *instance,
                   Window *arg0, int *arg1, int *arg2, boolean *arg3)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_MOUSECLICK,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1, (void*)(intptr_t)arg2,
        (void*)(intptr_t)arg3);
}

static inline void
FcitxX11AddCompositeHandler(FcitxInstance *instance,
                            FcitxX11CompositeHandler arg0, void *arg1)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_ADDCOMPOSITEHANDLER,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1);
}

static inline void
FcitxX11GetScreenGeometry(FcitxInstance *instance,
                          int *arg0, int *arg1, FcitxRect *arg2)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_GETSCREENGEOMETRY,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1, (void*)(intptr_t)arg2);
}

static inline void
FcitxX11ProcessRemainEvent(FcitxInstance *instance)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_PROCESSREMAINEVENT);
}

static inline void
FcitxX11GetDPI(FcitxInstance *instance,
               int *arg0, int *arg1)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), FCITX_X11_GETDPI,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1);
}

static inline unsigned int
FcitxX11RegSelectNotify(FcitxInstance *instance,
                        const char *arg0, void *arg1,
                        X11SelectionNotifyCallback arg2,
                        void *arg3, FcitxDestroyNotify arg4)
{
    return (unsigned int)(intptr_t)FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), 12,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1, (void*)(intptr_t)arg2,
        (void*)(intptr_t)arg3, (void*)(intptr_t)arg4);
}

static inline void
FcitxX11RemoveSelectNotify(FcitxInstance *instance,
                           unsigned int arg0)
{
    FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), 13,
        (void*)(intptr_t)arg0);
}

static inline Window
FcitxX11DefaultEventWindow(FcitxInstance *instance)
{
    return (Window)(intptr_t)FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), 14);
}


static inline unsigned int
FcitxX11RequestConvertSelect(FcitxInstance *instance,
                             const char *arg0, const char *arg1, void *arg2,
                             X11ConvertSelectionCallback arg3,
                             void *arg4, FcitxDestroyNotify arg5)
{
    return (unsigned int)(intptr_t)FcitxModuleInvokeVaArgs(
        FcitxX11GetAddon(instance), 15,
        (void*)(intptr_t)arg0, (void*)(intptr_t)arg1, (void*)(intptr_t)arg2,
        (void*)(intptr_t)arg3, (void*)(intptr_t)arg4, (void*)(intptr_t)arg5);
}

#endif
