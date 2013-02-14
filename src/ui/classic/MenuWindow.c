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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
#include <ctype.h>
#include <math.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/log.h"
#include <cairo-xlib.h>
#include "fcitx/ui.h"
#include "fcitx/module.h"

#include "skin.h"
#include "classicui.h"
#include "MenuWindow.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

#define MENU_WINDOW_WIDTH   100
#define MENU_WINDOW_HEIGHT  100

static boolean ReverseColor(XlibMenu * Menu, int shellIndex);
static void MenuMark(XlibMenu* menu, int y, int i);
static void DrawArrow(XlibMenu* menu, int line_y, int i);
static void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth);
static void DisplayText(XlibMenu* menu, int shellindex, int line_y, int fontHeight);
static void DrawDivLine(XlibMenu * menu, int line_y);
static boolean MenuWindowEventHandler(void *arg, XEvent* event);
static int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth);
static void CloseAllMenuWindow(FcitxClassicUI *classicui);
static void CloseAllSubMenuWindow(XlibMenu *xlibMenu);
static void CloseOtherSubMenuWindow(XlibMenu *xlibMenu, XlibMenu* subMenu);
static boolean IsMouseInOtherMenu(XlibMenu *xlibMenu, int x, int y);
static void InitXlibMenu(XlibMenu* menu);
static void ReloadXlibMenu(void* arg, boolean enabled);

#define GetMenuItem(m, i) ((FcitxMenuItem*) utarray_eltptr(&(m)->shell, (i)))

void InitXlibMenu(XlibMenu* menu)
{
    FcitxClassicUI* classicui = menu->owner;
    char        strWindowName[] = "Fcitx Menu Window";
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    int depth;
    Colormap cmap;
    Visual * vs;
    Display* dpy = classicui->dpy;
    int iScreen = classicui->iScreen;

    vs = ClassicUIFindARGBVisual(classicui);
    ClassicUIInitWindowAttribute(classicui, &vs, &cmap, &attrib, &attribmask, &depth);

    //开始只创建一个简单的窗口不做任何动作
    menu->menuWindow = XCreateWindow(dpy,
                                     RootWindow(dpy, iScreen),
                                     0, 0,
                                     MENU_WINDOW_WIDTH, MENU_WINDOW_HEIGHT,
                                     0, depth, InputOutput,
                                     vs, attribmask, &attrib);

    if (menu->menuWindow == (Window) NULL)
        return;

    XSetTransientForHint(dpy, menu->menuWindow, DefaultRootWindow(dpy));

    menu->menu_x_cs = cairo_xlib_surface_create(
                                    dpy,
                                    menu->menuWindow,
                                    vs,
                                    MENU_WINDOW_WIDTH,
                                    MENU_WINDOW_HEIGHT);

    menu->menu_cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               MENU_WINDOW_WIDTH, MENU_WINDOW_HEIGHT);

    XSelectInput(dpy, menu->menuWindow, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask | StructureNotifyMask);

    ClassicUISetWindowProperty(classicui, menu->menuWindow, FCITX_WINDOW_MENU, strWindowName);

    menu->iPosX = 100;
    menu->iPosY = 100;
    menu->width = cairo_image_surface_get_height(menu->menu_cs);
}


XlibMenu* CreateMainMenuWindow(FcitxClassicUI *classicui)
{
    XlibMenu* menu = CreateXlibMenu(classicui);
    menu->menushell = &classicui->mainMenu;

    return menu;
}

