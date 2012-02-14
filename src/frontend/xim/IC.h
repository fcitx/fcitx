/******************************************************************

         Copyright 1994, 1995 by Sun Microsystems, Inc.
         Copyright 1993, 1994 by Hewlett-Packard Company

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Sun Microsystems, Inc.
and Hewlett-Packard not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.
Sun Microsystems, Inc. and Hewlett-Packard make no representations about
the suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.

SUN MICROSYSTEMS INC. AND HEWLETT-PACKARD COMPANY DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
SUN MICROSYSTEMS, INC. AND HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Author: Hidetoshi Tajima(tajima@Eng.Sun.COM) Sun Microsystems, Inc.

******************************************************************/
#ifndef _FCITX_IC_H_
#define _FCITX_IC_H_

#include "fcitx/frontend.h"

struct _FcitxXimFrontend;

/**
 * XIM Preedit Attributes
 **/
typedef struct {
    XRectangle      area;   /* area */
    XRectangle      area_needed;    /* area needed */
    XPoint          spot_location;  /* spot location */
    Colormap        cmap;   /* colormap */
    CARD32          foreground; /* foreground */
    CARD32          background; /* background */
    Pixmap          bg_pixmap;  /* background pixmap */
    char           *base_font;  /* base font of fontset */
    CARD32          line_space; /* line spacing */
    Cursor          cursor; /* cursor */
} PreeditAttributes;

/**
 * XIM Status Attributes
 **/
typedef struct {
    XRectangle      area;   /* area */
    XRectangle      area_needed;    /* area needed */
    Colormap        cmap;   /* colormap */
    CARD32          foreground; /* foreground */
    CARD32          background; /* background */
    Pixmap          bg_pixmap;  /* background pixmap */
    char           *base_font;  /* base font of fontset */
    CARD32          line_space; /* line spacing */
    Cursor          cursor; /* cursor */
} StatusAttributes;

/**
 * Input Context for Fcitx XIM Frontend
 **/
typedef struct _FcitxXimIC {
    CARD16          id;     /* ic id */
    INT32           input_style;    /* input style */
    Window          client_win; /* client window */
    Window          focus_win;  /* focus window */
    char           *resource_name;  /* resource name */
    char           *resource_class; /* resource class */
    PreeditAttributes pre_attr; /* preedit attributes */
    StatusAttributes sts_attr;  /* status attributes */

    CARD16 connect_id;
    int bPreeditStarted;
    uint onspot_preedit_length;
    boolean bHasCursorLocation;
    int offset_x;
    int offset_y;
} FcitxXimIC;

struct FcitxXimFrontend;

void     XimCreateIC(void* arg, FcitxInputContext* context, void *priv);
void     XimDestroyIC(void* arg, FcitxInputContext* arg1);
boolean  XimCheckIC(void* arg, FcitxInputContext* arg1, void* arg2);
void     XimSetIC(struct _FcitxXimFrontend* xim, IMChangeICStruct * call_data);
void     XimGetIC(struct _FcitxXimFrontend* xim, IMChangeICStruct * call_data);
boolean  XimCheckICFromSameApplication(void* arg, FcitxInputContext* icToCheck, FcitxInputContext* ic);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
