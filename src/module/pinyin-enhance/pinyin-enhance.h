/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#ifndef _PINYIN_ENHANCE_H
#define _PINYIN_ENHANCE_H

#include <stdint.h>
#include "fcitx-utils/utf8.h"

typedef struct {
    char word[UTF8_MAX_LENGTH + 1];
    int8_t count;
} FcitxPYEnhancePYList;

typedef struct {
    int8_t konsonant;
    int8_t vokal;
    int8_t tone;
} FcitxPYEnhancePY;

#ifdef __cplusplus
extern "C" {
#endif

    static inline FcitxPYEnhancePY*
    pinyin_enhance_pylist_get(const FcitxPYEnhancePYList *pylist, int index)
    {
        FcitxPYEnhancePY *py;
        py = (FcitxPYEnhancePY*)(((void*)(intptr_t)pylist) +
                                 sizeof(FcitxPYEnhancePYList));
        return py + index;
    }

#ifdef __cplusplus
}
#endif

#endif /* _PINYIN_ENHANCE_H */
