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
#include "TrayWindow.h"
#include "fcitx-utils/utils.h"

#define MAIN_BAR_MAX_WIDTH 2
#define MAIN_BAR_MAX_HEIGHT 2

static boolean MainWindowEventHandler(void *arg, XEvent* event);
static void MainWindowUpdateStatusGeometry(MainWindow* mainWindow, FcitxClassicUIStatus *privstat, SkinImage *image, int x, int y);
static void ReloadMainWindow(void* arg, boolean enabled);
static void MainWindowInit(MainWindow* mainWindow);
static void MainWindowMoveWindow(FcitxXlibWindow* window);
static void MainWindowCalculateContentSize(FcitxXlibWindow* window, unsigned int* width, unsigned int* height);
static void MainWindowPaint(FcitxXlibWindow* window, cairo_t* c);

static inline boolean MainWindowShouldShow(MainWindow* mainWindow)
{
    FcitxXlibWindow* window = &mainWindow->parent;
    FcitxClassicUI* classicui = window->owner;
    FcitxInstance *instance = window->owner->owner;
    FcitxInputContext2* ic2 = (FcitxInputContext2*) FcitxInstanceGetCurrentIC(instance);
    return (window->owner->hideMainWindow == HM_SHOW)
        || (window->owner->hideMainWindow == HM_AUTO && ((ic2 && ic2->switchBySwitchKey) || FcitxInstanceGetCurrentState(window->owner->owner) == IS_ACTIVE))
        || (window->owner->hideMainWindow == HM_HIDE_WHEN_TRAY_AVAILABLE && !(classicui->notificationItemAvailable || classicui->trayWindow->bTrayMapped));
}


void MainWindowInit(MainWindow* mainWindow)
{
    FcitxXlibWindow* window = &mainWindow->parent;
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin* sc = &classicui->skin;
    int swidth = 0, sheight = 0;

    GetScreenSize(classicui, &swidth, &sheight);
    SkinImage *back = LoadImage(sc, sc->skinMainBar.background.background, false);
    int w = MAIN_BAR_MAX_WIDTH, h = MAIN_BAR_MAX_HEIGHT;
    if (back) {
        w = cairo_image_surface_get_width(back->image);
        h = cairo_image_surface_get_height(back->image);
    }

    if (classicui->iMainWindowOffsetX + w > swidth)
        classicui->iMainWindowOffsetX = swidth - w;

    if (classicui->iMainWindowOffsetY + h > sheight)
        classicui->iMainWindowOffsetY = sheight - h;

    FcitxXlibWindowInit(&mainWindow->parent,
                        MAIN_BAR_MAX_WIDTH, MAIN_BAR_MAX_HEIGHT,
                        classicui->iMainWindowOffsetX, classicui->iMainWindowOffsetY,
                        "Fcitx Main Window",
                        FCITX_WINDOW_DOCK,
                        &sc->skinMainBar.background,
                        ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask,
                        MainWindowMoveWindow,
                        MainWindowCalculateContentSize,
                        MainWindowPaint
    );
}

MainWindow* MainWindowCreate(FcitxClassicUI* classicui)
{
    MainWindow *mainWindow;

    mainWindow = FcitxXlibWindowCreate(classicui, sizeof(MainWindow));
    MainWindowInit(mainWindow);

    FcitxX11AddXEventHandler(classicui->owner,
                             MainWindowEventHandler, mainWindow);
    FcitxX11AddCompositeHandler(classicui->owner,
                                ReloadMainWindow, mainWindow);
    return mainWindow;
}

void MainWindowShow(MainWindow* mainWindow)
{
    FcitxXlibWindow* window = &mainWindow->parent;
    FcitxClassicUI* classicui = window->owner;
    if (MainWindowShouldShow(mainWindow)) {
        FcitxXlibWindowPaint(&mainWindow->parent);
        XMapRaised(classicui->dpy, window->wId);
    } else {
        MainWindowClose(mainWindow);
    }
}

