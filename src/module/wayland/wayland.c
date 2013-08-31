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

#include <fcitx/module.h>
#include <fcitx-utils/log.h>
#include "wayland-internal.h"
#include "wayland-global.h"
#include "wayland-input.h"
#include "epoll-utils.h"

static void* FxWaylandCreate(FcitxInstance *instance);
static void FxWaylandSetFD(void *self);
static void FxWaylandProcessEvent(void *self);
static void FxWaylandDestroy(void *self);
DECLARE_ADDFUNCTIONS(Wayland)

FCITX_DEFINE_PLUGIN(fcitx_wayland, module, FcitxModule) = {
    .Create = FxWaylandCreate,
    .SetFD = FxWaylandSetFD,
    .ProcessEvent = FxWaylandProcessEvent,
    .Destroy = FxWaylandDestroy,
    .ReloadConfig = NULL
};

typedef struct {
    FcitxWayland *wl;
    FcitxWaylandReadAllCallback cb;
    void *data;
    FcitxWaylandTask task;
    char *buff;
    size_t len;
    size_t alloc;
    size_t limit;
} FxWaylandReadData;

static inline void
FxWaylandReadDataEnsureSize(FxWaylandReadData *read_data, size_t size)
{
    size_t whole_len = read_data->len + size;
    if (whole_len < read_data->alloc)
        return;
    read_data->alloc = fcitx_utils_align_to(whole_len + 1, 4096);
    read_data->buff = realloc(read_data->buff, read_data->alloc);
}

#define READ_SIZE (4096)

static void
FxWaylandReadTaskHandler(FcitxWaylandTask *task, uint32_t events)
{
    FxWaylandReadData *read_data;
    read_data = fcitx_container_of(task, FxWaylandReadData, task);
    size_t len;
    boolean first_time = true;

    while (true) {
        FxWaylandReadDataEnsureSize(read_data, READ_SIZE);
        len = read(task->fd, read_data->buff + read_data->len, READ_SIZE);
        read_data->len += len;
        if (first_time && (len <= 0 || events & EPOLLERR || events & EPOLLHUP))
            goto done;
        if (len <= 0) {
            if (!read_data->limit || read_data->len < read_data->limit) {
                return;
            }
            break;
        }
    }
done:
    read_data->buff[read_data->len] = '\0';
    fx_epoll_del_task(read_data->wl->epoll_fd, task);
    close(task->fd);
    read_data->cb(read_data->data, read_data->buff, read_data->len);
    free(read_data);
}

static void*
FxWaylandReadAll(FcitxWayland *wl, int fd, FcitxWaylandReadAllCallback cb,
                 void *data, size_t limit)
{
    if (fcitx_unlikely(!cb || fd < 0))
        return NULL;
    FxWaylandReadData *read_data = fcitx_utils_new(FxWaylandReadData);
    read_data->cb = cb;
    read_data->data = data;
    read_data->limit = limit;
    read_data->task.fd = fd;
    read_data->task.handler = FxWaylandReadTaskHandler;
    if (fcitx_unlikely(
            fx_epoll_add_task(wl->epoll_fd, &read_data->task,
                              EPOLLIN | EPOLLERR | EPOLLHUP) == -1)) {
        free(read_data);
        return NULL;
    }
    return read_data;
}

typedef struct {
    FcitxWayland *wl;
    FcitxWaylandSyncCallback cb;
    void *data;
} FxWaylandSyncData;

static void
SyncCallback(void *data, struct wl_callback *callback, uint32_t serial)
{
    FxWaylandSyncData *sync_data = data;
    wl_callback_destroy(callback);
    sync_data->cb(sync_data->data, serial);
    free(sync_data);
}

static const struct wl_callback_listener sync_listener = {
    .done = SyncCallback
};

static boolean
FxWaylandSync(FcitxWayland *wl, FcitxWaylandSyncCallback cb, void *data)
{
    if (fcitx_unlikely(!cb))
        return false;
    struct wl_callback *callback = wl_display_sync(wl->dpy);
    if (fcitx_unlikely(!callback))
        return false;
    FxWaylandSyncData *sync_data = fcitx_utils_new(FxWaylandSyncData);
    sync_data->wl = wl;
    sync_data->cb = cb;
    sync_data->data = data;
    wl_callback_add_listener(callback, &sync_listener, sync_data);
    return true;
}

