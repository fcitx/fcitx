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

#ifndef X11STUFF_H
#define X11STUFF_H
#include <X11/Xlib.h>
#include <stdint.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/instance.h>
#include <fcitx/addon.h>
#include <fcitx/module.h>

#define FCITX_X11_NAME "fcitx-x11"
#define FCITX_X11_GETDISPLAY 0
#define FCITX_X11_GETDISPLAY_RETURNTYPE Display*
#define FCITX_X11_ADDXEVENTHANDLER 1
#define FCITX_X11_ADDXEVENTHANDLER_RETURNTYPE void
#define FCITX_X11_REMOVEXEVENTHANDLER 2
#define FCITX_X11_REMOVEXEVENTHANDLER_RETURNTYPE void
#define FCITX_X11_FINDARGBVISUAL 3
#define FCITX_X11_FINDARGBVISUAL_RETURNTYPE Visual*
#define FCITX_X11_INITWINDOWATTR 4
#define FCITX_X11_INITWINDOWATTR_RETURNTYPE void
#define FCITX_X11_SETWINDOWPROP 5
#define FCITX_X11_SETWINDOWPROP_RETURNTYPE void
#define FCITX_X11_GETSCREENSIZE 6
#define FCITX_X11_GETSCREENSIZE_RETURNTYPE void
#define FCITX_X11_MOUSECLICK 7
#define FCITX_X11_MOUSECLICK_RETURNTYPE void
#define FCITX_X11_ADDCOMPOSITEHANDLER 8
#define FCITX_X11_ADDCOMPOSITEHANDLER_RETURNTYPE void
#define FCITX_X11_GETSCREENGEOMETRY 9
#define FCITX_X11_GETSCREENGEOMETRY_RETURNTYPE void
#define FCITX_X11_PROCESSREMAINEVENT 10
#define FCITX_X11_PROCESSREMAINEVENT_RETURNTYPE void
#define FCITX_X11_GETDPI 11
#define FCITX_X11_GETDPI_RETURNTYPE void

typedef boolean (*FcitxX11XEventHandler)(void *instance, XEvent *event);
typedef void (*FcitxX11CompositeHandler)(void *instance, boolean enable);

typedef struct _FcitxRect {
    int x1, y1, x2, y2;
} FcitxRect;

typedef enum _FcitxXWindowType {
    FCITX_WINDOW_UNKNOWN,
    FCITX_WINDOW_DOCK,
    FCITX_WINDOW_MENU,
    FCITX_WINDOW_DIALOG
} FcitxXWindowType;

typedef void (*FcitxDestroyNotify)(void*);
typedef void (*X11SelectionNotifyCallback)(void *owner, const char *selection,
                                           int subtype, void *data);
typedef void (*X11ConvertSelectionCallback)(
    void *owner, const char *sel_str, const char *tgt_str, int format,
    size_t nitems, const void *buff, void *data);

// Well won't work if there are multiple instances, but that will also break
// lots of other things as well.
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
// kate: indent-mode cstyle; space-indent on; indent-width 0;
