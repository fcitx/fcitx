/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#ifndef TABLEPINYINWRAPPER_H
#define TABLEPINYINWRAPPER_H

#include "fcitx/addon.h"
#include "fcitx/candidate.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/ime.h"

INPUT_RETURN_VALUE Table_PYGetCandWord(void *arg,
                                       FcitxCandidateWord *candidateWord);

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
