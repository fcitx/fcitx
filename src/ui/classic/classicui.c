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

#include "classicui.h"
#include "classicuiinterface.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx/instance.h"
#include "fcitx/frontend.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#include "MenuWindow.h"
#include "MessageWindow.h"
#include "fcitx/hook.h"
#include "fcitx-utils/utils.h"
#include "module/notificationitem/fcitx-notificationitem.h"

struct _FcitxSkin;
static boolean MainMenuAction(FcitxUIMenu* menu, int index);
static void UpdateMainMenu(FcitxUIMenu* menu);

static void* ClassicUICreate(FcitxInstance* instance);
static void ClassicUICloseInputWindow(void* arg);
static void ClassicUIShowInputWindow(void* arg);
static void ClassicUIMoveInputWindow(void* arg);
static void ClassicUIRegisterMenu(void *arg, FcitxUIMenu* menu);
static void ClassicUIUpdateStatus(void *arg, FcitxUIStatus* status);
static void ClassicUIRegisterStatus(void *arg, FcitxUIStatus* status);
static void ClassicUIUpdateComplexStatus(void *arg, FcitxUIComplexStatus* status);
static void ClassicUIRegisterComplexStatus(void *arg, FcitxUIComplexStatus* status);
static void ClassicUIOnInputFocus(void *arg);
static void ClassicUIOnInputUnFocus(void *arg);
static void ClassicUIOnTriggerOn(void *arg);
static void ClassicUIOnTriggerOff(void *arg);
static void ClassicUIDisplayMessage(void *arg, char *title, char **msg, int length);
static void ClassicUIInputReset(void *arg);
static void ReloadConfigClassicUI(void *arg);
static void ClassicUISuspend(void *arg);
static void ClassicUIResume(void *arg);
static void ClassicUIDelayedInitTray(void* arg);
static void ClassicUIDelayedShowTray(void* arg);
static void ClassicUINotificationItemAvailable(void* arg, boolean avaiable);

static FcitxConfigFileDesc* GetClassicUIDesc();
static void ClassicUIMainWindowSizeHint(void *arg, int* x, int* y,
                                        int* w, int* h);

DECLARE_ADDFUNCTIONS(ClassicUI)

FCITX_DEFINE_PLUGIN(fcitx_classic_ui, ui, FcitxUI) = {
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
    ClassicUIOnTriggerOff,
    ClassicUIDisplayMessage,
    ClassicUIMainWindowSizeHint,
    ReloadConfigClassicUI,
    ClassicUISuspend,
    ClassicUIResume,
    NULL,
    ClassicUIRegisterComplexStatus,
    ClassicUIUpdateComplexStatus,
    NULL
};

void* ClassicUICreate(FcitxInstance* instance)
{
    FcitxAddon *classicuiaddon = Fcitx_ClassicUI_GetAddon(instance);
    FcitxClassicUI *classicui = fcitx_utils_new(FcitxClassicUI);
    classicui->owner = instance;
    if (!LoadClassicUIConfig(classicui)) {
        free(classicui);
        return NULL;
    }
    if (GetSkinDesc() == NULL) {
        free(classicui);
        return NULL;
    }
    classicui->dpy = FcitxX11GetDisplay(instance);
    if (classicui->dpy == NULL) {
        free(classicui);
        return NULL;
    }

    FcitxX11GetDPI(instance, &classicui->dpi, NULL);
    if (classicui->dpi <= 0)
        classicui->dpi = 96;

    if (LoadSkinConfig(&classicui->skin, &classicui->skinType)) {
        free(classicui);
        return NULL;
    }

    classicui->isfallback = FcitxUIIsFallback(instance, classicuiaddon);

    classicui->iScreen = DefaultScreen(classicui->dpy);

    classicui->protocolAtom = XInternAtom(classicui->dpy, "WM_PROTOCOLS", False);
    classicui->killAtom = XInternAtom(classicui->dpy, "WM_DELETE_WINDOW", False);


    InitSkinMenu(classicui);
    FcitxUIRegisterMenu(instance, &classicui->skinMenu);
    /* Main Menu Initial */
    FcitxMenuInit(&classicui->mainMenu);
    classicui->mainMenu.UpdateMenu = UpdateMainMenu;
    classicui->mainMenu.MenuAction = MainMenuAction;
    classicui->mainMenu.priv = classicui;
    classicui->mainMenu.mark = -1;

    classicui->inputWindow = CreateInputWindow(classicui);
    classicui->mainWindow = CreateMainWindow(classicui);
    classicui->trayWindow = CreateTrayWindow(classicui);
    classicui->messageWindow = CreateMessageWindow(classicui);
    classicui->mainMenuWindow = CreateMainMenuWindow(classicui);

    FcitxIMEventHook resethk;
    resethk.arg = classicui;
    resethk.func = ClassicUIInputReset;
    FcitxInstanceRegisterResetInputHook(instance, resethk);

    DisplaySkin(classicui, classicui->skinType);

    FcitxClassicUIAddFunctions(instance);

    FcitxInstanceAddTimeout(instance, 0, ClassicUIDelayedInitTray, classicui);

    return classicui;
}

