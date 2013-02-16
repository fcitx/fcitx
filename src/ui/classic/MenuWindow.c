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
#include "MainWindow.h"
#include "TrayWindow.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"

#define MENU_WINDOW_WIDTH   100
#define MENU_WINDOW_HEIGHT  100

static boolean ReverseColor(XlibMenu * Menu, int shellIndex);
static void XlibMenuMoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth);
static boolean MenuWindowEventHandler(void *arg, XEvent* event);
static int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth);
static void CloseAllMenuWindow(FcitxClassicUI *classicui);
static void CloseAllSubMenuWindow(XlibMenu *xlibMenu);
static void CloseOtherSubMenuWindow(XlibMenu *xlibMenu, XlibMenu* subMenu);
static boolean IsMouseInOtherMenu(XlibMenu *xlibMenu, int x, int y);
static void XlibMenuInit(XlibMenu* menu);
static void XlibMenuMoveWindow(FcitxXlibWindow* window);
static void XlibMenuCalculateContentSize(FcitxXlibWindow* window, unsigned int* width, unsigned int* height);
static void XlibMenuReload(void* arg, boolean enabled);
static void XlibMenuPaint(FcitxXlibWindow* window, cairo_t* c);
static void XlibMenuHide(XlibMenu * menu);

#define GetMenuItem(m, i) ((FcitxMenuItem*) utarray_eltptr(&(m)->shell, (i)))

void XlibMenuInit(XlibMenu* menu)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &classicui->skin;

    FcitxXlibWindowInit(&menu->parent,
                        MENU_WINDOW_WIDTH, MENU_WINDOW_HEIGHT,
                        0, 0,
                        "Fcitx Menu Window",
                        FCITX_WINDOW_MENU,
                        &menu->parent.owner->skin.skinMenu.background,
                        KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask,
                        XlibMenuMoveWindow,
                        XlibMenuCalculateContentSize,
                        XlibMenuPaint
    );

    int dpi = sc->skinFont.respectDPI? classicui->dpi: 0;

    FcitxCairoTextContext* ctc = FcitxCairoTextContextCreate(NULL);
    FcitxCairoTextContextSet(ctc, classicui->menuFont, sc->skinFont.menuFontSize, dpi);
    menu->fontheight = FcitxCairoTextContextFontHeight(ctc);
    FcitxCairoTextContextFree(ctc);


    menu->iPosX = 100;
    menu->iPosY = 100;
}


XlibMenu* MainMenuWindowCreate(FcitxClassicUI *classicui)
{
    XlibMenu* menu = XlibMenuCreate(classicui);
    menu->menushell = &classicui->mainMenu;

    return menu;
}

void XlibMenuShow(XlibMenu* menu)
{
    if (!menu->visible) {
        FcitxMenuUpdate(menu->menushell);
    }
    FcitxXlibWindowPaint(&menu->parent);
    if (!menu->visible) {
        XMapRaised(menu->parent.owner->dpy, menu->parent.wId);
    }
    menu->visible = true;
}

void XlibMenuHide(XlibMenu* menu)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxClassicUI* classicui = window->owner;
    menu->visible = false;
    XUnmapWindow(classicui->dpy, window->wId);
}

