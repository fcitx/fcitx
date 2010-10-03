/***************************************************************************
 *   Copyright (C) 2005 by Yunfan                                          *
 *   yunfan_zg@163.com                                                     *
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

#include "ui/AboutWindow.h"
#include "ui/ui.h"
#include "core/xim.h"
#include "fcitx-config/configfile.h"
#include "version.h"

#include <ctype.h>

#include <iconv.h>
#include <X11/Xatom.h>

extern Display *dpy;
extern int      iScreen;

int             ABOUT_WINDOW_WIDTH;

char            AboutCaption[] = "关于 - FCITX";
char            AboutTitle[] = "小企鹅中文输入法";
char            AboutEmail[] = "yuking_net@sohu.com";
char            AboutCopyRight[] = "(c) 2005, Yuking";
char            strTitle[100];

AboutWindow aboutWindow;
extern Atom killAtom;
static void            InitAboutWindowProperty (void);

Bool CreateAboutWindow (void)
{
    strcpy (strTitle, AboutTitle);
    strcat (strTitle, " ");
    strcat (strTitle, FCITX_VERSION);

    aboutWindow.color.r = aboutWindow.color.g = aboutWindow.color.b = 220.0 / 256;
    aboutWindow.fontColor.r = aboutWindow.fontColor.g = aboutWindow.fontColor.b = 0;
    aboutWindow.fontSize = 11;

    ABOUT_WINDOW_WIDTH = StringWidth (strTitle, gs.fontZh, aboutWindow.fontSize ) + 50;
    aboutWindow.window =
	XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), (DisplayWidth (dpy, iScreen) - ABOUT_WINDOW_WIDTH) / 2, (DisplayHeight (dpy, iScreen) - ABOUT_WINDOW_HEIGHT) / 2, ABOUT_WINDOW_WIDTH, ABOUT_WINDOW_HEIGHT, 0, WhitePixel (dpy, DefaultScreen (dpy)), WhitePixel (dpy, DefaultScreen (dpy)));

    aboutWindow.surface = cairo_xlib_surface_create(dpy, aboutWindow.window, DefaultVisual(dpy, iScreen), ABOUT_WINDOW_WIDTH, ABOUT_WINDOW_HEIGHT); 
    if (aboutWindow.window == None)
	return False;

    InitAboutWindowProperty ();
    XSelectInput (dpy, aboutWindow.window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask );

    return True;
}

void InitAboutWindowProperty (void)
{
    Atom            about_wm_window_type = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom            type_toolbar = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    XSetTransientForHint (dpy, aboutWindow.window, DefaultRootWindow (dpy));

    XChangeProperty (dpy, aboutWindow.window, about_wm_window_type, XA_ATOM, 32, PropModeReplace, (void *) &type_toolbar, 1);

    XSetWMProtocols (dpy, aboutWindow.window, &killAtom, 1);

	char           *p;

	p = AboutCaption;

    XTextProperty   tp;
    Xutf8TextListToTextProperty(dpy, &p, 1, XUTF8StringStyle, &tp);
    XSetWMName (dpy, aboutWindow.window, &tp);
    XFree(tp.value);
}

void DisplayAboutWindow (void)
{
    XMapRaised (dpy, aboutWindow.window);
    XMoveWindow (dpy, aboutWindow.window, (DisplayWidth (dpy, iScreen) - ABOUT_WINDOW_WIDTH) / 2, (DisplayHeight (dpy, iScreen) - ABOUT_WINDOW_HEIGHT) / 2);
}

void DrawAboutWindow (void)
{
    cairo_t *c = cairo_create(aboutWindow.surface);
    cairo_set_source_rgb(c, aboutWindow.color.r, aboutWindow.color.g, aboutWindow.color.b);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c);

    OutputString (c, strTitle, gs.fontZh, aboutWindow.fontSize, (ABOUT_WINDOW_WIDTH - StringWidth (strTitle, gs.fontZh, aboutWindow.fontSize)) / 2, aboutWindow.fontSize + 6 + 30, &aboutWindow.fontColor);

    OutputString (c, AboutEmail, gs.fontZh, aboutWindow.fontSize, (ABOUT_WINDOW_WIDTH - StringWidth (AboutEmail, gs.fontZh, aboutWindow.fontSize)) / 2, aboutWindow.fontSize + 6 + 60, &aboutWindow.fontColor);

    OutputString (c, AboutCopyRight, gs.fontZh, aboutWindow.fontSize, (ABOUT_WINDOW_WIDTH - StringWidth (AboutCopyRight, gs.fontZh, aboutWindow.fontSize)) / 2,aboutWindow.fontSize + 6 + 80, &aboutWindow.fontColor);

    cairo_destroy(c);
}