static void
FxWaylandExit(FcitxWayland *wl)
{
    printf("%s\n", __func__);
    fx_epoll_del_task(wl->epoll_fd, &wl->dpy_task);
    // TODO
}

static void
FxWaylandScheduleFlush(FcitxWayland *wl)
{
    if (wl->scheduled_flush)
        return;
    wl->scheduled_flush = true;
    fx_epoll_mod_task(wl->epoll_fd, &wl->dpy_task,
                      EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP);
}

static void
FxWaylandDisplayTaskHandler(FcitxWaylandTask *task, uint32_t events)
{
    FcitxWayland *wl = fcitx_container_of(task, FcitxWayland, dpy_task);
    wl->dpy_events = events;
    int ret;
    if (events & EPOLLERR || events & EPOLLHUP) {
        FxWaylandExit(wl);
        return;
    }
    if (events & EPOLLIN) {
        ret = wl_display_dispatch(wl->dpy);
        if (ret == -1) {
            FxWaylandExit(wl);
            return;
        }
    }
    if (events & EPOLLOUT) {
        ret = wl_display_flush(wl->dpy);
        if (ret == 0) {
            wl->scheduled_flush = false;
            fx_epoll_mod_task(wl->epoll_fd, &wl->dpy_task,
                              EPOLLIN | EPOLLERR | EPOLLHUP);
        } else if (ret == -1 && errno != EAGAIN) {
            FxWaylandExit(wl);
            return;
        }
    }
}

void
FcitxWaylandLogFunc(const char *fmt, va_list ap)
{
    /* all of the log's (at least on the client side) are errors. */
    FcitxLogFuncV(FCITX_ERROR, __FILE__, __LINE__, fmt, ap);
}

typedef struct {
    int id;
    const char *iface_name;
    const struct wl_interface *iface;
    void (**listener)();
    FcitxWayland *wl;
    struct wl_proxy **ret;
    void (*destroy)(struct wl_proxy *proxy);
    boolean failed;
    uint32_t ver;
} FxWaylandSingletonListener;

#define FXWL_DEF_SINGLETON(_wl, field, name, _iface, _listener,         \
                           _destroy, _ver) {                            \
        .id = -1,                                                       \
        .iface_name = name,                                             \
        .iface = &_iface,                                               \
        .wl = _wl,                                                      \
        .ret = (struct wl_proxy**)(&wl->field),                         \
        .listener = (void (**)())_listener,                             \
        .destroy = (void (*)(struct wl_proxy*))_destroy,                \
        .failed = true,                                                 \
        .ver = _ver                                                     \
    }

static void
FxWaylandHandleSingletonAdded(void *data, uint32_t name, const char *iface,
                              uint32_t ver)
{
    FxWaylandSingletonListener *singleton = data;
    FcitxWayland *wl = singleton->wl;
    if (*singleton->ret) {
        singleton->failed = true;
        return;
    }
    struct wl_proxy *proxy =
        wl_registry_bind(wl->registry, name, singleton->iface, singleton->ver);
    *singleton->ret = proxy;
    singleton->failed = false;
    if (singleton->listener) {
        wl_proxy_add_listener(proxy, singleton->listener, wl);
    }
}

static void
FxWaylandShmFormatHandler(void *data, struct wl_shm *shm, uint32_t format)
{
    FcitxWayland *wl = data;
    FCITX_UNUSED(shm);
    wl->shm_formats |= (1 << format);
}

static const struct wl_shm_listener fx_shm_listenr = {
    .format = FxWaylandShmFormatHandler
};

