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

#include <string.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/utarray.h"
#include "sort_common.h"

#define MINIMUM_INSERT_SORT 10

static void
fcitx_msort_r_with_tmp(
    void *b,
    size_t n,
    size_t s,
    int(*cmp)(const void *, const void *, void *),
    void* thunk,
    char* t)
{
    char* tmp;

    if (n < MINIMUM_INSERT_SORT) {
        insertion_sort(b, n, s, cmp, thunk);
        return;
    }

    /*
     * merge sort
     * (n_a_start - n_a_end) (n_b_start, n_b_end)
     */

    size_t n_a = n / 2;
    size_t n_b = n - n_a;
    void* b_a = b;
    void* b_b = ((char*) b) + n_a * s;

    fcitx_msort_r_with_tmp(b_a, n_a, s, cmp, thunk, t);
    fcitx_msort_r_with_tmp(b_b, n_b, s, cmp, thunk, t);

    tmp = t;

    while (n_a > 0 && n_b > 0) {
        if (cmp(b_a, b_b, thunk) <= 0) {
            memcpy(tmp, b_a, s);
            b_a += s;
            n_a --;
        } else {
            memcpy(tmp, b_b, s);
            b_b += s;
            n_b --;
        }
        tmp += s;
    }

    if (n_a > 0) {
        memcpy(tmp, b_a, n_a * s);
    }
    memcpy(b, t, (n - n_b) * s);
}


FCITX_EXPORT_API
void
fcitx_msort_r(void *b, size_t n, size_t s, int(*cmp)(const void *, const void*, void *), void* thunk)
{
    if (n < MINIMUM_INSERT_SORT) {
        insertion_sort(b, n, s, cmp, thunk);
    } else {
        const size_t size = n * s;

        char* tmp = malloc(size);

        if (!tmp) {
            fcitx_qsort_r(b, n, s, cmp, thunk);
        } else {
            fcitx_msort_r_with_tmp(b, n, s, cmp, thunk, tmp);
            free(tmp);
        }
    }
}