boolean MenuWindowEventHandler(void *arg, XEvent* event)
{
    XlibMenu* menu = (XlibMenu*) arg;
    FcitxClassicUI* classicui = menu->owner;
    if (event->xany.window == menu->menuWindow) {
        switch (event->type) {
        case MapNotify:
            FcitxMenuUpdate(menu->menushell);
            break;
        case Expose:
            DrawXlibMenu(menu);
            break;
        case LeaveNotify: {
            int x = event->xcrossing.x_root;
            int y = event->xcrossing.y_root;

            if (!IsMouseInOtherMenu(menu, x, y)) {
                CloseAllSubMenuWindow(menu);
            }
        }
        break;
        case MotionNotify: {
            int offseth = 0;
            GetMenuSize(menu);
            int i = SelectShellIndex(menu, event->xmotion.x, event->xmotion.y, &offseth);
            boolean flag = ReverseColor(menu, i);
            FcitxMenuItem *item = GetMenuItem(menu->menushell, i);
            if (!flag) {
                DrawXlibMenu(menu);

                if (item && item->type == MENUTYPE_SUBMENU && item->subMenu) {
                    XlibMenu* subxlibmenu = (XlibMenu*) item->subMenu->uipriv[classicui->isfallback];
                    CloseOtherSubMenuWindow(menu, subxlibmenu);
                    MoveSubMenu(subxlibmenu, menu, offseth);
                    DrawXlibMenu(subxlibmenu);
                    XMapRaised(menu->owner->dpy, subxlibmenu->menuWindow);
                }
            }
            if (item == NULL)
                CloseOtherSubMenuWindow(menu, NULL);
        }
        break;
        case ButtonPress: {
            switch (event->xbutton.button) {
            case Button1: {
                int offseth;
                int i = SelectShellIndex(menu, event->xmotion.x, event->xmotion.y, &offseth);
                if (menu->menushell->MenuAction) {
                    if (menu->menushell->MenuAction(menu->menushell, i))
                        CloseAllMenuWindow(menu->owner);
                }
            }
            break;
            case Button3:
                CloseAllMenuWindow(menu->owner);
                break;
            }
        }
        break;
        }
        return true;
    }
    return false;
}

void CloseAllMenuWindow(FcitxClassicUI *classicui)
{
    FcitxInstance* instance = classicui->owner;
    FcitxUIMenu** menupp;
    UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
    for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp)
        ) {
        XlibMenu* xlibMenu = (XlibMenu*)(*menupp)->uipriv[classicui->isfallback];
        XUnmapWindow(classicui->dpy, xlibMenu->menuWindow);
    }
    XUnmapWindow(classicui->dpy, classicui->mainMenuWindow->menuWindow);
}

void CloseOtherSubMenuWindow(XlibMenu *xlibMenu, XlibMenu* subMenu)
{
    FcitxClassicUI* classicui = xlibMenu->owner;
    FcitxMenuItem *menu;
    for (menu = (FcitxMenuItem *) utarray_front(&xlibMenu->menushell->shell);
            menu != NULL;
            menu = (FcitxMenuItem *) utarray_next(&xlibMenu->menushell->shell, menu)
        ) {
        if (menu->type == MENUTYPE_SUBMENU && menu->subMenu && menu->subMenu->uipriv[classicui->isfallback] != subMenu) {
            CloseAllSubMenuWindow((XlibMenu *)menu->subMenu->uipriv[classicui->isfallback]);
        }
    }
}

void CloseAllSubMenuWindow(XlibMenu *xlibMenu)
{
    FcitxClassicUI* classicui = xlibMenu->owner;
    FcitxMenuItem *menu;
    for (menu = (FcitxMenuItem *) utarray_front(&xlibMenu->menushell->shell);
            menu != NULL;
            menu = (FcitxMenuItem *) utarray_next(&xlibMenu->menushell->shell, menu)
        ) {
        if (menu->type == MENUTYPE_SUBMENU && menu->subMenu) {
            CloseAllSubMenuWindow((XlibMenu *)menu->subMenu->uipriv[classicui->isfallback]);
        }
    }
    XUnmapWindow(xlibMenu->owner->dpy, xlibMenu->menuWindow);
}

