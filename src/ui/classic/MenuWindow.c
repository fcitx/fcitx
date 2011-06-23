/***************************************************************************
 *   Copyright (C) 2009~2010 by t3swing                                    *
 *   t3swing@sina.com                                                      *
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
#include <ctype.h>
#include <math.h>
#include <iconv.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/cutils.h>
#include <cairo-xlib.h>
#include <fcitx/ui.h>
#include <fcitx/module.h>
#include <module/x11/x11stuff.h>

#include "skin.h"
#include "classicui.h"
#include "MenuWindow.h"

static void DestroyXlibMenu(XlibMenu *menu);
static boolean ReverseColor(XlibMenu * Menu,int shellIndex);
static void MenuMark(XlibMenu* menu, int y, int i);
static void DrawArrow(cairo_t *cr, XlibMenu *menu, int line_y);
static void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth, int dwidth, int dheight);
static void DisplayText(XlibMenu * menu,int shellindex,int line_y);
static void DrawDivLine(XlibMenu * menu,int line_y);
static boolean MenuWindowEventHandler(void *instance, XEvent* event);

#define GetMenuShell(m, i) ((MenuShell*) utarray_eltptr(&(m)->shell, (i)))

XlibMenu* CreateMainMenuWindow(FcitxClassicUI *classicui)
{
    XlibMenu* menu = CreateXlibMenu(classicui);
    
    FcitxModuleFunctionArg arg;
    arg.args[0] = MenuWindowEventHandler;
    arg.args[1] = menu;
    InvokeFunction(classicui->owner, FCITX_X11, ADDXEVENTHANDLER, arg);
    return menu;
}

boolean MenuWindowEventHandler(void *instance, XEvent* event)
{
    XlibMenu* menu = (XlibMenu*) instance;
    if (event->xany.window == menu->menuWindow)
    {
        switch(event->type)
        {
            case Expose:
                DrawXlibMenu(menu);
                break;
            case ButtonPress:
                break;
        }
        return true;
    }
    return false;
}

XlibMenu* CreateXlibMenu(FcitxClassicUI *classicui)
{
    char        strWindowName[]="Fcitx Menu Window";
    XlibMenu *menu = fcitx_malloc0(sizeof(XlibMenu));
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    int depth;
    Colormap cmap;
    Visual * vs;
    XGCValues xgv;
    GC gc;
    Display* dpy = classicui->dpy;
    int iScreen = classicui->iScreen;
    menu->owner = classicui;
    
    vs=FindARGBVisual (classicui);
    InitWindowAttribute(dpy, iScreen, &vs, &cmap, &attrib, &attribmask, &depth);

    //开始只创建一个简单的窗口不做任何动作
    menu->menuWindow =XCreateWindow (dpy,
                                     RootWindow (dpy, iScreen),
                                     0, 0,
                                     MENU_WINDOW_WIDTH,MENU_WINDOW_HEIGHT,
                                     0, depth, InputOutput,
                                     vs, attribmask, &attrib);

    if (menu->menuWindow == (Window) NULL)
        return False;

    XSetTransientForHint (dpy, menu->menuWindow, DefaultRootWindow (dpy));

    XChangeProperty (dpy, menu->menuWindow, classicui->windowTypeAtom, XA_ATOM, 32, PropModeReplace, (void *) &classicui->typeMenuAtom, 1);
    
    menu->pixmap = XCreatePixmap(dpy,
                                 menu->menuWindow,
                                 MENU_WINDOW_WIDTH,
                                 MENU_WINDOW_HEIGHT,
                                 depth);

    xgv.foreground = WhitePixel(dpy, iScreen);
    gc = XCreateGC(dpy, menu->pixmap, GCForeground, &xgv);
    XFillRectangle(
        dpy,
        menu->pixmap,
        gc,
        0,
        0,
        MENU_WINDOW_WIDTH,
        MENU_WINDOW_HEIGHT);
    menu->menu_cs=cairo_xlib_surface_create(dpy,
                                            menu->pixmap,
                                            vs,
                                            MENU_WINDOW_WIDTH,MENU_WINDOW_HEIGHT);
    XFreeGC(dpy, gc);

    XSelectInput (dpy, menu->menuWindow, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask );
    
    SetWindowProperty(classicui, menu->menuWindow, FCITX_WINDOW_MENU, strWindowName);
    
    FcitxSkin *sc = &menu->owner->skin;
    menu->iPosX=100;
    menu->iPosY=100;
    menu->width=cairo_image_surface_get_height(menu->menu_cs);
    menu->font_size=sc->skinFont.menuFontSize;
    strcpy(menu->font,menu->owner->menuFont);
    menu->mark=-1;

    return menu;
}

void GetMenuSize(XlibMenu * menu)
{
    int i=0;
    int winheight=0;
    int fontheight=0;
    int menuwidth = 0;
    FcitxSkin *sc = &menu->owner->skin;

    winheight = sc->skinMenu.marginTop + sc->skinMenu.marginBottom;//菜单头和尾都空8个pixel
    fontheight= menu->font_size;
    for (i=0;i<utarray_len(&menu->menu->shell);i++)
    {
        if ( GetMenuShell(menu->menu, i)->type == MENUSHELL)
            winheight += 6+fontheight;
        else if ( GetMenuShell(menu->menu, i)->type == DIVLINE)
            winheight += 5;
        
        int width = StringWidth(GetMenuShell(menu->menu, i)->tipstr, menu->font, menu->font_size);
        if (width > menuwidth)
            menuwidth = width;        
    }
    menu->height = winheight;
    menu->width = menuwidth + sc->skinMenu.marginLeft + sc->skinMenu.marginRight + 15 + 20;
}

//根据Menu内容来绘制菜单内容
void DrawXlibMenu(XlibMenu * menu)
{
    FcitxSkin *sc = &menu->owner->skin;
    FcitxClassicUI *classicui = menu->owner;
    Display* dpy = classicui->dpy;
    GC gc = XCreateGC( dpy, menu->menuWindow, 0, NULL );
    int i=0;
    int fontheight;
    int iPosY = 0;

    fontheight= menu->font_size;

    GetMenuSize(menu);

    DrawMenuBackground(sc, menu);

    iPosY=sc->skinMenu.marginTop;
    for (i=0;i<utarray_len(&menu->menu->shell);i++)
    {
        if ( GetMenuShell(menu->menu, i)->type == MENUSHELL)
        {
            DisplayText( menu,i,iPosY);
            if (menu->mark == i)//void menuMark(Display * dpy,xlibMenu * Menu,int y,int i)
                MenuMark(menu,iPosY,i);
            iPosY=iPosY+6+fontheight;
        }
        else if ( GetMenuShell(menu->menu, i)->type == DIVLINE)
        {
            DrawDivLine(menu,iPosY);
            iPosY+=5;
        }
    }
    
    XResizeWindow(dpy, menu->menuWindow, menu->width, menu->height);
    XCopyArea (dpy,
               menu->pixmap,
               menu->menuWindow,
               gc,
               0,
               0,
               menu->width,
               menu->height, 0, 0);
    XFreeGC(dpy, gc);

}

void DisplayXlibMenu(XlibMenu * menu)
{
    FcitxClassicUI *classicui = menu->owner;
    Display* dpy = classicui->dpy;
    XMapRaised (dpy, menu->menuWindow);
    XMoveWindow(dpy, menu->menuWindow, menu->iPosX, menu->iPosY);
}

void DrawDivLine(XlibMenu * menu,int line_y)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginLeft = sc->skinMenu.marginLeft;
    int marginRight = sc->skinMenu.marginRight;
    cairo_t * cr;
    cr=cairo_create(menu->menu_cs);
    fcitx_cairo_set_color(cr, &sc->skinMenu.lineColor);
    cairo_set_line_width (cr, 2);
    cairo_move_to(cr, marginLeft + 3, line_y+3);
    cairo_line_to(cr, menu->width - marginRight - 3, line_y+3);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

void MenuMark(XlibMenu * menu,int y,int i)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginLeft = sc->skinMenu.marginLeft;
    double size = (menu->font_size * 0.7 ) / 2;
    cairo_t *cr;
    cr = cairo_create(menu->menu_cs);
    if (GetMenuShell(menu->menu, i)->isselect == 0)
    {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);
    }
    else
    {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_translate(cr, marginLeft + 7, y + (menu->font_size / 2.0) );
    cairo_arc(cr, 0, 0, size , 0., 2*M_PI);
    cairo_fill(cr);
    cairo_destroy(cr);
}

/*
* 显示菜单上面的文字信息,只需要指定窗口,窗口宽度,需要显示文字的上边界,字体,显示的字符串和是否选择(选择后反色)
* 其他都固定,如背景和文字反色不反色的颜色,反色框和字的位置等
*/
void DisplayText(XlibMenu * menu,int shellindex,int line_y)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginLeft = sc->skinMenu.marginLeft;
    int marginRight = sc->skinMenu.marginRight;
    cairo_t *  cr;
    cr=cairo_create(menu->menu_cs);

    SetFontContext(cr, menu->font, menu->font_size);

    if (GetMenuShell(menu->menu, shellindex)->isselect ==0)
    {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);

        OutputStringWithContext(cr, GetMenuShell(menu->menu, shellindex)->tipstr , 15 + marginLeft ,line_y);
    }
    else
    {
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        fcitx_cairo_set_color(cr, &sc->skinMenu.activeColor);
        cairo_rectangle (cr, marginLeft ,line_y, menu->width - marginRight - marginLeft,menu->font_size+4);
        cairo_fill (cr);

        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
        OutputStringWithContext(cr, GetMenuShell(menu->menu, shellindex)->tipstr , 15 + marginLeft ,line_y);
    }
    ResetFontContext();
    cairo_destroy(cr);
}

