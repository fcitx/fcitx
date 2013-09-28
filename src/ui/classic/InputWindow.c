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

#include <string.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <libintl.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "fcitx/ui.h"
#include "fcitx/module.h"
#include "fcitx/profile.h"
#include "fcitx/frontend.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx/candidate.h"
#include "fcitx/hook.h"
#include "fcitx-utils/utils.h"

#include "InputWindow.h"
#include "classicui.h"
#include "skin.h"
#include "MainWindow.h"
#include "fcitx-utils/log.h"

#define CANDIDATE_HIGHLIGHT(INDEX) ((2 << 16) | INDEX)
#define PREVNEXT_HIGHLIGHT(PREV) ((1 << 16) | PREV)

static boolean InputWindowEventHandler(void *arg, XEvent* event);
static void InputWindowInit(InputWindow* inputWindow);
static void InputWindowReload(void* arg, boolean enabled);
static void InputWindowMoveWindow(FcitxXlibWindow* window);
static void InputWindowCalculateContentSize(FcitxXlibWindow* window, unsigned int* width, unsigned int* height);
static void InputWindowPaint(FcitxXlibWindow* window, cairo_t* c);

void InputWindowInit(InputWindow* inputWindow)
{
    FcitxXlibWindow* window = &inputWindow->parent;
    FcitxXlibWindowInit(window,
                        INPUTWND_WIDTH,
                        INPUTWND_HEIGHT,
                        0, 0,
                        "Fcitx Input Window",
                        FCITX_WINDOW_DOCK,
                        &window->owner->skin.skinInputBar.background,
                        ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | ExposureMask | LeaveWindowMask,
                        InputWindowMoveWindow,
                        InputWindowCalculateContentSize,
                        InputWindowPaint
    );

    inputWindow->iOffsetX = 0;
    inputWindow->iOffsetY = 8;
}

InputWindow* InputWindowCreate(FcitxClassicUI *classicui)
{
    InputWindow* inputWindow = FcitxXlibWindowCreate(classicui, sizeof(InputWindow));
    InputWindowInit(inputWindow);

    FcitxX11AddXEventHandler(classicui->owner,
                             InputWindowEventHandler, inputWindow);
    FcitxX11AddCompositeHandler(classicui->owner,
                                InputWindowReload, inputWindow);

    inputWindow->msgUp = FcitxMessagesNew();
    inputWindow->msgDown = FcitxMessagesNew();
    return inputWindow;
}