boolean MenuWindowEventHandler(void *arg, XEvent* event)
{
    XlibMenu* menu = (XlibMenu*) arg;
    FcitxXlibWindow* window = arg;
    FcitxClassicUI* classicui = window->owner;
    if (event->xany.window == window->wId) {
        switch (event->type) {
        case Expose:
            FcitxXlibWindowPaint(&menu->parent);
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
            int i = SelectShellIndex(menu, event->xmotion.x, event->xmotion.y, &offseth);
            boolean flag = ReverseColor(menu, i);
            FcitxMenuItem *item = GetMenuItem(menu->menushell, i);
            if (!flag) {
                XlibMenuShow(menu);
                if (item && item->type == MENUTYPE_SUBMENU && item->subMenu) {
                    XlibMenu* subxlibmenu = (XlibMenu*) item->subMenu->uipriv[classicui->isfallback];
                    CloseOtherSubMenuWindow(menu, subxlibmenu);
                    XlibMenuMoveSubMenu(subxlibmenu, menu, offseth);
                    XlibMenuShow(subxlibmenu);
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
                        CloseAllMenuWindow(classicui);
                }
            }
            break;
            case Button3:
                CloseAllMenuWindow(classicui);
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
        XlibMenuHide(xlibMenu);
    }
    XlibMenuHide(classicui->mainMenuWindow);
}

void CloseOtherSubMenuWindow(XlibMenu *xlibMenu, XlibMenu* subMenu)
{
    FcitxClassicUI* classicui = xlibMenu->parent.owner;
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
    FcitxClassicUI* classicui = xlibMenu->parent.owner;
    FcitxMenuItem *menu;
    for (menu = (FcitxMenuItem *) utarray_front(&xlibMenu->menushell->shell);
            menu != NULL;
            menu = (FcitxMenuItem *) utarray_next(&xlibMenu->menushell->shell, menu)
        ) {
        if (menu->type == MENUTYPE_SUBMENU && menu->subMenu) {
            CloseAllSubMenuWindow((XlibMenu *)menu->subMenu->uipriv[classicui->isfallback]);
        }
    }
    XlibMenuHide(xlibMenu);
}

boolean IsMouseInOtherMenu(XlibMenu *xlibMenu, int x, int y)
{
    FcitxClassicUI *classicui = xlibMenu->parent.owner;
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
        XGetWindowAttributes(classicui->dpy, otherXlibMenu->parent.wId, &attr);
        if (attr.map_state != IsUnmapped &&
                FcitxUIIsInBox(x, y, attr.x, attr.y, attr.width, attr.height)) {
            return true;
        }
    }

    XlibMenu* otherXlibMenu = classicui->mainMenuWindow;
    if (otherXlibMenu == xlibMenu)
        return false;
    XWindowAttributes attr;
    XGetWindowAttributes(classicui->dpy, otherXlibMenu->parent.wId, &attr);
    if (attr.map_state != IsUnmapped &&
            FcitxUIIsInBox(x, y, attr.x, attr.y, attr.width, attr.height)) {
        return true;
    }
    return false;
}

XlibMenu* XlibMenuCreate(FcitxClassicUI *classicui)
{
    XlibMenu *menu = FcitxXlibWindowCreate(classicui, sizeof(XlibMenu));
    XlibMenuInit(menu);

    FcitxX11AddXEventHandler(classicui->owner, MenuWindowEventHandler, menu);
    FcitxX11AddCompositeHandler(classicui->owner, XlibMenuReload, menu);
    return menu;
}

void XlibMenuCalculateContentSize(FcitxXlibWindow* window, unsigned int* width, unsigned int* height)
{
    XlibMenu* menu = (XlibMenu*) window;
    FcitxClassicUI* classicui = window->owner;
    int i = 0;
    int winheight = 0;
    int menuwidth = 0;
    FcitxSkin *sc = &classicui->skin;
    int dpi = sc->skinFont.respectDPI? classicui->dpi: 0;

    FcitxCairoTextContext* ctc = FcitxCairoTextContextCreate(NULL);
    FcitxCairoTextContextSet(ctc, classicui->menuFont, sc->skinFont.menuFontSize, dpi);

    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU)
            winheight += 6 + menu->fontheight;
        else if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_DIVLINE)
            winheight += 5;

        int width = FcitxCairoTextContextStringWidth(ctc, GetMenuItem(menu->menushell, i)->tipstr);
        if (width > menuwidth)
            menuwidth = width;
    }

    FcitxCairoTextContextFree(ctc);

    /* region for mark and arrow */
    menuwidth += 15 + 20;

    window->contentWidth = menuwidth;

    *height = winheight;
    *width = menuwidth;
}

