#include "XlibWindow.h"
#include "classicui.h"
#include <cairo-xlib.h>
#include <X11/extensions/shape.h>

void* FcitxXlibWindowCreate(FcitxClassicUI* classicui, size_t size)
{
    FcitxXlibWindow* window = fcitx_utils_malloc0(size);
    window->owner = classicui;
    return window;
}

void FcitxXlibWindowInit(FcitxXlibWindow* window,
                         uint width, uint height,
                         int x, int y,
                         char* name,
                         FcitxXWindowType windowType,
                         FcitxWindowBackground* background,
                         long int inputMask,
                         FcitxMoveWindowFunc moveWindow,
                         FcitxCalculateContentSizeFunc calculateContentSize,
                         FcitxPaintContentFunc paintContent
                        )
{
    FcitxClassicUI* classicui = window->owner;
    int depth;
    Colormap cmap;
    int iScreen = classicui->iScreen;
    Display* dpy = classicui->dpy;
    FcitxSkin *sc = &classicui->skin;
    window->wId = None;
    window->height = height;
    window->width = width;
    window->background = background;
    window->MoveWindow = moveWindow;
    window->CalculateContentSize = calculateContentSize;
    window->paintContent = paintContent;
    window->oldContentHeight = 0;
    window->oldContentWidth = 0;

    SkinImage *back = NULL;

    if (background) {
        back = LoadImage(sc, background->background, false);
        /* for cache reason */
        LoadImage(sc, background->overlay, false);
    }

    if (back) {
        window->width = cairo_image_surface_get_width(back->image);
        window->height = cairo_image_surface_get_height(back->image);
    }

    Visual* vs = ClassicUIFindARGBVisual(classicui);

    XSetWindowAttributes attrib;
    unsigned long        attribmask;
    FcitxX11InitWindowAttribute(classicui->owner, &vs, &cmap, &attrib, &attribmask, &depth);

    window->wId = XCreateWindow(dpy,
                                RootWindow(dpy, iScreen),
                                x, y,
                                window->width,
                                window->height,
                                0,
                                depth, InputOutput,
                                vs, attribmask,
                                &attrib);

    window->xlibSurface = cairo_xlib_surface_create(dpy, window->wId, vs, window->width, window->height);

    if (background && back) {
        window->contentSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, window->width, window->height);
        window->backgroundSurface = cairo_surface_create_similar(window->contentSurface, CAIRO_CONTENT_COLOR_ALPHA,
                                                                     window->width, window->height);
    }

    XSelectInput(dpy, window->wId, inputMask);

    ClassicUISetWindowProperty(classicui, window->wId, windowType, name);
}


static inline
FcitxRect RectUnion(FcitxRect rt1, FcitxRect rt2)
{
    FcitxRect rt = {
        FCITX_MIN(rt1.x1,rt2.x1),
        FCITX_MIN(rt1.y1,rt2.y1),
        FCITX_MAX(rt1.x2,rt2.x2),
        FCITX_MAX(rt1.y2,rt2.y2)
    };

    return rt;
}

void FcitxXlibWindowPaintBackground(FcitxXlibWindow* window,
                                    cairo_t* c,
                                    unsigned int offX, unsigned int offY,
                                    unsigned int contentWidth, unsigned int contentHeight,
                                    unsigned int overlayX, unsigned int overlayY
                                   )
{
    FcitxClassicUI* classicui = window->owner;
    /* just clean the background with nothing */
    cairo_save(c);
    cairo_set_source_rgba(c, 0, 0, 0, 0);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c);
    cairo_restore(c);
    int backgrondWidth = contentWidth;
    int backgrondHeight = contentHeight;
    do {
        SkinImage* back = NULL;
        if (!window->background || (back = LoadImage(&window->owner->skin, window->background->background, false)) == NULL) {
            break;
        }

        backgrondWidth += window->background->marginLeft + window->background->marginRight;
        backgrondHeight += window->background->marginTop + window->background->marginBottom;

        if (window->epoch != classicui->epoch || window->oldContentWidth != contentWidth || window->oldContentHeight != contentHeight) {
            window->epoch = classicui->epoch;
            window->oldContentHeight = contentHeight;
            window->oldContentWidth = contentWidth;

            EnlargeCairoSurface(&window->backgroundSurface, backgrondWidth, backgrondHeight);

            cairo_t* backgroundcr = cairo_create(window->backgroundSurface);
            DrawResizableBackground(backgroundcr,
                                    back->image,
                                    backgrondWidth, backgrondHeight,
                                    window->background->marginLeft,
                                    window->background->marginTop,
                                    window->background->marginRight,
                                    window->background->marginBottom,
                                    window->background->fillV,
                                    window->background->fillH
                                );
            cairo_destroy(backgroundcr);
            cairo_surface_flush(window->backgroundSurface);
        }

        cairo_save(c);
        cairo_translate(c, offX, offY);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_surface(c, window->backgroundSurface, 0, 0);
        cairo_rectangle(c, 0, 0, backgrondWidth, backgrondHeight);
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);

    } while (0);

    SkinImage* overlay = NULL;
    do {
        if (!window->background || (overlay = LoadImage(&window->owner->skin, window->background->overlay, false)) == NULL) {
            break;
        }

        cairo_save(c);
        cairo_translate(c, overlayX, overlayY);
        cairo_set_operator(c, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface(c, overlay->image, 0, 0);
        cairo_rectangle(c, 0, 0, cairo_image_surface_get_width(overlay->image), cairo_image_surface_get_height(overlay->image));
        cairo_clip(c);
        cairo_paint(c);
        cairo_restore(c);
    } while (0);

    if (overlay
        || window->background->clickMarginLeft != 0
        || window->background->clickMarginRight != 0
        || window->background->clickMarginTop != 0
        || window->background->clickMarginBottom != 0) {
        XRectangle r[1];
        r[0].x = 0;
        r[0].y = 0;
        r[0].width = backgrondWidth - window->background->clickMarginLeft - window->background->clickMarginRight;
        r[0].height = backgrondHeight - window->background->clickMarginTop - window->background->clickMarginBottom;
        XShapeCombineRectangles(classicui->dpy, window->wId, ShapeInput,
                                offX + window->background->clickMarginLeft,
                                offY + window->background->clickMarginTop,
                                r, 1, ShapeSet, Unsorted
                               );
    } else {
        XShapeCombineMask(classicui->dpy, window->wId, ShapeBounding, 0, 0,
                          None, ShapeSet);
    }
}