void InputWindowCalculateContentSize(FcitxXlibWindow* window, unsigned int* width, unsigned int* height)
{
    InputWindow* inputWindow = (InputWindow*) window;
    FcitxInstance* instance = window->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateLayoutHint layout = FcitxCandidateWordGetLayoutHint(candList);
    FcitxClassicUI* classicui = window->owner;

    inputWindow->cursorPos = FcitxUINewMessageToOldStyleMessage(instance, inputWindow->msgUp, inputWindow->msgDown);

    boolean vertical = window->owner->bVerticalList;
    if (layout == CLH_Vertical)
        vertical = true;
    else if (layout == CLH_Horizontal)
        vertical = false;

    inputWindow->vertical = vertical;

    /* begin evalulation */
    FcitxMessages* msgup = inputWindow->msgUp;
    FcitxMessages* msgdown = inputWindow->msgDown;
    FcitxSkin* sc = &classicui->skin;

    int i;
    FcitxRect* candRect = inputWindow->candRect;
    char **strUp = inputWindow->strUp;
    char **strDown = inputWindow->strDown;
    int *posUpX = inputWindow->posUpX, *posUpY = inputWindow->posUpY;
    int *posDownX = inputWindow->posDownX, *posDownY = inputWindow->posDownY;
    int newHeight = 0, newWidth = 0;
    int pixelCursorPos = 0;
    int inputWidth = 0, outputWidth = 0;
    int outputHeight = 0;
    int iChar = inputWindow->cursorPos;
    int strWidth = 0, strHeight = 0;

    inputWidth = 0;
    int dpi = sc->skinFont.respectDPI? classicui->dpi : 0;
    FCITX_UNUSED(dpi);

    FcitxCairoTextContext* ctc = FcitxCairoTextContextCreate(NULL);
    FcitxCairoTextContextSet(ctc, window->owner->font, window->owner->fontSize > 0 ? window->owner->fontSize : sc->skinFont.fontSize, dpi);

    int fontHeight = FcitxCairoTextContextFontHeight(ctc);
    inputWindow->fontHeight = fontHeight;
    for (i = 0; i < FcitxMessagesGetMessageCount(msgup) ; i++) {
        char *trans = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(msgup, i));
        if (trans)
            strUp[i] = trans;
        else
            strUp[i] = FcitxMessagesGetMessageString(msgup, i);
        posUpX[i] = inputWidth;

        FcitxCairoTextContextStringSize(ctc, strUp[i], &strWidth, &strHeight);

        if (sc->skinFont.respectDPI)
            posUpY[i] = sc->skinInputBar.iInputPos + fontHeight - strHeight;
        else
            posUpY[i] = sc->skinInputBar.iInputPos - strHeight;
        inputWidth += strWidth;
        if (FcitxInputStateGetShowCursor(input)) {
            int length = strlen(FcitxMessagesGetMessageString(msgup, i));
            if (iChar >= 0) {
                if (iChar < length) {
                    char strTemp[MESSAGE_MAX_LENGTH];
                    char *strGBKT = NULL;
                    strncpy(strTemp, strUp[i], iChar);
                    strTemp[iChar] = '\0';
                    strGBKT = strTemp;
                    FcitxCairoTextContextStringSize(ctc, strGBKT, &strWidth, &strHeight);
                    // if the cursor is between two part, add extra pixel for pretty output
                    pixelCursorPos = posUpX[i] + strWidth;
                }
                iChar -= length;
            }
        }

    }

    if (iChar >= 0)
        pixelCursorPos = inputWidth;

    inputWindow->pixelCursorPos = pixelCursorPos;

    outputWidth = 0;
    outputHeight = 0;
    int currentX = 0;
    int offsetY;
    if (sc->skinFont.respectDPI)
        offsetY = (FcitxMessagesGetMessageCount(msgup) ? (sc->skinInputBar.iInputPos + fontHeight) : 0)
                 + (FcitxMessagesGetMessageCount(msgdown) ? sc->skinInputBar.iOutputPos : 0);
    else
        offsetY = sc->skinInputBar.iOutputPos - fontHeight;
    int candidateIndex = -1;
    int lastRightBottomX = 0, lastRightBottomY = 0;
    for (i = 0; i < FcitxMessagesGetMessageCount(msgdown) ; i++) {
        char *trans = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(msgdown, i));
        if (trans)
            strDown[i] = trans;
        else
            strDown[i] = FcitxMessagesGetMessageString(msgdown, i);

        if (vertical) { /* vertical */
            if (FcitxMessagesGetMessageType(msgdown, i) == MSG_INDEX) {
                if (currentX > outputWidth)
                    outputWidth = currentX;
                if (i != 0) {
                    currentX = 0;
                }
            }
            posDownX[i] = currentX;
            FcitxCairoTextContextStringSize(ctc, strDown[i], &strWidth, &strHeight);
            if (FcitxMessagesGetMessageType(msgdown, i) == MSG_INDEX && i != 0)
                outputHeight += fontHeight + 2;
            currentX += strWidth;
        } else { /* horizontal */
            posDownX[i] = outputWidth;
            FcitxCairoTextContextStringSize(ctc, strDown[i], &strWidth, &strHeight);
            outputWidth += strWidth;
        }
        posDownY[i] = offsetY + outputHeight;

        if (FcitxMessagesGetMessageType(msgdown, i) == MSG_INDEX) {
            candidateIndex ++;

            if (candidateIndex > 0 && candidateIndex - 1 < 10) {
                candRect[candidateIndex - 1].x2 = lastRightBottomX;
                candRect[candidateIndex - 1].y2 = lastRightBottomY;
            }

            if (candidateIndex < 10) {
                candRect[candidateIndex].x1 = posDownX[i];
                candRect[candidateIndex].y1 = posDownY[i];
            }
        }

        lastRightBottomX = posDownX[i] + strWidth;
        lastRightBottomY = posDownY[i] + strHeight;
    }

    if (candidateIndex >= 0 && candidateIndex < 10) {
        candRect[candidateIndex].x2 = lastRightBottomX;
        candRect[candidateIndex].y2 = lastRightBottomY;
    }

    if (vertical && currentX > outputWidth)
        outputWidth = currentX;

    newHeight = offsetY + outputHeight + (FcitxMessagesGetMessageCount(msgdown) || !sc->skinFont.respectDPI ? fontHeight : 0);

    newWidth = (inputWidth < outputWidth) ? outputWidth : inputWidth;

    /* round to ROUND_SIZE in order to decrease resize */
    newWidth = (newWidth / ROUND_SIZE) * ROUND_SIZE + ROUND_SIZE;

    if (vertical) { /* vertical */
        newWidth = (newWidth < INPUT_BAR_VMIN_WIDTH) ? INPUT_BAR_VMIN_WIDTH : newWidth;
    } else {
        newWidth = (newWidth < INPUT_BAR_HMIN_WIDTH) ? INPUT_BAR_HMIN_WIDTH : newWidth;
    }

    FcitxCairoTextContextFree(ctc);

    *width = newWidth;
    *height = newHeight;
}