void XlibMenunPaintDivLine(XlibMenu * menu, cairo_t* cr, int line_y)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &classicui->skin;
    cairo_save(cr);
    fcitx_cairo_set_color(cr, &sc->skinMenu.lineColor);
    cairo_set_line_width(cr, 2);
    cairo_move_to(cr, 3, line_y + 3);
    cairo_line_to(cr, window->contentWidth - 3, line_y + 3);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void XlibMenuPaintMark(XlibMenu * menu, cairo_t* cr, int y, int i)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxSkin* sc = &window->owner->skin;
    double size = (sc->skinFont.menuFontSize * 0.7) / 2;
    cairo_save(cr);
    if (GetMenuItem(menu->menushell, i)->isselect == 0) {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);
    } else {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_translate(cr, 7, y + (sc->skinFont.menuFontSize / 2.0));
    cairo_arc(cr, 0, 0, size , 0., 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);
}

/*
* 显示菜单上面的文字信息,只需要指定窗口,窗口宽度,需要显示文字的上边界,字体,显示的字符串和是否选择(选择后反色)
* 其他都固定,如背景和文字反色不反色的颜色,反色框和字的位置等
*/
void XlibMenuPaintText(XlibMenu * menu, cairo_t* c, FcitxCairoTextContext* ctc, int shellindex, int line_y, int fontHeight)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &classicui->skin;

    cairo_save(c);
    if (GetMenuItem(menu->menushell, shellindex)->isselect == 0) {
        FcitxCairoTextContextOutputString(ctc, GetMenuItem(menu->menushell, shellindex)->tipstr, 15 , line_y, &sc->skinFont.menuFontColor[MENU_INACTIVE]);
    } else {
        cairo_set_operator(c, CAIRO_OPERATOR_OVER);
        fcitx_cairo_set_color(c, &sc->skinMenu.activeColor);
        cairo_rectangle(c, 0 , line_y, window->contentWidth, fontHeight + 4);
        cairo_fill(c);

        FcitxCairoTextContextOutputString(ctc, GetMenuItem(menu->menushell, shellindex)->tipstr, 15 , line_y, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_restore(c);
}

void XlibMenuPaintArrow(XlibMenu* menu, cairo_t* cr, int line_y, int i)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &classicui->skin;

    double size = sc->skinFont.menuFontSize * 0.4;
    double offset = (sc->skinFont.menuFontSize - size) / 2;
    cairo_save(cr);
    if (GetMenuItem(menu->menushell, i)->isselect == 0) {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_INACTIVE]);
    } else {
        fcitx_cairo_set_color(cr, &sc->skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_move_to(cr, window->contentWidth - 1 - size, line_y + offset);
    cairo_line_to(cr, window->contentWidth - 1 - size, line_y + size * 2 + offset);
    cairo_line_to(cr, window->contentWidth - 1, line_y + size + offset);
    cairo_line_to(cr, window->contentWidth - 1 - size , line_y + offset);
    cairo_fill(cr);
    cairo_restore(cr);
}

/**
*返回鼠标指向的菜单在menu中是第多少项
*/
int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    int i;
    int contentX = window->contentX;
    int contentY = window->contentY;

    if (x < contentX)
        return -1;

    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU) {
            if (y > contentY + 1 && y < contentY + 6 + menu->fontheight - 1) {
                if (offseth)
                    *offseth = contentY;
                return i;
            }
            contentY = contentY + 6 + menu->fontheight;
        } else if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_DIVLINE) {
            contentY += 5;
        }
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

void XlibMenuReload(void* arg, boolean enabled)
{
    FcitxXlibWindow* window = arg;
    FcitxClassicUI* classicui = window->owner;
    XlibMenu* menu = (XlibMenu*) arg;
    boolean visable = WindowIsVisable(classicui->dpy, window->wId);

    FcitxXlibWindowDestroy(window);

    XlibMenuInit(menu);
    if (visable)
        XMapWindow(classicui->dpy, window->wId);
}

