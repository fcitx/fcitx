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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"

#include <limits.h>
#include <unistd.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xatom.h>

#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "fcitx/module.h"
#include "x11stuff.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include "xerrorhandler.h"
#include <fcitx-utils/log.h>

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
static void* X11ScreenGeometry(void* arg, FcitxModuleFunctionArg args);
static void InitComposite(FcitxX11* x11stuff);
static void X11HandlerComposite(FcitxX11* x11priv, boolean enable);
static boolean X11GetCompositeManager(FcitxX11* x11stuff);
static void X11InitScreen(FcitxX11* x11stuff);

static inline boolean RectIntersects(FcitxRect rt1, FcitxRect rt2);
static inline int RectWidth(FcitxRect r);
static inline int RectHeight(FcitxRect r);

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

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* X11Create(FcitxInstance* instance)
{
    FcitxX11* x11priv = fcitx_utils_malloc0(sizeof(FcitxX11));
    FcitxAddon* x11addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_X11_NAME);
    x11priv->dpy = XOpenDisplay(NULL);
    if (x11priv->dpy == NULL)
        return false;

    x11priv->owner = instance;
    x11priv->iScreen = DefaultScreen(x11priv->dpy);
    x11priv->windowTypeAtom = XInternAtom(x11priv->dpy, "_NET_WM_WINDOW_TYPE", False);
    x11priv->typeMenuAtom = XInternAtom(x11priv->dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    x11priv->typeDialogAtom = XInternAtom(x11priv->dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    x11priv->typeDockAtom = XInternAtom(x11priv->dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
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
    AddFunction(x11addon, X11ScreenGeometry);
    InitComposite(x11priv);

    X11InitScreen(x11priv);

    XWindowAttributes attr;
    XGetWindowAttributes(x11priv->dpy, DefaultRootWindow(x11priv->dpy), &attr);
    if ((attr.your_event_mask & StructureNotifyMask) != StructureNotifyMask) {
        XSelectInput(x11priv->dpy, DefaultRootWindow(x11priv->dpy), attr.your_event_mask | StructureNotifyMask);
    }

    InitXErrorHandler(x11priv);
    return x11priv;
}

void X11SetFD(void* arg)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    int fd = ConnectionNumber(x11priv->dpy);
    FD_SET(fd, FcitxInstanceGetReadFDSet(x11priv->owner));

    if (FcitxInstanceGetMaxFD(x11priv->owner) < fd)
        FcitxInstanceSetMaxFD(x11priv->owner, fd);
}


void X11ProcessEvent(void* arg)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    XEvent event;
    while (XPending(x11priv->dpy)) {
        XNextEvent(x11priv->dpy, &event);  //等待一个事件发生

        FcitxInstanceLock(x11priv->owner);
        /* 处理X事件 */
        if (XFilterEvent(&event, None) == False) {
            if (event.type == DestroyNotify) {
                if (event.xany.window == x11priv->compManager)
                    X11HandlerComposite(x11priv, false);
            } else if (event.type == ClientMessage) {
                if (event.xclient.data.l[1] == x11priv->compManagerAtom) {
                    if (X11GetCompositeManager(x11priv))
                        X11HandlerComposite(x11priv, true);
                }
            } else if (event.type == ConfigureNotify) {
                if (event.xconfigure.window == DefaultRootWindow(x11priv->dpy))
                    X11InitScreen(x11priv);
            }

            FcitxXEventHandler* handler;
            for (handler = (FcitxXEventHandler *) utarray_front(&x11priv->handlers);
                    handler != NULL;
                    handler = (FcitxXEventHandler *) utarray_next(&x11priv->handlers, handler))
                if (handler->eventHandler(handler->instance, &event))
                    break;
        }
        FcitxInstanceUnlock(x11priv->owner);
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
    for (i = 0 ;
            i < utarray_len(&x11priv->handlers);
            i ++) {
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
    x11stuff->compManagerAtom = XInternAtom(x11stuff->dpy, atom_names, False);

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
    xvi = XGetVisualInfo(dpy,  VisualScreenMask | VisualDepthMask | VisualClassMask, &temp, &nvi);
    if (!xvi)
        return 0;
    visual = 0;
    for (i = 0; i < nvi; i++) {
        format = XRenderFindVisualFormat(dpy, xvi[i].visual);
        if (format->type == PictTypeDirect && format->direct.alphaMask) {
            visual = xvi[i].visual;
            break;
        }
    }

    XFree(xvi);
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

    switch (*type) {
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
        XChangeProperty(x11priv->dpy, window, x11priv->windowTypeAtom, XA_ATOM, 32, PropModeReplace, (void *) wintype, 1);

    pid_t pid = getpid();
    XChangeProperty(x11priv->dpy, window, x11priv->pidAtom, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&pid, 1);

    char res_name[] = "fcitx";
    char res_class[] = "fcitx";
    XClassHint ch;
    ch.res_name = res_name;
    ch.res_class = res_class;
    XSetClassHint(x11priv->dpy, window, &ch);

    if (windowTitle) {
        XTextProperty   tp;
        memset(&tp, 0, sizeof(XTextProperty));
        Xutf8TextListToTextProperty(x11priv->dpy, &windowTitle, 1, XUTF8StringStyle, &tp);
        if (tp.value) {
            XSetWMName(x11priv->dpy, window, &tp);
            XFree(tp.value);
        }
    }

    return NULL;
}

void* X11GetScreenSize(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    int* width = args.args[0];
    int* height = args.args[1];

    if (width != NULL)
        (*width) = RectWidth(x11priv->rects[x11priv->defaultScreen]);
    if (height != NULL)
        (*height) = RectHeight(x11priv->rects[x11priv->defaultScreen]);

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

    if (enable) {
        x11priv->compManager = XGetSelectionOwner(x11priv->dpy, x11priv->compManagerAtom);

        if (x11priv->compManager) {
            XSetWindowAttributes attrs;
            attrs.event_mask = StructureNotifyMask;
            XChangeWindowAttributes(x11priv->dpy, x11priv->compManager, CWEventMask, &attrs);
        }
    } else {
        x11priv->compManager = None;
    }

    FcitxCompositeChangedHandler* handler;
    for (handler = (FcitxCompositeChangedHandler *) utarray_front(&x11priv->comphandlers);
            handler != NULL;
            handler = (FcitxCompositeChangedHandler *) utarray_next(&x11priv->comphandlers, handler))
        handler->eventHandler(handler->instance, enable);
}

boolean X11GetCompositeManager(FcitxX11* x11stuff)
{
    x11stuff->compManager = XGetSelectionOwner(x11stuff->dpy, x11stuff->compManagerAtom);

    if (x11stuff->compManager) {
        XSetWindowAttributes attrs;
        attrs.event_mask = StructureNotifyMask;
        XChangeWindowAttributes(x11stuff->dpy, x11stuff->compManager, CWEventMask, &attrs);
        return true;
    } else
        return false;
}

void X11InitScreen(FcitxX11* x11stuff)
{
    // get the screen count
    int newScreenCount = ScreenCount(x11stuff->dpy);
#ifdef HAVE_XINERAMA

    XineramaScreenInfo *xinerama_screeninfo = 0;

    // we ignore the Xinerama extension when using the display is
    // using traditional multi-screen (with multiple root windows)
    if (newScreenCount == 1) {
        int unused;
        x11stuff->bUseXinerama = (XineramaQueryExtension(x11stuff->dpy, &unused, &unused)
                                  && XineramaIsActive(x11stuff->dpy));
    }

    if (x11stuff->bUseXinerama) {
        xinerama_screeninfo =
            XineramaQueryScreens(x11stuff->dpy, &newScreenCount);
    }

    if (xinerama_screeninfo) {
        x11stuff->defaultScreen = 0;
    } else
#endif // HAVE_XINERAMA
    {
        x11stuff->defaultScreen = DefaultScreen(x11stuff->dpy);
        newScreenCount = ScreenCount(x11stuff->dpy);
        x11stuff->bUseXinerama = false;
    }

    if (x11stuff->rects)
        free(x11stuff->rects);
    x11stuff->rects = fcitx_utils_malloc0(sizeof(FcitxRect) * newScreenCount);

    // get the geometry of each screen
    int i, j, x, y, w, h;
    for (i = 0, j = 0; i < newScreenCount; i++, j++) {

#ifdef HAVE_XINERAMA
        if (x11stuff->bUseXinerama) {
            x = xinerama_screeninfo[i].x_org;
            y = xinerama_screeninfo[i].y_org;
            w = xinerama_screeninfo[i].width;
            h = xinerama_screeninfo[i].height;
        } else
#endif // HAVE_XINERAMA
        {
            x = 0;
            y = 0;
            w = WidthOfScreen(ScreenOfDisplay(x11stuff->dpy, i));
            h = HeightOfScreen(ScreenOfDisplay(x11stuff->dpy, i));
        }

        FcitxRect rect;
        rect.x1 = x;
        rect.y1 = y;
        rect.x2 = x + w - 1;
        rect.y2 = y + h - 1;

        x11stuff->rects[j] = rect;

        if (x11stuff->bUseXinerama && j > 0 && RectIntersects(x11stuff->rects[j - 1], x11stuff->rects[j])) {
            // merge a "cloned" screen with the previous, hiding all crtcs
            // that are currently showing a sub-rect of the previous screen
            if ((RectWidth(x11stuff->rects[j])* RectHeight(x11stuff->rects[j])) >
                    (RectWidth(x11stuff->rects[j - 1])*RectHeight(x11stuff->rects[j - 1])))
                x11stuff->rects[j - 1] = x11stuff->rects[j];
            j--;
        }
    }

    x11stuff->screenCount = j;

#ifdef HAVE_XINERAMA
    if (x11stuff->bUseXinerama && x11stuff->screenCount == 1)
        x11stuff->bUseXinerama = false;

    if (xinerama_screeninfo)
        XFree(xinerama_screeninfo);
#endif // HAVE_XINERAMA
}

inline
boolean RectIntersects(FcitxRect rt1, FcitxRect rt2)
{
    int l1 = rt1.x1;
    int r1 = rt1.x1;
    if (rt1.x2 - rt1.x1 + 1 < 0)
        l1 = rt1.x2;
    else
        r1 = rt1.x2;

    int l2 = rt2.x1;
    int r2 = rt2.x1;
    if (rt2.x2 - rt2.x1 + 1 < 0)
        l2 = rt2.x2;
    else
        r2 = rt2.x2;

    if (l1 > r2 || l2 > r1)
        return false;

    int t1 = rt1.y1;
    int b1 = rt1.y1;
    if (rt1.y2 - rt1.y1 + 1 < 0)
        t1 = rt1.y2;
    else
        b1 = rt1.y2;

    int t2 = rt2.y1;
    int b2 = rt2.y1;
    if (rt2.y2 - rt2.y1 + 1 < 0)
        t2 = rt2.y2;
    else
        b2 = rt2.y2;

    if (t1 > b2 || t2 > b1)
        return false;

    return true;
}

inline
int RectWidth(FcitxRect r)
{
    return r.x2 - r.x1 + 1;
}

inline
int RectHeight(FcitxRect r)
{
    return r.y2 - r.y1 + 1;
}

int PointToRect(int x, int y, FcitxRect r)
{
    int dx = 0;
    int dy = 0;
    if (x < r.x1)
        dx = r.x1 - x;
    else if (x > r.x2)
        dx = x - r.x2;
    if (y < r.y1)
        dy = r.y1 - y;
    else if (y > r.y2)
        dy = y - r.y2;
    return dx + dy;
}

void* X11ScreenGeometry(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11stuff = (FcitxX11*)arg;
    int x = *(int*) args.args[0];
    int y = *(int*) args.args[1];
    FcitxRect* rect = (FcitxRect*) args.args[2];

    int closestScreen = -1;
    int shortestDistance = INT_MAX;
    int i;
    for (i = 0; i < x11stuff->screenCount; ++i) {
        int thisDistance = PointToRect(x, y, x11stuff->rects[i]);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = i;
        }
    }

    if (closestScreen < 0 || closestScreen >= x11stuff->screenCount)
        closestScreen = x11stuff->defaultScreen;

    *rect = x11stuff->rects[closestScreen];

    return NULL;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
