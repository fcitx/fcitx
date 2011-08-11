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

#ifndef CLASSICUIINTERFACE_H
#define CLASSICUIINTERFACE_H

#include <cairo.h>
#include <fcitx-config/fcitx-config.h>

#define FCITX_CLASSIC_UI_NAME "fcitx-classic-ui"
#define FCITX_CLASSIC_UI_LOADIMAGE 0
#define FCITX_CLASSIC_UI_LOADIMAGE_RETURNTYPE cairo_surface_t*
#define FCITX_CLASSIC_UI_GETKEYBOARDFONTCOLOR 1
#define FCITX_CLASSIC_UI_GETKEYBOARDFONTCOLOR_RETURNTYPE ConfigColor *
#define FCITX_CLASSIC_UI_GETFONT 2
#define FCITX_CLASSIC_UI_GETFONT_RETURNTYPE char**

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0; 
