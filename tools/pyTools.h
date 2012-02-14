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

#ifndef _PY_TOOLS_H
#define _PY_TOOLS_H

#include "fcitx-utils/utf8.h"

/**
 * Code table for Pinyin
 **/

struct _PYMB {
    int PYFAIndex;
    char HZ[UTF8_MAX_LENGTH + 1];
    int UserPhraseCount;

    struct {
        int Length;
        char *Map;
        char *Phrase;
        int Index;
        int Hit;
    } *UserPhrase;
};

struct _HZMap {
    char Map[3];
    int BaseCount;
    char **HZ;
    int *Index;
};

int LoadPYBase(FILE *, struct _HZMap **);
void LoadPYMB(FILE *fi, struct _PYMB **pPYMB, int isUser);

#endif /* _PY_TOOLS_H */


// kate: indent-mode cstyle; space-indent on; indent-width 4;
