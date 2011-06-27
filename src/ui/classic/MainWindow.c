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
/**
 * @file   MainWindow.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  主窗口
 *
 *
 */

#include <cairo-xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <X11/Xatom.h>

#include "MainWindow.h"
#include "fcitx-utils/cutils.h"
#include "module/x11/x11stuff.h"
#include "classicui.h"
#include "skin.h"
#include <fcitx/backend.h>
#include "fcitx/module.h"
#include <fcitx/instance.h>
#include <fcitx/ime-internal.h>
#include <libintl.h>
#include "AboutWindow.h"
#include "MenuWindow.h"

static boolean MainWindowEventHandler(void *instance, XEvent* event);
static void UpdateStatusGeometry(FcitxClassicUIStatus *privstat, SkinImage *image, int x, int y);

#define FCITX_MAX(a,b) ((a) > (b)?(a) : (b))

#define MAIN_BAR_MAX_WIDTH 400
#define MAIN_BAR_MAX_HEIGHT 400

MainWindow* CreateMainWindow (FcitxClassicUI* classicui)
{
    MainWindow *mainWindow;
    int depth;
    Colormap cmap;
    Visual * vs;
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    GC gc;
    char        strWindowName[] = "Fcitx Main Window";
    int swidth, sheight;
    XGCValues xgv;
    Display* dpy = classicui->dpy;
    int iScreen = classicui->iScreen;
    FcitxSkin *sc = &classicui->skin;
    
    mainWindow = fcitx_malloc0(sizeof(MainWindow));
    mainWindow->owner = classicui;
    mainWindow->dpy = dpy;
    
    GetScreenSize(classicui, &swidth, &sheight);
    SkinImage *back = LoadImage(sc, sc->skinMainBar.backImg, false);

    vs = ClassicUIFindARGBVisual(classicui);

    if (classicui->iMainWindowOffsetX + cairo_image_surface_get_width(back->image) > swidth )
        classicui->iMainWindowOffsetX = swidth - cairo_image_surface_get_width(back->image);
    
    if (classicui->iMainWindowOffsetY + cairo_image_surface_get_height(back->image) > sheight )
        classicui->iMainWindowOffsetY = sheight - cairo_image_surface_get_height(back->image);

    ClassicUIInitWindowAttribute(classicui, &vs, &cmap, &attrib, &attribmask, &depth);
    mainWindow->window=XCreateWindow (dpy,
                                     RootWindow(dpy, iScreen),
                                     classicui->iMainWindowOffsetX,
                                     classicui->iMainWindowOffsetY,
                                     cairo_image_surface_get_width(back->image),
                                     cairo_image_surface_get_height(back->image),
                                     0, depth,InputOutput, vs,attribmask, &attrib);

    if (mainWindow->window == None)
        return NULL;

    xgv.foreground = WhitePixel(dpy, iScreen);
    mainWindow->pm_main_bar = XCreatePixmap(
                                 dpy,
                                 mainWindow->window,
                                 MAIN_BAR_MAX_WIDTH,
                                 MAIN_BAR_MAX_HEIGHT,
                                 depth);
    gc = XCreateGC(dpy,mainWindow->pm_main_bar, GCForeground, &xgv);
    XFillRectangle(dpy, mainWindow->pm_main_bar, gc, 0, 0,cairo_image_surface_get_width(back->image), cairo_image_surface_get_height(back->image));
    mainWindow->cs_main_bar=cairo_xlib_surface_create(
                               dpy,
                               mainWindow->pm_main_bar,
                               vs,
                               MAIN_BAR_MAX_WIDTH,
                               MAIN_BAR_MAX_HEIGHT);
    XFreeGC(dpy,gc);

    mainWindow->main_win_gc = XCreateGC( dpy, mainWindow->window, 0, NULL );
    XChangeWindowAttributes (dpy, mainWindow->window, attribmask, &attrib);
    XSelectInput (dpy, mainWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask);

    ClassicUISetWindowProperty(classicui, mainWindow-> window, FCITX_WINDOW_DOCK, strWindowName);

    FcitxModuleFunctionArg arg;
    arg.args[0] = MainWindowEventHandler;
    arg.args[1] = mainWindow;
    InvokeFunction(classicui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);
    return mainWindow;
}

