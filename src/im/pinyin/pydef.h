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

#ifndef PYDEF_H
#define PYDEF_H

#define MAX_WORDS_USER_INPUT    32
#define MAX_PY_PHRASE_LENGTH    10
#define MAX_PY_LENGTH       6

#define FCITX_PINYIN_NAME "fcitx-pinyin"
#define FCITX_PINYIN_LOADBASEDICT 0
#define FCITX_PINYIN_LOADBASEDICT_RETURNTYPE void
#define FCITX_PINYIN_PYGETPYBYHZ 1
#define FCITX_PINYIN_PYGETPYBYHZ_RETURNTYPE void
#define FCITX_PINYIN_DOPYINPUT 2
#define FCITX_PINYIN_DOPYINPUT_RETURNTYPE void
#define FCITX_PINYIN_PYGETCANDWORDS 3
#define FCITX_PINYIN_PYGETCANDWORDS_RETURNTYPE void
#define FCITX_PINYIN_PYGETFINDSTRING 4
#define FCITX_PINYIN_PYGETFINDSTRING_RETURNTYPE char*
#define FCITX_PINYIN_PYRESET 5
#define FCITX_PINYIN_PYRESET_RETURNTYPE void
#define FCITX_PINYIN_SP2QP 6
#define FCITX_PINYIN_SP2QP_RETURNTYPE char*
#define FCITX_PINYIN_ADDUSERPHRASE 7
#define FCITX_PINYIN_ADDUSERPHRASE_RETURNTYPE void

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
