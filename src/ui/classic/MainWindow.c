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
/**
 * @file   MainWindow.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  主窗口
 *
 *
 */

#include <cairo-xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <X11/Xatom.h>
#include <limits.h>
#include <libintl.h>

#include "fcitx/frontend.h"
#include "fcitx/module.h"
#include "fcitx/instance.h"

#include "MainWindow.h"
#include "fcitx-utils/log.h"
#include "classicui.h"
#include "skin.h"
#include "MenuWindow.h"
#include "fcitx-utils/utils.h"

#define FCITX_MAX(a,b) ((a) > (b)?(a) : (b))

#define MAIN_BAR_MAX_WIDTH 2
#define MAIN_BAR_MAX_HEIGHT 2

static boolean MainWindowEventHandler(void *arg, XEvent* event);
static void UpdateStatusGeometry(FcitxClassicUIStatus *privstat, SkinImage *image, int x, int y);
static void ReloadMainWindow(void* arg, boolean enabled);
static void InitMainWindow(MainWindow* mainWindow);
static void UpdateMenuGeometry(MainWindow* mainWindow, XlibMenu* menuWindow);

void InitMainWindow(MainWindow* mainWindow)
{
    FcitxClassicUI* classicui = mainWindow->owner;
    int depth;
    Colormap cmap;
    Visual * vs;
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    char        strWindowName[] = "Fcitx Main Window";
    int swidth, sheight;
    Display* dpy = classicui->dpy;
    int iScreen = classicui->iScreen;
    FcitxSkin *sc = &classicui->skin;
    mainWindow->dpy = dpy;

    GetScreenSize(classicui, &swidth, &sheight);
    SkinImage *back = LoadImage(sc, sc->skinMainBar.backImg, false);

    vs = ClassicUIFindARGBVisual(classicui);

    if (classicui->iMainWindowOffsetX + cairo_image_surface_get_width(back->image) > swidth)
        classicui->iMainWindowOffsetX = swidth - cairo_image_surface_get_width(back->image);

    if (classicui->iMainWindowOffsetY + cairo_image_surface_get_height(back->image) > sheight)
        classicui->iMainWindowOffsetY = sheight - cairo_image_surface_get_height(back->image);

    ClassicUIInitWindowAttribute(classicui, &vs, &cmap, &attrib, &attribmask, &depth);
    mainWindow->window = XCreateWindow(dpy,
                                       RootWindow(dpy, iScreen),
                                       classicui->iMainWindowOffsetX,
                                       classicui->iMainWindowOffsetY,
                                       cairo_image_surface_get_width(back->image),
                                       cairo_image_surface_get_height(back->image),
                                       0, depth, InputOutput, vs, attribmask, &attrib);

    if (mainWindow->window == None)
        return;

    mainWindow->cs_x_main_bar = cairo_xlib_surface_create(
                                    dpy,
                                    mainWindow->window,
                                    vs,
                                    MAIN_BAR_MAX_WIDTH,
                                    MAIN_BAR_MAX_HEIGHT);
    mainWindow->cs_main_bar = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                         MAIN_BAR_MAX_WIDTH,
                                                         MAIN_BAR_MAX_HEIGHT);

    XChangeWindowAttributes(dpy, mainWindow->window, attribmask, &attrib);
    XSelectInput(dpy, mainWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask);

    ClassicUISetWindowProperty(classicui, mainWindow-> window, FCITX_WINDOW_DOCK, strWindowName);
}

MainWindow* CreateMainWindow(FcitxClassicUI* classicui)
{
    MainWindow *mainWindow;

    mainWindow = fcitx_utils_malloc0(sizeof(MainWindow));
    mainWindow->owner = classicui;
    InitMainWindow(mainWindow);

    FcitxX11AddXEventHandler(classicui->owner,
                             MainWindowEventHandler, mainWindow);
    FcitxX11AddCompositeHandler(classicui->owner,
                                ReloadMainWindow, mainWindow);
    return mainWindow;
}

void DisplayMainWindow(Display* dpy, MainWindow* mainWindow)
{
    FcitxLog(DEBUG, _("DISPLAY MainWindow"));
    XMapRaised(dpy, mainWindow->window);
}

