/***************************************************************************
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
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

#include <unistd.h>
#include <errno.h>

#include <fcitx-utils/log.h>
#include "wayland-global.h"
#include "epoll-utils.h"

static inline FcitxHandlerKey*
FxWaylandGetGeneralHandlers(FcitxWayland *wl)
{
    if (fcitx_unlikely(!wl->general_handlers)) {
        wl->general_handlers =
            fcitx_handler_table_find_strkey(wl->global_handlers, "", true);
    }
    return wl->general_handlers;
}

static FcitxWaylandObject*
FxWaylandAddGlobal(FcitxWayland *wl, uint32_t name, const char *iface,
                   uint32_t ver)
{
    FcitxHandlerKey *key;
    FcitxWaylandObject *wl_obj;
    FcitxWaylandIfaceData *iface_data;
    key = fcitx_handler_table_find_strkey(wl->global_handlers, iface, true);
    wl_obj = fcitx_utils_new(FcitxWaylandObject);
    wl_obj->key = key;
    wl_obj->name = name;
    wl_obj->version = ver;
    iface_data = fcitx_handler_key_get_data(wl->global_handlers, key);
    HASH_ADD_KEYPTR(hh, wl->wl_objs, &wl_obj->name, sizeof(uint32_t), wl_obj);
    wl_list_insert(&iface_data->obj_list, &wl_obj->link);
    return wl_obj;
}

static void
FxWaylandRemoveGlobal(FcitxWayland *wl, FcitxWaylandObject *wl_obj)
{
    HASH_DEL(wl->wl_objs, wl_obj);
    wl_list_remove(&wl_obj->link);
}

static void
FxWaylandRunGlobalAdded(FcitxWayland *wl, FcitxHandlerKey *key,
                        FcitxWaylandObject *wl_obj)
{
    FcitxHandlerTable *table = wl->global_handlers;
    int id = fcitx_handler_key_first_id(table, key);
    if (id == FCITX_OBJECT_POOL_INVALID_ID)
        return;
    const FcitxWaylandGlobalHandler *handler;
    const char *iface = fcitx_handler_key_get_key(table, key, NULL);
    int next_id;
    for (;(handler = fcitx_handler_table_get_by_id(table, id));id = next_id) {
        next_id = fcitx_handler_table_next_id(table, handler);
        if (handler->added) {
            handler->added(handler->data, wl_obj->name, iface, wl_obj->version);
        }
    }
}

static void
FxWaylandRunGlobalRemoved(FcitxWayland *wl, FcitxHandlerKey *key,
                          FcitxWaylandObject *wl_obj)
{
    FcitxHandlerTable *table = wl->global_handlers;
    int id = fcitx_handler_key_first_id(table, key);
    if (id == FCITX_OBJECT_POOL_INVALID_ID)
        return;
    const FcitxWaylandGlobalHandler *handler;
    const char *iface = fcitx_handler_key_get_key(table, key, NULL);
    int next_id;
    for (;(handler = fcitx_handler_table_get_by_id(table, id));id = next_id) {
        next_id = fcitx_handler_table_next_id(table, handler);
        if (handler->removed) {
            handler->removed(handler->data, wl_obj->name,
                             iface, wl_obj->version);
        }
    }
}

static void
FxWaylandGlobalAdded(void *self, struct wl_registry *wl_registry,
                     uint32_t name, const char *iface, uint32_t ver)
{
    FcitxWayland *wl = (FcitxWayland*)self;
    FcitxWaylandObject *wl_obj;
    printf("%s, %s, %x\n", __func__, iface, name);
    wl_obj = FxWaylandAddGlobal(wl, name, iface, ver);
    FxWaylandRunGlobalAdded(wl, wl_obj->key, wl_obj);
    FxWaylandRunGlobalAdded(wl, FxWaylandGetGeneralHandlers(wl), wl_obj);
}

static void
FxWaylandGlobalRemoved(void *self, struct wl_registry *wl_registry,
                       uint32_t name)
{
    FcitxWayland *wl = (FcitxWayland*)self;
    FcitxWaylandObject *wl_obj;
    HASH_FIND(hh, wl->wl_objs, &name, sizeof(uint32_t), wl_obj);
    FxWaylandRemoveGlobal(wl, wl_obj);
    FxWaylandRunGlobalRemoved(wl, wl_obj->key, wl_obj);
    FxWaylandRunGlobalRemoved(wl, FxWaylandGetGeneralHandlers(wl), wl_obj);
}

static const struct wl_registry_listener fx_reg_listener = {
    .global = FxWaylandGlobalAdded,
    .global_remove = FxWaylandGlobalRemoved
};

static void
FxWaylandGlobalHandlerFree(void *p)
{
    FcitxWaylandGlobalHandler *handler = p;
    FCITX_UNUSED(handler);
}

static void
FxWaylandIfaceDataInit(void *_data, void *self)
{
    FcitxWayland *wl = (FcitxWayland*)self;
    FcitxWaylandIfaceData *data = _data;
    data->wl = wl;
    wl_list_init(&data->obj_list);
}

static void
FxWaylandIfaceDataFree(void *_data)
{
    FcitxWaylandIfaceData *data = _data;
    FcitxWayland *wl = data->wl;
    FcitxWaylandObject *pos;
    FcitxWaylandObject *tmp;
    wl_list_for_each_safe (pos, tmp, &data->obj_list, link) {
        FxWaylandRemoveGlobal(wl, pos);
    }
}

int
FxWaylandRegGlobalHandler(FcitxWayland *wl, const char *iface,
                          FcitxWaylandHandleGlobalAdded added,
                          FcitxWaylandHandleGlobalRemoved removed,
                          void *data, boolean run_exists)
{
    const FcitxWaylandGlobalHandler handler = {
        .added = added,
        .removed = removed,
        .data = data
    };
    FcitxHandlerKey *key;
    if (!iface || !*iface) {
        iface = "";
        key = FxWaylandGetGeneralHandlers(wl);
    } else {
        key = fcitx_handler_table_find_strkey(wl->global_handlers,
                                              iface, true);
    }
    int res;
    res = fcitx_handler_key_prepend(wl->global_handlers, key, &handler);
    if (!(run_exists && added))
        return res;
    if (*iface) {
        FcitxWaylandIfaceData *iface_data;
        iface_data = fcitx_handler_key_get_data(wl->global_handlers, key);
        FcitxWaylandObject *pos;
        FcitxWaylandObject *tmp;
        wl_list_for_each_safe (pos, tmp, &iface_data->obj_list, link) {
            added(data, pos->name, iface, pos->version);
        }
    } else {
        FcitxWaylandObject *wl_obj;
        for (wl_obj = wl->wl_objs;wl_obj;
             wl_obj = (FcitxWaylandObject*)wl_obj->hh.next) {
            added(data, wl_obj->name, iface, wl_obj->version);
        }
    }
    return res;
}

void
FxWaylandRemoveGlobalHandler(FcitxWayland *wl, int id)
{
    if (id < 0)
        return;
    fcitx_handler_table_remove_by_id(wl->global_handlers, id);
}

boolean
FxWaylandGlobalInit(FcitxWayland *wl)
{
    if (fcitx_unlikely(wl_registry_add_listener(wl->registry,
                                                &fx_reg_listener, wl) == -1))
        return false;
    const FcitxHandlerKeyDataVTable wl_key_vtable = {
        .size = sizeof(FcitxWaylandIfaceData),
        .init = FxWaylandIfaceDataInit,
        .free = FxWaylandIfaceDataFree,
        .owner = wl
    };
    wl->global_handlers = fcitx_handler_table_new_with_keydata(
        sizeof(FcitxWaylandGlobalHandler), FxWaylandGlobalHandlerFree,
        &wl_key_vtable);
    return wl->global_handlers != NULL;
}
