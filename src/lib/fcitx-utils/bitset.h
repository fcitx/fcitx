/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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


#ifndef _FCITX_UTILS_BITSET_H_
#define _FCITX_UTILS_BITSET_H_

#include <string.h>
#include <stdint.h>
#include <fcitx-utils/utils.h>

typedef struct _FcitxBitSet FcitxBitSet;

struct _FcitxBitSet {
    size_t size;
    uint8_t data[1];
};

static inline size_t fcitx_bitset_datasize(size_t bits)
{
    return (bits + 7) / 8;
}

static inline size_t fcitx_bitset_size(size_t bits)
{
    return (sizeof(size_t) + fcitx_bitset_datasize(bits));
}

/**
 * create a bitset with size
 *
 * @param bits bits inside this bitset
 *
 * @return bitset
 */
static inline FcitxBitSet* fcitx_bitset_new(size_t bits)
{
    /* round up */
    FcitxBitSet* bitset = fcitx_utils_malloc0(fcitx_bitset_size(bits));
    bitset->size = bits;
    return bitset;
}

static inline void fcitx_bitset_copy(FcitxBitSet* dst, FcitxBitSet* src)
{
    if (dst->size != src->size)
        return;
    memcpy(dst->data, src->data, fcitx_bitset_datasize(dst->size));
}

/**
 * clear entire bitset
 *
 * @param bitset bitset
 * @return void
 */
static inline void fcitx_bitset_clear(FcitxBitSet* bitset)
{
    memset(bitset->data, 0, fcitx_bitset_datasize(bitset->size));
}

/**
 * check a bit in bitset is set
 *
 * @param bitset bitset
 * @param offset bit offset
 * @return non-zero for is set
 */
static inline uint8_t fcitx_bitset_isset(FcitxBitSet* bitset, size_t offset)
{
    size_t byte = offset / 8;
    size_t byteoffset = offset % 8;
    return bitset->data[byte] & (1 << byteoffset);
}

/**
 * set a bit in bitset
 *
 * @param bitset bitset
 * @param offset bit offset
 * @return void
 */
static inline void fcitx_bitset_set(FcitxBitSet* bitset, size_t offset)
{
    size_t byte = offset / 8;
    size_t byteoffset = offset % 8;
    bitset->data[byte] |= (1 << byteoffset);
}

/**
 * clear a bit in bitset
 *
 * @param bitset bitset
 * @param offset bit offset
 * @return void
 */
static inline void fcitx_bitset_unset(FcitxBitSet* bitset, size_t offset)
{
    size_t byte = offset / 8;
    size_t byteoffset = offset % 8;
    uint8_t s = (1 << byteoffset);
    s = ~s;
    bitset->data[byte] &= s;
}

/**
 * free bitset is set
 *
 * @param bitset bitset
 * @return void
 */
#define fcitx_bitset_free(bitset) free(bitset)

#endif