void DrawMainWindow(MainWindow* mainWindow)
{
    FcitxClassicUI* classicui = mainWindow->owner;
    FcitxSkin *sc = &mainWindow->owner->skin;
    FcitxInstance *instance = mainWindow->owner->owner;
    FcitxInputContext2* ic2 = (FcitxInputContext2*) FcitxInstanceGetCurrentIC(classicui->owner);

    FcitxLog(DEBUG, _("DRAW MainWindow"));

    FcitxUIStatus* status;
    UT_array* uistats = FcitxInstanceGetUIStats(instance);
    for (status = (FcitxUIStatus*) utarray_front(uistats);
            status != NULL;
            status = (FcitxUIStatus*) utarray_next(uistats, status)
        ) {
        FcitxClassicUIStatus* privstat =  GetPrivateStatus(status);
        if (privstat == NULL)
            continue;
        /* reset status */
        privstat->x = privstat->y = -1;
        privstat->w = privstat->h = 0;
        privstat->avail = false;
    }

    FcitxUIComplexStatus* compstatus;
    UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
    for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
            compstatus != NULL;
            compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
        ) {
        FcitxClassicUIStatus* privstat =  GetPrivateStatus(compstatus);
        if (privstat == NULL)
            continue;
        /* reset status */
        privstat->x = privstat->y = -1;
        privstat->w = privstat->h = 0;
        privstat->avail = false;
    }

    if (mainWindow->owner->hideMainWindow == HM_SHOW
        || (mainWindow->owner->hideMainWindow == HM_AUTO && ((ic2 && ic2->switchBySwitchKey) || FcitxInstanceGetCurrentState(mainWindow->owner->owner) == IS_ACTIVE))) {
        SkinImage* activeIcon = LoadImage(sc, sc->skinMainBar.active, false);
        cairo_t *c;

        c = cairo_create(mainWindow->cs_main_bar);
        //把背景清空
        cairo_save(c);
        cairo_set_source_rgba(c, 0, 0, 0, 0);
        cairo_rectangle(c, 0, 0, SIZEX, SIZEY);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_fill(c);
        cairo_restore(c);

        cairo_set_operator(c, CAIRO_OPERATOR_OVER);
        int width = 0, height = 0;
        /* Check placement */
        if (utarray_len(&mainWindow->owner->skin.skinMainBar.skinPlacement) != 0) {
            SkinImage* back = LoadImage(sc, sc->skinMainBar.backImg, false);
            if (back == NULL)
                return;
            width = cairo_image_surface_get_width(back->image);
            height = cairo_image_surface_get_height(back->image);
            EnlargeCairoSurface(&mainWindow->cs_main_bar, width, height);
            XResizeWindow(mainWindow->dpy, mainWindow->window, width, height);
            DrawResizableBackground(c, back->image, height, width, 0, 0, 0, 0, F_RESIZE, F_COPY);


            SkinPlacement* sp;
            for (sp = (SkinPlacement*) utarray_front(&sc->skinMainBar.skinPlacement);
                    sp != NULL;
                    sp = (SkinPlacement*) utarray_next(&sc->skinMainBar.skinPlacement, sp)) {
                if (strcmp(sp->name, "logo") == 0) {
                    SkinImage* logo = LoadImage(sc, sc->skinMainBar.logo, false);
                    if (logo) {
                        DrawImage(c, logo->image, sp->x, sp->y, mainWindow->logostat.mouse);
                        UpdateStatusGeometry(&mainWindow->logostat, logo, sp->x, sp->y);
                    }
                } else if (strcmp(sp->name, "im") == 0) {
                    SkinImage* imicon = NULL;
                    if (FcitxInstanceGetCurrentStatev2(instance) != IS_ACTIVE || FcitxInstanceGetCurrentIM(instance) == NULL)
                        imicon = LoadImage(sc, sc->skinMainBar.eng, false);
                    else {
                        imicon = GetIMIcon(classicui, sc, sc->skinMainBar.active, 3, false);
                    }

                    if (imicon) {
                        DrawImage(c, imicon->image, sp->x, sp->y, mainWindow->imiconstat.mouse);
                        UpdateStatusGeometry(&mainWindow->imiconstat, imicon, sp->x, sp->y);
                    }
                } else {
                    status = FcitxUIGetStatusByName(instance, sp->name);
                    if (status && status->visible) {
                        FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
                        if (privstat == NULL)
                            continue;

                        boolean active = status->getCurrentStatus(status->arg);
                        char *path;
                        fcitx_utils_alloc_cat_str(path, status->name, active ?
                                                  "_active.png" :
                                                  "_inactive.png");
                        SkinImage* statusicon = LoadImage(sc, path, false);
                        free(path);
                        if (statusicon == NULL)
                            continue;
                        privstat->avail = true;
                        DrawImage(c, statusicon->image, sp->x, sp->y,
                                  privstat->mouse);
                        UpdateStatusGeometry(privstat, statusicon, sp->x, sp->y);
                    }

                    compstatus = FcitxUIGetComplexStatusByName(instance, sp->name);
                    if (compstatus && compstatus->visible) {
                        FcitxClassicUIStatus* privstat = GetPrivateStatus(compstatus);
                        if (privstat == NULL)
                            continue;

                        const char* icon = compstatus->getIconName(
                            compstatus->arg);
                        const char *path;
                        char *tmpstr = NULL;
                        if (icon[0] != '/') {
                            fcitx_utils_alloc_cat_str(tmpstr,
                                                      compstatus->name, ".png");
                            path = tmpstr;
                        } else {
                            path = icon;
                        }
                        SkinImage *statusicon = LoadImage(sc, path, false);
                        fcitx_utils_free(tmpstr);
                        if (statusicon == NULL)
                            continue;
                        privstat->avail = true;
                        DrawImage(c, statusicon->image, sp->x, sp->y,
                                  privstat->mouse);
                        UpdateStatusGeometry(privstat, statusicon, sp->x, sp->y);
                    }
                }
            }
        } else {
            /* Only logo and input status is hard-code, other should be status */
            int currentX = sc->skinMainBar.marginLeft;
            height = 0;
            SkinImage* back = LoadImage(sc, sc->skinMainBar.backImg, false);
            SkinImage* logo = LoadImage(sc, sc->skinMainBar.logo, false);
            SkinImage* imicon;
            int imageheight;
            if (logo) {
                currentX += cairo_image_surface_get_width(logo->image);
                imageheight = cairo_image_surface_get_height(logo->image);
                if (imageheight > height)
                    height = imageheight;
            }

            if (FcitxInstanceGetCurrentStatev2(instance) != IS_ACTIVE || FcitxInstanceGetCurrentIM(instance) == NULL)
                imicon = LoadImage(sc, sc->skinMainBar.eng, false);
            else {
                imicon = GetIMIcon(classicui, sc, sc->skinMainBar.active, 3, false);
            }

            if (imicon) {
                currentX += cairo_image_surface_get_width(imicon->image);
                imageheight = cairo_image_surface_get_height(imicon->image);
                if (imageheight > height)
                    height = imageheight;
            }


            FcitxUIComplexStatus* compstatus;
            UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
            for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
                 compstatus != NULL;
                 compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
                ) {
                if (!compstatus->visible)
                    continue;
                const char *icon = compstatus->getIconName(compstatus->arg);
                char *tmpstr = NULL;
                const char *path;
                if (icon[0] != '/') {
                    if (icon[0] == '\0') {
                        path = compstatus->shortDescription;
                    } else {
                        fcitx_utils_alloc_cat_str(tmpstr, icon, ".png");
                        path = tmpstr;
                    }
                } else {
                    path = icon;
                }
                SkinImage* statusicon = NULL;
                if (icon[0] != '\0')
                    statusicon = LoadImage(sc, path, false);
                if (statusicon == NULL || statusicon->textIcon) {
                    if (activeIcon && icon[0] == '\0') {
                        statusicon = LoadImageWithText(
                            classicui, sc, path, compstatus->shortDescription,
                            cairo_image_surface_get_width(activeIcon->image),
                            cairo_image_surface_get_height(activeIcon->image),
                            true);
                    }
                } else {
                    if (icon[0] == '/' && activeIcon) {
                        ResizeSurface(
                            &statusicon->image,
                            cairo_image_surface_get_width(activeIcon->image),
                            cairo_image_surface_get_height(activeIcon->image));
                    }
                }
                fcitx_utils_free(tmpstr);
                if (statusicon == NULL)
                    continue;

                currentX += cairo_image_surface_get_width(statusicon->image);
                imageheight = cairo_image_surface_get_height(statusicon->image);
                if (imageheight > height)
                    height = imageheight;
            }
            FcitxUIStatus* status;
            UT_array* uistats = FcitxInstanceGetUIStats(instance);
            for (status = (FcitxUIStatus*) utarray_front(uistats);
                    status != NULL;
                    status = (FcitxUIStatus*) utarray_next(uistats, status)
                ) {
                if (!status->visible)
                    continue;
                boolean active = status->getCurrentStatus(status->arg);
                char *path;
                fcitx_utils_alloc_cat_str(path, status->name, active ?
                                          "_active.png" : "_inactive.png");
                SkinImage* statusicon = LoadImage(sc, path, false);
                if (statusicon == NULL || statusicon->textIcon) {
                    if (activeIcon) {
                        statusicon = LoadImageWithText(classicui, sc, path, status->shortDescription,
                                                       cairo_image_surface_get_width(activeIcon->image),
                                                       cairo_image_surface_get_height(activeIcon->image),
                                                       active
                                                      );
                    }
                }
                free(path);
                if (statusicon == NULL)
                    continue;
                currentX += cairo_image_surface_get_width(statusicon->image);
                imageheight = cairo_image_surface_get_height(statusicon->image);
                if (imageheight > height)
                    height = imageheight;
            }
            width = currentX + sc->skinMainBar.marginRight;
            height += sc->skinMainBar.marginTop + sc->skinMainBar.marginBottom;

            EnlargeCairoSurface(&mainWindow->cs_main_bar, width, height);
            XResizeWindow(mainWindow->dpy, mainWindow->window, width, height);
            DrawResizableBackground(c,
                                    back->image,
                                    height,
                                    width,
                                    sc->skinMainBar.marginLeft,
                                    sc->skinMainBar.marginTop,
                                    sc->skinMainBar.marginRight,
                                    sc->skinMainBar.marginBottom,
                                    sc->skinMainBar.fillV,
                                    sc->skinMainBar.fillH
                                   );

            currentX = sc->skinMainBar.marginLeft;
            if (logo) {
                DrawImage(c, logo->image, currentX, sc->skinMainBar.marginTop, mainWindow->logostat.mouse);
                UpdateStatusGeometry(&mainWindow->logostat, logo, currentX, sc->skinMainBar.marginTop);
                currentX += cairo_image_surface_get_width(logo->image);
            }

            if (imicon) {
                DrawImage(c, imicon->image, currentX, sc->skinMainBar.marginTop, mainWindow->imiconstat.mouse);
                UpdateStatusGeometry(&mainWindow->imiconstat, imicon, currentX, sc->skinMainBar.marginTop);
                currentX += cairo_image_surface_get_width(imicon->image);
            }

            for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
                 compstatus != NULL;
                 compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
                ) {
                FcitxClassicUIStatus* privstat = GetPrivateStatus(compstatus);
                if (privstat == NULL)
                    continue;
                if (!compstatus->visible)
                    continue;
                const char *icon = compstatus->getIconName(compstatus->arg);
                const char *path;
                char *tmpstr = NULL;
                if (icon[0] != '/') {
                    if (icon[0] == '\0') {
                        path = compstatus->shortDescription;
                    } else {
                        fcitx_utils_alloc_cat_str(tmpstr,
                                                  compstatus->name, ".png");
                        path = tmpstr;
                    }
                } else {
                    path = icon;
                }
                SkinImage *statusicon = LoadImage(sc, path, false);
                fcitx_utils_free(tmpstr);
                if (statusicon == NULL)
                    continue;
                privstat->avail = true;
                DrawImage(c, statusicon->image, currentX, sc->skinMainBar.marginTop, privstat->mouse);
                UpdateStatusGeometry(privstat, statusicon, currentX, sc->skinMainBar.marginTop);
                currentX += cairo_image_surface_get_width(statusicon->image);
            }

            for (status = (FcitxUIStatus*) utarray_front(uistats);
                 status != NULL;
                 status = (FcitxUIStatus*) utarray_next(uistats, status)) {
                FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
                if (privstat == NULL)
                    continue;
                if (!status->visible)
                    continue;
                /* reset status */
                boolean active =  status->getCurrentStatus(status->arg);
                char *path;
                fcitx_utils_alloc_cat_str(path, status->name, active ?
                                          "_active.png" : "_inactive.png");
                SkinImage* statusicon = LoadImage(sc, path, false);
                free(path);
                if (statusicon == NULL)
                    continue;
                privstat->avail = true;
                DrawImage(c, statusicon->image, currentX, sc->skinMainBar.marginTop, privstat->mouse);
                UpdateStatusGeometry(privstat, statusicon, currentX, sc->skinMainBar.marginTop);
                currentX += cairo_image_surface_get_width(statusicon->image);
            }
        }

        cairo_destroy(c);
        _CAIRO_SETSIZE(mainWindow->cs_x_main_bar, width, height);

        c = cairo_create(mainWindow->cs_x_main_bar);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(c, mainWindow->cs_main_bar, 0, 0);
        cairo_rectangle(c, 0, 0, width, height);
        cairo_clip(c);
        cairo_paint(c);
        cairo_destroy(c);
        cairo_surface_flush(mainWindow->cs_x_main_bar);

        XMapRaised(mainWindow->dpy, mainWindow->window);
    } else
        XUnmapWindow(mainWindow->dpy, mainWindow->window);
}