boolean IsInRect(int x, int y, FcitxRect* rect)
{
    return rect->x2 - rect->x1 > 0
        && rect->y2 - rect->y1 > 0
        && x >= rect->x1 && x <= rect->x2 && y >= rect->y1 && y <= rect->y2;
}


boolean InputWindowEventHandler(void *arg, XEvent* event)
{
    FcitxXlibWindow* window = arg;
    InputWindow* inputWindow = arg;
    FcitxInstance* instance = window->owner->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    if (event->xany.window == window->wId) {
        switch (event->type) {
        case MotionNotify:
            {
                FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
                int             x,
                                y;
                x = event->xbutton.x;
                y = event->xbutton.y;

                boolean flag = false;
                int i;
                FcitxCandidateWord* candWord;
                uint32_t newHighlight = 0;
                for (candWord = FcitxCandidateWordGetCurrentWindow(candList), i = 0;
                     candWord != NULL;
                     candWord = FcitxCandidateWordGetCurrentWindowNext(candList, candWord), i ++) {
                    if (IsInRect(x - window->contentX, y - window->contentY, &inputWindow->candRect[i])) {
                        newHighlight = CANDIDATE_HIGHLIGHT(i);
                        flag = true;
                        break;
                    }
                }

                if (!flag) {
                    if (IsInRect(x - window->contentX, y - window->contentY, &inputWindow->prevRect)) {
                        newHighlight = PREVNEXT_HIGHLIGHT(true);
                    } else if (IsInRect(x - window->contentX, y - window->contentY, &inputWindow->nextRect)) {
                        newHighlight = PREVNEXT_HIGHLIGHT(false);
                    }
                }

                if (newHighlight != inputWindow->highlight) {
                    inputWindow->highlight = newHighlight;
                    FcitxXlibWindowPaint(&inputWindow->parent);
                }
            }
            break;
        case Expose:
            FcitxXlibWindowPaint(&inputWindow->parent);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
                case Button1: {

                    MainWindowSetMouseStatus(window->owner->mainWindow, NULL, RELEASE, RELEASE);
                    int             x,
                                    y;
                    x = event->xbutton.x;
                    y = event->xbutton.y;

                    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);

                    boolean flag = false;
                    int i;
                    FcitxCandidateWord* candWord;
                    for (candWord = FcitxCandidateWordGetCurrentWindow(candList), i = 0;
                         candWord != NULL;
                         candWord = FcitxCandidateWordGetCurrentWindowNext(candList, candWord), i ++) {
                        if (IsInRect(x - window->contentX, y - window->contentY, &inputWindow->candRect[i])) {
                            FcitxInstanceChooseCandidateByIndex(instance, i);

                            flag = true;
                            break;
                        }
                    }

                    if (flag)
                        break;

                    if (IsInRect(x - window->contentX, y - window->contentY, &inputWindow->prevRect)) {
                        FcitxCandidateWordGoPrevPage(candList);
                        FcitxInstanceProcessInputReturnValue(window->owner->owner, IRV_DISPLAY_CANDWORDS);
                    } else if (IsInRect(x - window->contentX, y - window->contentY, &inputWindow->nextRect)) {
                        FcitxCandidateWordGoNextPage(candList);
                        FcitxInstanceProcessInputReturnValue(window->owner->owner, IRV_DISPLAY_CANDWORDS);
                    } else if (ClassicUIMouseClick(window->owner, window->wId, &x, &y)) {

                        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(window->owner->owner);

                        if (ic) {
                            FcitxInstanceSetWindowOffset(window->owner->owner, ic, x - inputWindow->iOffsetX, y  - inputWindow->iOffsetY);
                        }

                        FcitxXlibWindowPaint(&inputWindow->parent);
                    }
                }
                break;
            }
            break;
        }
        return true;
    }
    return false;
}