void DrawArrow(cairo_t *cr, XlibMenu *menu, int line_y)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginRight = sc->skinMenu.marginRight;
    double size = menu->font_size * 0.4;
    double offset = (menu->font_size - size) / 2;
    cairo_move_to(cr,menu->width - marginRight - 1 - size, line_y + offset);
    cairo_line_to(cr,menu->width - marginRight - 1 - size, line_y+size * 2 + offset);
    cairo_line_to(cr,menu->width - marginRight - 1, line_y + size + offset );
    cairo_line_to(cr,menu->width - marginRight - 1 - size ,line_y + offset);
    cairo_fill (cr);
}

/**
*返回鼠标指向的菜单在menu中是第多少项
*/
int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth)
{
    FcitxSkin *sc = &menu->owner->skin;
    int i;
    int winheight=sc->skinMenu.marginTop;
    int fontheight;
    int marginLeft = sc->skinMenu.marginLeft;

    if (x < marginLeft)
        return -1;

    fontheight= menu->font_size;
    for (i=0;i<utarray_len(&menu->menu->shell);i++)
    {
        if (GetMenuShell(menu->menu, i)->type == MENUSHELL)
        {
            if (y>winheight+1 && y<winheight+6+fontheight-1)
            {
                if (offseth)
                    *offseth = winheight;
                return i;
            }
            winheight=winheight+6+fontheight;
        }
        else if (GetMenuShell(menu->menu, i)->type == DIVLINE)
            winheight+=5;
    }
    return -1;
}

boolean ReverseColor(XlibMenu * menu,int shellIndex)
{
    boolean flag = False;
    int i;

    int last = -1;

    for (i=0;i<utarray_len(&menu->menu->shell);i++)
    {
        if (GetMenuShell(menu->menu, i)->isselect)
            last = i;

        GetMenuShell(menu->menu, i)->isselect=0;
    }
    if (shellIndex == last)
        flag = True;
    if (shellIndex >=0 && shellIndex < utarray_len(&menu->menu->shell))
        GetMenuShell(menu->menu, shellIndex)->isselect = 1;
    return flag;
}

void ClearSelectFlag(XlibMenu * menu)
{
    int i;
    for (i=0;i< utarray_len(&menu->menu->shell);i++)
    {
        GetMenuShell(menu->menu, i)->isselect=0;
    }
}

void DestroyXlibMenu(XlibMenu *menu)
{
    cairo_surface_destroy(menu->menu_cs);
    XFreePixmap(menu->owner->dpy, menu->pixmap);
    XDestroyWindow(menu->owner->dpy, menu->menuWindow);
}