void MainWindowCalculateContentSize(FcitxXlibWindow* window, unsigned int* width, unsigned int* height)
{
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &window->owner->skin;
    FcitxInstance *instance = window->owner->owner;

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

    if (!MainWindowShouldShow((MainWindow*) window)) {
        return;
    }
    SkinImage* activeIcon = LoadImage(sc, sc->skinMainBar.active, false);
    int newWidth = 0, newHeight = 0;
    /* Check placement */
    if (utarray_len(&window->owner->skin.skinMainBar.skinPlacement) != 0) {
        SkinImage* back = LoadImage(sc, window->background->background, false);
        if (back == NULL)
            return;
        newWidth = cairo_image_surface_get_width(back->image);
        newHeight = cairo_image_surface_get_height(back->image);
    } else {
        /* Only logo and input status is hard-code, other should be status */
        int currentX = 0;
        newHeight = 0;
        SkinImage* logo = LoadImage(sc, sc->skinMainBar.logo, false);
        SkinImage* imicon;
        int imageheight;
        if (logo) {
            currentX += cairo_image_surface_get_width(logo->image);
            imageheight = cairo_image_surface_get_height(logo->image);
            if (imageheight > newHeight)
                newHeight = imageheight;
        }

        if (FcitxInstanceGetCurrentStatev2(instance) != IS_ACTIVE || FcitxInstanceGetCurrentIM(instance) == NULL)
            imicon = LoadImage(sc, sc->skinMainBar.eng, false);
        else {
            imicon = GetIMIcon(classicui, sc, sc->skinMainBar.active, 3, false);
        }

        if (imicon) {
            currentX += cairo_image_surface_get_width(imicon->image);
            imageheight = cairo_image_surface_get_height(imicon->image);
            if (imageheight > newHeight)
                newHeight = imageheight;
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
            if (imageheight > newHeight)
                newHeight = imageheight;
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
            if (imageheight > newHeight)
                newHeight = imageheight;
        }
        newWidth = currentX;
    }

    *width = newWidth;
    *height = newHeight;
}

void MainWindowPaint(FcitxXlibWindow* window, cairo_t* c)
{
    MainWindow* mainWindow = (MainWindow*) window;
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &window->owner->skin;
    FcitxInstance *instance = window->owner->owner;

    if (!MainWindowShouldShow(mainWindow)) {
        return;
    }

    FcitxUIStatus* status;
    FcitxUIComplexStatus* compstatus;
    /* Check placement */
    if (utarray_len(&window->owner->skin.skinMainBar.skinPlacement) != 0) {
        SkinPlacement* sp;
        for (sp = (SkinPlacement*) utarray_front(&sc->skinMainBar.skinPlacement);
                sp != NULL;
                sp = (SkinPlacement*) utarray_next(&sc->skinMainBar.skinPlacement, sp)) {
            if (strcmp(sp->name, "logo") == 0) {
                SkinImage* logo = LoadImage(sc, sc->skinMainBar.logo, false);
                if (logo) {
                    DrawImage(c, logo->image, sp->x, sp->y, mainWindow->logostat.mouse);
                    MainWindowUpdateStatusGeometry(mainWindow, &mainWindow->logostat, logo, sp->x, sp->y);
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
                    MainWindowUpdateStatusGeometry(mainWindow, &mainWindow->imiconstat, imicon, sp->x, sp->y);
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
                    MainWindowUpdateStatusGeometry(mainWindow, privstat, statusicon, sp->x, sp->y);
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
                    MainWindowUpdateStatusGeometry(mainWindow, privstat, statusicon, sp->x, sp->y);
                }
            }
        }
    } else {
        /* Only logo and input status is hard-code, other should be status */
        int currentX = 0;
        SkinImage* logo = LoadImage(sc, sc->skinMainBar.logo, false);
        SkinImage* imicon;

        if (FcitxInstanceGetCurrentStatev2(instance) != IS_ACTIVE || FcitxInstanceGetCurrentIM(instance) == NULL)
            imicon = LoadImage(sc, sc->skinMainBar.eng, false);
        else {
            imicon = GetIMIcon(classicui, sc, sc->skinMainBar.active, 3, false);
        }

        if (logo) {
            DrawImage(c, logo->image, currentX, 0, mainWindow->logostat.mouse);
            MainWindowUpdateStatusGeometry(mainWindow, &mainWindow->logostat, logo, currentX, 0);
            currentX += cairo_image_surface_get_width(logo->image);
        }

        if (imicon) {
            DrawImage(c, imicon->image, currentX, 0, mainWindow->imiconstat.mouse);
            MainWindowUpdateStatusGeometry(mainWindow, &mainWindow->imiconstat, imicon, currentX, 0);
            currentX += cairo_image_surface_get_width(imicon->image);
        }

        UT_array* uicompstats = FcitxInstanceGetUIComplexStats(instance);
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
            DrawImage(c, statusicon->image, currentX, 0, privstat->mouse);
            MainWindowUpdateStatusGeometry(mainWindow, privstat, statusicon, currentX, 0);
            currentX += cairo_image_surface_get_width(statusicon->image);
        }

        UT_array* uistats = FcitxInstanceGetUIStats(instance);
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
            DrawImage(c, statusicon->image, currentX, 0, privstat->mouse);
            MainWindowUpdateStatusGeometry(mainWindow, privstat, statusicon, currentX, 0);
            currentX += cairo_image_surface_get_width(statusicon->image);
        }
    }
}

