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

#include "config.h"
#include "fcitx/fcitx.h"
#include "fcitx-utils/uthash.h"
#include "handler-table.h"
#include "objpool.h"

typedef struct {
    int first;
    int last;
    UT_hash_handle hh;
} FcitxHandlerKey;

typedef struct {
    int prev;
    int next;
    FcitxHandlerKey *key;
} FcitxHandlerObj;

struct _FcitxHandlerTable {
    size_t obj_size;
    FcitxFreeContentFunc free_func;
    FcitxHandlerKey *keys;
    FcitxObjPool* objs;
};

FCITX_EXPORT_API FcitxHandlerTable*
fcitx_handler_table_new(size_t obj_size, FcitxFreeContentFunc free_func)
{
    FcitxHandlerTable *table = fcitx_utils_new(FcitxHandlerTable);
    table->obj_size = obj_size;
    table->free_func = free_func;
    table->objs = fcitx_obj_pool_new(obj_size + sizeof(FcitxHandlerObj));
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
    key_struct->first = key_struct->last = FCITX_OBJECT_POOL_INVALID_ID;
    memcpy(key_struct + 1, key, keysize);
    key = key_struct + 1;
    HASH_ADD_KEYPTR(hh, table->keys, key, keysize, key_struct);
    return key_struct;
}

static inline FcitxHandlerObj*
fcitx_handler_table_get_obj(FcitxHandlerTable *table, int id)
{
    return fcitx_obj_pool_get(table->objs, id);
}

FCITX_EXPORT_API int
fcitx_handler_table_append(FcitxHandlerTable *table, size_t keysize,
                           const void *key, const void *obj)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, true);
    int new_id;
    new_id = fcitx_obj_pool_alloc_id(table->objs);
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_handler_table_get_obj(table, new_id);
    obj_struct->key = key_struct;
    obj_struct->next = FCITX_OBJECT_POOL_INVALID_ID;
    memcpy(obj_struct + 1, obj, table->obj_size);
    int id = key_struct->last;
    if (id == FCITX_OBJECT_POOL_INVALID_ID) {
        key_struct->last = key_struct->first = new_id;
        obj_struct->prev = FCITX_OBJECT_POOL_INVALID_ID;
    } else {
        key_struct->last = new_id;
        obj_struct->prev = id;
        fcitx_handler_table_get_obj(table, id)->next = new_id;
    }
    return new_id;
}

FCITX_EXPORT_API int
fcitx_handler_table_prepend(FcitxHandlerTable *table, size_t keysize,
                            const void *key, const void *obj)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, true);
    int new_id;
    new_id = fcitx_obj_pool_alloc_id(table->objs);
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_handler_table_get_obj(table, new_id);
    obj_struct->key = key_struct;
    obj_struct->prev = FCITX_OBJECT_POOL_INVALID_ID;
    memcpy(obj_struct + 1, obj, table->obj_size);
    int id = key_struct->first;
    if (id == FCITX_OBJECT_POOL_INVALID_ID) {
        key_struct->last = key_struct->first = new_id;
        obj_struct->next = FCITX_OBJECT_POOL_INVALID_ID;
    } else {
        key_struct->first = new_id;
        obj_struct->next = id;
        fcitx_handler_table_get_obj(table, id)->prev = new_id;
    }
    return new_id;
}

FCITX_EXPORT_API void*
fcitx_handler_table_get_by_id(FcitxHandlerTable *table, int id)
{
    if (id == FCITX_OBJECT_POOL_INVALID_ID)
        return NULL;
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_handler_table_get_obj(table, id);
    return obj_struct + 1;
}

FCITX_EXPORT_API int
fcitx_handler_table_first_id(FcitxHandlerTable *table, size_t keysize,
                             const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, false);
    if (!key_struct)
        return FCITX_OBJECT_POOL_INVALID_ID;
    return key_struct->first;
}

FCITX_EXPORT_API int
fcitx_handler_table_last_id(FcitxHandlerTable *table, size_t keysize,
                            const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, false);
    if (!key_struct)
        return FCITX_OBJECT_POOL_INVALID_ID;
    return key_struct->last;
}

