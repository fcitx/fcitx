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
#ifndef _FCITX_H_
#define _FCITX_H_

/**
 * @mainpage Fcitx
 * fcitx is a lightweight Input Method Framework, written by C.
 * It can be used under X11 to support international input.
 *
 */

/**
 * @file fcitx.h
 * @author CS Slayer <wengxt@gmail.com>
 * @brief some misc definition for Fcitx
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(FCITX_HAVE_CONFIG_H)
#include "config.h"
#define _(msgid) gettext(msgid)
#define __(msgid) (msgid)
#endif

#define FCITX_EXPORT_API __attribute__ ((visibility("default")))

#define FCITX_UNUSED(x) (void)(x)

#define FCITX_ABI_VERSION 5

#ifdef __cplusplus
}
#endif

#endif/*_FCITX_H_*/

// kate: indent-mode cstyle; space-indent on; indent-width 0;