void ClassicUIDelayedInitTray(void* arg) {
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    // FcitxLog(INFO, "yeah we delayed!");
    if (!classicui->bUseTrayIcon)
        return;
    /*
     * if this return false, something wrong happened and callback
     * will never be called, show tray directly
     */
    if (FcitxNotificationItemEnable(classicui->owner, ClassicUINotificationItemAvailable, classicui)) {
        if (!classicui->trayTimeout)
            classicui->trayTimeout = FcitxInstanceAddTimeout(classicui->owner, 100, ClassicUIDelayedShowTray, classicui);
    } else {
        ReleaseTrayWindow(classicui->trayWindow);
        InitTrayWindow(classicui->trayWindow);
    }
}

void ClassicUIDelayedShowTray(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    classicui->trayTimeout = 0;
    if (!classicui->bUseTrayIcon)
        return;
    ReleaseTrayWindow(classicui->trayWindow);
    InitTrayWindow(classicui->trayWindow);
}

void ClassicUISetWindowProperty(FcitxClassicUI* classicui, Window window, FcitxXWindowType type, char *windowTitle)
{
    FcitxX11SetWindowProp(classicui->owner, &window, &type, windowTitle);
}

static void ClassicUIInputReset(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    if (classicui->isSuspend)
        return;
    DrawMainWindow(classicui->mainWindow);
    DrawTrayWindow(classicui->trayWindow);
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
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    DrawMainWindow(classicui->mainWindow);
}

void ClassicUIUpdateComplexStatus(void* arg, FcitxUIComplexStatus* status)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    DrawMainWindow(classicui->mainWindow);
}

void ClassicUIRegisterComplexStatus(void* arg, FcitxUIComplexStatus* status)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    status->uipriv[classicui->isfallback] = fcitx_utils_malloc0(sizeof(FcitxClassicUIStatus));
}


static void ClassicUIRegisterMenu(void *arg, FcitxUIMenu* menu)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    XlibMenu* xlibMenu = CreateXlibMenu(classicui);
    menu->uipriv[classicui->isfallback] = xlibMenu;
    xlibMenu->menushell = menu;
}

static void ClassicUIRegisterStatus(void *arg, FcitxUIStatus* status)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    FcitxSkin* sc = &classicui->skin;
    status->uipriv[classicui->isfallback] = fcitx_utils_new(FcitxClassicUIStatus);
    char *name;

    fcitx_utils_alloc_cat_str(name, status->name, "_active.png");
    LoadImage(sc, name, false);
    free(name);

    fcitx_utils_alloc_cat_str(name, status->name, "_inactive.png");
    LoadImage(sc, name, false);
    free(name);
}

static void ClassicUIOnInputFocus(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    if (classicui->isSuspend)
        return;
    DrawMainWindow(classicui->mainWindow);
    DrawTrayWindow(classicui->trayWindow);
}

static void ClassicUIOnInputUnFocus(void *arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    if (classicui->isSuspend)
        return;
    DrawMainWindow(classicui->mainWindow);
    DrawTrayWindow(classicui->trayWindow);
}

void ClassicUISuspend(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    classicui->isSuspend = true;
    classicui->notificationItemAvailable = false;
    CloseInputWindowInternal(classicui->inputWindow);
    CloseMainWindow(classicui->mainWindow);
    ReleaseTrayWindow(classicui->trayWindow);
    /* always call this function will not do anything harm */
    FcitxNotificationItemDisable(classicui->owner);
}

void ClassicUIResume(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    classicui->isSuspend = false;
    ClassicUIDelayedInitTray(classicui);
}

void ClassicUINotificationItemAvailable(void* arg, boolean avaiable) {
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    /* ClassicUISuspend has already done all clean up */
    if (classicui->isSuspend)
        return;
    classicui->notificationItemAvailable = avaiable;
    if (!avaiable) {
        ReleaseTrayWindow(classicui->trayWindow);
        InitTrayWindow(classicui->trayWindow);
    } else {
        if (classicui->trayTimeout) {
            FcitxInstanceRemoveTimeoutById(classicui->owner, classicui->trayTimeout);
        }
        ReleaseTrayWindow(classicui->trayWindow);
    }
}

void ActivateWindow(Display *dpy, int iScreen, Window window)
{
    XEvent ev;

    memset(&ev, 0, sizeof(ev));

    static Atom _NET_ACTIVE_WINDOW;
    if (_NET_ACTIVE_WINDOW == None)
        _NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);

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
    FcitxX11GetScreenSize(classicui->owner, width, height);
}