FCITX_EXPORT_API int
fcitx_handler_table_next_id(FcitxHandlerTable *table, const void *obj)
{
    FCITX_UNUSED(table);
    const FcitxHandlerObj *obj_struct = obj - sizeof(FcitxHandlerObj);
    return obj_struct->next;
}

FCITX_EXPORT_API int
fcitx_handler_table_prev_id(FcitxHandlerTable *table, const void *obj)
{
    FCITX_UNUSED(table);
    const FcitxHandlerObj *obj_struct = obj - sizeof(FcitxHandlerObj);
    return obj_struct->prev;
}

FCITX_EXPORT_API void*
fcitx_handler_table_first(FcitxHandlerTable *table, size_t keysize,
                          const void *key)
{
    int id = fcitx_handler_table_first_id(table, keysize, key);
    return fcitx_handler_table_get_by_id(table, id);
}

FCITX_EXPORT_API void*
fcitx_handler_table_last(FcitxHandlerTable *table, size_t keysize,
                         const void *key)
{
    int id = fcitx_handler_table_last_id(table, keysize, key);
    return fcitx_handler_table_get_by_id(table, id);
}

FCITX_EXPORT_API void*
fcitx_handler_table_next(FcitxHandlerTable *table, const void *obj)
{
    int id = fcitx_handler_table_next_id(table, obj);
    return fcitx_handler_table_get_by_id(table, id);
}

FCITX_EXPORT_API void*
fcitx_handler_table_prev(FcitxHandlerTable *table, const void *obj)
{
    int id = fcitx_handler_table_prev_id(table, obj);
    return fcitx_handler_table_get_by_id(table, id);
}

FCITX_EXPORT_API void
fcitx_handler_table_remove_by_id(FcitxHandlerTable *table, int id)
{
    if (id == FCITX_OBJECT_POOL_INVALID_ID)
        return;
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_handler_table_get_obj(table, id);
    int prev = obj_struct->prev;
    int next = obj_struct->next;
    if (prev == FCITX_OBJECT_POOL_INVALID_ID) {
        obj_struct->key->first = next;
    } else {
        fcitx_handler_table_get_obj(table, prev)->next = next;
    }
    if (next == FCITX_OBJECT_POOL_INVALID_ID) {
        obj_struct->key->last = prev;
    } else {
        fcitx_handler_table_get_obj(table, next)->prev = prev;
    }
    if (table->free_func)
        table->free_func(obj_struct + 1);
    fcitx_obj_pool_free_id(table->objs, id);
}

static void
fcitx_handler_table_free_key(FcitxHandlerTable *table,
                             FcitxHandlerKey *key_struct)
{
    int id;
    int next_id;
    FcitxHandlerObj *obj_struct;
    for (id = key_struct->first;id != FCITX_OBJECT_POOL_INVALID_ID;id = next_id) {
        obj_struct = fcitx_handler_table_get_obj(table, id);
        next_id = obj_struct->next;
        if (table->free_func)
            table->free_func(obj_struct + 1);
        fcitx_obj_pool_free_id(table->objs, id);
    }
    HASH_DELETE(hh, table->keys, key_struct);
    free(key_struct);
}

FCITX_EXPORT_API void
fcitx_handler_table_remove_key(FcitxHandlerTable *table, size_t keysize,
                               const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_key_struct(table, keysize, key, false);
    if (!key_struct)
        return;
    fcitx_handler_table_free_key(table, key_struct);
}

FCITX_EXPORT_API void
fcitx_handler_table_free(FcitxHandlerTable *table)
{
    FcitxHandlerKey* key_struct;
    FcitxHandlerKey* next_key;
    for(key_struct = table->keys;key_struct;key_struct = next_key) {
        next_key = key_struct->hh.next;
        fcitx_handler_table_free_key(table, key_struct);
    }
    fcitx_obj_pool_free(table->objs);
    free(table);
}
