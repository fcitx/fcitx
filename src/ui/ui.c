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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "core/fcitx.h"

#include <X11/Xlocale.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <iconv.h>

#include "ui/ui.h"
#include "core/xim.h"
#include "core/MyErrorsHandlers.h"
#include "ui/MainWindow.h"
#include "ui/MessageWindow.h"
#include "ui/InputWindow.h"
#include "ui/MenuWindow.h"
#include "im/special/vk.h"
#include "core/IC.h"
#include "tools/utf8.h"
#include "im/pinyin/sp.h"
#include "ui/AboutWindow.h"
#include "ui/TrayWindow.h"
#include "ui/skin.h"
#include "core/ime.h"
#include "fcitx-config/profile.h"
#include "fcitx-config/configfile.h"
#include "ui/AboutWindow.h"
#include "fcitx-config/cutils.h"

Display        *dpy;
int             iScreen;
extern int      iVKWindowX;
extern int      iVKWindowY;
extern Window   ximWindow;
extern VKWindow vkWindow;
extern Bool     bMainWindow_Hiden;
unsigned char   iCurrentVK;
extern CARD16   connect_id;
extern Bool     bVK;
extern Bool     bIsDisplaying;

Atom protocolAtom;
Atom killAtom;

// added by yunfan
// **********************************

Bool
InitX(void)
{
    if ((dpy = XOpenDisplay ((char *) NULL)) == NULL) {
        FcitxLog (ERROR, _("FCITX can only run under X"));
        return False;
    }

    SetMyXErrorHandler();
    iScreen = DefaultScreen(dpy);
    
    protocolAtom = XInternAtom (dpy, "WM_PROTOCOLS", False);
    killAtom = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
    return True;
}

/*
 * 有关界面的消息都在这里处理
 *     有关tray重画的问题，此处的解决方案似乎很dirt
 */
