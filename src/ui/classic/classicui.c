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

#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <cairo.h>
#include <limits.h>
#include <libintl.h>
#include <errno.h>

#include "fcitx/fcitx.h"
#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "module/x11/x11stuff.h"

#include "classicui.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/cutils.h"
#include <fcitx/instance.h>
#include <fcitx/backend.h>
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"

struct FcitxSkin;

void* ClassicUICreate(FcitxInstance* instance);
static void ClassicUICloseInputWindow(void* arg);
static void ClassicUIShowInputWindow(void* arg);
static void ClassicUIMoveInputWindow(void* arg);
static void ClassicUIRegisterMenu(void *arg, FcitxUIMenu* menu);
static void ClassicUIUpdateStatus(void *arg, FcitxUIStatus* status);
static void ClassicUIRegisterStatus(void *arg, FcitxUIStatus* status);
static void ClassicUIOnInputFocus(void *arg);
static void ClassicUIOnInputUnFocus(void *arg);
static void ClassicUIOnTriggerOn(void *arg);
static void ClassicUiOnTriggerOff(void *arg);
static ConfigFileDesc* GetClassicUIDesc();

static void LoadClassicUIConfig();
static void SaveClassicUIConfig(FcitxClassicUI* classicui);

FCITX_EXPORT_API
FcitxUI ui = {
    ClassicUICreate,
    ClassicUICloseInputWindow,
    ClassicUIShowInputWindow,
    ClassicUIMoveInputWindow,
    ClassicUIUpdateStatus,
    ClassicUIRegisterStatus,
    ClassicUIRegisterMenu,
    ClassicUIOnInputFocus,
    ClassicUIOnInputUnFocus,
    ClassicUIOnTriggerOn,
    ClassicUiOnTriggerOff
};