FcitxRect GetScreenGeometry(FcitxClassicUI* classicui, int x, int y)
{
    FcitxRect result = { 0, 0 , 0 , 0 };
    FcitxX11GetScreenGeometry(classicui->owner, &x, &y, &result);
    return result;
}

CONFIG_DESC_DEFINE(GetClassicUIDesc, "fcitx-classic-ui.desc")

boolean LoadClassicUIConfig(FcitxClassicUI* classicui)
{
    FcitxConfigFileDesc* configDesc = GetClassicUIDesc();
    if (configDesc == NULL)
        return false;
    FILE *fp;
    char *file;
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-classic-ui.config", "r", &file);
    FcitxLog(DEBUG, "Load Config File %s", file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
            SaveClassicUIConfig(classicui);
    }

    FcitxConfigFile *cfile = FcitxConfigParseConfigFileFp(fp, configDesc);

    FcitxClassicUIConfigBind(classicui, cfile, configDesc);
    FcitxConfigBindSync(&classicui->gconfig);

    if (fp)
        fclose(fp);
    return true;
}

void SaveClassicUIConfig(FcitxClassicUI *classicui)
{
    FcitxConfigFileDesc* configDesc = GetClassicUIDesc();
    char *file;
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-classic-ui.config", "w", &file);
    FcitxLog(DEBUG, "Save Config to %s", file);
    FcitxConfigSaveConfigFileFp(fp, &classicui->gconfig, configDesc);
    free(file);
    if (fp)
        fclose(fp);
}

boolean IsInRspArea(int x0, int y0, FcitxClassicUIStatus* status)
{
    return FcitxUIIsInBox(x0, y0, status->x, status->y, status->w, status->h);
}

boolean
ClassicUIMouseClick(FcitxClassicUI* classicui, Window window, int *x, int *y)
{
    boolean bMoved = false;
    FcitxX11MouseClick(classicui->owner, &window, x, y, &bMoved);
    return bMoved;
}

void ClassicUIOnTriggerOn(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    FcitxInstance *instance = classicui->owner;
    if (FcitxInstanceGetCurrentStatev2(instance) == IS_ACTIVE) {
        DrawMainWindow(classicui->mainWindow);
    }
    DrawTrayWindow(classicui->trayWindow);
}

void ClassicUIOnTriggerOff(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    DrawMainWindow(classicui->mainWindow);
    DrawTrayWindow(classicui->trayWindow);
}

void ClassicUIDisplayMessage(void* arg, char* title, char** msg, int length)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    XMapRaised(classicui->dpy, classicui->messageWindow->window);
    DrawMessageWindow(classicui->messageWindow, title, msg, length);
}


