/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "x11stuff.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include <unistd.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xatom.h>
#include "xerrorhandler.h"

static void* X11Create(FcitxInstance* instance);
static void X11SetFD(void* arg);
static void X11ProcessEvent(void *arg);
static void* X11GetDisplay(void* x11priv, FcitxModuleFunctionArg arg);
static void* X11AddEventHandler(void* x11priv, FcitxModuleFunctionArg arg);
static void* X11RemoveEventHandler(void* x11priv, FcitxModuleFunctionArg arg);
static void* X11FindARGBVisual(void* arg, FcitxModuleFunctionArg args);
static void* X11InitWindowAttribute(void* arg, FcitxModuleFunctionArg args);
static void* X11SetWindowProperty(void* arg, FcitxModuleFunctionArg args);
static void* X11GetScreenSize(void *arg, FcitxModuleFunctionArg args);
static void* X11MouseClick(void *arg, FcitxModuleFunctionArg args);
static void* X11AddCompositeHandler(void* arg, FcitxModuleFunctionArg args);
static void InitComposite(FcitxX11* x11stuff);
static void X11HandlerComposite(FcitxX11* x11priv, boolean enable);
static boolean X11GetCompositeManager(FcitxX11* x11stuff);

const UT_icd handler_icd = {sizeof(FcitxXEventHandler), 0, 0, 0};
const UT_icd comphandler_icd = {sizeof(FcitxCompositeChangedHandler), 0, 0, 0};

FCITX_EXPORT_API
FcitxModule module = {
    X11Create,
    X11SetFD,
    X11ProcessEvent,
    NULL,
    NULL
};

void* X11Create(FcitxInstance* instance)
{
    FcitxX11* x11priv = fcitx_malloc0(sizeof(FcitxX11));
    FcitxAddon* x11addon = GetAddonByName(&instance->addons, FCITX_X11_NAME);
    x11priv->dpy = XOpenDisplay(NULL);
    if (x11priv->dpy == NULL)
        return false;

    x11priv->owner = instance;
    x11priv->iScreen = DefaultScreen(x11priv->dpy);
    x11priv->windowTypeAtom = XInternAtom (x11priv->dpy, "_NET_WM_WINDOW_TYPE", False);
    x11priv->typeMenuAtom = XInternAtom (x11priv->dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    x11priv->typeDialogAtom = XInternAtom (x11priv->dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    x11priv->typeDockAtom = XInternAtom (x11priv->dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    x11priv->pidAtom = XInternAtom(x11priv->dpy, "_NET_WM_PID", False);

    utarray_init(&x11priv->handlers, &handler_icd);
    utarray_init(&x11priv->comphandlers, &comphandler_icd);

    /* ensure the order ! */
    AddFunction(x11addon, X11GetDisplay);
    AddFunction(x11addon, X11AddEventHandler);
    AddFunction(x11addon, X11RemoveEventHandler);
    AddFunction(x11addon, X11FindARGBVisual);
    AddFunction(x11addon, X11InitWindowAttribute);
    AddFunction(x11addon, X11SetWindowProperty);
    AddFunction(x11addon, X11GetScreenSize);
    AddFunction(x11addon, X11MouseClick);
    AddFunction(x11addon, X11AddCompositeHandler);
    InitComposite(x11priv);

    InitXErrorHandler (x11priv);
    return x11priv;
}

void X11SetFD(void* arg)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    int fd = ConnectionNumber(x11priv->dpy);
    FD_SET(fd, &x11priv->owner->rfds);

    if (x11priv->owner->maxfd < fd)
        x11priv->owner->maxfd = fd;
}


void X11ProcessEvent(void* arg)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    XEvent event;
    while (XPending(x11priv->dpy))
    {
        XNextEvent (x11priv->dpy, &event); //等待一个事件发生

        FcitxLock(x11priv->owner);
        /* 处理X事件 */
        if (XFilterEvent (&event, None) == False)
        {
            if (event.type == DestroyNotify)
            {
                if (event.xany.window == x11priv->compManager)
                    X11HandlerComposite(x11priv, false);
            }
            else if (event.type == ClientMessage) {
                if (event.xclient.data.l[1] == x11priv->compManagerAtom)
                {
                    if (X11GetCompositeManager(x11priv))
                        X11HandlerComposite(x11priv, true);
                }
            }


            FcitxXEventHandler* handler;
            for ( handler = (FcitxXEventHandler *) utarray_front(&x11priv->handlers);
                    handler != NULL;
                    handler = (FcitxXEventHandler *) utarray_next(&x11priv->handlers, handler))
                if (handler->eventHandler (handler->instance, &event))
                    break;
        }
        FcitxUnlock(x11priv->owner);
    }
}

void* X11GetDisplay(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    return x11priv->dpy;
}

void* X11AddEventHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    FcitxXEventHandler handler;
    handler.eventHandler = args.args[0];
    handler.instance = args.args[1];
    utarray_push_back(&x11priv->handlers, &handler);
    return NULL;
}