void DisplayMainWindow (Display* dpy, MainWindow* mainWindow)
{
    FcitxLog(DEBUG, _("DISPLAY MainWindow"));

    if (!mainWindow->bMainWindowHidden)
        XMapRaised (dpy, mainWindow->window);
}

void DrawMainWindow (MainWindow* mainWindow)
{
    FcitxSkin *sc = &mainWindow->owner->skin;
    FcitxInstance *instance = mainWindow->owner->owner;
    cairo_t *c;

    if ( mainWindow->bMainWindowHidden )
        return;

    FcitxLog(DEBUG, _("DRAW MainWindow"));

    c=cairo_create(mainWindow->cs_main_bar);
    //把背景清空
    cairo_save(c);
    cairo_set_source_rgba(c, 0, 0, 0,0);
    cairo_rectangle (c, 0, 0, SIZEX, SIZEY);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_fill(c);
    cairo_restore(c);

    cairo_set_operator(c, CAIRO_OPERATOR_OVER);

    if (mainWindow->owner->hideMainWindow == HM_SHOW || (mainWindow->owner->hideMainWindow == HM_AUTO && (GetCurrentState(mainWindow->owner->owner) != IS_CLOSED)))
    {
        /* Only logo and input status is hard-code, other should be status */
        // extern mouse_e ms_logo,ms_punc,ms_corner,ms_lx,ms_chs,ms_lock,ms_vk,ms_py;
        int currentX = sc->skinMainBar.marginLeft;
        int height = 0;
        SkinImage* back = LoadImage(sc, sc->skinMainBar.backImg, false);
        SkinImage* logo = LoadImage(sc, sc->skinMainBar.logo, false);
        SkinImage* imicon;
        if (logo)
        {
            currentX += cairo_image_surface_get_width(logo->image);
            height = MAX(height, cairo_image_surface_get_height(logo->image));
        }
        
        if (GetCurrentState(instance) != IS_ACTIVE )
            imicon = LoadImage(sc, sc->skinMainBar.eng, false);
        else 
        {
            FcitxIM* im = GetCurrentIM(instance);
            char path[PATH_MAX];
            snprintf(path, PATH_MAX, "%s.png", im->strIconName );
            imicon = LoadImage(sc, path, false);
            if (imicon == NULL)
                imicon = LoadImage(sc, sc->skinMainBar.active, false);
        }
        currentX += cairo_image_surface_get_width(imicon->image);
        height = MAX(height, cairo_image_surface_get_height(imicon->image));
        
        FcitxUIStatus* status;
        for(status = (FcitxUIStatus*) utarray_front(&instance->uistats);
            status != NULL;
            status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
        )
        {
            boolean active = status->getCurrentStatus(status->arg);
            char path[PATH_MAX];
            if (active)
                snprintf(path, PATH_MAX, "%s_active.png", status->name );
            else
                snprintf(path, PATH_MAX, "%s_inactive.png", status->name );
            SkinImage* statusicon = LoadImage(sc, path, false);
            if (statusicon == NULL)
                continue;
            currentX += cairo_image_surface_get_width(statusicon->image);
            height = MAX(height, cairo_image_surface_get_height(statusicon->image));            
        }
        
        int width = currentX + sc->skinMainBar.marginRight;
        height += sc->skinMainBar.marginTop + sc->skinMainBar.marginBottom;
        
        XResizeWindow(mainWindow->dpy, mainWindow->window, width, height);
        DrawResizableBackground(c, back->image, height, width, sc->skinMainBar.marginLeft, sc->skinMainBar.marginTop, sc->skinMainBar.marginRight, sc->skinMainBar.marginBottom);
        
        currentX = sc->skinMainBar.marginLeft;
        if (logo)
        {
            DrawImage(c, logo->image, currentX, sc->skinMainBar.marginTop, mainWindow->logostat.mouse);
            UpdateStatusGeometry( &mainWindow->logostat, logo, currentX, sc->skinMainBar.marginTop);
            currentX += cairo_image_surface_get_width(logo->image);
        }
        DrawImage(c, imicon->image, currentX, sc->skinMainBar.marginTop, mainWindow->imiconstat.mouse);
        UpdateStatusGeometry( &mainWindow->imiconstat, imicon, currentX, sc->skinMainBar.marginTop);
        currentX += cairo_image_surface_get_width(imicon->image);
        
        for(status = (FcitxUIStatus*) utarray_front(&instance->uistats);
            status != NULL;
            status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
        )
        {
            FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
            /* reset status */
            privstat->x = privstat->y = -1; privstat->w = privstat->h = 0;
            boolean active = status->getCurrentStatus(status->arg);
            char path[PATH_MAX];
            if (active)
                snprintf(path, PATH_MAX, "%s_active.png", status->name );
            else
                snprintf(path, PATH_MAX, "%s_inactive.png", status->name );
            SkinImage* statusicon = LoadImage(sc, path, false);
            if (statusicon == NULL)
                continue;
            DrawImage(c, statusicon->image, currentX, sc->skinMainBar.marginTop, privstat->mouse);
            UpdateStatusGeometry(privstat, statusicon, currentX, sc->skinMainBar.marginTop);
            currentX += cairo_image_surface_get_width(statusicon->image);
        }
        
        XCopyArea (mainWindow->dpy, mainWindow->pm_main_bar, mainWindow->window, mainWindow->main_win_gc, 0, 0, width,
                   height, 0, 0);
    }
    else
        XUnmapWindow (mainWindow->dpy, mainWindow->window);

    cairo_destroy(c);
}

