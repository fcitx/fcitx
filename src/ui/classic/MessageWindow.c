/***************************************************************************
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"

#include "MessageWindow.h"
#include "fcitx-utils/log.h"

#include <ctype.h>
#include <X11/Xatom.h>
#include <string.h>
#include "classicui.h"
#include "fcitx/module.h"
#include <cairo-xlib.h>
#include <X11/Xutil.h>
#include "fcitx-utils/utils.h"

#define MESSAGE_WINDOW_MARGIN 20
#define MESSAGE_WINDOW_LINESPACE 2
static void            InitMessageWindowProperty(MessageWindow* messageWindow);
static boolean MessageWindowEventHandler(void *arg, XEvent* event);
static void DisplayMessageWindow(MessageWindow *messageWindow);

MessageWindow* CreateMessageWindow(FcitxClassicUI * classicui)
{
    MessageWindow* messageWindow = fcitx_utils_malloc0(sizeof(MessageWindow));
    Display *dpy = classicui->dpy;
    int iScreen = classicui->iScreen;
    messageWindow->owner = classicui;

    messageWindow->color.r = messageWindow->color.g = messageWindow->color.b = 220.0 / 256;
    messageWindow->fontColor.r = messageWindow->fontColor.g = messageWindow->fontColor.b = 0;
    messageWindow->fontSize = 15;
    messageWindow->width = 1;
    messageWindow->height = 1;

    messageWindow->window =
        XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, WhitePixel(dpy, DefaultScreen(dpy)), WhitePixel(dpy, DefaultScreen(dpy)));

    messageWindow->surface = cairo_xlib_surface_create(dpy, messageWindow->window, DefaultVisual(dpy, iScreen), 1, 1);
    if (messageWindow->window == None)
        return False;

    InitMessageWindowProperty(messageWindow);
    XSelectInput(dpy, messageWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask);

    FcitxX11AddXEventHandler(classicui->owner,
                             MessageWindowEventHandler, messageWindow);

    return messageWindow;
}

boolean MessageWindowEventHandler(void *arg, XEvent* event)
{
    MessageWindow* messageWindow = (MessageWindow*) arg;
    if (event->type == ClientMessage
            && event->xclient.data.l[0] == messageWindow->owner->killAtom
            && event->xclient.window == messageWindow->window
       ) {
        XUnmapWindow(messageWindow->owner->dpy, messageWindow->window);
        return true;
    }

    if (event->xany.window == messageWindow->window) {
        switch (event->type) {
        case Expose:
            DrawMessageWindow(messageWindow, NULL, NULL, 0);
            DisplayMessageWindow(messageWindow);
            break;
        case ButtonRelease: {
            switch (event->xbutton.button) {
            case Button1:
                XUnmapWindow(messageWindow->owner->dpy, messageWindow->window);
                break;
            }
        }
        break;
        }
        return true;
    }
    return false;
}
void InitMessageWindowProperty(MessageWindow *messageWindow)
{
    FcitxClassicUI* classicui = messageWindow->owner;
    Display *dpy = classicui->dpy;
    XSetTransientForHint(dpy, messageWindow->window, DefaultRootWindow(dpy));

    ClassicUISetWindowProperty(classicui, messageWindow->window, FCITX_WINDOW_DIALOG, "Fcitx - Message");

    XSetWMProtocols(dpy, messageWindow->window, &classicui->killAtom, 1);
}

void DisplayMessageWindow(MessageWindow *messageWindow)
{
    FcitxClassicUI* classicui = messageWindow->owner;
    Display *dpy = classicui->dpy;
    int dwidth, dheight;
    GetScreenSize(classicui, &dwidth, &dheight);
    XMapRaised(dpy, messageWindow->window);
    XMoveWindow(dpy, messageWindow->window, (dwidth - messageWindow->width) / 2, (dheight - messageWindow->height) / 2);
}

void DrawMessageWindow(MessageWindow* messageWindow, char *title, char **msg, int length)
{
    FcitxClassicUI* classicui = messageWindow->owner;
    Display *dpy = classicui->dpy;
    int i = 0;
    if (title) {
        if (messageWindow->title)
            free(messageWindow->title);
        messageWindow->title = strdup(title);
    } else if (!messageWindow->title)
        return;

    title = messageWindow->title;
    FcitxLog(DEBUG, "%s", title);

    XTextProperty   tp;
    memset(&tp, 0, sizeof(XTextProperty));
    if (tp.value) {
        Xutf8TextListToTextProperty(dpy, &title, 1, XUTF8StringStyle, &tp);
        XSetWMName(dpy, messageWindow->window, &tp);
        XFree(tp.value);
    }

    if (msg) {
        if (messageWindow->msg) {
            for (i = 0 ; i < messageWindow->length; i++)
                free(messageWindow->msg[i]);
            free(messageWindow->msg);
        }
        messageWindow->length = length;
        messageWindow->msg = malloc(sizeof(char*) * length);
        for (i = 0; i < messageWindow->length; i++)
            messageWindow->msg[i] = strdup(msg[i]);
    } else {
        if (!messageWindow->msg)
            return;
    }
    msg = messageWindow->msg;
    length = messageWindow->length;

    if (!msg || length == 0)
        return;

    messageWindow->height = MESSAGE_WINDOW_MARGIN * 2 + length * (messageWindow->fontSize + MESSAGE_WINDOW_LINESPACE);
    messageWindow->width = 0;

    for (i = 0; i < length ; i ++) {
        int width = StringWidth(msg[i], classicui->font, messageWindow->fontSize, false);
        if (width > messageWindow->width)
            messageWindow->width = width;
    }

    messageWindow->width += MESSAGE_WINDOW_MARGIN * 2;
    XResizeWindow(dpy, messageWindow->window, messageWindow->width, messageWindow->height);
    _CAIRO_SETSIZE(messageWindow->surface,
                   messageWindow->width, messageWindow->height);

    cairo_t *c = cairo_create(messageWindow->surface);
    cairo_set_source_rgb(c, messageWindow->color.r, messageWindow->color.g, messageWindow->color.b);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);

    SetFontContext(c, classicui->font, messageWindow->fontSize, 0);

    cairo_paint(c);

    cairo_set_source_rgb(c, messageWindow->fontColor.r, messageWindow->fontColor.g, messageWindow->fontColor.b);

    int x, y;
    x = MESSAGE_WINDOW_MARGIN;
    y = MESSAGE_WINDOW_MARGIN;
    for (i = 0; i < length ; i ++) {
        OutputStringWithContext(c, 0, msg[i], x, y);
        y += messageWindow->fontSize + MESSAGE_WINDOW_LINESPACE;
    }

    ResetFontContext();
    cairo_destroy(c);
    cairo_surface_flush(messageWindow->surface);

    ActivateWindow(dpy, classicui->iScreen, messageWindow->window);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
