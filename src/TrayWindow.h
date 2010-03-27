#ifndef _TRAY_WINDOW_H
#define _TRAY_WINDOW_H

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>
#include <string.h>
#include "tray.h"

#define INACTIVE_ICON 0
#define ACTIVE_ICON   1

Bool CreateTrayWindow();
void DrawTrayWindow(int f_state, int x, int y, int w, int h);
void tray_win_deinit(tray_win_t *f_tray);
void tray_win_redraw(void);
void tray_event_handler(XEvent* event);

#endif