void InputWindowMoveWindow(FcitxXlibWindow* window)
{
    InputWindow* inputWindow = (InputWindow*) window;
    int x = 0, y = 0, w = 0, h = 0;

    FcitxInputContext* ic = FcitxInstanceGetCurrentIC(window->owner->owner);
    FcitxInstanceGetWindowRect(window->owner->owner, ic, &x, &y, &w, &h);
    FcitxRect rect = GetScreenGeometry(window->owner, x, y);

    int iTempInputWindowX, iTempInputWindowY;

    if (x < rect.x1)
        iTempInputWindowX = rect.x1;
    else
        iTempInputWindowX = x + inputWindow->iOffsetX;

    if (y < rect.y1)
        iTempInputWindowY = rect.y1;
    else
        iTempInputWindowY = y + h + inputWindow->iOffsetY;

    if ((iTempInputWindowX + window->width) > rect.x2)
        iTempInputWindowX =  rect.x2 - window->width;

    if ((iTempInputWindowY + window->height) >  rect.y2) {
        if (iTempInputWindowY >  rect.y2)
            iTempInputWindowY =  rect.y2 - window->height - 40;
        else /* better position the window */
            iTempInputWindowY = iTempInputWindowY - window->height - ((h == 0)?40:h) - 2 * inputWindow->iOffsetY;
    }
    XMoveWindow(window->owner->dpy, window->wId, iTempInputWindowX, iTempInputWindowY);
}

void InputWindowClose(InputWindow* inputWindow)
{
    XUnmapWindow(inputWindow->parent.owner->dpy, inputWindow->parent.wId);
}

void InputWindowReload(void* arg, boolean enabled)
{
    InputWindow* inputWindow = (InputWindow*) arg;
    boolean visable = WindowIsVisable(inputWindow->parent.owner->dpy, inputWindow->parent.wId);
    FcitxXlibWindowDestroy(&inputWindow->parent);

    InputWindowInit(inputWindow);

    if (visable)
        InputWindowShow(inputWindow);
}

void InputWindowShow(InputWindow* inputWindow)
{
    if (!WindowIsVisable(inputWindow->parent.owner->dpy, inputWindow->parent.wId))
        InputWindowMoveWindow(&inputWindow->parent);
    XMapRaised(inputWindow->parent.owner->dpy, inputWindow->parent.wId);
    FcitxXlibWindowPaint(&inputWindow->parent);
}

