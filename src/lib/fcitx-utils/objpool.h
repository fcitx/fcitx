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

/**
 * @addtogroup FcitxUtils
 * @{
 */

/**
 * @file objpool.h
 *
 * a simple naive object pool for fcitx, scenerio is many small allocation,
 * but don't need much free.
 *
 * A common usage for object pool
 * @code
 * FcitxObjPool* pool = fcitx_obj_pool_new();
 * int id = fcitx_obj_pool_alloc_id(pool);
 * void* data = fcitx_obj_pool_get(pool, id);
 * ...
 * fcitx_obj_pool_free(pool);
 * @endcode
 *
 */

#ifndef __FCITX_UTILS_OBJPOOL_H
#define __FCITX_UTILS_OBJPOOL_H

#include <stdint.h>
#include <stdlib.h>
#include <fcitx-utils/utils.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 0 is a valid id, instead, -1 is not */
#define FCITX_OBJECT_POOL_INVALID_ID (-1)
#define FCITX_OBJECT_POOL_ALLOCED_ID (-2)
#define FCITX_OBJ_POOL_INIT_SIZE (4)

    typedef struct _FcitxObjPool FcitxObjPool;

    /**
     * Object pool
     * @since 4.2.7
     **/
    struct _FcitxObjPool {
        void *array;
        size_t alloc;
        size_t ele_size;
        int next_free;
    };

    /**
     * init a pool
     *
     * @param pool pool to be initialized
     * @param size size of object
     * @return void
     *
     * @since 4.2.7
     */
    void fcitx_obj_pool_init(FcitxObjPool *pool, size_t size);
    void fcitx_obj_pool_init_with_prealloc(FcitxObjPool *pool, size_t size,
                                           size_t prealloc);
#define fcitx_obj_pool_init(p, s)                                       \
    fcitx_obj_pool_init_with_prealloc(p, s, FCITX_OBJ_POOL_INIT_SIZE)

    /**
     * create an object pool, with same object size
     *
     * @param size size of object
     * @return newly created pool
     * @since 4.2.7
     **/
    FcitxObjPool *fcitx_obj_pool_new(size_t size);
    FcitxObjPool *fcitx_obj_pool_new_with_prealloc(size_t size, size_t prealloc);
#define fcitx_obj_pool_new(s)                                           \
    fcitx_obj_pool_new_with_prealloc(s, FCITX_OBJ_POOL_INIT_SIZE)

   /**
     * allocate an id from pool
     *
     * @param pool pool
     * @return void
     * @since 4.2.7
     **/
    int fcitx_obj_pool_alloc_id(FcitxObjPool *pool);

   /**
     * free an id from pool, never use a free'd id, since it will be reused.
     *
     * @param pool pool
     * @return free succeeded or failed
     * @since 4.2.7
     **/
    boolean fcitx_obj_pool_free_id(FcitxObjPool *pool, int i);


    static inline size_t
    fcitx_obj_pool_offset(FcitxObjPool *pool, int i)
    {
        return i * pool->ele_size;
    }

    /**
     * this function need to be used everything before you need to access the data,
     * unless you didn't alloc id from the pool
     *
     * @param pool pool
     * @return void
     * @since 4.2.7
     **/
    static inline void*
    fcitx_obj_pool_get(FcitxObjPool *pool, int i)
    {
        return pool->array + fcitx_obj_pool_offset(pool, i) + sizeof(int);
    }

    /**
     * free an inited-pool
     *
     * @param pool pool
     * @return void
     * @since 4.2.7
     **/
    void fcitx_obj_pool_done(FcitxObjPool *pool);

    /**
     * free an object pool
     *
     * @param pool pool
     * @return void
     * @since 4.2.7
     **/
    void fcitx_obj_pool_free(FcitxObjPool *pool);

#ifdef __cplusplus
}
#endif

#endif // OBJPOOL_H

/**
 * @}
 */