void* X11AddCompositeHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    FcitxCompositeChangedHandler handler;
    handler.eventHandler = args.args[0];
    handler.instance = args.args[1];
    utarray_push_back(&x11priv->comphandlers, &handler);
    return NULL;
}

void* X11RemoveEventHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    FcitxXEventHandler* handler;
    int i = 0;
    for ( i = 0 ;
            i < utarray_len(&x11priv->handlers);
            i ++)
    {
        handler = (FcitxXEventHandler*) utarray_eltptr(&x11priv->handlers, i);
        if (handler->instance == args.args[0])
            break;
    }
    utarray_erase(&x11priv->handlers, i, 1);
    return NULL;
}

void InitComposite(FcitxX11* x11stuff)
{
    char *atom_names = NULL;
    asprintf(&atom_names, "_NET_WM_CM_S%d", x11stuff->iScreen);
    x11stuff->compManagerAtom = XInternAtom (x11stuff->dpy, atom_names, False);

    free(atom_names);
    X11GetCompositeManager(x11stuff);
}

void*
X11FindARGBVisual(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11stuff = (FcitxX11*)arg;
    XVisualInfo *xvi;
    XVisualInfo temp;
    int         nvi;
    int         i;
    XRenderPictFormat   *format;
    Visual      *visual;
    Display*    dpy = x11stuff->dpy;
    int         scr = x11stuff->iScreen;

    if (x11stuff->compManager == None)
        return NULL;

    temp.screen = scr;
    temp.depth = 32;
    temp.class = TrueColor;
    xvi = XGetVisualInfo (dpy,  VisualScreenMask |VisualDepthMask |VisualClassMask,&temp,&nvi);
    if (!xvi)
        return 0;
    visual = 0;
    for (i = 0; i < nvi; i++)
    {
        format = XRenderFindVisualFormat (dpy, xvi[i].visual);
        if (format->type == PictTypeDirect && format->direct.alphaMask)
        {
            visual = xvi[i].visual;
            break;
        }
    }

    XFree (xvi);
    return visual;
}

void *
X11InitWindowAttribute(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    Visual ** vs = args.args[0];
    Colormap * cmap = args.args[1];
    XSetWindowAttributes * attrib = args.args[2];
    unsigned long *attribmask = args.args[3];
    int *depth = args.args[4];
    Display* dpy = x11priv->dpy;
    int iScreen = x11priv->iScreen;
    attrib->bit_gravity = NorthWestGravity;
    attrib->backing_store = WhenMapped;
    attrib->save_under = True;
    if (*vs) {
        *cmap =
            XCreateColormap(dpy, RootWindow(dpy, iScreen), *vs, AllocNone);

        attrib->override_redirect = True;       // False;
        attrib->background_pixel = 0;
        attrib->border_pixel = 0;
        attrib->colormap = *cmap;
        *attribmask =
            (CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWSaveUnder |
             CWColormap | CWBitGravity | CWBackingStore);
        *depth = 32;
    } else {
        *cmap = DefaultColormap(dpy, iScreen);
        *vs = DefaultVisual(dpy, iScreen);
        attrib->override_redirect = True;       // False;
        attrib->background_pixel = 0;
        attrib->border_pixel = 0;
        *attribmask = (CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWSaveUnder
                       | CWBitGravity | CWBackingStore);
        *depth = DefaultDepth(dpy, iScreen);
    }
    return NULL;
}

