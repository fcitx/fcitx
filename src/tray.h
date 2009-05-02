#ifndef _TRAY_H_
#define _TRAY_H_

extern int tray_init(Display* dpy, Window win);
extern void tray_handle_client_message(Display *dpy, Window win, XEvent *an_event);

#endif