void FcitxXlibWindowPaint(FcitxXlibWindow* window)
{
    FcitxClassicUI* classicui = window->owner;
    FcitxSkin *sc = &classicui->skin;
    int oldWidth = window->width, oldHeight = window->height;
    unsigned int contentHeight = 0, contentWidth = 0;
    window->CalculateContentSize(window, &contentWidth, &contentHeight);
    FcitxRect rect, mergedRect;
    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = contentWidth + (window->background ? (window->background->marginLeft + window->background->marginRight) : 0);
    rect.y2 = contentHeight + (window->background ? (window->background->marginTop + window->background->marginBottom) : 0);
    mergedRect = rect;

    unsigned int overlayX = 0, overlayY = 0;
    unsigned int offX = 0, offY = 0;
    SkinImage* overlayImage = NULL;
    if (window->background) {
        if (window->background->overlay[0]) {
            overlayImage =  LoadImage(sc, window->background->overlay, false);
        }

        unsigned int overlayW = 0, overlayH = 0;
    #define _POS(P, X, Y) case P: overlayX = X; overlayY = Y; break;
        switch (window->background->dock) {
            _POS(OD_TopLeft, 0, 0)
            _POS(OD_TopCenter, rect.x2 / 2, 0)
            _POS(OD_TopRight, rect.x2, 0)
            _POS(OD_CenterLeft, 0, rect.y2 / 2)
            _POS(OD_Center, rect.x2 / 2, rect.y2 / 2)
            _POS(OD_CenterRight, rect.x2, rect.y2 / 2)
            _POS(OD_BottomLeft, 0, rect.y2)
            _POS(OD_BottomCenter, rect.x2 / 2, rect.y2)
            _POS(OD_BottomRight, rect.x2, rect. y2)
        }

        overlayX += window->background->overlayOffsetX;
        overlayY += window->background->overlayOffsetY;

        if (overlayImage) {
            overlayW = cairo_image_surface_get_width(overlayImage->image);
            overlayH = cairo_image_surface_get_height(overlayImage->image);
        }

        FcitxRect overlayRect = {overlayX, overlayY, overlayX + overlayW, overlayY + overlayH };
        mergedRect = RectUnion(rect, overlayRect);
        offX = 0 - mergedRect.x1;
        offY = 0 - mergedRect.y1;
        overlayX = overlayX - mergedRect.x1;
        overlayY = overlayY - mergedRect.y1;
    }

    int width = mergedRect.x2 - mergedRect.x1;
    int height = mergedRect.y2 - mergedRect.y1;

    EnlargeCairoSurface(&window->contentSurface, width, height);
    cairo_t* c = cairo_create(window->contentSurface);
    FcitxXlibWindowPaintBackground(window, c, offX, offY, contentWidth, contentHeight, overlayX, overlayY);

    if (overlayImage) {
        cairo_save(c);
        cairo_set_operator(c, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface(c, overlayImage->image, overlayX, overlayY);
        cairo_paint(c);
        cairo_restore(c);
    }

    window->contentX = offX + (window->background ? window->background->marginLeft : 0);
    window->contentY = offY + (window->background ? window->background->marginTop : 0);
    window->contentWidth = contentWidth;
    window->contentHeight = contentHeight;
    cairo_save(c);
    cairo_translate(c, window->contentX, window->contentY);
    window->paintContent(window, c);
    cairo_restore(c);
    cairo_destroy(c);
    cairo_surface_flush(window->contentSurface);

    boolean resizeFlag = false;
    if (width != oldWidth || height != oldHeight) {
        resizeFlag = true;
        window->width = width;
        window->height = height;
    }

    window->MoveWindow(window);
    if (resizeFlag) {
        cairo_xlib_surface_set_size(window->xlibSurface,
                                    window->width,
                                    window->height);
        XResizeWindow(window->owner->dpy,
                      window->wId,
                      window->width,
                      window->height);
    }

    cairo_t* xlibc = cairo_create(window->xlibSurface);
    cairo_set_operator(xlibc, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(xlibc, window->contentSurface, 0, 0);
    cairo_rectangle(xlibc, 0, 0, window->width, window->height);
    cairo_clip(xlibc);
    cairo_paint(xlibc);
    cairo_destroy(xlibc);
    cairo_surface_flush(window->xlibSurface);
}

void FcitxXlibWindowDestroy(FcitxXlibWindow* window)
{
    cairo_surface_destroy(window->contentSurface);
    cairo_surface_destroy(window->backgroundSurface);
    cairo_surface_destroy(window->xlibSurface);
    XDestroyWindow(window->owner->dpy, window->wId);
    window->wId = None;
}
