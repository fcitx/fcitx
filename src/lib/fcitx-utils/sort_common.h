/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#ifndef _FCITX_SORT_COMMON_H
#define _FCITX_SORT_COMMON_H

#include <stdlib.h>

/* swap size bytes between a_ and b_ */
static inline void swap(void *a_, void *b_, size_t size)
{
    if (a_ == b_) return;
    {
        size_t i, nlong = size / sizeof(long);
        long *a = (long *) a_, *b = (long *) b_;
        for (i = 0; i < nlong; ++i) {
            long c = a[i];
            a[i] = b[i];
            b[i] = c;
        }
        a_ = (void*)(a + nlong);
        b_ = (void*)(b + nlong);
    }
    {
        size_t i;
        char *a = (char *) a_, *b = (char *) b_;
        size = size % sizeof(long);
        for (i = 0; i < size; ++i) {
            char c = a[i];
            a[i] = b[i];
            b[i] = c;
        }
    }
}

static inline void insertion_sort(void *base_, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *thunk)
{
    char *base = (char *) base_;
    size_t i, j;
    for (i = 0; i < nmemb; ++i) {
        for (j = i ; j-- > 0;) {
            if (compar(base + j * size, base + (j + 1) * size, thunk) <= 0) {
                break;
            }
            swap(base + j * size, base + (j + 1) * size, size);
        }
    }
}

#endif