boolean IsMouseInOtherMenu(XlibMenu *xlibMenu, int x, int y)
{
    FcitxClassicUI *classicui = xlibMenu->owner;
    FcitxInstance* instance = classicui->owner;
    FcitxUIMenu** menupp;
    UT_array* uimenus = FcitxInstanceGetUIMenus(instance);
    for (menupp = (FcitxUIMenu **) utarray_front(uimenus);
            menupp != NULL;
            menupp = (FcitxUIMenu **) utarray_next(uimenus, menupp)
        ) {

        XlibMenu* otherXlibMenu = (XlibMenu*)(*menupp)->uipriv[classicui->isfallback];
        if (otherXlibMenu == xlibMenu)
            continue;
        XWindowAttributes attr;
        XGetWindowAttributes(classicui->dpy, otherXlibMenu->menuWindow, &attr);
        if (attr.map_state != IsUnmapped &&
                FcitxUIIsInBox(x, y, attr.x, attr.y, attr.width, attr.height)) {
            return true;
        }
    }

    XlibMenu* otherXlibMenu = classicui->mainMenuWindow;
    if (otherXlibMenu == xlibMenu)
        return false;
    XWindowAttributes attr;
    XGetWindowAttributes(classicui->dpy, otherXlibMenu->menuWindow, &attr);
    if (attr.map_state != IsUnmapped &&
            FcitxUIIsInBox(x, y, attr.x, attr.y, attr.width, attr.height)) {
        return true;
    }
    return false;
}

XlibMenu* CreateXlibMenu(FcitxClassicUI *classicui)
{
    XlibMenu *menu = fcitx_utils_malloc0(sizeof(XlibMenu));
    menu->owner = classicui;
    InitXlibMenu(menu);

    FcitxX11AddXEventHandler(classicui->owner, MenuWindowEventHandler, menu);
    FcitxX11AddCompositeHandler(classicui->owner, ReloadXlibMenu, menu);
    return menu;
}

void GetMenuSize(XlibMenu * menu)
{
    int i = 0;
    int winheight = 0;
    int fontheight = 0;
    int menuwidth = 0;
    FcitxSkin *sc = &menu->owner->skin;
    int dpi = sc->skinFont.respectDPI? menu->owner->dpi: 0;
    FCITX_UNUSED(dpi);

    winheight = sc->skinMenu.marginTop + sc->skinMenu.marginBottom;//菜单头和尾都空8个pixel
    fontheight = FontHeight(menu->owner->menuFont, sc->skinFont.menuFontSize, dpi);
    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU)
            winheight += 6 + fontheight;
        else if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_DIVLINE)
            winheight += 5;

        int width = StringWidth(GetMenuItem(menu->menushell, i)->tipstr, menu->owner->menuFont, sc->skinFont.menuFontSize, dpi);
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
    GC gc = XCreateGC(dpy, menu->menuWindow, 0, NULL);
    int i = 0;
    int fontheight;
    int iPosY = 0;
    int dpi = sc->skinFont.respectDPI? menu->owner->dpi: 0;
    FCITX_UNUSED(dpi);

    fontheight = FontHeight(menu->owner->menuFont, sc->skinFont.menuFontSize, dpi);
    SkinImage *background = LoadImage(sc, sc->skinMenu.backImg, false);

    GetMenuSize(menu);
    EnlargeCairoSurface(&menu->menu_cs, menu->width, menu->height);

    if (background) {
        cairo_t* cr = cairo_create(menu->menu_cs);
        DrawResizableBackground(cr, background->image, menu->height, menu->width,
                                sc->skinMenu.marginLeft,
                                sc->skinMenu.marginTop,
                                sc->skinMenu.marginRight,
                                sc->skinMenu.marginBottom,
                                sc->skinMenu.fillV,
                                sc->skinMenu.fillH
                            );

        cairo_destroy(cr);
    }

    iPosY = sc->skinMenu.marginTop;
    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU) {
            DisplayText(menu, i, iPosY, fontheight);
            if (menu->menushell->mark == i)
                MenuMark(menu, iPosY, i);

            if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU)
                DrawArrow(menu, iPosY, i);
            iPosY = iPosY + 6 + fontheight;
        } else if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_DIVLINE) {
            DrawDivLine(menu, iPosY);
            iPosY += 5;
        }
    }
    XResizeWindow(dpy, menu->menuWindow, menu->width, menu->height);
    _CAIRO_SETSIZE(menu->menu_x_cs, menu->width, menu->height);
    cairo_t* c = cairo_create(menu->menu_x_cs);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(c, menu->menu_cs, 0, 0);
    cairo_rectangle(c, 0, 0, menu->width, menu->height);
    cairo_clip(c);
    cairo_paint(c);
    cairo_destroy(c);
    cairo_surface_flush(menu->menu_x_cs);
    XFreeGC(dpy, gc);
}