void ReloadMainWindow(void *arg, boolean enabled)
{
    MainWindow* mainWindow = (MainWindow*) arg;
    boolean visable = WindowIsVisable(mainWindow->parent.owner->dpy, mainWindow->parent.wId);
    FcitxXlibWindowDestroy(&mainWindow->parent);

    MainWindowInit(mainWindow);

    if (visable)
        FcitxXlibWindowPaint(&mainWindow->parent);
}

void MainWindowUpdateStatusGeometry(MainWindow* mainWindow, FcitxClassicUIStatus *privstat, SkinImage *image, int x, int y)
{
    FcitxXlibWindow* window = (FcitxXlibWindow*) mainWindow;
    privstat->x = x + window->contentX;
    privstat->y = y + window->contentY;
    privstat->w = cairo_image_surface_get_width(image->image);
    privstat->h = cairo_image_surface_get_height(image->image);
}

void MainWindowClose(MainWindow *mainWindow)
{
    if (mainWindow->parent.owner->hideMainWindow != HM_SHOW || mainWindow->parent.owner->isSuspend)
        XUnmapWindow(mainWindow->parent.owner->dpy, mainWindow->parent.wId);
}

boolean MainWindowSetMouseStatus(MainWindow *mainWindow, MouseE* mouseE, MouseE value, MouseE other)
{
    FcitxClassicUI* classicui = mainWindow->parent.owner;
    FcitxInstance *instance = classicui->owner;
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
    FcitxXlibWindow* window = arg;
    FcitxClassicUI* classicui = mainWindow->parent.owner;
    FcitxInstance *instance = classicui->owner;
    MouseE *mouse;

    if (event->xany.window == window->wId) {
        switch (event->type) {
        case Expose:
            FcitxXlibWindowPaint(&mainWindow->parent);
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
            if (MainWindowSetMouseStatus(mainWindow, mouse, MOTION, RELEASE))
                FcitxXlibWindowPaint(&mainWindow->parent);
            break;
        case LeaveNotify:
            if (MainWindowSetMouseStatus(mainWindow, NULL, RELEASE, RELEASE))
                FcitxXlibWindowPaint(&mainWindow->parent);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1: {
                mouse = NULL;
                if (IsInRspArea(event->xbutton.x, event->xbutton.y, &mainWindow->logostat)) {
                    mouse = &mainWindow->logostat.mouse;

                    classicui->iMainWindowOffsetX = event->xbutton.x;
                    classicui->iMainWindowOffsetY = event->xbutton.y;

                    if (!ClassicUIMouseClick(classicui, window->wId, &classicui->iMainWindowOffsetX, &classicui->iMainWindowOffsetY)) {
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
                                    menuWindow->anchor = MA_MainWindow;
                                    XlibMenuShow(menuWindow);
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
                if (MainWindowSetMouseStatus(mainWindow, mouse, PRESS, RELEASE))
                    FcitxXlibWindowPaint(&mainWindow->parent);
                if (mouse == NULL) {
                    classicui->iMainWindowOffsetX = event->xbutton.x;
                    classicui->iMainWindowOffsetY = event->xbutton.y;
                    ClassicUIMouseClick(classicui, window->wId, &classicui->iMainWindowOffsetX, &classicui->iMainWindowOffsetY);
                    SaveClassicUIConfig(classicui);
                }
            }
            break;
            case Button3: {
                XlibMenu *mainMenuWindow = classicui->mainMenuWindow;
                mainMenuWindow->anchor = MA_MainWindow;
                XlibMenuShow(mainMenuWindow);
            }
            break;

            }
            break;
        case ButtonRelease:
            switch (event->xbutton.button) {
            case Button1:
                if (MainWindowSetMouseStatus(mainWindow, NULL, RELEASE, RELEASE))
                    FcitxXlibWindowPaint(&mainWindow->parent);
                break;
            }
            break;
        }
        return true;
    }
    return false;
}

void MainWindowMoveWindow(FcitxXlibWindow* window)
{

}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
