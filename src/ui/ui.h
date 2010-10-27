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
#include <pango/pangocairo.h>
#include "fcitx-config/fcitx-config.h"

Bool InitX (void);
void MyXEventHandler (XEvent * event);

void OutputString (cairo_t* c, const char *str, const char *font, int fontSize, int x, int y, ConfigColor* color);
void OutputStringWithContext(cairo_t * c, PangoFontDescription* desc, const char *str, int x, int y);
int StringWidth (const char *str, const char *font, int fontSize);
int StringWidthWithContext(cairo_t * c, PangoFontDescription* fontDesc, const char *str);
int FontHeight (const char *font);
int FontHeightWithContext(cairo_t* c, PangoFontDescription* fontDesc);

Bool MouseClick (int *x, int *y, Window window);
Bool IsWindowVisible(Window window);
void InitWindowAttribute(Visual** vs, Colormap *cmap, XSetWindowAttributes *attrib, unsigned long *attribmask, int* depth);
void ActiveWindow(Display *dpy, Window window);
PangoFontDescription* GetPangoFontDescription(const char* font, int size);

Bool IsInBox(int x0, int y0, int x1, int y1, int x2, int y2);
Bool IsInRspArea(int x0, int y0, FcitxImage img);

#endif
