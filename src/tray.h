#ifndef _TRAY_H_
#define _TRAY_H_

typedef struct tray_win {
    Window window;

    XImage* icon[2];
    Pixmap picon[2];
    Pixmap icon_mask[2];
    GC gc;

    XVisualInfo visual;
} tray_win_t;

int tray_init(Display* dpy, tray_win_t* tray);
void tray_handle_client_message(Display *dpy, Window win, XEvent *an_event);
int tray_find_dock(Display *dpy, Window win);
XVisualInfo* tray_get_visual(Display *dpy, tray_win_t* tray);
void tray_send_opcode( Display* dpy,  Window w, long message,
        long data1, long data2, long data3 );
Window tray_get_dock(Display* dpy);

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define TRAY_ICON_WIDTH 22
#define TRAY_ICON_HEIGHT 22

#define ATOM_SELECTION 0
#define ATOM_MANAGER 1
#define ATOM_SYSTEM_TRAY_OPCODE 2
#define ATOM_ORIENTATION 3
#define ATOM_VISUAL 4

#endif
