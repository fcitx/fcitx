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

typedef struct _FcitxCairoTextContext FcitxCairoTextContext;

FcitxCairoTextContext* FcitxCairoTextContextCreate(cairo_t* cr);
void FcitxCairoTextContextFree(FcitxCairoTextContext* ctc);
void FcitxCairoTextContextSet(FcitxCairoTextContext* ctc, const char* font, int fontSize, int dpi);
void FcitxCairoTextContextStringSize(FcitxCairoTextContext* ctc, const char* str, int* w, int* h);
void FcitxCairoTextContextStringSizeStrict(FcitxCairoTextContext* ctc, const char* str, int* w, int* h);
int FcitxCairoTextContextStringWidth(FcitxCairoTextContext* ctc, const char *str);
int FcitxCairoTextContextFontHeight(FcitxCairoTextContext* ctc);
void FcitxCairoTextContextOutputString(FcitxCairoTextContext* ctc, const char *str, int x, int y, FcitxConfigColor* color);

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
