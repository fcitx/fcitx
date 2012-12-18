/* An alternative to qsort, with an identical interface.
   This file is part of the GNU C Library.
   Copyright (C) 1992, 1995, 1996, 1997 Free Software Foundation, Inc.
   Written by Mike Haertel, September 1988.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc.
   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
   */

/* comes from ftp://ftp.usa.openbsd.org/pub/OpenBSD/distfiles//pan-0.14.2.tar.gz
 * fcitx need a stable sort to work properly.
 */


#include <stdlib.h>
#include <string.h>
#include "utarray.h"
#include "fcitx/fcitx.h"

static inline void iswap(void *a, void *b, size_t size)
{
    register char *x = a;
    register char *y = b;
    register char *z = x + size;
    while (x < z) {
        register char tmp = *x;
        *x = *y;
        *y = tmp;
        ++x;
        ++y;
    }
}

static void
fcitx_msort_r_with_tmp(
    void *b,
    size_t n,
    size_t s,
    int(*cmp)(const void *, const void *, void *),
    void* thunk,
    char* t)
{
    char *tmp;
    char *b1, *b2;
    size_t n1, n2;
    const int opsiz = sizeof(unsigned long int);

    if (n <= 1)
        return;

    n1 = n / 2;
    n2 = n - n1;
    b1 = b;
    b2 = (char *) b + (n1 * s);

    fcitx_msort_r_with_tmp(b1, n1, s, cmp, thunk, t);
    fcitx_msort_r_with_tmp(b2, n2, s, cmp, thunk, t);

    tmp = t;

    if (s == opsiz && (b1 - (char *) 0) % opsiz == 0)
        /* operating on aligned words.  Use direct word stores. */
        while (n1 > 0 && n2 > 0) {
            if ((*cmp)(b1, b2, thunk) <= 0) {
                --n1;
                *((unsigned long int *) tmp) =
                    *((unsigned long int *) b1);
                tmp += opsiz;
                b1 += opsiz;
            } else {
                --n2;
                *((unsigned long int *) tmp) =
                    *((unsigned long int *) b2);
                tmp += opsiz;
                b2 += opsiz;
            }
        }
    else
        while (n1 > 0 && n2 > 0) {
            if ((*cmp)(b1, b2, thunk) <= 0) {
                memcpy(tmp, b1, s);
                tmp += s;
                b1 += s;
                --n1;
            } else {
                memcpy(tmp, b2, s);
                tmp += s;
                b2 += s;
                --n2;
            }
        }
    if (n1 > 0)
        memcpy(tmp, b1, n1 * s);
    memcpy(b, t, (n - n2) * s);
}

FCITX_EXPORT_API
void
fcitx_msort_r(void *b, size_t n, size_t s, int(*cmp)(const void *, const void*, void *), void* thunk)
{
    if (n < 10) {
        size_t i;
        while (n > 1) {
            char *min = b;
            char *tmp = min + s;
            for (i = 1; i < n; ++i) {
                if (cmp(tmp, min, thunk) < 0)
                    min = tmp;
                tmp += s;
            }
            iswap(min, b, s);
            b += s;
            n -= 1;
        }
    } else {
        const size_t size = n * s;

        char *tmp = malloc(size);
        if (tmp == NULL) {
            /* Couldn't get space, so fall back to the system sorter */
            fcitx_qsort_r(b, n, s, cmp, thunk);
        } else {
            fcitx_msort_r_with_tmp(b, n, s, cmp, thunk, tmp);
            free(tmp);
        }
    }
}