void* ClassicUICreate(FcitxInstance* instance)
{
    FcitxModuleFunctionArg arg;
    FcitxClassicUI* classicui = fcitx_malloc0(sizeof(FcitxClassicUI));
    classicui->owner = instance;
    classicui->dpy = InvokeFunction(instance, FCITX_X11, GETDISPLAY, arg);
    
    XLockDisplay(classicui->dpy);
    
    classicui->iScreen = DefaultScreen(classicui->dpy);
    
    classicui->protocolAtom = XInternAtom (classicui->dpy, "WM_PROTOCOLS", False);
    classicui->killAtom = XInternAtom (classicui->dpy, "WM_DELETE_WINDOW", False);
    classicui->windowTypeAtom = XInternAtom (classicui->dpy, "_NET_WM_WINDOW_TYPE", False);
    classicui->typeMenuAtom = XInternAtom (classicui->dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    classicui->typeDialogAtom = XInternAtom (classicui->dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    classicui->typeDockAtom = XInternAtom (classicui->dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    classicui->pidAtom = XInternAtom(classicui->dpy, "_NET_WM_PID", False);
    
    InitComposite(classicui);
    LoadClassicUIConfig(classicui);
    LoadSkinConfig(&classicui->skin, &classicui->skinType);
    classicui->skin.skinType = &classicui->skinType;

    classicui->inputWindow = CreateInputWindow(classicui);
    classicui->mainWindow = CreateMainWindow(classicui);
    classicui->trayWindow = CreateTrayWindow(classicui);
    
    XUnlockDisplay(classicui->dpy);
    return classicui;
}

void SetWindowProperty(FcitxClassicUI* classicui, Window window, FcitxXWindowType type, char *windowTitle)
{
    Atom* wintype = NULL;
    switch(type)
    {
        case FCITX_WINDOW_DIALOG:
            wintype = &classicui->typeDialogAtom;
            break;
        case FCITX_WINDOW_DOCK:
            wintype = &classicui->typeDockAtom;
            break;
        case FCITX_WINDOW_MENU:
            wintype = &classicui->typeMenuAtom;
            break;
        default:
            wintype = NULL;
            break;
    }
    if (wintype)
        XChangeProperty (classicui->dpy, window, classicui->windowTypeAtom, XA_ATOM, 32, PropModeReplace, (void *) wintype, 1);

    pid_t pid = getpid();
    XChangeProperty(classicui->dpy, window, classicui->pidAtom, XA_CARDINAL, 32,
            PropModeReplace, (unsigned char *)&pid, 1);
    
    if (windowTitle)
    {
        XTextProperty   tp;
        Xutf8TextListToTextProperty(classicui->dpy, &windowTitle, 1, XUTF8StringStyle, &tp);
        XSetWMName (classicui->dpy, window, &tp);
        XFree(tp.value);
    }
}

static void ClassicUICloseInputWindow(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    CloseInputWindowInternal(classicui->inputWindow);
}

static void ClassicUIShowInputWindow(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    ShowInputWindowInternal(classicui->inputWindow);
}

static void ClassicUIMoveInputWindow(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    MoveInputWindowInternal(classicui->inputWindow);
}

static void ClassicUIUpdateStatus(void *arg, FcitxUIStatus* status)
{
#ifdef UNUSED
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
#endif
}

static void ClassicUIRegisterMenu(void *arg, FcitxUIMenu* menu)
{
}

static void ClassicUIRegisterStatus(void *arg, FcitxUIStatus* status)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    FcitxSkin* sc = &classicui->skin;
    status->priv = fcitx_malloc0(sizeof(FcitxClassicUIStatus));
    char activename[PATH_MAX], inactivename[PATH_MAX];
    sprintf(activename, "%s_active.png", status->name);
    sprintf(inactivename, "%s_inactive.png", status->name);
    
    LoadImage(sc, activename, false);
    LoadImage(sc, inactivename, false);
}

static void ClassicUIOnInputFocus(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    FcitxInstance *instance = classicui->owner;
    if (GetCurrentState(instance) == IS_ACTIVE)
    {
        DrawMainWindow(classicui->mainWindow);
        ShowMainWindow(classicui->mainWindow);
    }
}

static void ClassicUIOnInputUnFocus(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    FcitxInstance *instance = classicui->owner;
    if (GetCurrentState(instance) == IS_ACTIVE)
    {
        DrawMainWindow(classicui->mainWindow);
        ShowMainWindow(classicui->mainWindow);
    }
}

int
StringWidth(const char *str, const char *font, int fontSize)
{
    if (!str || str[0] == 0)
        return 0;
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);
    SetFontContext(c, font, fontSize);

    int             width = StringWidthWithContext(c, str);
    ResetFontContext();

    cairo_destroy(c);
    cairo_surface_destroy(surface);

    return width;
}

#ifdef _ENABLE_PANGO
int
StringWidthWithContextReal(cairo_t * c, PangoFontDescription* fontDesc, const char *str)
{
    if (!str || str[0] == 0)
        return 0;
    if (!utf8_check_string(str))
        return 0;

    int             width;
    PangoLayout *layout = pango_cairo_create_layout (c);
    pango_layout_set_text (layout, str, -1);
    pango_layout_set_font_description (layout, fontDesc);
    pango_layout_get_pixel_size(layout, &width, NULL);
    g_object_unref (layout);

    return width;
}
#else

int
StringWidthWithContextReal(cairo_t * c, const char *str)
{
    if (!str || str[0] == 0)
        return 0;
    if (!utf8_check_string(str))
        return 0;
    cairo_text_extents_t extents;
    cairo_text_extents(c, str, &extents);
    int             width = extents.x_advance;
    return width;
}
#endif

int
FontHeight(const char *font, int fontSize)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);

    SetFontContext(c, font, fontSize);
    int             height = FontHeightWithContext(c);
    ResetFontContext();

    cairo_destroy(c);
    cairo_surface_destroy(surface);
    return height;
}

