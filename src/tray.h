#ifndef _TRAY_H_
#define _TRAY_H_

int tray_init(Display* dpy, Window win);
void tray_handle_client_message(Display *dpy, Window win, XEvent *an_event);
int tray_find_dock(Display *dpy, Window win);

#endif