void DisplayXlibMenu(XlibMenu * menu)
{
    FcitxClassicUI *classicui = menu->owner;
    Display* dpy = classicui->dpy;
    XMapRaised(dpy, menu->menuWindow);
    XMoveWindow(dpy, menu->menuWindow, menu->iPosX, menu->iPosY);
}

void DrawDivLine(XlibMenu * menu, int line_y)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginLeft = sc->skinMenu.marginLeft;
    int marginRight = sc->skinMenu.marginRight;
    cairo_t * cr;
    cr = cairo_create(menu->menu_cs);
    fcitx_cairo_set_color(cr, &sc->skinMenu.lineColor);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, marginLeft + 3, line_y + 3);
    cairo_line_to(cr, menu->width - marginRight - 3, line_y + 3);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

void MenuMark(XlibMenu * menu, int y, int i)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginLeft = sc->skinMenu.marginLeft;
    double size = (sc->skinFont.menuFontSize * 0.7) / 2;
    cairo_t *cr;
    cr = cairo_create(menu->menu_cs);
    if (GetMenuItem(menu->menushell, i)->isselect == 0) {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);
    } else {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_translate(cr, marginLeft + 7, y + (sc->skinFont.menuFontSize / 2.0));
    cairo_arc(cr, 0, 0, size , 0., 2 * M_PI);
    cairo_fill(cr);
    cairo_destroy(cr);
}

/*
* 显示菜单上面的文字信息,只需要指定窗口,窗口宽度,需要显示文字的上边界,字体,显示的字符串和是否选择(选择后反色)
* 其他都固定,如背景和文字反色不反色的颜色,反色框和字的位置等
*/
void DisplayText(XlibMenu * menu, int shellindex, int line_y, int fontHeight)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginLeft = sc->skinMenu.marginLeft;
    int marginRight = sc->skinMenu.marginRight;
    cairo_t *  cr;
    cr = cairo_create(menu->menu_cs);
    int dpi = sc->skinFont.respectDPI? menu->owner->dpi: 0;
    FCITX_UNUSED(dpi);

    SetFontContext(cr, menu->owner->menuFont, sc->skinFont.menuFontSize, dpi);

    if (GetMenuItem(menu->menushell, shellindex)->isselect == 0) {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);

        OutputStringWithContext(cr, dpi, GetMenuItem(menu->menushell, shellindex)->tipstr, 15 + marginLeft , line_y);
    } else {
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        fcitx_cairo_set_color(cr, &sc->skinMenu.activeColor);
        cairo_rectangle(cr, marginLeft , line_y, menu->width - marginRight - marginLeft, fontHeight + 4);
        cairo_fill(cr);

        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
        OutputStringWithContext(cr, dpi, GetMenuItem(menu->menushell, shellindex)->tipstr , 15 + marginLeft , line_y);
    }
    ResetFontContext();
    cairo_destroy(cr);
}

void DrawArrow(XlibMenu* menu, int line_y, int i)
{
    FcitxSkin *sc = &menu->owner->skin;
    int marginRight = sc->skinMenu.marginRight;
    cairo_t* cr = cairo_create(menu->menu_cs);
    double size = sc->skinFont.menuFontSize * 0.4;
    double offset = (sc->skinFont.menuFontSize - size) / 2;
    if (GetMenuItem(menu->menushell, i)->isselect == 0) {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);
    } else {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_move_to(cr, menu->width - marginRight - 1 - size, line_y + offset);
    cairo_line_to(cr, menu->width - marginRight - 1 - size, line_y + size * 2 + offset);
    cairo_line_to(cr, menu->width - marginRight - 1, line_y + size + offset);
    cairo_line_to(cr, menu->width - marginRight - 1 - size , line_y + offset);
    cairo_fill(cr);
    cairo_destroy(cr);
}

