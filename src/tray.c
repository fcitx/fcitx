#ifdef _ENABLE_TRAY

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "tray.h"

#define ATOM_SYSTEM_TRAY 0
#define ATOM_SYSTEM_TRAY_OPCODE 1
#define ATOM_NET_SYSTEM_TRAY_MESSAGE_DATA 2
#define ATOM_XEMBED_INFO 3
#define ATOM_XEMBED 4
#define ATOM_MANAGER 5

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

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

static Atom Atoms[6];
static Window Dock = None;
static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

static int tray_find_dock(Display *dpy, Window win);
static void tray_set_xembed_info (Display* dpy, Window win, int flags);
static void tray_send_opcode( Display* dpy,  Window w, long message,
        long data1, long data2, long data3 );
static void tray_map_window (Display* dpy, Window win);

    static int
error_handler(Display     *display,
        XErrorEvent *error)
{
    trapped_error_code = error->error_code;
    return 0;
}

    static void
trap_errors(void)
{
    trapped_error_code = 0;
    old_error_handler = XSetErrorHandler(error_handler);
}

    static int
untrap_errors(void)
{
    XSetErrorHandler(old_error_handler);
    return trapped_error_code;
}

    int
tray_init(Display* dpy, Window win)
{
    static char *atom_names[] = {
        "_NET_SYSTEM_TRAY_S0", "_NET_SYSTEM_TRAY_OPCODE", 
        "_NET_SYSTEM_TRAY_MESSAGE_DATA",
        "_XEMBED_INFO", "_XEMBED",
        "MANAGER"
    };

    XInternAtoms (dpy, atom_names, 6, False, Atoms);

    tray_set_xembed_info(dpy, win, 0);

    /* Find the dock */
    return tray_find_dock(dpy, win);
}

    static int
tray_find_dock(Display *dpy, Window win)
{
    XGrabServer (dpy);

    Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SYSTEM_TRAY]);

    if (!Dock)
        XSelectInput(dpy, RootWindow(dpy, DefaultScreen(dpy)),
                StructureNotifyMask);

    XUngrabServer (dpy);
    XFlush (dpy);

    if (Dock != None) {
        tray_send_opcode(dpy, Dock, SYSTEM_TRAY_REQUEST_DOCK, win, 0, 0);
        return 1;
    } 

    return 0;
}

    void
tray_handle_client_message(Display *dpy, Window win, XEvent *an_event)
{
    if (an_event->xclient.message_type == Atoms[ATOM_XEMBED]) {
        switch (an_event->xclient.data.l[1]) {
            case XEMBED_EMBEDDED_NOTIFY:
            case XEMBED_WINDOW_ACTIVATE:
                tray_map_window (dpy, win);
                break;
        }
    }

    if (an_event->xclient.message_type == Atoms[ATOM_MANAGER] && Dock == None) { 
        if (an_event->xclient.data.l[1] == Atoms[ATOM_SYSTEM_TRAY])
            tray_find_dock(dpy, win);
    }
}

    void
tray_set_xembed_info (Display* dpy, Window win, int flags)
{
    CARD32 list[2];

    list[0] = MAX_SUPPORTED_XEMBED_VERSION;
    list[1] = flags;
    XChangeProperty (dpy, win, Atoms[ATOM_XEMBED_INFO], XA_CARDINAL, 32,
            PropModeReplace, (unsigned char *) list, 2);
}

static void tray_send_opcode(Display* dpy, Window w,
        long message, long data1, long data2, long data3)
{
    XEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = Atoms[ATOM_SYSTEM_TRAY_OPCODE];
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1;
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

    trap_errors();
    XSendEvent(dpy, Dock, False, NoEventMask, &ev);
    XSync(dpy, False);
    if (untrap_errors()) {
        fprintf(stderr, "Tray.c : X error %i on opcode send\n",
                trapped_error_code );
    }
}

    void
tray_map_window (Display* dpy, Window win)
{
    XMapWindow(dpy, win);  /* dock will probably do this as well */
    tray_set_xembed_info (dpy, win, XEMBED_MAPPED);
}

#endif