void* X11SetWindowProperty(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    Atom* wintype = NULL;

    Window window = *(Window*) args.args[0];
    FcitxXWindowType *type = args.args[1];
    char *windowTitle = args.args[2];

    switch (*type)
    {
    case FCITX_WINDOW_DIALOG:
        wintype = &x11priv->typeDialogAtom;
        break;
    case FCITX_WINDOW_DOCK:
        wintype = &x11priv->typeDockAtom;
        break;
    case FCITX_WINDOW_MENU:
        wintype = &x11priv->typeMenuAtom;
        break;
    default:
        wintype = NULL;
        break;
    }
    if (wintype)
        XChangeProperty (x11priv->dpy, window, x11priv->windowTypeAtom, XA_ATOM, 32, PropModeReplace, (void *) wintype, 1);

    pid_t pid = getpid();
    XChangeProperty(x11priv->dpy, window, x11priv->pidAtom, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&pid, 1);
    XChangeProperty( x11priv->dpy, window, XA_WM_CLASS, XA_STRING, 8, PropModeReplace, (const unsigned char*) "Fcitx", strlen("Fcitx") + 1);

    if (windowTitle)
    {
        XTextProperty   tp;
        Xutf8TextListToTextProperty(x11priv->dpy, &windowTitle, 1, XUTF8StringStyle, &tp);
        XSetWMName (x11priv->dpy, window, &tp);
        XFree(tp.value);
    }

    return NULL;
}

void* X11GetScreenSize(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    int* width = args.args[0];
    int* height = args.args[1];
    XWindowAttributes attrs;
    if (XGetWindowAttributes(x11priv->dpy, RootWindow(x11priv->dpy, x11priv->iScreen), &attrs) < 0) {
        printf("ERROR\n");
    }
    if (width != NULL)
        (*width) = attrs.width;
    if (height != NULL)
        (*height) = attrs.height;

    return NULL;
}

void*
X11MouseClick(void *arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    Window window = *(Window*) args.args[0];
    int *x = args.args[1];
    int *y = args.args[2];
    XEvent          evtGrabbed;
    boolean           *bMoved = args.args[3];

    // To motion the window
    while (1) {
        XMaskEvent(x11priv->dpy,
                   PointerMotionMask | ButtonReleaseMask | ButtonPressMask,
                   &evtGrabbed);
        if (ButtonRelease == evtGrabbed.xany.type) {
            if (Button1 == evtGrabbed.xbutton.button)
                break;
        } else if (MotionNotify == evtGrabbed.xany.type) {
            static Time     LastTime;

            if (evtGrabbed.xmotion.time - LastTime < 20)
                continue;

            XMoveWindow(x11priv->dpy, window, evtGrabbed.xmotion.x_root - *x,
                        evtGrabbed.xmotion.y_root - *y);
            XRaiseWindow(x11priv->dpy, window);

            *bMoved = true;
            LastTime = evtGrabbed.xmotion.time;
        }
    }

    *x = evtGrabbed.xmotion.x_root - *x;
    *y = evtGrabbed.xmotion.y_root - *y;

    return NULL;
}

void X11HandlerComposite(FcitxX11* x11priv, boolean enable)
{

    if (enable)
    {
        x11priv->compManager = XGetSelectionOwner(x11priv->dpy, x11priv->compManagerAtom);

        if (x11priv->compManager)
        {
            XSetWindowAttributes attrs;
            attrs.event_mask = StructureNotifyMask;
            XChangeWindowAttributes (x11priv->dpy, x11priv->compManager, CWEventMask, &attrs);
        }
    }
    else {
        x11priv->compManager = None;
    }

    FcitxCompositeChangedHandler* handler;
    for ( handler = (FcitxCompositeChangedHandler *) utarray_front(&x11priv->comphandlers);
            handler != NULL;
            handler = (FcitxCompositeChangedHandler *) utarray_next(&x11priv->comphandlers, handler))
        handler->eventHandler (handler->instance, enable);
}

boolean X11GetCompositeManager(FcitxX11* x11stuff)
{
    x11stuff->compManager = XGetSelectionOwner(x11stuff->dpy, x11stuff->compManagerAtom);

    if (x11stuff->compManager)
    {
        XSetWindowAttributes attrs;
        attrs.event_mask = StructureNotifyMask;
        XChangeWindowAttributes (x11stuff->dpy, x11stuff->compManager, CWEventMask, &attrs);
        return true;
    }
    else
        return false;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0; 