void
MyXEventHandler(XEvent * event)
{
    switch (event->type) {
    case ConfigureNotify:
#ifdef _ENABLE_TRAY
        TrayEventHandler(event);
#endif
        break;
    case ReparentNotify:
#ifdef _ENABLE_TRAY
        TrayEventHandler(event);
#endif
        break;
    case ClientMessage:
        if ((event->xclient.message_type == protocolAtom)
            && ((Atom) event->xclient.data.l[0] == killAtom)) {
            XUnmapWindow(dpy, event->xclient.window);
            DrawMainWindow();
        }
#ifdef _ENABLE_TRAY
        TrayEventHandler(event);
#endif
        break;
    case Expose:
#ifdef _DEBUG
    FcitxLog(DEBUG, _("XEvent--Expose"));
#endif
        if (event->xexpose.count > 0)
            break;
        if (event->xexpose.window == mainWindow.window)
            DrawMainWindow();
        else if (event->xexpose.window == vkWindow.window)
            DrawVKWindow();
        else if (event->xexpose.window == inputWindow.window)
            DrawInputWindow();
        else if (event->xexpose.window == mainMenu.menuWindow)
            DrawXlibMenu(dpy, &mainMenu);
        else if (event->xexpose.window == vkMenu.menuWindow) {
            if (iCurrentVK >= 0)
                vkMenu.mark = iCurrentVK;
            DrawXlibMenu(dpy, &vkMenu);
        } else if (event->xexpose.window == imMenu.menuWindow) {
            if (gs.iIMIndex >= 0)
                imMenu.mark = gs.iIMIndex;
            DrawXlibMenu(dpy, &imMenu);
        } else if (event->xexpose.window == skinMenu.menuWindow) {
            int             i = 0;
            for (i = 0;
                 i < skinBuf->i ;
                 i ++)
            {
                char **s = (char**)utarray_eltptr(skinBuf, i);
                if (strcmp(*s, fc.skinType) == 0)
                {
                    skinMenu.mark = i;
                    break;
                }
            }
            DrawXlibMenu(dpy, &skinMenu);
        }
#ifdef _ENABLE_TRAY
        else if (event->xexpose.window == tray.window) {
            TrayEventHandler(event);
        }
#endif
        // added by yunfan
        else if (event->xexpose.window == aboutWindow.window)
            DrawAboutWindow();
        // ******************************
        else if (event->xexpose.window == messageWindow.window)
            DrawMessageWindow(NULL, NULL, 0);
        break;
    case DestroyNotify:
#ifdef _ENABLE_TRAY
        TrayEventHandler(event);
#endif
        break;
    case ButtonPress:
        switch (event->xbutton.button) {
        case Button1:
            set_mouse_status(RELEASE);
            if (event->xbutton.window == inputWindow.window) {
                int             x,
                                y;
                x = event->xbutton.x;
                y = event->xbutton.y;
                MouseClick(&x, &y, inputWindow.window);
                DrawInputWindow();
            } else if (event->xbutton.window == mainWindow.window) {

                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.logo)) {
                    fcitxProfile.iMainWindowOffsetX = event->xbutton.x;
                    fcitxProfile.iMainWindowOffsetY = event->xbutton.y;
                    ms_logo = PRESS;
                    if (!MouseClick
                        (&fcitxProfile.iMainWindowOffsetX, &fcitxProfile.iMainWindowOffsetY, mainWindow.window)) {
                        if (ConnectIDGetState(connect_id) != IS_CHN) {
                            SetIMState((ConnectIDGetState(connect_id) ==
                                        IS_ENG) ? False : True);
                        }
                    }
                    DrawMainWindow();

#ifdef _ENABLE_TRAY
                    if (ConnectIDGetState(connect_id) == IS_CHN)
                        DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size,
                                       tray.size);
                    else
                        DrawTrayWindow(INACTIVE_ICON, 0, 0, tray.size,
                                       tray.size);
#endif
                } else
                    if (IsInRspArea
                        (event->xbutton.x, event->xbutton.y,
                         sc.skinMainBar.zhpunc)
                        || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                       sc.skinMainBar.
                                       enpunc)) {
                    ms_punc = PRESS;
                } else
                    if (IsInRspArea
                        (event->xbutton.x, event->xbutton.y,
                         sc.skinMainBar.halfcorner)
                        || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                       sc.skinMainBar.
                                       fullcorner)) {
                    ms_corner = PRESS;
                } else
                    if (IsInRspArea
                        (event->xbutton.x, event->xbutton.y,
                         sc.skinMainBar.nolegend)
                        || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                       sc.skinMainBar.
                                       legend)) {
                    ms_lx = PRESS;
                } else
                    if (IsInRspArea
                        (event->xbutton.x, event->xbutton.y,
                         sc.skinMainBar.chs)
                        || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                       sc.skinMainBar.
                                       cht)) {
                    ms_chs = PRESS;
                } else
                    if (IsInRspArea
                        (event->xbutton.x, event->xbutton.y,
                         sc.skinMainBar.unlock)
                        || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                       sc.skinMainBar.
                                       lock)) {
                    ms_lock = PRESS;
                } else
                    if (IsInRspArea
                        (event->xbutton.x, event->xbutton.y,
                         sc.skinMainBar.novk)
                        || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                       sc.skinMainBar.
                                       vk)) {
                    ms_vk = PRESS;
                } else if (!fcitxProfile.bCorner
                           && IsInRspArea(event->xbutton.x,
                                          event->xbutton.y,
                                          sc.skinMainBar.
                                          chs)) {
                    ms_py = PRESS;
                }
                DrawMainWindow();
                set_mouse_status(RELEASE);
            }
#ifdef _ENABLE_TRAY
            else if (event->xbutton.window == tray.window) {
                SetIMState((ConnectIDGetState(connect_id) ==
                            IS_ENG) ? False : True);
                DrawMainWindow();
                if (ConnectIDGetState(connect_id) == IS_CHN)
                    DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size,
                                   tray.size);
                else
                    DrawTrayWindow(INACTIVE_ICON, 0, 0, tray.size,
                                   tray.size);
            }
