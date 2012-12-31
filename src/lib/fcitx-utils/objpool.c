/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#include <stdint.h>

#include "utils.h"
#include "objpool.h"
#include "fcitx/fcitx.h"

FCITX_EXPORT_API
void
fcitx_obj_pool_done(FcitxObjPool *pool)
{
    free(pool->array);
}

FCITX_EXPORT_API
FcitxObjPool*
(fcitx_obj_pool_new)(size_t size)
{
    return fcitx_obj_pool_new(size);
}

FCITX_EXPORT_API
FcitxObjPool*
fcitx_obj_pool_new_with_prealloc(size_t size, size_t prealloc)
{
    FcitxObjPool *pool = fcitx_utils_new(FcitxObjPool);
    fcitx_obj_pool_init_with_prealloc(pool, size, prealloc);
    return pool;
}

FCITX_EXPORT_API
void
fcitx_obj_pool_free(FcitxObjPool *pool)
{
    fcitx_obj_pool_done(pool);
    free(pool);
}

FCITX_EXPORT_API
void
(fcitx_obj_pool_init)(FcitxObjPool* pool, size_t size)
{
    fcitx_obj_pool_init(pool, size);
}

FCITX_EXPORT_API
void
fcitx_obj_pool_init_with_prealloc(FcitxObjPool* pool, size_t size,
                                  size_t prealloc)
{
    size = fcitx_utils_align_to(size + sizeof(int), sizeof(int));
    pool->ele_size = size;
    pool->next_free = 0;
    pool->alloc = size * prealloc;
    pool->array = malloc(pool->alloc);

    size_t offset = 0;
    unsigned int i = 0;
    for (offset = 0, i = 0; i < prealloc - 1; offset += size, i++) {
        *(int*)(pool->array + offset) = i + 1;
    }
    *(int*)(pool->array + offset) = FCITX_OBJECT_POOL_INVALID_ID;
}

FCITX_EXPORT_API
int fcitx_obj_pool_alloc_id(FcitxObjPool* pool)
{
    size_t offset;
    int id = 0;
    if (pool->next_free >= 0) {
        id = pool->next_free;
        offset = fcitx_obj_pool_offset(pool, pool->next_free);
        pool->next_free = *(int*)(pool->array + offset);
        *(int*)(pool->array + offset) = FCITX_OBJECT_POOL_ALLOCED_ID;
    }
    else {
        pool->alloc *= 2;
        pool->array = realloc(pool->array, pool->alloc);

        id = pool->alloc / 2 / pool->ele_size;
        pool->next_free = id + 1;
        *(int*)(pool->array + pool->alloc / 2) = FCITX_OBJECT_POOL_ALLOCED_ID;

        size_t offset;
        unsigned int i;
        for (offset = pool->alloc / 2 + pool->ele_size,
                 i = pool->alloc / 2 / pool->ele_size + 1;
             i < pool->alloc / pool->ele_size - 1;
             offset += pool->ele_size, i++) {
            *(int*)(pool->array + offset) = i + 1;
        }
        *(int*)(pool->array + offset) = FCITX_OBJECT_POOL_INVALID_ID;
    }
    return id;
}

FCITX_EXPORT_API
boolean
fcitx_obj_pool_free_id(FcitxObjPool* pool, int i)
{
    unsigned int offset = fcitx_obj_pool_offset(pool, i);
    if (offset >= pool->alloc)
        return false;
    if (*(int*)(pool->array + offset) != FCITX_OBJECT_POOL_ALLOCED_ID)
        return false;
    *(int*)(pool->array + offset) = pool->next_free;
    pool->next_free = i;
    return true;
}
