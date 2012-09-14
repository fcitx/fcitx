/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
#ifndef CAIROSTUFF_H
#define CAIROSTUFF_H

#include "config.h"
#include "fcitx-config/fcitx-config.h"
#include <cairo.h>

#if 0
#define _CAIRO_DESTROY(X) \
  do { \
      cairo_surface_t *_surface = cairo_get_target(X); \
      cairo_destroy(X); \
      cairo_surface_flush(_surface); \
  } while(0)

#define _CAIRO_SETSIZE(X, Y, Z) \
  do { \
      cairo_surface_t* _surface = cairo_xlib_surface_create( \
          cairo_xlib_surface_get_display(X), \
          cairo_xlib_surface_get_drawable(X), \
          cairo_xlib_surface_get_visual(X), \
          (Y), \
          (Z) \
          ); \
      cairo_surface_destroy(X); \
      X = _surface; \
  } while(0)
#else
    #define _CAIRO_SETSIZE(X, Y, Z) cairo_xlib_surface_set_size(X, Y, Z);
#endif

int StringWidth(const char* str, const char* font, int fontSize, int dpi);
int FontHeight(const char *font, int fontSize, int dpi);
void OutputString(cairo_t* c, const char* str, const char* font, int fontSize, int dpi, int x, int y, FcitxConfigColor* color);

void StringSizeStrict(const char* str, const char* font, int fontSize, int dpi, int* w, int* h);

#ifdef _ENABLE_PANGO

#include <pango/pango.h>

#define OutputStringWithContext(c,dpi,str,x,y) OutputStringWithContextReal(c, fontDesc, dpi, str, x, y)
#define StringSizeWithContext(c,dpi,str,w,h) StringSizeWithContextReal(c, fontDesc, dpi, str, w, h)
#define FontHeightWithContext(c,dpi) FontHeightWithContextReal(c, fontDesc, dpi)

PangoFontDescription* GetPangoFontDescription(const char* font, int size, int dpi);
void OutputStringWithContextReal(cairo_t * c, PangoFontDescription* desc, int dpi, const char *str, int x, int y);
void StringSizeWithContextReal(cairo_t* c, PangoFontDescription* fontDesc, int dpi, const char* str, int* w, int* h);
int FontHeightWithContextReal(cairo_t* c, PangoFontDescription* fontDesc, int dpi);

#define SetFontContext(context, fontname, size, dpi) \
    PangoFontDescription* fontDesc = GetPangoFontDescription(fontname, size, dpi)

#define ResetFontContext() \
    do { \
        pango_font_description_free(fontDesc); \
    } while(0)

#else

#define OutputStringWithContext(c,dpi,str,x,y) OutputStringWithContextReal(c, str, x, y)
#define StringSizeWithContext(c,dpi,str,w,h) StringSizeWithContextReal(c, str, w, h)
#define FontHeightWithContext(c,dpi) FontHeightWithContextReal(c)

void OutputStringWithContextReal(cairo_t * c, const char *str, int x, int y);
void StringSizeWithContextReal(cairo_t * c, const char *str, int* x, int* h);
int FontHeightWithContextReal(cairo_t* c);

#define SetFontContext(context, fontname, size, dpi) \
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
