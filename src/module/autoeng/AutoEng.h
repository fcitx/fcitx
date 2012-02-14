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
/**
 * @file   AutoEng.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * Auto Switch to English State
 *
 */

#ifndef _FCITX_AUTOENG_H_
#define _FCITX_AUTOENG_H_

#define MAX_AUTO_TO_ENG 10
#include "fcitx-config/fcitx-config.h"

typedef struct _AUTO_ENG {
    char            str[MAX_AUTO_TO_ENG + 1];
} AUTO_ENG;

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
