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
#include "module/x11/x11stuff.h"

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
#include "AboutWindow.h"
#include "MessageWindow.h"
#include "fcitx/hook.h"
#include <fcitx-utils/utils.h>

struct _FcitxSkin;
boolean MainMenuAction(FcitxUIMenu* menu, int index);

static void* ClassicUICreate(FcitxInstance* instance);
static void ClassicUICloseInputWindow(void* arg);
static void ClassicUIShowInputWindow(void* arg);
static void ClassicUIMoveInputWindow(void* arg);
static void ClassicUIRegisterMenu(void *arg, FcitxUIMenu* menu);
static void ClassicUIUpdateStatus(void *arg, FcitxUIStatus* status);
static void ClassicUIRegisterStatus(void *arg, FcitxUIStatus* status);
static void ClassicUIOnInputFocus(void *arg);
static void ClassicUIOnInputUnFocus(void *arg);
static void ClassicUIOnTriggerOn(void *arg);
static void ClassicUIOnTriggerOff(void *arg);
static void ClassicUIDisplayMessage(void *arg, char *title, char **msg, int length);
static void ClassicUIInputReset(void *arg);
static void ReloadConfigClassicUI(void *arg);
static void ClassicUISuspend(void *arg);
static void ClassicUIResume(void *arg);

static FcitxConfigFileDesc* GetClassicUIDesc();
static void ClassicUIMainWindowSizeHint(void *arg, int* x, int* y, int* w, int* h);

static void* ClassicUILoadImage(void *arg, FcitxModuleFunctionArg args);
static void* ClassicUIGetKeyBoardFontColor(void* arg, FcitxModuleFunctionArg args);
static void* ClassicUIGetFont(void *arg, FcitxModuleFunctionArg args);

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
    ClassicUIOnTriggerOff,
    ClassicUIDisplayMessage,
    ClassicUIMainWindowSizeHint,
    ReloadConfigClassicUI,
    ClassicUISuspend,
    ClassicUIResume,
    NULL,
    NULL,
    NULL,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* ClassicUICreate(FcitxInstance* instance)
{
    FcitxAddon* classicuiaddon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_CLASSIC_UI_NAME);
    FcitxModuleFunctionArg arg;
    FcitxClassicUI* classicui = fcitx_utils_malloc0(sizeof(FcitxClassicUI));
    classicui->owner = instance;
    if (!LoadClassicUIConfig(classicui)) {
        free(classicui);
        return NULL;
    }
    if (GetSkinDesc() == NULL) {
        free(classicui);
        return NULL;
    }
    classicui->dpy = InvokeFunction(instance, FCITX_X11, GETDISPLAY, arg);
    if (classicui->dpy == NULL) {
        free(classicui);
        return NULL;
    }

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
    FcitxMenuAddMenuItem(&classicui->mainMenu, _("About Fcitx"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(&classicui->mainMenu, _("Online Help"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(&classicui->mainMenu, NULL, MENUTYPE_DIVLINE, NULL);

    FcitxUIMenu **menupp;
    UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
    for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp)
        ) {
        FcitxUIMenu * menup = *menupp;
        if (!menup->isSubMenu)
            FcitxMenuAddMenuItem(&classicui->mainMenu, menup->name, MENUTYPE_SUBMENU, menup);
    }
    FcitxMenuAddMenuItem(&classicui->mainMenu, NULL, MENUTYPE_DIVLINE, NULL);
    FcitxMenuAddMenuItem(&classicui->mainMenu, _("Configure"), MENUTYPE_SIMPLE, NULL);
    FcitxMenuAddMenuItem(&classicui->mainMenu, _("Exit"), MENUTYPE_SIMPLE, NULL);
    classicui->mainMenu.MenuAction = MainMenuAction;
    classicui->mainMenu.priv = classicui;
    classicui->mainMenu.mark = -1;


    classicui->inputWindow = CreateInputWindow(classicui);
    classicui->mainWindow = CreateMainWindow(classicui);
    classicui->trayWindow = CreateTrayWindow(classicui);
    classicui->aboutWindow = CreateAboutWindow(classicui);
    classicui->messageWindow = CreateMessageWindow(classicui);
    classicui->mainMenuWindow = CreateMainMenuWindow(classicui);

    FcitxIMEventHook resethk;
    resethk.arg = classicui;
    resethk.func = ClassicUIInputReset;
    FcitxInstanceRegisterResetInputHook(instance, resethk);

    DisplaySkin(classicui, classicui->skinType);

    /* ensure order ! */
    AddFunction(classicuiaddon, ClassicUILoadImage);
    AddFunction(classicuiaddon, ClassicUIGetKeyBoardFontColor);
    AddFunction(classicuiaddon, ClassicUIGetFont);

    return classicui;
}

void ClassicUISetWindowProperty(FcitxClassicUI* classicui, Window window, FcitxXWindowType type, char *windowTitle)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = &window;
    arg.args[1] = &type;
    arg.args[2] = windowTitle;
    InvokeFunction(classicui->owner, FCITX_X11, SETWINDOWPROP, arg);
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
    status->uipriv[classicui->isfallback] = fcitx_utils_malloc0(sizeof(FcitxClassicUIStatus));
    char* activename, *inactivename;
    asprintf(&activename, "%s_active.png", status->name);
    asprintf(&inactivename, "%s_inactive.png", status->name);

    LoadImage(sc, activename, false);
    LoadImage(sc, inactivename, false);

    free(activename);
    free(inactivename);
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
    CloseInputWindowInternal(classicui->inputWindow);
    CloseMainWindow(classicui->mainWindow);
    ReleaseTrayWindow(classicui->trayWindow);
}

