/***************************************************************************
 *   Copyright (C) 2002~2010 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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

#include "fcitx/fcitx.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "fcitx/fcitx.h"
#include "tray.h"
#include "TrayWindow.h"
#include "fcitx-utils/log.h"
#include <libintl.h>
#include "classicui.h"

#define MAX_SUPPORTED_XEMBED_VERSION 1

#define XEMBED_MAPPED          (1 << 0)

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY  0
#define XEMBED_WINDOW_ACTIVATE  1
#define XEMBED_WINDOW_DEACTIVATE  2
#define XEMBED_REQUEST_FOCUS 3
#define XEMBED_FOCUS_IN  4
#define XEMBED_FOCUS_OUT  5
#define XEMBED_FOCUS_NEXT 6
#define XEMBED_FOCUS_PREV 7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON 10
#define XEMBED_MODALITY_OFF 11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

static int iTrappedErrorCode = 0;
static int (*hOldErrorHandler)(Display *d, XErrorEvent *e);

/* static void tray_map_window (Display* dpy, Window win); */

static int
ErrorHandler(Display     *display,
             XErrorEvent *error)
{
    iTrappedErrorCode = error->error_code;
    return 0;
}

static void
TrapErrors(void)
{
    iTrappedErrorCode = 0;
    hOldErrorHandler = XSetErrorHandler(ErrorHandler);
}

static int
UntrapErrors(void)
{
    XSetErrorHandler(hOldErrorHandler);
    return iTrappedErrorCode;
}

int
InitTray(Display* dpy, TrayWindow* tray)
{
    char *atom_names[] = {
        NULL,
        "MANAGER",
        "_NET_SYSTEM_TRAY_OPCODE",
        "_NET_SYSTEM_TRAY_ORIENTATION",
        "_NET_SYSTEM_TRAY_VISUAL"
    };

    asprintf(&atom_names[0], "_NET_SYSTEM_TRAY_S%d", tray->owner->iScreen);

    XInternAtoms(dpy, atom_names, 5, False, tray->atoms);
    tray->size = 22;

    free(atom_names[0]);

    XWindowAttributes attr;
    XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &attr);
    if ((attr.your_event_mask & StructureNotifyMask) != StructureNotifyMask) {
        XSelectInput(dpy, DefaultRootWindow(dpy), attr.your_event_mask | StructureNotifyMask); // for MANAGER selection
    }
    return True;
}

int
TrayFindDock(Display *dpy, TrayWindow* tray)
{
    if (tray->window == None) {
        tray->bTrayMapped = False;
        return 0;
    }

    XGrabServer(dpy);

    tray->dockWindow = XGetSelectionOwner(dpy, tray->atoms[ATOM_SELECTION]);

    if (tray->dockWindow != None)
        XSelectInput(dpy, tray->dockWindow,
                     StructureNotifyMask | PropertyChangeMask);

    XUngrabServer(dpy);
    XFlush(dpy);

    if (tray->dockWindow != None) {
        TraySendOpcode(dpy, tray->dockWindow, tray, SYSTEM_TRAY_REQUEST_DOCK, tray->window, 0, 0);
        tray->bTrayMapped = True;
        return 1;
    } else {
        tray->bTrayMapped = False;
        ReleaseTrayWindow(tray);
    }

    return 0;
}

void TraySendOpcode(Display* dpy, Window dock, TrayWindow* tray,
                    long message, long data1, long data2, long data3)
{
    XEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = dock;
    ev.xclient.message_type = tray->atoms[ATOM_SYSTEM_TRAY_OPCODE];
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1;
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

    TrapErrors();
    XSendEvent(dpy, dock, False, NoEventMask, &ev);
    XSync(dpy, False);
    if (UntrapErrors()) {
        FcitxLog(WARNING, _("X error %i on opcode send"),
                 iTrappedErrorCode);
    }
}

XVisualInfo* TrayGetVisual(Display* dpy, TrayWindow* tray)
{
    if (tray->visual.visual) {
        return &tray->visual;
    }

    tray->dockWindow = XGetSelectionOwner(dpy, tray->atoms[ATOM_SELECTION]);

    if (tray->dockWindow != None) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_remaining;
        unsigned char *data = 0;
        int result = XGetWindowProperty(dpy, tray->dockWindow, tray->atoms[ATOM_VISUAL], 0, 1,
                                        False, XA_VISUALID, &actual_type,
                                        &actual_format, &nitems, &bytes_remaining, &data);
        VisualID vid = 0;
        if (result == Success && data && actual_type == XA_VISUALID && actual_format == 32 &&
                nitems == 1 && bytes_remaining == 0)
            vid = *(VisualID*)data;
        if (data)
            XFree(data);
        if (vid == 0)
            return 0;

        uint mask = VisualIDMask;
        XVisualInfo *vi, rvi;
        int count;
        rvi.visualid = vid;
        vi = XGetVisualInfo(dpy, mask, &rvi, &count);
        if (vi) {
            tray->visual = vi[0];
            XFree((char*)vi);
        }
        if (tray->visual.depth != 32)
            memset(&tray->visual, 0, sizeof(XVisualInfo));
    }

    return tray->visual.visual ? &tray->visual : 0;

}

Window TrayGetDock(Display* dpy, TrayWindow* tray)
{
    Window dock = XGetSelectionOwner(dpy, tray->atoms[ATOM_SELECTION]);
    return dock;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
