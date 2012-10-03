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

#include "fcitx-utils/utils.h"
#include "fcitx-utils/uthash.h"
#include "x11handlertable.h"

#define OBJ_POOL_INIT_SIZE (4)

typedef struct {
    void *array;
    unsigned int alloc;
    unsigned int size;
    unsigned int ele_size;
    unsigned int last_empty;
} FcitxObjPool;

typedef struct {
    unsigned int first;
    unsigned int last;
    UT_hash_handle hh;
} FcitxHandlerKey;

typedef struct {
    unsigned int prev;
    unsigned int next;
    FcitxHandlerKey *key;
} FcitxHandlerObj;

struct _FcitxHandlerTable {
    size_t obj_size;
    FcitxFreeContentFunc free_func;
    FcitxHandlerKey *keys;
    FcitxObjPool objs;
};

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

static void
fcitx_obj_pool_init(FcitxObjPool *pool, unsigned int size)
{
    unsigned int rem = size % sizeof(int);
    if (rem) {
        size += 2 * sizeof(int) - rem;
    } else {
        size += sizeof(int);
    }
    pool->ele_size = size;
    pool->size = 0;
    pool->last_empty = 0;
    pool->alloc = size * OBJ_POOL_INIT_SIZE;
    pool->array = malloc(pool->alloc);
}

static unsigned int
fcitx_obj_pool_alloc(FcitxObjPool *pool)
{
    unsigned int offset;
    for (offset = fcitx_obj_pool_offset(pool, pool->last_empty);
         offset < pool->size;
         pool->last_empty++, offset += pool->ele_size) {
        if (!*(int*)(pool->array + offset)) {
            goto found;
        }
    }
    pool->size += pool->ele_size;
    if (pool->size >= pool->alloc) {
        pool->alloc *= 2;
        pool->array = realloc(pool->array, pool->alloc);
    }
found:
    *(int*)(pool->array + offset) = 1;
    return pool->last_empty;
}

static void
fcitx_obj_pool_free(FcitxObjPool *pool, unsigned int i)
{
    unsigned int offset = fcitx_obj_pool_offset(pool, i);
    if (offset >= pool->size)
        return;
    *(int*)(pool->array + offset) = 0;
}

static void
fcitx_obj_pool_done(FcitxObjPool *pool)
{
    free(pool->array);
}

FcitxHandlerTable*
fcitx_handler_table_new(size_t obj_size, FcitxFreeContentFunc free_func)
{
    FcitxHandlerTable *table = fcitx_utils_new(FcitxHandlerTable);
    table->obj_size = obj_size;
    table->free_func = free_func;
    fcitx_obj_pool_init(&table->objs, obj_size + sizeof(FcitxHandlerObj));
    return table;
}

static FcitxHandlerKey*
fcitx_handler_table_key_struct(FcitxHandlerTable *table, size_t keysize,
                               const void *key, boolean create)
{
    FcitxHandlerKey *key_struct = NULL;
    HASH_FIND(hh, table->keys, key, keysize, key_struct);
    if (key_struct || !create)
        return key_struct;
    key_struct = malloc(sizeof(FcitxHandlerKey) + keysize);
    key_struct->first = key_struct->last = INVALID_ID;
    memcpy(key_struct + 1, key, keysize);
    key = key_struct + 1;
    HASH_ADD_KEYPTR(hh, table->keys, key, keysize, key_struct);
    return key_struct;
}

static inline FcitxHandlerObj*
fcitx_handler_table_get_obj(FcitxHandlerTable *table, unsigned int id)
{
    return fcitx_obj_pool_get(&table->objs, id);
}

unsigned int
fcitx_handler_table_append(FcitxHandlerTable *table, size_t keysize,
                           const void *key, const void *obj)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, true);
    unsigned int new_id;
    new_id = fcitx_obj_pool_alloc(&table->objs);
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_obj_pool_get(&table->objs, new_id);
    obj_struct->key = key_struct;
    obj_struct->next = INVALID_ID;
    memcpy(obj_struct + 1, obj, table->obj_size);
    unsigned int id = key_struct->last;
    if (id == INVALID_ID) {
        key_struct->last = key_struct->first = new_id;
        obj_struct->prev = INVALID_ID;
    } else {
        key_struct->last = new_id;
        obj_struct->prev = id;
        fcitx_handler_table_get_obj(table, id)->next = new_id;
    }
    return new_id;
}

