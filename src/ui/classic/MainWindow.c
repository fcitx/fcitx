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
#include "fcitx-config/cutils.h"
#include "module/x11/x11stuff.h"
#include "classicui.h"
#include "skin.h"
#include <core/backend.h>
#include "core/module.h"

static boolean MainWindowEventHandler(void *instance, XEvent* event);

MainWindow* CreateMainWindow (Display* dpy, int iScreen, FcitxSkin* sc, HIDE_MAINWINDOW hideMainWindow)
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

    mainWindow = malloc0(sizeof(MainWindow));
    
    mainWindow->hideMode = hideMainWindow;
    
    GetScreenSize(dpy, iScreen, &swidth, &sheight);
    LoadMainBarImage(mainWindow, sc);

    vs = FindARGBVisual(dpy, iScreen);

    if (classicui.iMainWindowOffsetX + cairo_image_surface_get_width(mainWindow->bar) > swidth )
        classicui.iMainWindowOffsetX = swidth - cairo_image_surface_get_width(mainWindow->bar);
    
    if (classicui.iMainWindowOffsetY + cairo_image_surface_get_height(mainWindow->bar) > sheight )
        classicui.iMainWindowOffsetY = sheight - cairo_image_surface_get_height(mainWindow->bar);

    InitWindowAttribute(dpy, iScreen, &vs, &cmap, &attrib, &attribmask, &depth);
    mainWindow->window=XCreateWindow (dpy,
                                     RootWindow(dpy, iScreen),
                                     classicui.iMainWindowOffsetX,
                                     classicui.iMainWindowOffsetY,
                                     cairo_image_surface_get_width(mainWindow->bar),
                                     cairo_image_surface_get_height(mainWindow->bar),
                                     0, depth,InputOutput, vs,attribmask, &attrib);

    if (mainWindow->window == None)
        return NULL;

    xgv.foreground = WhitePixel(dpy, iScreen);
    mainWindow->pm_main_bar = XCreatePixmap(
                                 dpy,
                                 mainWindow->window,
                                 cairo_image_surface_get_width(mainWindow->bar),
                                 cairo_image_surface_get_height(mainWindow->bar),
                                 depth);
    gc = XCreateGC(dpy,mainWindow->pm_main_bar, GCForeground, &xgv);
    XFillRectangle(dpy, mainWindow->pm_main_bar, gc, 0, 0,cairo_image_surface_get_width(mainWindow->bar), cairo_image_surface_get_height(mainWindow->bar));
    mainWindow->cs_main_bar=cairo_xlib_surface_create(
                               dpy,
                               mainWindow->pm_main_bar,
                               vs,
                               cairo_image_surface_get_width(mainWindow->bar),
                               cairo_image_surface_get_height(mainWindow->bar));
    XFreeGC(dpy,gc);

    mainWindow->main_win_gc = XCreateGC( dpy, mainWindow->window, 0, NULL );
    XChangeWindowAttributes (dpy, mainWindow->window, attribmask, &attrib);
    XSelectInput (dpy, mainWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask);
    

    XTextProperty   tp;
    /* Set the name of the window */
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, mainWindow->window, &tp);

    FcitxModuleFunctionArg arg;
    arg.args[0] = MainWindowEventHandler;
    arg.args[1] = mainWindow;
    InvokeFunction(FCITX_X11, ADDXEVENTHANDLER, arg);
    return mainWindow;
}

void DisplayMainWindow (Display* dpy, MainWindow* mainWindow)
{
#ifdef _DEBUG
    FcitxLog(DEBUG, _("DISPLAY MainWindow"));
#endif

    if (!mainWindow->bMainWindowHidden)
        XMapRaised (dpy, mainWindow->window);
}

