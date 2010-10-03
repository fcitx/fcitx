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
#ifndef _FCITX_H_
#define _FCITX_H_

#define _GNU_SOURCE

#include <libintl.h>

#define _(msgid) gettext(msgid)
#define __(msgid) (msgid)

#include "tools/utf8.h"
#include "core/im.h"

#define EIM_MAX		4

#define KEYM_MASK	0xff0000
#define KEYM_CTRL	0x010000
#define KEYM_SHIFT	0x020000
#define KEYM_ALT	0x040000
#define KEYM_SUPER	0x080000

#define VK_CODE(x)	((x)&0xffff)

#define VK_ENTER	'\r'
#define VK_BACKSPACE	'\b'
#define VK_TAB		'\t'
#define VK_ESC		0x1b
#define VK_SPACE	0x20
#define VK_LSHIFT	0xe1
#define VK_RSHIFT	0xe2
#define VK_LCTRL	0xe3
#define VK_RCTRL	0xe4
#define VK_CAPSLOCK	0xe5
#define VK_LALT		0xe9
#define VK_RALT		0xea
#define VK_DELETE	0xff

#define VK_HOME		0xff50
#define VK_LEFT		0xff51
#define VK_UP		0xff52
#define VK_RIGHT	0xff53
#define VK_DOWN		0xff54
#define VK_PGUP		0xff55
#define VK_PGDN		0xff56
#define VK_END		0xff57
#define VK_INSERT	0xff63

#define MAX_CODE_LEN	63

#endif/*_FCITX_H_*/