void InputWindowPaint(FcitxXlibWindow* window, cairo_t* c)
{
    InputWindow* inputWindow = (InputWindow*) window;
    FcitxClassicUI* classicui = window->owner;
    FcitxInstance* instance = classicui->owner;
    FcitxInputState* input = FcitxInstanceGetInputState(instance);
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxSkin* sc = &window->owner->skin;

    FcitxMessages* msgup = inputWindow->msgUp;
    FcitxMessages* msgdown = inputWindow->msgDown;
    char **strUp = inputWindow->strUp;
    char **strDown = inputWindow->strDown;
    int *posUpX = inputWindow->posUpX, *posUpY = inputWindow->posUpY;
    int *posDownX = inputWindow->posDownX, *posDownY = inputWindow->posDownY;

    cairo_save(c);
    cairo_set_operator(c, CAIRO_OPERATOR_OVER);

    SkinImage* prev = LoadImage(sc, sc->skinInputBar.backArrow, false);
    SkinImage* next = LoadImage(sc, sc->skinInputBar.forwardArrow, false);

    inputWindow->prevRect.x1 = inputWindow->prevRect.y1 = inputWindow->prevRect.x2 = inputWindow->prevRect.y2 = 0;
    inputWindow->nextRect.x1 = inputWindow->nextRect.y1 = inputWindow->nextRect.x2 = inputWindow->nextRect.y2 = 0;

    if (FcitxCandidateWordHasPrev(candList)
        || FcitxCandidateWordHasNext(candList)
    ) {
        if (prev && next) {
            int x, y;
            x = window->contentWidth - sc->skinInputBar.iBackArrowX + window->background->marginRight - window->background->marginLeft;
            y = sc->skinInputBar.iBackArrowY - window->background->marginTop;
            cairo_set_source_surface(c, prev->image, x, y);
            if (FcitxCandidateWordHasPrev(candList)) {
                inputWindow->prevRect.x1 = x;
                inputWindow->prevRect.y1 = y;
                inputWindow->prevRect.x2 = x + cairo_image_surface_get_width(prev->image);
                inputWindow->prevRect.y2 = y + cairo_image_surface_get_height(prev->image);

                if (inputWindow->highlight == PREVNEXT_HIGHLIGHT(true)) {
                    cairo_paint_with_alpha(c, 0.7);
                } else {
                    cairo_paint(c);
                }
            }
            else {
                cairo_paint_with_alpha(c, 0.3);
            }

            x = window->contentWidth - sc->skinInputBar.iForwardArrowX + window->background->marginRight - window->background->marginLeft;
            y = sc->skinInputBar.iForwardArrowY - window->background->marginTop;
            cairo_set_source_surface(c, next->image, x, y);
            if (FcitxCandidateWordHasNext(candList)) {
                inputWindow->nextRect.x1 = x;
                inputWindow->nextRect.y1 = y;
                inputWindow->nextRect.x2 = x + cairo_image_surface_get_width(prev->image);
                inputWindow->nextRect.y2 = y + cairo_image_surface_get_height(prev->image);
                if (inputWindow->highlight == PREVNEXT_HIGHLIGHT(false)) {
                    cairo_paint_with_alpha(c, 0.7);
                } else {
                    cairo_paint(c);
                }
            } else {
                cairo_paint_with_alpha(c, 0.3);
            }
        }
    }
    cairo_restore(c);
    cairo_save(c);

    /* draw text */
    FcitxCairoTextContext* ctc = FcitxCairoTextContextCreate(c);
    int dpi = sc->skinFont.respectDPI? classicui->dpi : 0;
    FcitxCairoTextContextSet(ctc, window->owner->font, window->owner->fontSize > 0 ? window->owner->fontSize : sc->skinFont.fontSize, dpi);

    int i;
    for (i = 0; i < FcitxMessagesGetMessageCount(msgup) ; i++) {
        FcitxCairoTextContextOutputString(ctc, strUp[i], posUpX[i], posUpY[i], &sc->skinFont.fontColor[FcitxMessagesGetMessageType(msgup, i) % 7]);
        if (strUp[i] != FcitxMessagesGetMessageString(msgup, i))
            free(strUp[i]);
    }

    int candidateIndex = -1;
    for (i = 0; i < FcitxMessagesGetMessageCount(msgdown) ; i++) {

        if (FcitxMessagesGetMessageType(msgdown, i) == MSG_INDEX) {
            candidateIndex ++;
        }

        FcitxConfigColor color = sc->skinFont.fontColor[FcitxMessagesGetMessageType(msgdown, i) % 7];
        double alpha = 1.0;
        if (CANDIDATE_HIGHLIGHT(candidateIndex) == inputWindow->highlight) {
            /* cal highlight color */
            color.r = (color.r + 0.5) / 2;
            color.g = (color.g + 0.5) / 2;
            color.b = (color.b + 0.5) / 2;
            alpha = 0.8;
        }
        cairo_set_source_rgba(c, color.r, color.g, color.b, alpha);

        FcitxCairoTextContextOutputString(ctc, strDown[i], posDownX[i], posDownY[i], NULL);
        if (strDown[i] != FcitxMessagesGetMessageString(msgdown, i))
            free(strDown[i]);
    }
    FcitxCairoTextContextFree(ctc);

    cairo_restore(c);

    // draw cursor
    if (FcitxMessagesGetMessageCount(msgup) && FcitxInputStateGetShowCursor(input)) {
        cairo_save(c);
        int cursorY1, cursorY2;
        if (sc->skinFont.respectDPI) {
            cursorY1 = sc->skinInputBar.iInputPos;
            cursorY2 = sc->skinInputBar.iInputPos + inputWindow->fontHeight;
        }
        else {
            cursorY1 = sc->skinInputBar.iInputPos - inputWindow->fontHeight;
            cursorY2 = sc->skinInputBar.iInputPos;
        }

        fcitx_cairo_set_color(c, &sc->skinInputBar.cursorColor);
        cairo_set_line_width(c, 1);
        cairo_move_to(c, inputWindow->pixelCursorPos + 0.5, cursorY1);
        cairo_line_to(c, inputWindow->pixelCursorPos + 0.5, cursorY2);
        cairo_stroke(c);

        cairo_restore(c);
    }

    FcitxMessagesSetMessageChanged(msgup, false);
    FcitxMessagesSetMessageChanged(msgdown, false);
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