void DrawMainWindow (MainWindow* mainWindow)
{
    int8_t            iIndex = 0;
    cairo_t *c;

    if ( mainWindow->bMainWindowHidden )
        return;

    iIndex = IS_CLOSED;
#ifdef _DEBUG
    FcitxLog(DEBUG, _("DRAW MainWindow"));
#endif

    c=cairo_create(mainWindow->cs_main_bar);
    //把背景清空
    cairo_set_source_rgba(c, 0, 0, 0,0);
    cairo_rectangle (c, 0, 0, SIZEX, SIZEY);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_fill(c);

    cairo_set_operator(c, CAIRO_OPERATOR_OVER);

    if (mainWindow->hideMode == HM_SHOW || (mainWindow->hideMode == HM_AUTO && (GetCurrentState() != IS_CLOSED)))
    {
        // extern mouse_e ms_logo,ms_punc,ms_corner,ms_lx,ms_chs,ms_lock,ms_vk,ms_py;
        DrawImage(&c, mainWindow->bar, 0, 0, RELEASE );
/* TODO:        DrawImage(&c, sc->skinMainBar.logo,sc->logo,ms_logo);
        DrawImage(&c, sc->skinMainBar.zhpunc,sc->punc[btmpPunc],ms_punc);
        DrawImage(&c, sc->skinMainBar.chs,sc->chs_t[fcitxProfile.bUseGBKT],ms_chs);
        DrawImage(&c, sc->skinMainBar.halfcorner,sc->corner[fcitxProfile.bCorner],ms_corner);
        DrawImage(&c, sc->skinMainBar.unlock,sc->lock[fcitxProfile.bLocked],ms_lock);
        DrawImage(&c, sc->skinMainBar.nolegend,lx[fcitxProfile.bUseLegend],ms_lx);
        DrawImage(&c, sc->skinMainBar.novk,vk[bVK],ms_vk);

        iIndex = GetCurrentState();
        if ( iIndex == 1 || iIndex ==0 )
        {
            //英文
            DrawImage(&c, sc->skinMainBar.eng,english,ms_py);
        }
        else
        {
            //默认码表显示
            if (im[gs.iIMIndex].icon)
                DrawImage(&c, im[gs.iIMIndex].image, im[gs.iIMIndex].icon , ms_py);
            else
            {   //如果非默认码表的内容,则临时加载png文件.
                //暂时先不能自定义码表图片.其他码表统一用一种图片。
                DrawImage(&c, sc->skinMainBar.chn,otherim,ms_py);
            }

        }*/
        XCopyArea (mainWindow->dpy, mainWindow->pm_main_bar, mainWindow->window, mainWindow->main_win_gc, 0, 0, cairo_image_surface_get_width(mainWindow->bar),\
                   cairo_image_surface_get_height(mainWindow->bar), 0, 0);
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
    InvokeFunction(FCITX_X11, REMOVEXEVENTHANDLER, arg);
    
    free(mainWindow);
}

void CloseMainWindow(MainWindow *mainWindow)
{
    XUnmapWindow (mainWindow->dpy, mainWindow->window);
}

boolean MainWindowEventHandler(void *instance, XEvent* event)
{
    MainWindow* mainWindow = instance;
    
    if (event->xany.window == mainWindow->window)
    {
        switch (event->type)
        {
            case Expose:
                DrawMainWindow(mainWindow);
                break;
            case ButtonPress:
                switch (event->xbutton.button) {
                    case Button1:
                        {/*
                            if (IsInRspArea
                                (event->xbutton.x, event->xbutton.y,
                                sc.skinMainBar.logo)) {
                                fcitxProfile.iMainWindowOffsetX = event->xbutton.x;
                                fcitxProfile.iMainWindowOffsetY = event->xbutton.y;
                                ms_logo = PRESS;
                                if (!MouseClick
                                    (&fcitxProfile.iMainWindowOffsetX, &fcitxProfile.iMainWindowOffsetY, mainWindow.window)) {
                                    if (GetCurrentState() != IS_ACTIVE) {
                                        SetIMState((GetCurrentState() == IS_ENG) ? False : True);
                                    }
                                }
                                DrawMainWindow(mainWindow);

                                if (GetCurrentState() == IS_ACTIVE)
                                    DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size,
                                                tray.size);
                                else
                                    DrawTrayWindow(INACTIVE_ICON, 0, 0, tray.size,
                                                tray.size);
                            }*/
                        }
                        break;
                }
                break;
        }
        return true;
    }
    return false;
}