#endif
            else if (event->xbutton.window == vkWindow.window) {
                if (!VKMouseKey(event->xbutton.x, event->xbutton.y)) {
                    iVKWindowX = event->xbutton.x;
                    iVKWindowY = event->xbutton.y;
                    MouseClick(&iVKWindowX, &iVKWindowY, vkWindow.window);
                    DrawVKWindow();
                }
            }
            else if (event->xbutton.window == messageWindow.window)
            {
                XUnmapWindow(dpy, messageWindow.window);
            }
            // added by yunfan
            else if (event->xbutton.window == aboutWindow.window) {
                XUnmapWindow(dpy, aboutWindow.window);
                DrawMainWindow();
            } else if (event->xbutton.window == mainMenu.menuWindow) {
                int             i;
                i = selectShellIndex(&mainMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (i == 0) {
                    DisplayAboutWindow();
                    XUnmapWindow(dpy, mainMenu.menuWindow);
                    DrawMainWindow();
                }
                // fcitx配置工具接口
                else if (i == 6) {
                    XUnmapWindow(dpy, mainMenu.menuWindow);
                    pid_t id;

                    id = fork();

                    if (id < 0)
                        FcitxLog(ERROR, _("Unable to create process"));
                    else if (id == 0)
                    {
                        id = fork();

                        if (id < 0)
                        {
                            FcitxLog(ERROR, _("Unable to create process"));
                            exit(1);
                        }
                        else if (id > 0)
                            exit(0);

                        execl(BINDIR "/fcitx-config", "fcitx-config", NULL);

                        exit(0);
                    }
                } else if (i == 7) {
                    exit(0);
                }
            } else if (event->xbutton.window == imMenu.menuWindow) {
                int idx = selectShellIndex(&imMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (idx >= 0)
                {
                    SelectIM(idx);
                    clearSelectFlag(&mainMenu);
                    XUnmapWindow(dpy, imMenu.menuWindow);
                    XUnmapWindow(dpy, mainMenu.menuWindow);
                }
            } else if (event->xbutton.window == skinMenu.menuWindow) {
                // 皮肤切换在此进行
                int             i;
                i = selectShellIndex(&skinMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (i >= 0)
                {
                    char **sskin = (char**) utarray_eltptr(skinBuf, i);
                    if (strcmp(fc.skinType, *sskin) != 0) {
                        clearSelectFlag(&mainMenu);
                        XUnmapWindow(dpy, mainMenu.menuWindow);
                        XUnmapWindow(dpy, vkMenu.menuWindow);
                        XUnmapWindow(dpy, imMenu.menuWindow);
                        XUnmapWindow(dpy, skinMenu.menuWindow);
                        DisplaySkin(*sskin);
                        SaveConfig();
                    }
                }
            } else if (event->xbutton.window == vkMenu.menuWindow) {
                int idx = selectShellIndex(&vkMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (idx >= 0)
                {
                    SelectVK(idx);
                    clearSelectFlag(&mainMenu);
                    XUnmapWindow(dpy, mainMenu.menuWindow);
                    XUnmapWindow(dpy, vkMenu.menuWindow);
                }
            }
            // ****************************
            SaveProfile();
            break;

        case Button3:

            if (event->xbutton.window == mainMenu.menuWindow) {
                clearSelectFlag(&mainMenu);
                XUnmapWindow(dpy, mainMenu.menuWindow);
                XUnmapWindow(dpy, vkMenu.menuWindow);
                XUnmapWindow(dpy, imMenu.menuWindow);
                XUnmapWindow(dpy, skinMenu.menuWindow);
            }
#ifdef _ENABLE_TRAY
            else if (event->xbutton.window == tray.window) {

                loadSkinDir();

                GetMenuHeight(dpy, &mainMenu);

                if (event->xbutton.x_root - event->xbutton.x +
                    mainMenu.width >= DisplayWidth(dpy, iScreen))
                    mainMenu.pos_x =
                        DisplayWidth(dpy,
                                     iScreen) - mainMenu.width -
                        event->xbutton.x;
                else
                    mainMenu.pos_x =
                        event->xbutton.x_root - event->xbutton.x;

                // 面板的高度是可以变动的，需要取得准确的面板高度，才能准确确定右键菜单位置。
                if (event->xbutton.y_root + mainMenu.height -
                    event->xbutton.y >= DisplayHeight(dpy, iScreen))
                    mainMenu.pos_y =
                        DisplayHeight(dpy,
                                      iScreen) - mainMenu.height -
                        event->xbutton.y - 15;
                else
                    mainMenu.pos_y = event->xbutton.y_root - event->xbutton.y + 25;     // +sc.skin_tray_icon.active_img.height;

                /*
                 * printf("%d %d :%d %d :%d
                 * \n",event->xbutton.x,event->xbutton.y,event->xbutton.x_root,\
                 * event->xbutton.y_root,sc.skin_tray_icon.active_img.height);
                 */
                DrawXlibMenu(dpy, &mainMenu);
                DisplayXlibMenu(dpy, &mainMenu);
            }
#endif
            else if (event->xbutton.window == mainWindow.window) {
                loadSkinDir();

                GetMenuHeight(dpy, &mainMenu);

                mainMenu.pos_x = fcitxProfile.iMainWindowOffsetX;
                mainMenu.pos_y =
                    fcitxProfile.iMainWindowOffsetY +
                    sc.skinMainBar.backImg.height + 5;
                if ((mainMenu.pos_y + mainMenu.height) >
                    DisplayHeight(dpy, iScreen))
                    mainMenu.pos_y = fcitxProfile.iMainWindowOffsetY - 5 - mainMenu.height;

                DrawXlibMenu(dpy, &mainMenu);
                DisplayXlibMenu(dpy, &mainMenu);
            }
            break;
        }
        break;
    case ButtonRelease:
        if (event->xbutton.window == mainWindow.window) {
            switch (event->xbutton.button) {
            case Button1:
                set_mouse_status(RELEASE);
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.logo))
                    DrawMainWindow();
                else if (IsInRspArea
                         (event->xbutton.x, event->xbutton.y,
                          sc.skinMainBar.zhpunc)
                         || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.
                                        enpunc))
                    ChangePunc();
                else if (IsInRspArea
                         (event->xbutton.x, event->xbutton.y,
                          sc.skinMainBar.halfcorner)
                         || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.
                                        fullcorner))
                    ChangeCorner();
                else if (IsInRspArea
                         (event->xbutton.x, event->xbutton.y,
                          sc.skinMainBar.nolegend)
                         || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.
                                        legend))
                    ChangeLegend();
                else if (IsInRspArea
                         (event->xbutton.x, event->xbutton.y,
                          sc.skinMainBar.chs)
                         || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.cht))
                    ChangeGBKT();
                else if (IsInRspArea
                         (event->xbutton.x, event->xbutton.y,
                          sc.skinMainBar.unlock)
                         || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.
                                        lock))
                    ChangeLock();
                else if (IsInRspArea
                         (event->xbutton.x, event->xbutton.y,
                          sc.skinMainBar.novk)
                         || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.
                                        vk))
                    SwitchVK();
                else if (!fcitxProfile.bCorner
                         && IsInRspArea(event->xbutton.x, event->xbutton.y,
                                        sc.skinMainBar.
                                        chn)) {
                    SwitchIM(-1);
                }

                break;

                // added by yunfan
            case Button2:
                DisplayAboutWindow();
                break;
                // ********************
            case Button3:

#ifdef _ENABLE_TRAY
                if (event->xbutton.window == tray.window) {
                    switch (event->xbutton.button) {
                    case Button1:
                        if (ConnectIDGetState(connect_id) != IS_CHN) {
                            SetIMState(True);
                            DrawMainWindow();

                            DrawTrayWindow(ACTIVE_ICON, 0, 0, tray.size,
                                           tray.size);
                        } else {
                            SetIMState(False);
                            DrawTrayWindow(INACTIVE_ICON, 0, 0, tray.size,
                                           tray.size);
                        }

                        break;
                    case Button2:
                        if (IsWindowVisible(mainWindow.window)) {
                            bMainWindow_Hiden = True;
                            XUnmapWindow(dpy, mainWindow.window);
                        } else {
                            bMainWindow_Hiden = False;
                            DisplayMainWindow();
                            DrawMainWindow();
                        }
                        break;
                    }
                }
#endif
                break;
            }
        }
        break;
    case FocusIn:
        if (ConnectIDGetState(connect_id) == IS_CHN)
            DisplayInputWindow();
        if (fc.hideMainWindow != HM_HIDE)
            XMapRaised(dpy, mainWindow.window);
        break;
    default:
        break;

    case MotionNotify:
        if (event->xany.window == mainWindow.window) {
            if (IsInRspArea
                (event->xbutton.x, event->xbutton.y,
                 sc.skinMainBar.logo)) {
                ms_logo = MOTION;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.zhpunc)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.enpunc)) {
                ms_punc = MOTION;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.halfcorner)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.
                                   fullcorner)) {
                ms_corner = MOTION;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.nolegend)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.legend)) {
                ms_lx = MOTION;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.chs)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.cht)) {
                ms_chs = MOTION;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.unlock)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.lock)) {
                ms_lock = MOTION;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.novk)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.vk)) {
                ms_vk = MOTION;
            } else if (!fcitxProfile.bCorner
                       && IsInRspArea(event->xbutton.x, event->xbutton.y,
                                      sc.skinMainBar.
                                      chn)) {
                ms_py = MOTION;
            }
            DrawMainWindow();
            set_mouse_status(RELEASE);
        } else if (event->xany.window == mainMenu.menuWindow) {
            MainMenuEvent(event->xmotion.x, event->xmotion.y);
        } else if (event->xany.window == imMenu.menuWindow) {
            IMMenuEvent(event->xmotion.x, event->xmotion.y);
        } else if (event->xany.window == vkMenu.menuWindow) {
            VKMenuEvent(event->xmotion.x, event->xmotion.y);
        } else if (event->xany.window == skinMenu.menuWindow) {
            SkinMenuEvent(event->xmotion.x, event->xmotion.y);
        }
        break;
    case LeaveNotify:
        if  (event->xcrossing.window == mainMenu.menuWindow) {
            int x = event->xcrossing.x_root;
            int y = event->xcrossing.y_root;
            XWindowAttributes attr;
            int i;
            Bool flag = False;
            Window wins[3] = { vkMenu.menuWindow, skinMenu.menuWindow, imMenu.menuWindow };
            for (i = 0;i< 3; i++)
            {
                XGetWindowAttributes(dpy, wins[i], &attr);
                if (attr.map_state != IsUnmapped &&
                        IsInBox(x, y, attr.x, attr.y, attr.x + attr.width, attr.y + attr.height))
                {
                    flag = True;
                    break;
                }
            }
            if (!flag)
            {
                clearSelectFlag(&mainMenu);
                XUnmapWindow(dpy, mainMenu.menuWindow);
                XUnmapWindow(dpy, vkMenu.menuWindow);
                XUnmapWindow(dpy, imMenu.menuWindow);
                XUnmapWindow(dpy, skinMenu.menuWindow);
            }
        }
        break;
    }
}