unsigned int
fcitx_handler_table_prepend(FcitxHandlerTable *table, size_t keysize,
                            const void *key, const void *obj)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, true);
    unsigned int new_id;
    new_id = fcitx_obj_pool_alloc(&table->objs);
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_obj_pool_get(&table->objs, new_id);
    obj_struct->key = key_struct;
    obj_struct->prev = INVALID_ID;
    memcpy(obj_struct + 1, obj, table->obj_size);
    unsigned int id = key_struct->first;
    if (id == INVALID_ID) {
        key_struct->last = key_struct->first = new_id;
        obj_struct->prev = INVALID_ID;
    } else {
        key_struct->first = new_id;
        obj_struct->next = id;
        fcitx_handler_table_get_obj(table, id)->prev = new_id;
    }
    return new_id;
}

static void*
fcitx_handler_table_get_by_id(FcitxHandlerTable *table, unsigned int id)
{
    if (id == INVALID_ID)
        return NULL;
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_obj_pool_get(&table->objs, id);
    return obj_struct + 1;
}

void*
fcitx_handler_table_first(FcitxHandlerTable *table, size_t keysize,
                          const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, false);
    if (!key_struct)
        return NULL;
    return fcitx_handler_table_get_by_id(table, key_struct->first);
}

void*
fcitx_handler_table_last(FcitxHandlerTable *table, size_t keysize,
                         const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, false);
    if (!key_struct)
        return NULL;
    return fcitx_handler_table_get_by_id(table, key_struct->last);
}

void*
fcitx_handler_table_next(FcitxHandlerTable *table, const void *obj)
{
    const FcitxHandlerObj *obj_struct = obj - sizeof(FcitxHandlerObj);
    return fcitx_handler_table_get_by_id(table, obj_struct->next);
}

void*
fcitx_handler_table_prev(FcitxHandlerTable *table, const void *obj)
{
    const FcitxHandlerObj *obj_struct = obj - sizeof(FcitxHandlerObj);
    return fcitx_handler_table_get_by_id(table, obj_struct->prev);
}

void
fcitx_handler_table_remove_by_id(FcitxHandlerTable *table, unsigned int id)
{
    if (id == INVALID_ID)
        return;
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_obj_pool_get(&table->objs, id);
    unsigned int prev = obj_struct->prev;
    unsigned int next = obj_struct->next;
    if (prev == INVALID_ID) {
        obj_struct->key->first = next;
    } else {
        fcitx_handler_table_get_obj(table, prev)->next = next;
    }
    if (next == INVALID_ID) {
        obj_struct->key->last = prev;
    } else {
        fcitx_handler_table_get_obj(table, next)->prev = prev;
    }
    if (table->free_func)
        table->free_func(obj_struct + 1);
    fcitx_obj_pool_free(&table->objs, id);
}

static void
fcitx_handler_table_free_key(FcitxHandlerTable *table,
                             FcitxHandlerKey *key_struct)
{
    unsigned int id;
    unsigned int next_id;
    FcitxHandlerObj *obj_struct;
    for (id = key_struct->first;id != INVALID_ID;id = next_id) {
        obj_struct = fcitx_obj_pool_get(&table->objs, id);
        next_id = obj_struct->next;
        if (table->free_func)
            table->free_func(obj_struct + 1);
        fcitx_obj_pool_free(&table->objs, id);
    }
    HASH_DELETE(hh, table->keys, key_struct);
    free(key_struct);
}

void
fcitx_handler_table_remove(FcitxHandlerTable *table, size_t keysize,
                           const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, false);
    if (!key_struct)
        return;
    fcitx_handler_table_free_key(table, key_struct);
}

void
fcitx_handler_table_free(FcitxHandlerTable *table)
{
    FcitxHandlerKey* key_struct;
    FcitxHandlerKey* next_key;
    for(key_struct = table->keys;key_struct;key_struct = next_key) {
        next_key = key_struct->hh.next;
        fcitx_handler_table_free_key(table, key_struct);
    }
    fcitx_obj_pool_done(&table->objs);
    free(table);
}
