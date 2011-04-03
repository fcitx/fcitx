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
#include <cairo.h>
#include <limits.h>
#include <libintl.h>
#include <errno.h>

#include "core/fcitx.h"
#include "core/ui.h"
#include "core/module.h"
#include "module/x11/x11stuff.h"

#include "classicui.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"



struct FcitxSkin;

typedef struct FcitxClassicUIStatus {
    MouseE mouse;
    cairo_surface_t* active;
    cairo_surface_t* inactive;
} FcitxClassicUIStatus;

FcitxClassicUI classicui;

#define GetPrivateStatus(status) ((FcitxClassicUIStatus*)(status)->priv)

static boolean ClassicUIInit();
static void ClassicUICloseInputWindow();
static void ClassicUIShowInputWindow();
static void ClassicUIMoveInputWindow();
static void ClassicUIUpdateStatus();
static void ClassicUIRegisterStatus(FcitxUIStatus* status);
static void ClassicUIOnInputFocus();
static ConfigFileDesc* GetClassicUIDesc();

static void LoadClassicUIConfig();
static void SaveClassicUIConfig();

FCITX_EXPORT_API
FcitxUI ui = {
    ClassicUIInit,
    ClassicUICloseInputWindow,
    ClassicUIShowInputWindow,
    ClassicUIMoveInputWindow,
    ClassicUIUpdateStatus,
    ClassicUIRegisterStatus,
    ClassicUIOnInputFocus
};