void ReloadMainWindow(void *arg, boolean enabled)
{
    MainWindow* mainWindow = (MainWindow*) arg;
    boolean visable = WindowIsVisable(mainWindow->dpy, mainWindow->window);
    cairo_surface_destroy(mainWindow->cs_main_bar);
    cairo_surface_destroy(mainWindow->cs_x_main_bar);
    XDestroyWindow(mainWindow->dpy, mainWindow->window);

    mainWindow->cs_main_bar = NULL;
    mainWindow->cs_x_main_bar = NULL;
    mainWindow->window = None;

    InitMainWindow(mainWindow);

    if (visable)
        DrawMainWindow(mainWindow);
}

void UpdateStatusGeometry(FcitxClassicUIStatus *privstat, SkinImage *image, int x, int y)
{
    privstat->x = x;
    privstat->y = y;
    privstat->w = cairo_image_surface_get_width(image->image);
    privstat->h = cairo_image_surface_get_height(image->image);
}

void CloseMainWindow(MainWindow *mainWindow)
{
    if (mainWindow->owner->hideMainWindow != HM_SHOW || mainWindow->owner->isSuspend)
        XUnmapWindow(mainWindow->dpy, mainWindow->window);
}

boolean SetMouseStatus(MainWindow *mainWindow, MouseE* mouseE, MouseE value, MouseE other)
{
    FcitxClassicUI* classicui = mainWindow->owner;
    FcitxInstance *instance = mainWindow->owner->owner;
    boolean changed = false;
    if (mouseE != &mainWindow->logostat.mouse) {
        if (mainWindow->logostat.mouse != other) {
            changed = true;
            mainWindow->logostat.mouse = other;
        }
    }
    if (mouseE != &mainWindow->imiconstat.mouse) {
        if (mainWindow->imiconstat.mouse != other) {
            changed = true;
            mainWindow->imiconstat.mouse = other;
        }
    }
    FcitxUIComplexStatus *compstatus;
    UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
    for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
         compstatus != NULL;
         compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
    ) {
        FcitxClassicUIStatus* privstat = GetPrivateStatus(compstatus);
        if (mouseE != &privstat->mouse) {
            if (privstat->mouse != other) {
                changed = true;
                privstat->mouse = other;
            }
        }
    }
    FcitxUIStatus *status;
    UT_array* uistats = FcitxInstanceGetUIStats(instance);
    for (status = (FcitxUIStatus*) utarray_front(uistats);
            status != NULL;
            status = (FcitxUIStatus*) utarray_next(uistats, status)
        ) {
        FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
        if (mouseE != &privstat->mouse) {
            if (privstat->mouse != other) {
                changed = true;
                privstat->mouse = other;
            }
        }
    }
    if (mouseE != NULL && *mouseE != value) {
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

    if (event->xany.window == mainWindow->window) {
        switch (event->type) {
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
                FcitxUIComplexStatus *compstatus;
                UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
                for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
                    compstatus != NULL;
                    compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
                ) {
                    FcitxClassicUIStatus* privstat = GetPrivateStatus(compstatus);
                    if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat))
                        mouse = &privstat->mouse;
                }
                FcitxUIStatus *status;
                UT_array* uistats = FcitxInstanceGetUIStats(instance);
                for (status = (FcitxUIStatus*) utarray_front(uistats);
                     status != NULL;
                     status = (FcitxUIStatus*) utarray_next(uistats, status)
                ) {
                    FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
                    if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat))
                        mouse = &privstat->mouse;
                }
            }
            if (SetMouseStatus(mainWindow, mouse, MOTION, RELEASE))
                DrawMainWindow(mainWindow);
            break;
        case LeaveNotify:
            if (SetMouseStatus(mainWindow, NULL, RELEASE, RELEASE))
                DrawMainWindow(mainWindow);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1: {
                mouse = NULL;
                if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->logostat)) {
                    mouse = &mainWindow->logostat.mouse;

                    classicui->iMainWindowOffsetX = event->xbutton.x;
                    classicui->iMainWindowOffsetY = event->xbutton.y;

                    if (!ClassicUIMouseClick(mainWindow->owner, mainWindow->window, &classicui->iMainWindowOffsetX, &classicui->iMainWindowOffsetY)) {
                        FcitxInstanceChangeIMState(instance, FcitxInstanceGetCurrentIC(instance));
                    }
                    SaveClassicUIConfig(classicui);
                } else if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->imiconstat)) {
                    mouse = &mainWindow->imiconstat.mouse;
                    FcitxInstanceSwitchIMByIndex(instance, -1);
                } else {
                    FcitxUIComplexStatus *compstatus;
                    UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
                    for (compstatus = (FcitxUIComplexStatus*) utarray_front(uicompstats);
                         compstatus != NULL;
                         compstatus = (FcitxUIComplexStatus*) utarray_next(uicompstats, compstatus)
                    ) {
                        FcitxClassicUIStatus* privstat = GetPrivateStatus(compstatus);
                        if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat)) {
                            mouse = &privstat->mouse;
                            if (!compstatus->toggleStatus) {
                                FcitxUIMenu* menu = FcitxUIGetMenuByStatusName(instance, compstatus->name);
                                if (menu) {
                                    XlibMenu *menuWindow = (XlibMenu*) menu->uipriv[classicui->isfallback];
                                    UpdateMenuGeometry(mainWindow, menuWindow);
                                    DrawXlibMenu(menuWindow);
                                    DisplayXlibMenu(menuWindow);
                                }
                            }
                            else
                                FcitxUIUpdateStatus(instance, compstatus->name);
                        }
                    }
                    FcitxUIStatus *status;
                    UT_array* uistats = FcitxInstanceGetUIStats(instance);
                    for (status = (FcitxUIStatus*) utarray_front(uistats);
                         status != NULL;
                         status = (FcitxUIStatus*) utarray_next(uistats, status)
                        ) {
                        FcitxClassicUIStatus* privstat = GetPrivateStatus(status);
                        if (IsInRspArea(event->xbutton.x, event->xbutton.y, privstat)) {
                            mouse = &privstat->mouse;
                            FcitxUIUpdateStatus(instance, status->name);
                        }
                    }
                }
                if (SetMouseStatus(mainWindow, mouse, PRESS, RELEASE))
                    DrawMainWindow(mainWindow);
                if (mouse == NULL) {
                    classicui->iMainWindowOffsetX = event->xbutton.x;
                    classicui->iMainWindowOffsetY = event->xbutton.y;
                    ClassicUIMouseClick(mainWindow->owner, mainWindow->window, &classicui->iMainWindowOffsetX, &classicui->iMainWindowOffsetY);
                    SaveClassicUIConfig(classicui);
                }
            }
            break;
            case Button3: {
                XlibMenu *mainMenuWindow = classicui->mainMenuWindow;
                UpdateMenuGeometry(mainWindow, mainMenuWindow);
                DrawXlibMenu(mainMenuWindow);
                DisplayXlibMenu(mainMenuWindow);

            }
            break;

            }
            break;
        case ButtonRelease:
            switch (event->xbutton.button) {
            case Button1:
                if (SetMouseStatus(mainWindow, NULL, RELEASE, RELEASE))
                    DrawMainWindow(mainWindow);
                break;
            }
            break;
        }
        return true;
    }
    return false;
}

static void UpdateMenuGeometry(MainWindow* mainWindow, XlibMenu* menuWindow)
{
    FcitxClassicUI* classicui = mainWindow->owner;

    unsigned int height;
    int sheight;
    XWindowAttributes attr;
    FcitxMenuUpdate(menuWindow->menushell);
    GetMenuSize(menuWindow);
    GetScreenSize(classicui, NULL, &sheight);
    XGetWindowAttributes(classicui->dpy, mainWindow->window, &attr);
    height = attr.height;

    menuWindow->iPosX = classicui->iMainWindowOffsetX;
    menuWindow->iPosY =
        classicui->iMainWindowOffsetY +
        height;
    if ((menuWindow->iPosY + menuWindow->height) >
            sheight)
        menuWindow->iPosY = classicui->iMainWindowOffsetY - 5 - menuWindow->height;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
