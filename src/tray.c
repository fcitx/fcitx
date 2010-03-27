#ifdef _ENABLE_TRAY

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "tray.h"

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

Atom Atoms[6];
static int trapped_error_code = 0;
static int (*old_error_handler) (Display *d, XErrorEvent *e);

extern int iScreen;

/* static void tray_map_window (Display* dpy, Window win); */

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
tray_init(Display* dpy, tray_win_t* tray)
{
    static char *atom_names[] = {
        NULL,
		"MANAGER", 
		"_NET_SYSTEM_TRAY_OPCODE",
		"_NET_SYSTEM_TRAY_ORIENTATION",
		"_NET_SYSTEM_TRAY_VISUAL"
    };

	atom_names[0] = strdup("_NET_SYSTEM_TRAY_S0");
	atom_names[0][17] += iScreen;

    XInternAtoms (dpy, atom_names, 5, False, Atoms);
    memset(&tray->visual, 0, sizeof(XVisualInfo));

    return True;
}

int
tray_find_dock(Display *dpy, Window win)
{
    Window Dock;
    
    XGrabServer (dpy);

    Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SELECTION]);

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

XVisualInfo* tray_get_visual(Display* dpy, tray_win_t* tray)
{
    Window Dock;
    Dock = XGetSelectionOwner(dpy, Atoms[ATOM_SELECTION]);

    if (Dock != None) {
        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_remaining;
        unsigned char *data = 0;
        int result = XGetWindowProperty(dpy, Dock, Atoms[ATOM_VISUAL], 0, 1,
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

/*    void
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
*/

void tray_send_opcode(Display* dpy, Window w,
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
    XSendEvent(dpy, w, False, NoEventMask, &ev);
    XSync(dpy, False);
    if (untrap_errors()) {
        fprintf(stderr, "Tray.c : X error %i on opcode send\n",
                trapped_error_code );
    }
}

Window tray_get_dock(Display* dpy)
{
    Window dock = XGetSelectionOwner(dpy, Atoms[ATOM_SELECTION]);
    return dock;
}

#endif