#ifdef _ENABLE_PANGO
int
FontHeightWithContextReal(cairo_t* c, PangoFontDescription* fontDesc)
{
    int height;

    if (pango_font_description_get_size_is_absolute(fontDesc)) /* it must be this case */
    {
        height = pango_font_description_get_size(fontDesc);
        height /= PANGO_SCALE;
    }
    else
        height = 0;

    return height;
}
#else

int
FontHeightWithContextReal(cairo_t * c)
{
    cairo_matrix_t matrix;
    cairo_get_font_matrix (c, &matrix);

    int             height = matrix.xx;
    return height;
}
#endif

/*
 * 以指定的颜色在窗口的指定位置输出字串
 */
void
OutputString(cairo_t * c, const char *str, const char *font, int fontSize, int x,
             int y, ConfigColor * color)
{
    if (!str || str[0] == 0)
        return;

    cairo_save(c);

    cairo_set_source_rgb(c, color->r, color->g, color->b);
    SetFontContext(c, font, fontSize);
    OutputStringWithContext(c, str, x, y);
    ResetFontContext();

    cairo_restore(c);
}

#ifdef _ENABLE_PANGO
void
OutputStringWithContextReal(cairo_t * c, PangoFontDescription* desc, const char *str, int x, int y)
{
    if (!str || str[0] == 0)
        return;
    if (!utf8_check_string(str))
        return;
    cairo_save(c);

    PangoLayout *layout;

    layout = pango_cairo_create_layout (c);
    pango_layout_set_text (layout, str, -1);
    pango_layout_set_font_description (layout, desc);
    cairo_move_to(c, x, y);
    pango_cairo_show_layout (c, layout);

    cairo_restore(c);
    g_object_unref (layout);
}
#else

void
OutputStringWithContextReal(cairo_t * c, const char *str, int x, int y)
{
    if (!str || str[0] == 0)
        return;
    if (!utf8_check_string(str))
        return;
    cairo_save(c);
    int             height = FontHeightWithContextReal(c);
    cairo_move_to(c, x, y + height);
    cairo_show_text(c, str);
    cairo_restore(c);
}
#endif

Bool
IsWindowVisible(Display* dpy, Window window)
{
    XWindowAttributes attrs;

    XGetWindowAttributes(dpy, window, &attrs);

    if (attrs.map_state == IsUnmapped)
        return False;

    return True;
}

void
InitWindowAttribute(Display* dpy, int iScreen, Visual ** vs, Colormap * cmap,
                    XSetWindowAttributes * attrib,
                    unsigned long *attribmask, int *depth)
{
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
}

void ActivateWindow(Display *dpy, int iScreen, Window window)
{
    XEvent ev;

    memset(&ev, 0, sizeof(ev));

    Atom _NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

    ev.xclient.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = _NET_ACTIVE_WINDOW;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = 1;
    ev.xclient.data.l[1] = CurrentTime;
    ev.xclient.data.l[2] = 0;

    XSendEvent(dpy, RootWindow(dpy, iScreen), False, SubstructureNotifyMask, &ev);
    XSync(dpy, False);
}

void GetScreenSize(FcitxClassicUI* classicui, int* width, int* height)
{
    XWindowAttributes attrs;
    if (XGetWindowAttributes(classicui->dpy, RootWindow(classicui->dpy, classicui->iScreen), &attrs) < 0) {
        printf("ERROR\n");
    }
    if (width != NULL)
        (*width) = attrs.width;
    if (height != NULL)
        (*height) = attrs.height;
}

void InitComposite(FcitxClassicUI* classicui)
{
    classicui->compManagerAtom = XInternAtom (classicui->dpy, "_NET_WM_CM_S0", False);

    classicui->compManager = XGetSelectionOwner(classicui->dpy, classicui->compManagerAtom);

    if (classicui->compManager)
    {
        XSetWindowAttributes attrs;
        attrs.event_mask = StructureNotifyMask;
        XChangeWindowAttributes (classicui->dpy, classicui->compManager, CWEventMask, &attrs);
    }
}