void ClassicUIResume(void* arg)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    classicui->isSuspend = false;
    InitTrayWindow(classicui->trayWindow);
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
    FcitxModuleFunctionArg arg;
    arg.args[0] = width;
    arg.args[1] = height;
    InvokeFunction(classicui->owner, FCITX_X11, GETSCREENSIZE, arg);
}

FcitxRect GetScreenGeometry(FcitxClassicUI* classicui, int x, int y)
{
    FcitxModuleFunctionArg arg;
    FcitxRect result = { 0, 0 , 0 , 0 };
    arg.args[0] = &x;
    arg.args[1] = &y;
    arg.args[2] = &result;
    InvokeFunction(classicui->owner, FCITX_X11, GETSCREENGEOMETRY, arg);

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
    fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-classic-ui.config", "rt", &file);
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
    FILE *fp = FcitxXDGGetFileUserWithPrefix("conf", "fcitx-classic-ui.config", "wt", &file);
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
    boolean            bMoved = false;
    FcitxModuleFunctionArg arg;
    arg.args[0] = &window;
    arg.args[1] = x;
    arg.args[2] = y;
    arg.args[3] = &bMoved;
    InvokeFunction(classicui->owner, FCITX_X11, MOUSECLICK, arg);

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

boolean MainMenuAction(FcitxUIMenu* menu, int index)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    int length = utarray_len(&menu->shell);
    if (index == 0) {
        DisplayAboutWindow(classicui->mainWindow->owner->aboutWindow);
    } else if (index == 1) {
        FILE* p = popen("xdg-open http://fcitx.github.com/handbook/ &", "r");
        if (p)
            pclose(p);
        else
            FcitxLog(ERROR, _("Unable to create process"));
    } else if (index == length - 1) { /* Exit */
        FcitxInstanceEnd(classicui->owner);
    } else if (index == length - 2) { /* Configuration */
        FILE* p = popen(BINDIR "/fcitx-configtool &", "r");
        if (p)
            pclose(p);
        else
            FcitxLog(ERROR, _("Unable to create process"));
    }
    return true;
}

void
ClassicUIInitWindowAttribute(FcitxClassicUI* classicui, Visual ** vs, Colormap * cmap,
                             XSetWindowAttributes * attrib,
                             unsigned long *attribmask, int *depth)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = vs;
    arg.args[1] = cmap;
    arg.args[2] = attrib;
    arg.args[3] = attribmask;
    arg.args[4] = depth;
    InvokeFunction(classicui->owner, FCITX_X11, INITWINDOWATTR, arg);
}

Visual * ClassicUIFindARGBVisual(FcitxClassicUI* classicui)
{
    FcitxModuleFunctionArg arg;
    return InvokeFunction(classicui->owner, FCITX_X11, FINDARGBVISUAL, arg);
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

void* ClassicUILoadImage(void *arg, FcitxModuleFunctionArg args)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    char *name = args.args[0];
    boolean *fallback = args.args[1];
    SkinImage* image = LoadImage(&classicui->skin, name, *fallback);
    if (image == NULL)
        return NULL;
    else
        return image->image;
}

void* ClassicUIGetKeyBoardFontColor(void *arg, FcitxModuleFunctionArg args)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    return &classicui->skin.skinKeyboard.keyColor;
}

void* ClassicUIGetFont(void *arg, FcitxModuleFunctionArg args)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) arg;
    return &classicui->font;
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

// kate: indent-mode cstyle; space-indent on; indent-width 0;
