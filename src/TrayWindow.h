#ifndef _TRAY_WINDOW_H
#define _TRAY_WINDOW_H

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/shape.h>
#include <string.h>
#include "tray.h"

#define INACTIVE_ICON 0
#define ACTIVE_ICON   1

typedef struct tray_win {
    Window window;

    Pixmap icon[2];
    Pixmap icon_mask[2];
    GC gc;
    XpmAttributes xpm_attr;
} tray_win_t;

Bool CreateTrayWindow();
void DrawTrayWindow(int f_state);
void tray_win_deinit(tray_win_t *f_tray);

#endif