/*
 * 判断鼠标点击处是否处于指定的区域内
 */
Bool
IsInBox(int x0, int y0, int x1, int y1, int x2, int y2)
{
    if (x0 >= x1 && x0 <= x2 && y0 >= y1 && y0 <= y2)
        return True;

    return False;
}

Bool
IsInRspArea(int x0, int y0, FcitxImage img)
{
    return IsInBox(x0, y0, img.response_x, img.response_y,
                   img.response_x + img.response_w,
                   img.response_y + img.response_h);
}

int
StringWidth(const char *str, const char *font, int fontSize)
{
    if (!str || str[0] == 0)
        return 0;
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);
    cairo_select_font_face(c, font, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(c, fontSize);

    int             width = StringWidthWithContext(c, str);

    cairo_destroy(c);
    cairo_surface_destroy(surface);

    return width;
}

int
StringWidthWithContext(cairo_t * c, const char *str)
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

int
FontHeight(const char *font)
{
    cairo_surface_t *surface =
        cairo_image_surface_create(CAIRO_FORMAT_RGB24, 10, 10);
    cairo_t        *c = cairo_create(surface);
    cairo_select_font_face(c, font, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(c, sc.skinFont.fontSize);

    int             height = FontHeightWithContext(c);

    cairo_destroy(c);
    cairo_surface_destroy(surface);
    return height;
}

int
FontHeightWithContext(cairo_t * c)
{
    char            str[] = "Ay中";
    cairo_text_extents_t extents;
    cairo_text_extents(c, str, &extents);

    int             height = extents.height;
    return height;
}

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
    cairo_select_font_face(c, gs.fontZh,
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(c, fontSize);
    OutputStringWithContext(c, str, x, y);

    cairo_restore(c);
}

void
OutputStringWithContext(cairo_t * c, const char *str, int x, int y)
{
    if (!str || str[0] == 0)
        return;
    if (!utf8_check_string(str))
        return;
    cairo_save(c);
    cairo_move_to(c, x, y);
    cairo_show_text(c, str);
    cairo_restore(c);
}

Bool
IsWindowVisible(Window window)
{
    XWindowAttributes attrs;

    XGetWindowAttributes(dpy, window, &attrs);

    if (attrs.map_state == IsUnmapped)
        return False;

    return True;
}

Bool
MouseClick(int *x, int *y, Window window)
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

void
InitWindowAttribute(Visual ** vs, Colormap * cmap,
                    XSetWindowAttributes * attrib,
                    unsigned long *attribmask, int *depth)
{
    if (*vs) {
        *cmap =
            XCreateColormap(dpy, RootWindow(dpy, iScreen), *vs, AllocNone);

        attrib->override_redirect = True;       // False;
        attrib->background_pixel = 0;
        attrib->border_pixel = 0;
        attrib->colormap = *cmap;
        *attribmask =
            (CWBackPixel | CWBorderPixel | CWOverrideRedirect |
             CWColormap);
        *depth = 32;
    } else {
        *cmap = DefaultColormap(dpy, iScreen);
        *vs = DefaultVisual(dpy, iScreen);
        attrib->override_redirect = True;       // False;
        attrib->background_pixel = WhitePixel(dpy, iScreen);
        attrib->border_pixel = BlackPixel(dpy, iScreen);
        *attribmask = (CWBackPixel | CWBorderPixel | CWOverrideRedirect);
        *depth = DefaultDepth(dpy, iScreen);
    }
}

void ActiveWindow(Display *dpy, Window window)
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
