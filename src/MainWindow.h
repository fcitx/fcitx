#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <X11/Xlib.h>

#define MAINWND_STARTX	500
#define _MAINWND_WIDTH	104
#define _MAINWND_WIDTH_COMPACT	32
#define MAINWND_STARTY	1
#define MAINWND_HEIGHT	20

typedef enum _HIDE_MAINWINDOW {
    HM_SHOW,
    HM_AUTO,
    HM_HIDE
} HIDE_MAINWINDOW;

Bool            CreateMainWindow (void);
void            DisplayMainWindow (void);
void            InitMainWindowColor (void);
void            ChangeLock (void);

#endif