/**
*返回鼠标指向的菜单在menu中是第多少项
*/
int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth)
{
    FcitxSkin *sc = &menu->owner->skin;
    int i;
    int winheight = sc->skinMenu.marginTop;
    int fontheight;
    int marginLeft = sc->skinMenu.marginLeft;
    int dpi = sc->skinFont.respectDPI? menu->owner->dpi: 0;

    if (x < marginLeft)
        return -1;

    fontheight = FontHeight(menu->owner->menuFont, sc->skinFont.menuFontSize, dpi);
    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU) {
            if (y > winheight + 1 && y < winheight + 6 + fontheight - 1) {
                if (offseth)
                    *offseth = winheight;
                return i;
            }
            winheight = winheight + 6 + fontheight;
        } else if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_DIVLINE)
            winheight += 5;
    }
    return -1;
}

boolean ReverseColor(XlibMenu * menu, int shellIndex)
{
    boolean flag = false;
    int i;

    int last = -1;

    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->isselect)
            last = i;

        GetMenuItem(menu->menushell, i)->isselect = 0;
    }
    if (shellIndex == last)
        flag = true;
    if (shellIndex >= 0 && shellIndex < utarray_len(&menu->menushell->shell))
        GetMenuItem(menu->menushell, shellIndex)->isselect = 1;
    return flag;
}

void ClearSelectFlag(XlibMenu * menu)
{
    int i;
    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        GetMenuItem(menu->menushell, i)->isselect = 0;
    }
}

void ReloadXlibMenu(void* arg, boolean enabled)
{
    XlibMenu* menu = (XlibMenu*) arg;
    boolean visable = WindowIsVisable(menu->owner->dpy, menu->menuWindow);
    cairo_surface_destroy(menu->menu_cs);
    cairo_surface_destroy(menu->menu_x_cs);
    XDestroyWindow(menu->owner->dpy, menu->menuWindow);

    menu->menu_cs = NULL;
    menu->menu_x_cs = NULL;
    menu->menuWindow = None;

    InitXlibMenu(menu);
    if (visable)
        XMapWindow(menu->owner->dpy, menu->menuWindow);
}

void CalMenuWindowPosition(XlibMenu *menu, int x, int y, int dodgeHeight)
{
    FcitxClassicUI *classicui = menu->owner;
    FcitxRect rect = GetScreenGeometry(classicui, x, y);

    if (x < rect.x1)
        menu->iPosX = rect.x1;
    else
        menu->iPosX = x;

    if (y < rect.y1)
        menu->iPosY = rect.y1;
    else
        menu->iPosY = y + dodgeHeight;

    if ((menu->iPosX + menu->width) > rect.x2)
        menu->iPosX =  rect.x2 - menu->width;

    if ((menu->iPosY + menu->height) >  rect.y2) {
        if (menu->iPosY >  rect.y2)
            menu->iPosY =  rect.y2 - menu->height;
        else /* better position the window */
            menu->iPosY = menu->iPosY - menu->height - dodgeHeight;
    }
}

void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth)
{
    int dwidth, dheight;
    FcitxSkin *sc = &parent->owner->skin;
    GetScreenSize(parent->owner, &dwidth, &dheight);
    FcitxMenuUpdate(sub->menushell);
    GetMenuSize(sub);
    sub->iPosX = parent->iPosX + parent->width - sc->skinMenu.marginRight - 4;
    sub->iPosY = parent->iPosY + offseth - sc->skinMenu.marginTop;

    if (sub->iPosX + sub->width > dwidth)
        sub->iPosX = parent->iPosX - sub->width + sc->skinMenu.marginLeft + 4;

    if (sub->iPosY + sub->height > dheight)
        sub->iPosY = dheight - sub->height;

    XMoveWindow(parent->owner->dpy, sub->menuWindow, sub->iPosX, sub->iPosY);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
