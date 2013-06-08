/***************************************************************************
 *   Copyright (C) 2012~2013 by Yichao Yu                                  *
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

struct _FcitxHandlerKey {
    int first;
    int last;
    UT_hash_handle hh;
};

typedef struct {
    int prev;
    int next;
    FcitxHandlerKey *key;
} FcitxHandlerObj;

struct _FcitxHandlerTable {
    size_t obj_size;
    FcitxDestroyNotify free_func;
    FcitxHandlerKey *keys;
    FcitxObjPool *objs;
    FcitxHandlerKeyDataVTable key_vtable;
};

FCITX_EXPORT_API FcitxHandlerTable*
fcitx_handler_table_new_with_keydata(
    size_t obj_size, FcitxDestroyNotify free_func,
    const FcitxHandlerKeyDataVTable *key_vtable)
{
    FcitxHandlerTable *table = fcitx_utils_new(FcitxHandlerTable);
    table->obj_size = obj_size;
    table->free_func = free_func;
    table->objs = fcitx_obj_pool_new(obj_size + sizeof(FcitxHandlerObj));
    if (key_vtable) {
        memcpy(&table->key_vtable, key_vtable,
               sizeof(FcitxHandlerKeyDataVTable));
    }
    return table;
}

FCITX_EXPORT_API FcitxHandlerTable*
(fcitx_handler_table_new)(size_t obj_size, FcitxDestroyNotify free_func)
{
    return fcitx_handler_table_new_with_keydata(obj_size, free_func, NULL);
}

FCITX_EXPORT_API FcitxHandlerKey*
(fcitx_handler_table_find_key)(FcitxHandlerTable *table, size_t keysize,
                               const void *key, boolean create)
{
    FcitxHandlerKey *key_struct = NULL;
    HASH_FIND(hh, table->keys, key, keysize, key_struct);
    if (key_struct || !create)
        return key_struct;
    key_struct = malloc(sizeof(FcitxHandlerKey) + keysize +
                        table->key_vtable.size + 1);
    key_struct->first = key_struct->last = FCITX_OBJECT_POOL_INVALID_ID;
    void *key_ptr = ((void*)(key_struct + 1)) + table->key_vtable.size;
    memcpy(key_ptr, key, keysize);
    ((char*)key_ptr)[keysize] = '\0';
    HASH_ADD_KEYPTR(hh, table->keys, key_ptr, keysize, key_struct);
    if (table->key_vtable.init) {
        table->key_vtable.init(key_struct + 1, key_ptr, keysize, table->key_vtable.owner);
    }
    return key_struct;
}

static inline FcitxHandlerObj*
fcitx_handler_table_get_obj(FcitxHandlerTable *table, int id)
{
    return fcitx_obj_pool_get(table->objs, id);
}

FCITX_EXPORT_API int
fcitx_handler_key_append(FcitxHandlerTable *table, FcitxHandlerKey *key_struct,
                         const void *obj)
{
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
fcitx_handler_table_append(FcitxHandlerTable *table, size_t keysize,
                           const void *key, const void *obj)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_find_key(table, keysize, key, true);
    return fcitx_handler_key_append(table, key_struct, obj);
}

FCITX_EXPORT_API int
fcitx_handler_key_prepend(FcitxHandlerTable *table, FcitxHandlerKey *key_struct,
                          const void *obj)
{
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

FCITX_EXPORT_API
boolean fcitx_handler_key_is_empty(FcitxHandlerTable* table, FcitxHandlerKey* key)
{
    FCITX_UNUSED(table);
    return key->last == FCITX_OBJECT_POOL_INVALID_ID
        && key->first == FCITX_OBJECT_POOL_INVALID_ID;
}

FCITX_EXPORT_API int
fcitx_handler_table_prepend(FcitxHandlerTable *table, size_t keysize,
                            const void *key, const void *obj)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_find_key(table, keysize, key, true);
    return fcitx_handler_key_prepend(table, key_struct, obj);
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

FCITX_EXPORT_API
FcitxHandlerKey* fcitx_handler_table_get_key_by_id(FcitxHandlerTable* table, int id)
{
    if (id == FCITX_OBJECT_POOL_INVALID_ID)
        return NULL;
    FcitxHandlerObj *obj_struct;
    obj_struct = fcitx_handler_table_get_obj(table, id);
    return obj_struct->key;
}

FCITX_EXPORT_API void*
fcitx_handler_key_get_data(FcitxHandlerTable *table, FcitxHandlerKey *key)
{
    FCITX_UNUSED(table);
    if (fcitx_unlikely(!key))
        return NULL;
    return key + 1;
}

FCITX_EXPORT_API const void*
fcitx_handler_key_get_key(FcitxHandlerTable *table, FcitxHandlerKey *key,
                          size_t *len)
{
    if (fcitx_unlikely(!key)) {
        if (len)
            *len = 0;
        return NULL;
    }
    if (len)
        *len = key->hh.keylen;
    return key->hh.key;
}

FCITX_EXPORT_API int
fcitx_handler_key_first_id(FcitxHandlerTable *table, FcitxHandlerKey *key)
{
    if (fcitx_unlikely(!key))
        return FCITX_OBJECT_POOL_INVALID_ID;
    return key->first;
}

FCITX_EXPORT_API int
fcitx_handler_key_last_id(FcitxHandlerTable *table, FcitxHandlerKey *key)
{
    if (fcitx_unlikely(!key))
        return FCITX_OBJECT_POOL_INVALID_ID;
    return key->last;
}

FCITX_EXPORT_API void*
fcitx_handler_key_first(FcitxHandlerTable *table, FcitxHandlerKey *key)
{
    int id = fcitx_handler_key_first_id(table, key);
    return fcitx_handler_table_get_by_id(table, id);
}

FCITX_EXPORT_API void*
fcitx_handler_key_last(FcitxHandlerTable *table, FcitxHandlerKey *key)
{
    int id = fcitx_handler_key_last_id(table, key);
    return fcitx_handler_table_get_by_id(table, id);
}

FCITX_EXPORT_API int
fcitx_handler_table_first_id(FcitxHandlerTable *table, size_t keysize,
                             const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_find_key(table, keysize, key, false);
    if (!key_struct)
        return FCITX_OBJECT_POOL_INVALID_ID;
    return key_struct->first;
}

FCITX_EXPORT_API int
fcitx_handler_table_last_id(FcitxHandlerTable *table, size_t keysize,
                            const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_find_key(table, keysize, key, false);
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
    if (table->key_vtable.free) {
        size_t len;
        const void* key_ptr = fcitx_handler_key_get_key(table, key_struct, &len);
        table->key_vtable.free(key_struct + 1, key_ptr, len, table->key_vtable.owner);
    }
    free(key_struct);
}

FCITX_EXPORT_API void
fcitx_handler_table_remove_by_id_full(FcitxHandlerTable *table, int id)
{
    FcitxHandlerKey* key = fcitx_handler_table_get_key_by_id(table, id);
    fcitx_handler_table_remove_by_id(table, id);
    if (fcitx_handler_key_is_empty(table, key)) {
        fcitx_handler_table_free_key(table, key);
    }
}

FCITX_EXPORT_API void
fcitx_handler_table_remove_key(FcitxHandlerTable *table, size_t keysize,
                               const void *key)
{
    FcitxHandlerKey *key_struct;
    key_struct = fcitx_handler_table_find_key(table, keysize, key, false);
    if (!key_struct)
        return;
    fcitx_handler_table_free_key(table, key_struct);
}

FCITX_EXPORT_API void
fcitx_handler_table_free(FcitxHandlerTable *table)
{
    FcitxHandlerKey* key_struct;
    FcitxHandlerKey* next_key;
    for(key_struct = table->keys; key_struct; key_struct = next_key) {
        next_key = key_struct->hh.next;
        fcitx_handler_table_free_key(table, key_struct);
    }
    fcitx_obj_pool_free(table->objs);
    free(table);
}