void CalMenuWindowPosition(XlibMenu *menu, int x, int y, int dodgeHeight)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) menu;
    FcitxClassicUI *classicui = window->owner;
    FcitxRect rect = GetScreenGeometry(classicui, x, y);

    if (x < rect.x1)
        menu->iPosX = rect.x1;
    else
        menu->iPosX = x;

    if (y < rect.y1)
        menu->iPosY = rect.y1;
    else
        menu->iPosY = y + dodgeHeight;

    if ((menu->iPosX + window->width) > rect.x2)
        menu->iPosX =  rect.x2 - window->width;

    if ((menu->iPosY + window->height) >  rect.y2) {
        if (menu->iPosY >  rect.y2)
            menu->iPosY =  rect.y2 - window->height;
        else /* better position the window */
            menu->iPosY = menu->iPosY - window->height - dodgeHeight;
    }
}

void XlibMenuMoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth)
{
    sub->anchor = MA_Menu;
    sub->anchorMenu = parent;
    sub->offseth = offseth;
}

void XlibMenuMoveWindow(FcitxXlibWindow* window)
{
    XlibMenu* menu = (XlibMenu*) window;
    FcitxClassicUI* classicui = window->owner;

    int sheight;
    GetScreenSize(classicui, NULL, &sheight);

    if (menu->anchor == MA_MainWindow) {
        menu->iPosX = classicui->iMainWindowOffsetX;
        menu->iPosY = classicui->iMainWindowOffsetY + classicui->mainWindow->parent.height;
        if ((menu->iPosY + window->height) > sheight) {
            menu->iPosY = classicui->iMainWindowOffsetY - 5 - window->height;
        }
    } else if (menu->anchor == MA_Menu) {
        int dwidth, dheight;
        XlibMenu* parent = menu->anchorMenu;
        GetScreenSize(classicui, &dwidth, &dheight);
        menu->iPosX = parent->iPosX + parent->parent.contentX + parent->parent.contentWidth - 4;
        menu->iPosY = parent->iPosY + menu->offseth - window->contentY;

        if (menu->iPosX + window->width > dwidth) {
            menu->iPosX = parent->iPosX - window->width + parent->parent.contentX + 4;
        }

        if (menu->iPosY + window->height > dheight) {
            menu->iPosY = dheight - window->height;
        }

    } else if (menu->anchor == MA_Tray) {
        CalMenuWindowPosition(menu, menu->trayX, menu->trayY, classicui->trayWindow->size);
    }

    menu->anchor = MA_None;
    XMoveWindow(classicui->dpy, window->wId, menu->iPosX, menu->iPosY);
}

void XlibMenuPaint(FcitxXlibWindow* window, cairo_t* c)
{
    XlibMenu* menu = (XlibMenu*) window;
    FcitxClassicUI *classicui = window->owner;
    FcitxSkin *sc = &classicui->skin;
    int i = 0;
    int iPosY = 0;
    int dpi = sc->skinFont.respectDPI? classicui->dpi: 0;

    FcitxCairoTextContext* ctc = FcitxCairoTextContextCreate(c);
    FcitxCairoTextContextSet(ctc, classicui->menuFont, sc->skinFont.menuFontSize, dpi);

    for (i = 0; i < utarray_len(&menu->menushell->shell); i++) {
        if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SIMPLE || GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU) {
            XlibMenuPaintText(menu, c, ctc, i, iPosY, menu->fontheight);
            if (menu->menushell->mark == i)
                XlibMenuPaintMark(menu, c, iPosY, i);

            if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_SUBMENU)
                XlibMenuPaintArrow(menu, c, iPosY, i);
            iPosY = iPosY + 6 + menu->fontheight;
        } else if (GetMenuItem(menu->menushell, i)->type == MENUTYPE_DIVLINE) {
            XlibMenunPaintDivLine(menu, c, iPosY);
            iPosY += 5;
        }
    }
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