static void*
FxWaylandCreate(FcitxInstance *instance)
{
    FcitxWayland *wl = fcitx_utils_new(FcitxWayland);
    wl_log_set_handler_client(FcitxWaylandLogFunc);

    wl->owner = instance;
    wl->dpy = wl_display_connect(NULL);
    if (!wl->dpy)
        goto free;

    wl->epoll_fd = fx_epoll_create_cloexec();
    if (fcitx_unlikely(wl->epoll_fd < 0))
        goto disconnect;

    wl->dpy_task.fd = wl_display_get_fd(wl->dpy);
    wl->dpy_task.handler = FxWaylandDisplayTaskHandler;
    if (fcitx_unlikely(fx_epoll_add_task(wl->epoll_fd, &wl->dpy_task,
                                         EPOLLIN | EPOLLERR | EPOLLHUP) == -1))
        goto close_epoll;

    wl->registry = wl_display_get_registry(wl->dpy);
    if (fcitx_unlikely(!wl->registry))
        goto close_epoll;
    if (fcitx_unlikely(!FxWaylandGlobalInit(wl)))
        goto destroy_registry;
    FxWaylandSingletonListener singleton_listeners[] = {
        FXWL_DEF_SINGLETON(wl, compositor, "wl_compositor",
                           wl_compositor_interface, NULL,
                           wl_compositor_destroy, 2),
        FXWL_DEF_SINGLETON(wl, shell, "wl_shell", wl_shell_interface, NULL,
                           wl_shell_destroy, 1),
        FXWL_DEF_SINGLETON(wl, shm, "wl_shm", wl_shm_interface,
                           &fx_shm_listenr, wl_shm_destroy, 1),
        FXWL_DEF_SINGLETON(wl, data_device_manager, "wl_data_device_manager",
                           wl_data_device_manager_interface, NULL,
                           wl_data_device_manager_destroy, 1),
    };

    const int singleton_count =
        sizeof(singleton_listeners) / sizeof(singleton_listeners[0]);
    int i;
    for (i = 0;i < singleton_count;i++) {
        FxWaylandSingletonListener *listener = singleton_listeners + i;
        listener->id = FxWaylandRegGlobalHandler(wl, listener->iface_name,
                                                 FxWaylandHandleSingletonAdded,
                                                 NULL, listener, true);
    }
    wl_display_roundtrip(wl->dpy);
    boolean failed = false;
    for (i = 0;i < singleton_count;i++) {
        FxWaylandSingletonListener *listener = singleton_listeners + i;
        FxWaylandRemoveGlobalHandler(wl, listener->id);
        if (listener->failed) {
            failed = true;
        }
    }
    if (failed)
        goto free_handlers;
    FxWaylandInputInit(wl);
    FxWaylandScheduleFlush(wl);
    FcitxWaylandAddFunctions(instance);
    return wl;
free_handlers:
    for (i = 0;i < singleton_count;i++) {
        FxWaylandSingletonListener *listener = singleton_listeners + i;
        if (*listener->ret) {
            listener->destroy(*listener->ret);
        }
    }
    fcitx_handler_table_free(wl->global_handlers);
destroy_registry:
    wl_registry_destroy(wl->registry);
close_epoll:
    close(wl->epoll_fd);
disconnect:
    wl_display_disconnect(wl->dpy);
free:
    free(wl);
    return NULL;
}

static void
FxWaylandDestroy(void *self)
{
    FcitxWayland *wl = (FcitxWayland*)self;
    wl_compositor_destroy(wl->compositor);
    wl_shell_destroy(wl->shell);
    wl_shm_destroy(wl->shm);
    wl_data_device_manager_destroy(wl->data_device_manager);
    fcitx_handler_table_free(wl->global_handlers);
    wl_registry_destroy(wl->registry);
    close(wl->epoll_fd);
    if (!(wl->dpy_events & EPOLLERR) && !(wl->dpy_events & EPOLLHUP)) {
        wl_display_flush(wl->dpy);
    }
    wl_display_disconnect(wl->dpy);
    free(self);
}

static void
FxWaylandSetFD(void *self)
{
    FcitxWayland *wl = (FcitxWayland*)self;
    int fd = wl->epoll_fd;
    FcitxInstance *instance = wl->owner;
    FD_SET(fd, FcitxInstanceGetReadFDSet(instance));

    if (FcitxInstanceGetMaxFD(instance) < fd) {
        FcitxInstanceSetMaxFD(instance, fd);
    }
}

static void
FxWaylandProcessEvent(void *self)
{
    FcitxWayland *wl = (FcitxWayland*)self;
    fx_epoll_dispatch(wl->epoll_fd);
    int ret = wl_display_flush(wl->dpy);
    if (ret != 0) {
        FxWaylandScheduleFlush(wl);
    }
}

#include "fcitx-wayland-addfunctions.h"