static void UpdateMainMenu(FcitxUIMenu* menu)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    FcitxInstance* instance = classicui->owner;
    FcitxMenuClear(menu);

    FcitxMenuAddMenuItem(menu, _("Online Help"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(menu, NULL, MENUTYPE_DIVLINE, NULL);
    boolean flag = false;

    FcitxUIStatus* status;
    UT_array* uistats = FcitxInstanceGetUIStats(instance);
    for (status = (FcitxUIStatus*) utarray_front(uistats);
            status != NULL;
            status = (FcitxUIStatus*) utarray_next(uistats, status)
        ) {
        FcitxClassicUIStatus* privstat =  GetPrivateStatus(status);
        if (privstat == NULL || !status->visible)
            continue;

        flag = true;
        FcitxMenuAddMenuItemWithData(menu, status->shortDescription, MENUTYPE_SIMPLE, NULL, strdup(status->name));
    }

    FcitxUIComplexStatus* compstatus;
    UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
    for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
            compstatus != NULL;
            compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
        ) {
        FcitxClassicUIStatus* privstat =  GetPrivateStatus(compstatus);
        if (privstat == NULL || !compstatus->visible)
            continue;
        if (FcitxUIGetMenuByStatusName(instance, compstatus->name))
            continue;

        flag = true;
        FcitxMenuAddMenuItemWithData(menu, compstatus->shortDescription, MENUTYPE_SIMPLE, NULL, strdup(compstatus->name));
    }

    if (flag)
        FcitxMenuAddMenuItem(menu, NULL, MENUTYPE_DIVLINE, NULL);

    FcitxUIMenu **menupp;
    UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
    for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp)
        ) {
        FcitxUIMenu * menup = *menupp;
        if (menup->isSubMenu)
            continue;

        if (!menup->visible)
            continue;

        if (menup->candStatusBind) {
            FcitxUIComplexStatus* compStatus = FcitxUIGetComplexStatusByName(instance, menup->candStatusBind);
            if (compStatus) {
                if (!compStatus->visible)
                    continue;
            }
        }

        FcitxMenuAddMenuItem(menu, menup->name, MENUTYPE_SUBMENU, menup);
    }
    FcitxMenuAddMenuItem(menu, NULL, MENUTYPE_DIVLINE, NULL);
    FcitxMenuAddMenuItem(menu, _("Configure Current Input Method"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(menu, _("Configure"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(menu, _("Restart"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(menu, _("Exit"), MENUTYPE_SIMPLE, NULL);
}

boolean MainMenuAction(FcitxUIMenu* menu, int index)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    FcitxInstance* instance = classicui->owner;
    int length = utarray_len(&menu->shell);
    if (index == 0) {
        char* args[] = {
            "xdg-open",
            "http://fcitx-im.org/",
            0
        };
        fcitx_utils_start_process(args);
    } else if (index == length - 1) { /* Exit */
        FcitxInstanceEnd(classicui->owner);
    } else if (index == length - 2) { /* Restart */
        fcitx_utils_launch_restart();
    } else if (index == length - 3) { /* Configuration */
        fcitx_utils_launch_configure_tool();
    } else if (index == length - 4) { /* Configuration */
        FcitxIM* im = FcitxInstanceGetCurrentIM(classicui->owner);
        if (im && im->owner) {
            fcitx_utils_launch_configure_tool_for_addon(im->uniqueName);
        }
        else {
            fcitx_utils_launch_configure_tool();
        }
    } else {
        FcitxMenuItem* item = (FcitxMenuItem*) utarray_eltptr(&menu->shell, index);
        if (item && item->type == MENUTYPE_SIMPLE && item->data) {
            const char* name = item->data;
            FcitxUIUpdateStatus(instance, name);
        }
    }
    return true;
}

void
ClassicUIInitWindowAttribute(FcitxClassicUI *classicui, Visual **vs,
                             Colormap *cmap, XSetWindowAttributes *attrib,
                             unsigned long *attribmask, int *depth)
{
    FcitxX11InitWindowAttribute(classicui->owner, vs, cmap, attrib,
                                attribmask, depth);
}

Visual * ClassicUIFindARGBVisual(FcitxClassicUI* classicui)
{
    return FcitxX11FindARGBVisual(classicui->owner);
}

void ClassicUIMainWindowSizeHint(void* arg, int* x, int* y, int* w, int* h)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    if (x) {
        *x = classicui->iMainWindowOffsetX;
    }
    if (y) {
        *y = classicui->iMainWindowOffsetY;
    }

    XWindowAttributes attr;
    XGetWindowAttributes(classicui->dpy, classicui->mainWindow->window, &attr);
    if (w) {
        *w = attr.width;
    }
    if (h) {
        *h = attr.height;
    }

}

void ReloadConfigClassicUI(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    LoadClassicUIConfig(classicui);
    DisplaySkin(classicui, classicui->skinType);
}

boolean WindowIsVisable(Display* dpy, Window window)
{
    XWindowAttributes attr;
    XGetWindowAttributes(dpy, window, &attr);
    return attr.map_state == IsViewable;
}

boolean EnlargeCairoSurface(cairo_surface_t** sur, int w, int h)
{
    int ow = cairo_image_surface_get_width(*sur);
    int oh = cairo_image_surface_get_height(*sur);

    if (ow >= w && oh >= h)
        return false;

    while (ow < w) {
        ow *= 2;
    }

    while (oh < h) {
        oh *= 2;
    }

    cairo_surface_destroy(*sur);
    *sur = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ow, oh);
    return true;
}

void ResizeSurface(cairo_surface_t** surface, int w, int h)
{
    int ow = cairo_image_surface_get_width(*surface);
    int oh = cairo_image_surface_get_height(*surface);

    if ((ow == w && oh == h) || w == 0 || h == 0 || ow == 0 || oh == 0)
        return;

    double scalex = (double)w / ow;
    double scaley = (double)h / oh;
    double scale = (scalex > scaley) ? scaley : scalex;

    int nw = ow * scale;
    int nh = oh * scale;

    cairo_surface_t* newsurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t* c = cairo_create(newsurface);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(c ,1, 1, 1, 0.0);
    cairo_paint(c);
    cairo_translate(c, (w - nw) / 2.0 , (h - nh) / 2.0);
    cairo_scale(c, scale, scale);
    cairo_set_source_surface(c, *surface, 0, 0);
    cairo_rectangle(c, 0, 0, ow, oh);
    cairo_clip(c);
    cairo_paint(c);
    cairo_destroy(c);

    cairo_surface_destroy(*surface);

    *surface = newsurface;
}

#include "fcitx-classic-ui-addfunctions.h"
