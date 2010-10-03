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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef _ENABLE_TRAY

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/extensions/Xrender.h>

#include "ui/TrayWindow.h"
#include "ui/tray.h"
#include "ui/skin.h"
#include "core/xim.h"
#include "fcitx-config/configfile.h"

TrayWindow tray;

extern Display *dpy;
extern int iScreen;
extern CARD16 connect_id;

Bool CreateTrayWindow() {
    XTextProperty tp;
    char   strWindowName[]="Fcitx Tray Window";
    if ( !fc.bUseTrayIcon )
            return False;

    InitTray(dpy, &tray);

    XVisualInfo* vi = TrayGetVisual(dpy, &tray);
    if (vi && vi->visual) {
        Window p = DefaultRootWindow (dpy);
        Colormap colormap = XCreateColormap(dpy, p, vi->visual, AllocNone);
        XSetWindowAttributes wsa;
        wsa.background_pixmap = 0;
        wsa.colormap = colormap;
        wsa.background_pixel = 0;
        wsa.border_pixel = 0;
        tray.window = XCreateWindow(dpy, p, -1, -1, 1, 1,
                0, vi->depth, InputOutput, vi->visual,
                CWBackPixmap|CWBackPixel|CWBorderPixel|CWColormap, &wsa);
    }
    else {
        tray.window = XCreateSimpleWindow (dpy, DefaultRootWindow(dpy), \
            -1, -1, 1, 1, 0, \
            BlackPixel (dpy, DefaultScreen (dpy)), \
            WhitePixel (dpy, DefaultScreen (dpy)));
        XSetWindowBackgroundPixmap(dpy, tray.window, ParentRelative);
    }
    if (tray.window == (Window) NULL)
        return False;

    load_tray_img();
    XSizeHints size_hints;
    size_hints.flags = PWinGravity | PBaseSize;
    size_hints.base_width = tray.size;
    size_hints.base_height = tray.size;
    XSetWMNormalHints(dpy, tray.window, &size_hints);

    //Set the name of the window
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, tray.window, &tp);
   
    if (vi && vi->visual)
        tray.cs = cairo_xlib_surface_create(dpy, tray.window, tray.visual.visual, 200, 200);
    else
    {
        Visual *target_visual = DefaultVisual (dpy, iScreen);
        tray.cs = cairo_xlib_surface_create(dpy, tray.window, target_visual, 200, 200);
    }

    XSelectInput (dpy, tray.window, ExposureMask | KeyPressMask | \
            ButtonPressMask | ButtonReleaseMask | StructureNotifyMask \
            | EnterWindowMask | PointerMotionMask | LeaveWindowMask | VisibilityChangeMask);
    return True;
}

void DrawTrayWindow(int f_state, int x, int y, int w, int h) {
    if ( !fc.bUseTrayIcon )
            return;

	cairo_t *c;
	cairo_surface_t *png_surface ;
    FcitxImage* skinImg;
    if (!tray.bTrayMapped) {
        if (!TrayFindDock(dpy, &tray))
			return;
    }

    /* 画png */
    if (f_state)
    {
        png_surface = trayActive;
        skinImg = &sc.skinTrayIcon.active;
    }
    else
    {
        png_surface = trayInactive;
        skinImg = &sc.skinTrayIcon.inactive;
    }

    c=cairo_create(tray.cs);

    XVisualInfo* vi = TrayGetVisual(dpy, &tray);
    if (vi && vi->visual)
    {
        /* 清空窗口 */
        cairo_set_source_rgba(c, 0, 0, 0, 0);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_paint(c);
    }
    else
    {
        XClearArea (dpy, tray.window, x, y, w, h, False);
    }

    if( strlen(skinImg->img_name) != 0 && strcmp( skinImg->img_name ,"NONE.img") != 0)
    {
        cairo_scale(c, ((double) tray.size) / skinImg->height, ((double) tray.size) / skinImg->width);
        cairo_set_source_surface(c, png_surface, x , y );
        cairo_set_operator(c, CAIRO_OPERATOR_OVER);
        cairo_paint_with_alpha(c,1);
    }
	
	cairo_destroy(c);

}

void DeInitTrayWindow(TrayWindow *f_tray) {
    ;
}

void RedrawTrayWindow(void) {
    TrayFindDock(dpy, &tray);
}

void TrayEventHandler(XEvent* event)
{
    if (!fc.bUseTrayIcon)
        return;
    switch (event->type) {
		case ClientMessage:
			if (event->xclient.message_type == tray.atoms[ATOM_MANAGER]
			 && event->xclient.data.l[1] == tray.atoms[ATOM_SELECTION])
			{
                TrayFindDock(dpy, &tray);
			}
			break;

		case Expose:
			if (event->xexpose.window == tray.window) {
                if (ConnectIDGetState (connect_id) == IS_CHN)
                    DrawTrayWindow (ACTIVE_ICON, 0, 0, tray.size, tray.size);
                else
                    DrawTrayWindow (INACTIVE_ICON, 0, 0, tray.size, tray.size);
			}
			break;
        case ConfigureNotify:
            if (tray.window == event->xconfigure.window)
            {
                int size = event->xconfigure.height;
                if (size != tray.size)
                {
                    tray.size = size;
                    XSizeHints size_hints;
                    size_hints.flags = PWinGravity | PBaseSize;
                    size_hints.base_width = tray.size;
                    size_hints.base_height = tray.size;
                    XSetWMNormalHints(dpy, tray.window, &size_hints);
                }

                if (ConnectIDGetState (connect_id) == IS_CHN)
                    DrawTrayWindow (ACTIVE_ICON, 0, 0, tray.size, tray.size);
                else
                    DrawTrayWindow (INACTIVE_ICON, 0, 0, tray.size, tray.size);
                break;
            }


		case DestroyNotify:
            tray.bTrayMapped = False;
			break;

        case ReparentNotify:
            {
                if (event->xreparent.parent == DefaultRootWindow(dpy) && event->xreparent.window == tray.window)
                {
                    tray.bTrayMapped = False;
                    if (ConnectIDGetState (connect_id) == IS_CHN)
                        DrawTrayWindow (ACTIVE_ICON, 0, 0, tray.size, tray.size);
                    else
                        DrawTrayWindow (INACTIVE_ICON, 0, 0, tray.size, tray.size);
                }
            }
            break;
	}
}

#endif
