/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
#ifndef CAIROSTUFF_H
#define CAIROSTUFF_H

#include "config.h"
#include <cairo.h>

int StringWidth(const char *str, const char *font, int fontSize);
void OutputString(cairo_t * c, const char *str, const char *font, int fontSize, int x,  int y, ConfigColor * color);

#ifdef _ENABLE_PANGO

#include <pango/pango.h>

#define OutputStringWithContext(c,str,x,y) OutputStringWithContextReal(c, fontDesc, str, x, y)
#define StringSizeWithContext(c,str,w,h) StringSizeWithContextReal(c, fontDesc, str, w, h)
#define FontHeightWithContext(c) FontHeightWithContextReal(c, fontDesc)

PangoFontDescription* GetPangoFontDescription(const char* font, int size);
void OutputStringWithContextReal(cairo_t * c, PangoFontDescription* desc, const char *str, int x, int y);
void StringSizeWithContextReal(cairo_t * c, PangoFontDescription* fontDesc, const char *str, int* x, int* h);
int FontHeightWithContextReal(cairo_t* c, PangoFontDescription* fontDesc);

#define SetFontContext(context, fontname, size) \
    PangoFontDescription* fontDesc = GetPangoFontDescription(fontname, size)

#define ResetFontContext() \
    do { \
        pango_font_description_free(fontDesc); \
    } while(0)

#else

#define OutputStringWithContext(c,str,x,y) OutputStringWithContextReal(c, str, x, y)
#define StringSizeWithContext(c,str,w,h) StringSizeWithContextReal(c, str, w, h)
#define FontHeightWithContext(c) FontHeightWithContextReal(c)

void OutputStringWithContextReal(cairo_t * c, const char *str, int x, int y);
void StringSizeWithContextReal(cairo_t * c, const char *str, int* x, int* h);
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

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
