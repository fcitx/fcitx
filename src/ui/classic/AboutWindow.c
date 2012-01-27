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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <ctype.h>
#include <X11/Xatom.h>
#include <cairo/cairo-xlib.h>

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "fcitx-utils/utils.h"
#include "module/x11/x11stuff.h"

#include "AboutWindow.h"
#include "classicui.h"

int             ABOUT_WINDOW_WIDTH;

char            AboutCaption[] = "关于 - FCITX";
char            AboutTitle[] = "小企鹅中文输入法";
char            AboutEmail[] = "yuking_net@sohu.com";
char            AboutCopyRight[] = "(c) 2005, Yuking";
char            strTitle[100];

static void            InitAboutWindowProperty(AboutWindow* aboutWindow);
static boolean AboutWindowEventHandler(void *arg, XEvent* event);

AboutWindow* CreateAboutWindow(FcitxClassicUI *classicui)
{
    AboutWindow *aboutWindow = fcitx_utils_malloc0(sizeof(AboutWindow));
    Display* dpy = classicui->dpy;
    int iScreen = classicui->iScreen;
    int dwidth, dheight;
    strcpy(strTitle, AboutTitle);
    strcat(strTitle, " ");
    strcat(strTitle, VERSION);
    GetScreenSize(classicui, &dwidth, &dheight);

    aboutWindow->owner = classicui;

    aboutWindow->color.r = aboutWindow->color.g = aboutWindow->color.b = 220.0 / 256;
    aboutWindow->fontColor.r = aboutWindow->fontColor.g = aboutWindow->fontColor.b = 0;
    aboutWindow->fontSize = 11;

    ABOUT_WINDOW_WIDTH = StringWidth(strTitle, classicui->font, aboutWindow->fontSize) + 50;
    aboutWindow->window =
        XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), (dwidth - ABOUT_WINDOW_WIDTH) / 2, (dheight - ABOUT_WINDOW_HEIGHT) / 2, ABOUT_WINDOW_WIDTH, ABOUT_WINDOW_HEIGHT, 0, WhitePixel(dpy, DefaultScreen(dpy)), WhitePixel(dpy, DefaultScreen(dpy)));

    aboutWindow->surface = cairo_xlib_surface_create(dpy, aboutWindow->window, DefaultVisual(dpy, iScreen), ABOUT_WINDOW_WIDTH, ABOUT_WINDOW_HEIGHT);
    if (aboutWindow->window == None)
        return NULL;

    InitAboutWindowProperty(aboutWindow);
    XSelectInput(dpy, aboutWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask);

    FcitxModuleFunctionArg arg;
    arg.args[0] = AboutWindowEventHandler;
    arg.args[1] = aboutWindow;
    InvokeFunction(classicui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);

    return aboutWindow;
}

boolean AboutWindowEventHandler(void *arg, XEvent* event)
{
    AboutWindow* aboutWindow = (AboutWindow*) arg;
    if (event->type == ClientMessage
            && event->xclient.data.l[0] == aboutWindow->owner->killAtom
            && event->xclient.window == aboutWindow->window
       ) {
        XUnmapWindow(aboutWindow->owner->dpy, aboutWindow->window);
        return true;
    }

    if (event->xany.window == aboutWindow->window) {
        switch (event->type) {
        case Expose:
            DrawAboutWindow(aboutWindow);
            break;
        case ButtonRelease: {
            switch (event->xbutton.button) {
            case Button1:
                XUnmapWindow(aboutWindow->owner->dpy, aboutWindow->window);
                break;
            }
        }
        break;
        }
        return true;
    }
    return false;
}

void InitAboutWindowProperty(AboutWindow* aboutWindow)
{
    FcitxClassicUI *classicui = aboutWindow->owner;
    Display* dpy = classicui->dpy;
    XSetTransientForHint(dpy, aboutWindow->window, DefaultRootWindow(dpy));

    ClassicUISetWindowProperty(classicui, aboutWindow->window, FCITX_WINDOW_DIALOG, AboutCaption);

    XSetWMProtocols(dpy, aboutWindow->window, &classicui->killAtom, 1);
}

void DisplayAboutWindow(AboutWindow* aboutWindow)
{
    FcitxClassicUI *classicui = aboutWindow->owner;
    Display* dpy = classicui->dpy;
    int dwidth, dheight;
    GetScreenSize(classicui, &dwidth, &dheight);
    XMapRaised(dpy, aboutWindow->window);
    XMoveWindow(dpy, aboutWindow->window, (dwidth - ABOUT_WINDOW_WIDTH) / 2, (dheight - ABOUT_WINDOW_HEIGHT) / 2);
}



void DrawAboutWindow(AboutWindow* aboutWindow)
{
    FcitxClassicUI *classicui = aboutWindow->owner;
    cairo_t *c = cairo_create(aboutWindow->surface);
    cairo_set_source_rgb(c, aboutWindow->color.r, aboutWindow->color.g, aboutWindow->color.b);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c);

    OutputString(c, strTitle, classicui->font, aboutWindow->fontSize, (ABOUT_WINDOW_WIDTH - StringWidth(strTitle, classicui->font, aboutWindow->fontSize)) / 2, 6 + 30, &aboutWindow->fontColor);

    OutputString(c, AboutEmail, classicui->font, aboutWindow->fontSize, (ABOUT_WINDOW_WIDTH - StringWidth(AboutEmail, classicui->font, aboutWindow->fontSize)) / 2, 6 + 60, &aboutWindow->fontColor);

    OutputString(c, AboutCopyRight, classicui->font, aboutWindow->fontSize, (ABOUT_WINDOW_WIDTH - StringWidth(AboutCopyRight, classicui->font, aboutWindow->fontSize)) / 2, 6 + 80, &aboutWindow->fontColor);

    cairo_destroy(c);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
