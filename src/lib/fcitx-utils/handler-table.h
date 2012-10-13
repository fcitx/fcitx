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

#ifndef _UTILS_HANDLER_TABLE_H
#define _UTILS_HANDLER_TABLE_H

#include <fcitx-utils/utils.h>

typedef struct _FcitxHandlerTable FcitxHandlerTable;

/* 0 is a valid id, instead, -1 is not */
#define INVALID_ID ((unsigned int)-1)

typedef struct {
    void *array;
    unsigned int alloc;
    unsigned int size;
    unsigned int ele_size;
    unsigned int last_empty;
} FcitxObjPool;

#ifdef __cplusplus
extern "C" {
#endif

    void fcitx_obj_pool_init(FcitxObjPool *pool, unsigned int size);
    unsigned int fcitx_obj_pool_alloc_id(FcitxObjPool *pool);
    void fcitx_obj_pool_free_id(FcitxObjPool *pool, unsigned int i);

    static inline unsigned int
    fcitx_obj_pool_offset(FcitxObjPool *pool, unsigned int i)
    {
        return i * pool->ele_size;
    }

    static inline void*
    fcitx_obj_pool_get(FcitxObjPool *pool, unsigned int i)
    {
        return pool->array + fcitx_obj_pool_offset(pool, i) + sizeof(int);
    }

    static inline void
    fcitx_obj_pool_done(FcitxObjPool *pool)
    {
        free(pool->array);
    }

    static inline FcitxObjPool*
    fcitx_obj_pool_new(unsigned int size)
    {
        FcitxObjPool *pool = fcitx_utils_new(FcitxObjPool);
        fcitx_obj_pool_init(pool, size);
        return pool;
    }

    static inline void
    fcitx_obj_pool_free(FcitxObjPool *pool)
    {
        fcitx_obj_pool_done(pool);
        free(pool);
    }

    FcitxHandlerTable *fcitx_handler_table_new(size_t obj_size,
                                               FcitxFreeContentFunc free_func);
    unsigned int fcitx_handler_table_append(FcitxHandlerTable *table,
                                            size_t keysize, const void *key,
                                            const void *obj);
    unsigned int fcitx_handler_table_prepend(FcitxHandlerTable *table,
                                             size_t keysize, const void *key,
                                             const void *obj);
    void *fcitx_handler_table_get_by_id(FcitxHandlerTable *table,
                                        unsigned int id);
    void *fcitx_handler_table_first(FcitxHandlerTable *table, size_t keysize,
                                    const void *key);
    void *fcitx_handler_table_last(FcitxHandlerTable *table, size_t keysize,
                                   const void *key);
    void *fcitx_handler_table_next(FcitxHandlerTable *table, const void *obj);
    void *fcitx_handler_table_prev(FcitxHandlerTable *table, const void *obj);
    /* for remove when iterating */
    unsigned int fcitx_handler_table_first_id(FcitxHandlerTable *table,
                                              size_t keysize, const void *key);
    unsigned int fcitx_handler_table_last_id(FcitxHandlerTable *table,
                                             size_t keysize, const void *key);
    unsigned int fcitx_handler_table_next_id(FcitxHandlerTable *table,
                                             const void *obj);
    unsigned int fcitx_handler_table_prev_id(FcitxHandlerTable *table,
                                             const void *obj);
    void fcitx_handler_table_remove_key(FcitxHandlerTable *table,
                                        size_t keysize, const void *key);
    void fcitx_handler_table_remove_by_id(FcitxHandlerTable *table,
                                          unsigned int id);
    void fcitx_handler_table_free(FcitxHandlerTable *table);

    static inline unsigned int
    fcitx_handler_table_append_strkey(FcitxHandlerTable *table,
                                      const char *keystr, const void *obj)
    {
        return fcitx_handler_table_append(table, strlen(keystr),
                                          (const void*)keystr, obj);
    }
    static inline unsigned int
    fcitx_handler_table_prepend_strkey(FcitxHandlerTable *table,
                                       const char *keystr, const void *obj)
    {
        return fcitx_handler_table_prepend(table, strlen(keystr),
                                           (const void*)keystr, obj);
    }
    static inline void*
    fcitx_handler_table_first_strkey(FcitxHandlerTable *table,
                                     const char *keystr)
    {
        return fcitx_handler_table_first(table, strlen(keystr),
                                         (const void*)keystr);
    }
    static inline void*
    fcitx_handler_table_last_strkey(FcitxHandlerTable *table,
                                    const char *keystr)
    {
        return fcitx_handler_table_last(table, strlen(keystr),
                                        (const void*)keystr);
    }
    static inline unsigned int
    fcitx_handler_table_first_id_strkey(FcitxHandlerTable *table,
                                        const char *keystr)
    {
        return fcitx_handler_table_first_id(table, strlen(keystr),
                                            (const void*)keystr);
    }
    static inline unsigned int
    fcitx_handler_table_last_id_strkey(FcitxHandlerTable *table,
                                       const char *keystr)
    {
        return fcitx_handler_table_last_id(table, strlen(keystr),
                                           (const void*)keystr);
    }
    static inline void
    fcitx_handler_table_remove_key_strkey(FcitxHandlerTable *table,
                                          const char *keystr)
    {
        fcitx_handler_table_remove_key(table, strlen(keystr),
                                       (const void*)keystr);
    }

#ifdef __cplusplus
}
#endif

#endif