void DestroyMainWindow(MainWindow* mainWindow)
{
    cairo_surface_destroy(mainWindow->cs_main_bar);
    XFreePixmap(mainWindow->dpy, mainWindow->pm_main_bar);
    XFreeGC(mainWindow->dpy, mainWindow->main_win_gc);
    XDestroyWindow(mainWindow->dpy, mainWindow->window);
    
    FcitxModuleFunctionArg arg;
    arg.args[0] = mainWindow;
    InvokeFunction(mainWindow->owner->owner, FCITX_X11, REMOVEXEVENTHANDLER, arg);
    
    free(mainWindow);
}

void UpdateStatusGeometry(FcitxClassicUIStatus *privstat, SkinImage *image, int x, int y)
{
    privstat->x = x;
    privstat->y = y;
    privstat->w = cairo_image_surface_get_width(image->image);
    privstat->h = cairo_image_surface_get_height(image->image);
}

void ShowMainWindow(MainWindow* mainWindow)
{
    XMapRaised (mainWindow->dpy, mainWindow->window);
}

void CloseMainWindow(MainWindow *mainWindow)
{
    XUnmapWindow (mainWindow->dpy, mainWindow->window);
}

boolean SetMouseStatus(MainWindow *mainWindow, MouseE* mouseE, MouseE value, MouseE other)
{
    FcitxInstance *instance = mainWindow->owner->owner;
    boolean changed = false;
    if (mouseE != &mainWindow->logostat.mouse)
    {
        if (mainWindow->logostat.mouse != other)
        {
            changed = true;
            mainWindow->logostat.mouse = other;
        }
    }
    if (mouseE != &mainWindow->imiconstat.mouse)
    {
        if (mainWindow->imiconstat.mouse != other)
        {
            changed = true;
            mainWindow->imiconstat.mouse = other;
        }
    }
    FcitxUIStatus *status;
    for(status = (FcitxUIStatus*) utarray_front(&instance->uistats);
        status != NULL;
        status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
    )
    {
        FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
        if (mouseE != &privstat->mouse)
        {
            if (privstat->mouse != other)
            {
                changed = true;
                privstat->mouse = other;
            }
        }
    }
    if (mouseE != NULL && *mouseE != value)
    {
        changed = true;
        *mouseE = value;
    }
    
    return changed;
}