CONFIG_DESC_DEFINE(GetClassicUIDesc, "fcitx-classic-ui.desc")

void LoadClassicUIConfig(FcitxClassicUI* classicui)
{
    FILE *fp;
    char *file;
    fp = GetXDGFileUser( "fcitx-classic-ui.config", "rt", &file);
    FcitxLog(INFO, _("Load Config File %s"), file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
        {
            SaveClassicUIConfig(classicui);
            LoadClassicUIConfig(classicui);
        }
        return;
    }
    
    ConfigFileDesc* configDesc = GetClassicUIDesc();
    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);
    
    FcitxClassicUIConfigBind(classicui, cfile, configDesc);
    ConfigBindSync(&classicui->gconfig);

    fclose(fp);

}

void SaveClassicUIConfig(FcitxClassicUI *classicui)
{
    ConfigFileDesc* configDesc = GetClassicUIDesc();
    char *file;
    FILE *fp = GetXDGFileUser("fcitx-classic-ui.config", "wt", &file);
    FcitxLog(INFO, "Save Config to %s", file);
    SaveConfigFileFp(fp, &classicui->gconfig, configDesc);
    free(file);
    fclose(fp);
}

#ifdef _ENABLE_PANGO
PangoFontDescription* GetPangoFontDescription(const char* font, int size)
{
    PangoFontDescription* desc;
    desc = pango_font_description_new ();
    pango_font_description_set_absolute_size(desc, size * PANGO_SCALE);
    pango_font_description_set_family(desc, font);
    return desc;
}
#endif

Visual * FindARGBVisual (FcitxClassicUI* classicui)
{
    XVisualInfo *xvi;
    XVisualInfo temp;
    int         nvi;
    int         i;
    XRenderPictFormat   *format;
    Visual      *visual;
    Display*    dpy = classicui->dpy;
    int         scr = classicui->iScreen;

    if (classicui->compManager == None)
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

boolean IsInRspArea(int x0, int y0, FcitxClassicUIStatus* status)
{
    return IsInBox(x0, y0, status->x, status->y, status->w, status->h);
}

/*
 * 判断鼠标点击处是否处于指定的区域内
 */
boolean
IsInBox(int x0, int y0, int x1, int y1, int w, int h)
{
    if (x0 >= x1 && x0 <= x1 + w && y0 >= y1 && y0 <= y1 + h)
        return true;

    return false;
}

Bool
MouseClick(int *x, int *y, Display* dpy, Window window)
{
    XEvent          evtGrabbed;
    Bool            bMoved = False;

    // To motion the window
    while (1) {
        XMaskEvent(dpy,
                   PointerMotionMask | ButtonReleaseMask | ButtonPressMask,
                   &evtGrabbed);
        if (ButtonRelease == evtGrabbed.xany.type) {
            if (Button1 == evtGrabbed.xbutton.button)
                break;
        } else if (MotionNotify == evtGrabbed.xany.type) {
            static Time     LastTime;

            if (evtGrabbed.xmotion.time - LastTime < 20)
                continue;

            XMoveWindow(dpy, window, evtGrabbed.xmotion.x_root - *x,
                        evtGrabbed.xmotion.y_root - *y);
            XRaiseWindow(dpy, window);

            bMoved = True;
            LastTime = evtGrabbed.xmotion.time;
        }
    }

    *x = evtGrabbed.xmotion.x_root - *x;
    *y = evtGrabbed.xmotion.y_root - *y;

    return bMoved;
}

void ClassicUIOnTriggerOn(void* arg)
{

    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    FcitxInstance *instance = classicui->owner;
    if (GetCurrentState(instance) == IS_ACTIVE)
    {
        DrawMainWindow(classicui->mainWindow);
        ShowMainWindow(classicui->mainWindow);
    }
}

void ClassicUiOnTriggerOff(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    CloseMainWindow(classicui->mainWindow);
}
