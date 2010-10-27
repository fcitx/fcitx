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
#ifndef _UI_H
#define _UI_H

#include <X11/Xlib.h>
#include <cairo.h>

#ifdef _ENABLE_PANGO
#include <pango/pangocairo.h>
#endif

#include "core/fcitx.h"
#include "fcitx-config/fcitx-config.h"

Bool InitX (void);
void MyXEventHandler (XEvent * event);

void OutputString (cairo_t* c, const char *str, const char *font, int fontSize, int x, int y, ConfigColor* color);
int StringWidth (const char *str, const char *font, int fontSize);
int FontHeight (const char *font);

Bool MouseClick (int *x, int *y, Window window);
Bool IsWindowVisible(Window window);
void InitWindowAttribute(Visual** vs, Colormap *cmap, XSetWindowAttributes *attrib, unsigned long *attribmask, int* depth);
void ActiveWindow(Display *dpy, Window window);

#ifdef _ENABLE_PANGO
#define OutputStringWithContext(c,str,x,y) OutputStringWithContextReal(c, fontDesc, str, x, y)
#define StringWidthWithContext(c,str) StringWidthWithContextReal(c, fontDesc, str)
#define FontHeightWithContext(c) FontHeightWithContextReal(c, fontDesc)

PangoFontDescription* GetPangoFontDescription(const char* font, int size);
void OutputStringWithContextReal(cairo_t * c, PangoFontDescription* desc, const char *str, int x, int y);
int StringWidthWithContextReal(cairo_t * c, PangoFontDescription* fontDesc, const char *str);
int FontHeightWithContextReal(cairo_t* c, PangoFontDescription* fontDesc);

#define SetFontContext(context, fontname, size) \
    PangoFontDescription* fontDesc = GetPangoFontDescription(fontname, size)
    
#define ResetFontContext() \
    do { \
        pango_font_description_free(fontDesc); \
    } while(0)

#else

#define OutputStringWithContext(c,str,x,y) OutputStringWithContextReal(c, str, x, y)
#define StringWidthWithContext(c,str) StringWidthWithContextReal(c, str)
#define FontHeightWithContext(c) FontHeightWithContextReal(c)

void OutputStringWithContextReal(cairo_t * c, const char *str, int x, int y);
int StringWidthWithContextReal(cairo_t * c, const char *str);
int FontHeightWithContextReal(cairo_t* c);

#define SetFontContext(context, fontname, size) \
    do { \
        cairo_select_font_face(context, fontname, \
                CAIRO_FONT_SLANT_NORMAL, \
                CAIRO_FONT_WEIGHT_NORMAL); \
        cairo_set_font_size(context, size); \
    } while (0)

#define ResetFontContext()

#endif

Bool IsInBox(int x0, int y0, int x1, int y1, int x2, int y2);
Bool IsInRspArea(int x0, int y0, FcitxImage img);

#endif