boolean MainWindowEventHandler(void *arg, XEvent* event)
{
    MainWindow* mainWindow = arg;
    FcitxInstance *instance = mainWindow->owner->owner;
    FcitxClassicUI *classicui = mainWindow->owner;
    MouseE *mouse;
    
    if (event->xany.window == mainWindow->window)
    {
        switch (event->type)
        {
            case Expose:
                DrawMainWindow(mainWindow);
                break;
            case MotionNotify:
                mouse = NULL;
                if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->logostat)) {
                    mouse = &mainWindow->logostat.mouse;
                } else if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->imiconstat)) {
                    mouse = &mainWindow->imiconstat.mouse;
                } else {
                    FcitxUIStatus *status;
                    for(status = (FcitxUIStatus*) utarray_front(&instance->uistats);
                        status != NULL;
                        status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
                    )
                    {
                        FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
                        if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat))
                            mouse = &privstat->mouse;
                    }
                }
                if  (SetMouseStatus(mainWindow, mouse, MOTION, RELEASE))
                    DrawMainWindow(mainWindow);
                break;
            case LeaveNotify:
                if(SetMouseStatus(mainWindow, NULL, RELEASE, RELEASE))
                    DrawMainWindow(mainWindow);
                break;
            case ButtonPress:
                switch (event->xbutton.button) {
                    case Button1:
                        {
                            mouse = NULL;
                            if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->logostat)) {
                                mouse = &mainWindow->logostat.mouse;
                                
                                classicui->iMainWindowOffsetX = event->xbutton.x;
                                classicui->iMainWindowOffsetY = event->xbutton.y;

                                if (!ClassicUIMouseClick(mainWindow->owner, mainWindow->window, &classicui->iMainWindowOffsetX, &classicui->iMainWindowOffsetY))
                                {
                                    if (GetCurrentState(instance) == IS_CLOSED) {
                                        EnableIM(instance, false);
                                    }
                                    else {
                                        CloseIM(instance, GetCurrentIC(instance));
                                    }
                                }
                            } else if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->imiconstat)) {
                                mouse = &mainWindow->imiconstat.mouse;
                                SwitchIM(instance, -1);
                            } else {
                                FcitxUIStatus *status;
                                for(status = (FcitxUIStatus*) utarray_front(&instance->uistats);
                                    status != NULL;
                                    status = (FcitxUIStatus*) utarray_next(&instance->uistats, status)
                                )
                                {
                                    FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
                                    if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat))
                                    {
                                        mouse = &privstat->mouse;
                                        UpdateStatus(instance, status->name);
                                    }
                                }
                            }
                            if  (SetMouseStatus(mainWindow, mouse, PRESS, RELEASE))
                                DrawMainWindow(mainWindow);
                        }
                        break;
                    case Button3:
                        {
                            XlibMenu *mainMenuWindow = classicui->mainMenuWindow;
                            unsigned int height; int sheight;
                            XWindowAttributes attr;
                            GetMenuSize(mainMenuWindow);
                            GetScreenSize(classicui, NULL, &sheight);
                            XGetWindowAttributes(classicui->dpy, mainWindow->window, &attr);
                            height = attr.height;

                            mainMenuWindow->iPosX = classicui->iMainWindowOffsetX;
                            mainMenuWindow->iPosY =
                                classicui->iMainWindowOffsetY +
                                height;
                            if ((mainMenuWindow->iPosY + mainMenuWindow->height) >
                                sheight)
                                mainMenuWindow->iPosY = classicui->iMainWindowOffsetY - 5 - mainMenuWindow->height;

                            DrawXlibMenu(mainMenuWindow);
                            DisplayXlibMenu(mainMenuWindow);

                        }
                        break;
                        
                }
                break;
            case ButtonRelease:
                switch (event->xbutton.button) {
                    case Button1:
                        if(SetMouseStatus(mainWindow, NULL, RELEASE, RELEASE))
                            DrawMainWindow(mainWindow);
                        break;
                    case Button2:
                        DisplayAboutWindow(mainWindow->owner->aboutWindow);
                        break;
                }
                break;
        }
        return true;
    }
    return false;
}
