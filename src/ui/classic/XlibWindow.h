#ifndef XLIBWINDOW_H
#define XLIBWINDOW_H

#include <cairo.h>
#include <X11/Xlib.h>

#include "module/x11/x11stuff.h"

struct _FcitxClassicUI;

typedef struct _FcitxXlibWindow FcitxXlibWindow;
typedef void (*FcitxMoveWindowFunc)(FcitxXlibWindow* window);
typedef void (*FcitxPaintContentFunc)(FcitxXlibWindow* window, cairo_t* c);
typedef void (*FcitxCalculateContentSizeFunc)(FcitxXlibWindow* window, unsigned int* contentWidth, unsigned int* contentHeight);

struct _FcitxXlibWindow {
    Window wId;
    struct _FcitxWindowBackground* background;
    unsigned int width, height;

    cairo_surface_t *xlibSurface;
    cairo_surface_t *contentSurface;
    cairo_surface_t *backgroundSurface;

    struct _FcitxClassicUI *owner;
    FcitxMoveWindowFunc MoveWindow;
    FcitxCalculateContentSizeFunc CalculateContentSize;
    FcitxPaintContentFunc paintContent;
    unsigned int oldContentWidth;
    unsigned int oldContentHeight;
    int contentX;
    int contentY;
    unsigned int contentHeight;
    unsigned int contentWidth;
    unsigned int epoch;
};

void* FcitxXlibWindowCreate(struct _FcitxClassicUI* classicui, size_t size);
void FcitxXlibWindowInit(FcitxXlibWindow* window,
                         uint width, uint height,
                         int x, int y,
                         char* name,
                         FcitxXWindowType windowType,
                         struct _FcitxWindowBackground* background,
                         long int inputMask,
                         FcitxMoveWindowFunc moveWindow,
                         FcitxCalculateContentSizeFunc calculateContentSize,
                         FcitxPaintContentFunc paintContent
                        );
void FcitxXlibWindowDestroy(FcitxXlibWindow* window);
void FcitxXlibWindowPaint(FcitxXlibWindow* window);

#endif // XLIBWINDOW_H