boolean ClassicUIInit()
{
    FcitxModuleFunctionArg arg;
    classicui.dpy = InvokeFunction(FCITX_X11, GETDISPLAY, arg);
    
    XLockDisplay(classicui.dpy);
    
    classicui.iScreen = DefaultScreen(classicui.dpy);
    
    classicui.protocolAtom = XInternAtom (classicui.dpy, "WM_PROTOCOLS", False);
    classicui.killAtom = XInternAtom (classicui.dpy, "WM_DELETE_WINDOW", False);
    classicui.windowTypeAtom = XInternAtom (classicui.dpy, "_NET_WM_WINDOW_TYPE", False);
    classicui.typeMenuAtom = XInternAtom (classicui.dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    classicui.typeDialogAtom = XInternAtom (classicui.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    
    InitComposite();
    LoadClassicUIConfig();
    LoadSkinConfig(&classicui.skin, &classicui.skinType);
    classicui.skin.skinType = &classicui.skinType;

    classicui.inputWindow = CreateInputWindow(classicui.dpy, classicui.iScreen, &classicui.skin, classicui.font);
    
    XUnlockDisplay(classicui.dpy);
    return true;
}

static void ClassicUICloseInputWindow()
{
}

static void ClassicUIShowInputWindow()
{
    ShowInputWindowInternal(classicui.inputWindow);
}

static void ClassicUIMoveInputWindow()
{
    MoveInputWindowInternal(classicui.inputWindow);
}

static void ClassicUIUpdateStatus()
{
}

static void ClassicUIRegisterStatus(FcitxUIStatus* status)
{
    FcitxSkin* sc = &classicui.skin;
    status->priv = malloc(sizeof(FcitxClassicUIStatus));
    FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
    char activename[PATH_MAX], inactivename[PATH_MAX];
    sprintf(activename, "%s_active.png", status->name);
    sprintf(inactivename, "%s_inactive.png", status->name);
    
    LoadImage(activename , *sc->skinType, &privstat->active, False);
    LoadImage(inactivename, *sc->skinType, &privstat->inactive, False);
}

static void ClassicUIOnInputFocus()
{
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

void GetScreenSize(Display* dpy, int iScreen, int *width, int *height)
{
    XWindowAttributes attrs;
    if (XGetWindowAttributes(classicui.dpy, RootWindow(dpy, iScreen), &attrs) < 0) {
        printf("ERROR\n");
    }
    if (width != NULL)
        (*width) = attrs.width;
    if (height != NULL)
        (*height) = attrs.height;
}

void InitComposite()
{
    classicui.compManagerAtom = XInternAtom (classicui.dpy, "_NET_WM_CM_S0", False);

    classicui.compManager = XGetSelectionOwner(classicui.dpy, classicui.compManagerAtom);

    if (classicui.compManager)
    {
        XSetWindowAttributes attrs;
        attrs.event_mask = StructureNotifyMask;
        XChangeWindowAttributes (classicui.dpy, classicui.compManager, CWEventMask, &attrs);
    }
}


ConfigFileDesc* GetClassicUIDesc()
{    
    static ConfigFileDesc * classicUIDesc = NULL;
    if (!classicUIDesc)
    {
        FILE *tmpfp;
        tmpfp = GetXDGFileData("fcitx-classic-ui.desc", "r", NULL);
        classicUIDesc = ParseConfigFileDescFp(tmpfp);
        fclose(tmpfp);
    }

    return classicUIDesc;
}

void LoadClassicUIConfig()
{
    FILE *fp;
    char *file;
    fp = GetXDGFileUser( "fcitx-classic-ui.config", "rt", &file);
    FcitxLog(INFO, _("Load Config File %s"), file);
    free(file);
    if (!fp) {
        if (errno == ENOENT)
        {
            SaveClassicUIConfig();
            LoadClassicUIConfig();
        }
        return;
    }
    
    ConfigFileDesc* configDesc = GetClassicUIDesc();
    ConfigFile *cfile = ParseConfigFileFp(fp, configDesc);
    
    FcitxClassicUIConfigBind(&classicui, cfile, configDesc);
    ConfigBindSync((GenericConfig*)&classicui);

    fclose(fp);

}

void SaveClassicUIConfig()
{
    ConfigFileDesc* configDesc = GetClassicUIDesc();
    char *file;
    FILE *fp = GetXDGFileUser("fcitx-classic-ui.config", "wt", &file);
    FcitxLog(INFO, "Save Config to %s", file);
    SaveConfigFileFp(fp, classicui.gconfig.configFile, configDesc);
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

Visual * FindARGBVisual (Display *dpy, int scr)
{
    XVisualInfo *xvi;
    XVisualInfo temp;
    int         nvi;
    int         i;
    XRenderPictFormat   *format;
    Visual      *visual;

    if (classicui.compManager == None)
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

/*
 * 判断鼠标点击处是否处于指定的区域内
 */
boolean
IsInBox(int x0, int y0, int x1, int y1, int x2, int y2)
{
    if (x0 >= x1 && x0 <= x2 && y0 >= y1 && y0 <= y2)
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


/*
*把鼠标状态初始化为某一种状态.
*/
Bool SetMouseStatus()
{
    /* TODO 
    Bool changed = False;
    int i = 0;
    for (i = 0 ;i < 8; i ++)
    {
        MouseE obj;
        if (msE[i] == e)
            obj = s;
        else
            obj = m;
        
        if (obj != *msE[i])
            changed = True;
        
        *msE[i] = obj;
    }
    
    return changed;*/
    return True;
}
#if 0

/*
 * 有关界面的消息都在这里处理
 *     有关tray重画的问题，此处的解决方案似乎很dirt
 */
void
MyXEventHandler(XEvent * event)
{
    int dwidth, dheight;
    GetScreenSize(&dwidth, &dheight);
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
        else if (event->xclient.data.l[1] == compManagerAtom)
            DisplaySkin(fc.skinType);
#ifdef _ENABLE_TRAY
        else
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
        if (event->xany.window == compManager)
            DisplaySkin(fc.skinType);
#ifdef _ENABLE_TRAY
        else
            TrayEventHandler(event);
#endif
        break;
    case ButtonPress:
        switch (event->xbutton.button) {
        case Button1:
            SetMouseStatus(RELEASE, NULL, 0);
            if (event->xbutton.window == inputWindow.window) {
                int             x,
                                y;
                x = event->xbutton.x;
                y = event->xbutton.y;
                MouseClick(&x, &y, inputWindow.window);
                
                if(!fcitxProfile.bTrackCursor)
                {
                    fcitxProfile.iInputWindowOffsetX = x;
                    fcitxProfile.iInputWindowOffsetY = y;
                }   
                
                if (CurrentIC)
                {
                    Window window = None, dst;
                    if (CurrentIC->focus_win)
                        window = CurrentIC->focus_win;
                    else if(CurrentIC->client_win)
                        window = CurrentIC->client_win;
                    
                    if (window != None)
                    {
                        XTranslateCoordinates(dpy, RootWindow(dpy, iScreen), window,
                            x, y,
                            &CurrentIC->offset_x, &CurrentIC->offset_y,
                            &dst
                        );
                    }
                }
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
                        if (GetCurrentState() != IS_CHN) {
                            SetIMState((GetCurrentState() == IS_ENG) ? False : True);
                        }
                    }
                    DrawMainWindow();

#ifdef _ENABLE_TRAY
                    if (GetCurrentState() == IS_CHN)
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
                SetMouseStatus(RELEASE, NULL, 0);
            }
#ifdef _ENABLE_TRAY
            else if (event->xbutton.window == tray.window) {
                SetIMState((GetCurrentState() == IS_ENG) ? False : True);
                DrawMainWindow();
                if (GetCurrentState() == IS_CHN)
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
                XWithdrawWindow(dpy, messageWindow.window, iScreen);
            }
            // added by yunfan
            else if (event->xbutton.window == aboutWindow.window) {
                XWithdrawWindow(dpy, aboutWindow.window, iScreen);
                DrawMainWindow();
            } else if (event->xbutton.window == mainMenu.menuWindow) {
                int             i;
                i = SelectShellIndex(&mainMenu, event->xbutton.x, event->xbutton.y, NULL);
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
                    CloseAllIM();
                    exit(0);
                }
            } else if (event->xbutton.window == imMenu.menuWindow) {
                int idx = SelectShellIndex(&imMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (idx >= 0)
                {
                    SelectIM(idx);
                    ClearSelectFlag(&mainMenu);
                    XUnmapWindow(dpy, imMenu.menuWindow);
                    XUnmapWindow(dpy, mainMenu.menuWindow);
                }
            } else if (event->xbutton.window == skinMenu.menuWindow) {
                // 皮肤切换在此进行
                int             i;
                i = SelectShellIndex(&skinMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (i >= 0)
                {
                    char **sskin = (char**) utarray_eltptr(skinBuf, i);
                    if (strcmp(fc.skinType, *sskin) != 0) {
                        ClearSelectFlag(&mainMenu);
                        XUnmapWindow(dpy, mainMenu.menuWindow);
                        XUnmapWindow(dpy, vkMenu.menuWindow);
                        XUnmapWindow(dpy, imMenu.menuWindow);
                        XUnmapWindow(dpy, skinMenu.menuWindow);
                        DisplaySkin(*sskin);
                        SaveConfig();
                    }
                }
            } else if (event->xbutton.window == vkMenu.menuWindow) {
                int idx = SelectShellIndex(&vkMenu, event->xbutton.x, event->xbutton.y, NULL);
                if (idx >= 0)
                {
                    SelectVK(idx);
                    ClearSelectFlag(&mainMenu);
                    XUnmapWindow(dpy, mainMenu.menuWindow);
                    XUnmapWindow(dpy, vkMenu.menuWindow);
                }
            }
            // ****************************
            SaveProfile();
            break;

        case Button3:

            if (event->xbutton.window == mainMenu.menuWindow) {
                ClearSelectFlag(&mainMenu);
                XUnmapWindow(dpy, mainMenu.menuWindow);
                XUnmapWindow(dpy, vkMenu.menuWindow);
                XUnmapWindow(dpy, imMenu.menuWindow);
                XUnmapWindow(dpy, skinMenu.menuWindow);
            }
#ifdef _ENABLE_TRAY
            else if (event->xbutton.window == tray.window) {

                LoadSkinDirectory();

                if (event->xbutton.x_root - event->xbutton.x +
                    mainMenu.width >= dwidth)
                    mainMenu.iPosX =
                        dwidth - mainMenu.width -
                        event->xbutton.x;
                else
                    mainMenu.iPosX =
                        event->xbutton.x_root - event->xbutton.x;

                // 面板的高度是可以变动的，需要取得准确的面板高度，才能准确确定右键菜单位置。
                if (event->xbutton.y_root + mainMenu.height -
                    event->xbutton.y >= dheight)
                    mainMenu.iPosY =
                        dheight - mainMenu.height -
                        event->xbutton.y - 15;
                else
                    mainMenu.iPosY = event->xbutton.y_root - event->xbutton.y + 25;     // +sc.skin_tray_icon.active_img.height;

                DrawXlibMenu(dpy, &mainMenu);
                DisplayXlibMenu(dpy, &mainMenu);
            }
#endif
            else if (event->xbutton.window == mainWindow.window) {
                LoadSkinDirectory();

                mainMenu.iPosX = fcitxProfile.iMainWindowOffsetX;
                mainMenu.iPosY =
                    fcitxProfile.iMainWindowOffsetY +
                    sc.skinMainBar.backImg.height + 5;
                if ((mainMenu.iPosY + mainMenu.height) >
                    dheight)
                    mainMenu.iPosY = fcitxProfile.iMainWindowOffsetY - 5 - mainMenu.height;

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
                SetMouseStatus(RELEASE, NULL, 0);
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
                        if (GetCurrentState() != IS_CHN) {
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
        if (GetCurrentState() == IS_CHN)
            DisplayInputWindow();
        if (fc.hideMainWindow != HM_HIDE)
            XMapRaised(dpy, mainWindow.window);
        break;
    default:
        break;

    case MotionNotify:
        if (event->xany.window == mainWindow.window) {
            MouseE *mouseOn = NULL;
            if (IsInRspArea
                (event->xbutton.x, event->xbutton.y,
                 sc.skinMainBar.logo)) {
                mouseOn = &ms_logo;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.zhpunc)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.enpunc)) {
                mouseOn = &ms_punc;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.halfcorner)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.
                                   fullcorner)) {
                mouseOn = &ms_corner;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.nolegend)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.legend)) {
                mouseOn = &ms_lx;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.chs)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.cht)) {
                mouseOn = &ms_chs;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.unlock)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.lock)) {
                mouseOn = &ms_lock;
            } else
                if (IsInRspArea
                    (event->xbutton.x, event->xbutton.y,
                     sc.skinMainBar.novk)
                    || IsInRspArea(event->xbutton.x, event->xbutton.y,
                                   sc.skinMainBar.vk)) {
                mouseOn = &ms_vk;
            } else if (!fcitxProfile.bCorner
                       && IsInRspArea(event->xbutton.x, event->xbutton.y,
                                      sc.skinMainBar.
                                      chn)) {
                mouseOn = &ms_py;
            }
            if (SetMouseStatus(RELEASE, mouseOn, MOTION))
                DrawMainWindow();
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
        if  (event->xcrossing.window == mainWindow.window) {
            if (SetMouseStatus(RELEASE, NULL , 0))
                DrawMainWindow();
        }
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
                ClearSelectFlag(&mainMenu);
                XUnmapWindow(dpy, mainMenu.menuWindow);
                XUnmapWindow(dpy, vkMenu.menuWindow);
                XUnmapWindow(dpy, imMenu.menuWindow);
                XUnmapWindow(dpy, skinMenu.menuWindow);
            }
        }
        break;
    }
}

#endif