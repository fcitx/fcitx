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
#include "wayland-input.h"
#include "wayland-global.h"

static void
FxWaylandPointerEnterHandler(void *data, struct wl_pointer *wl_pointer,
                             uint32_t serial, struct wl_surface *surface,
                             wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    // TODO
}

static void
FxWaylandPointerLeaveHandler(void *data, struct wl_pointer *wl_pointer,
                             uint32_t serial, struct wl_surface *surface)
{
    // TODO
}

static void
FxWaylandPointerMotionHandler(void *data, struct wl_pointer *wl_pointer,
                              uint32_t time, wl_fixed_t surface_x,
                              wl_fixed_t surface_y)
{
    // TODO
}

static void
FxWaylandPointerButtonHandler(void *data, struct wl_pointer *wl_pointer,
                              uint32_t serial, uint32_t time,
                              uint32_t button, uint32_t state)
{
    // TODO
}

static void
FxWaylandPointerAxisHandler(void *data, struct wl_pointer *wl_pointer,
                            uint32_t time, uint32_t axis, wl_fixed_t value)
{
    // TODO
}

static const struct wl_pointer_listener fx_pointer_listener = {
    .enter = FxWaylandPointerEnterHandler,
    .leave = FxWaylandPointerLeaveHandler,
    .motion = FxWaylandPointerMotionHandler,
    .button = FxWaylandPointerButtonHandler,
    .axis = FxWaylandPointerAxisHandler,
};

static void
FxWaylandKeyboardKeymapHandler(void *data, struct wl_keyboard *wl_keyboard,
                               uint32_t format, int32_t fd, uint32_t size)
{
    // TODO
}

static void
FxWaylandKeyboardEnterHandler(void *data, struct wl_keyboard *wl_keyboard,
                              uint32_t serial, struct wl_surface *surface,
                              struct wl_array *keys)
{
    // TODO
}

static void
FxWaylandKeyboardLeaveHandler(void *data, struct wl_keyboard *wl_keyboard,
                              uint32_t serial, struct wl_surface *surface)
{
    // TODO
}

static void
FxWaylandKeyboardKeyHandler(void *data, struct wl_keyboard *wl_keyboard,
                            uint32_t serial, uint32_t time,
                            uint32_t key, uint32_t state)
{
    // TODO
}

static void
FxWaylandKeyboardModifiersHandler(void *data, struct wl_keyboard *wl_keyboard,
                                  uint32_t serial, uint32_t mods_depressed,
                                  uint32_t mods_latched, uint32_t mods_locked,
                                  uint32_t group)
{
    // TODO
}

static const struct wl_keyboard_listener fx_keyboard_listener = {
    .keymap = FxWaylandKeyboardKeymapHandler,
    .enter = FxWaylandKeyboardEnterHandler,
    .leave = FxWaylandKeyboardLeaveHandler,
    .key = FxWaylandKeyboardKeyHandler,
    .modifiers = FxWaylandKeyboardModifiersHandler,
};

static void
FxWaylandTouchDownListener(void *data, struct wl_touch *wl_touch,
                           uint32_t serial, uint32_t time,
                           struct wl_surface *surface, int32_t id,
                           wl_fixed_t x, wl_fixed_t y)
{
    // TODO
}

static void
FxWaylandTouchUpListener(void *data, struct wl_touch *wl_touch,
                         uint32_t serial, uint32_t time, int32_t id)
{
    // TODO
}

static void
FxWaylandTouchMotionListener(void *data, struct wl_touch *wl_touch,
                             uint32_t time, int32_t id,
                             wl_fixed_t x, wl_fixed_t y)
{
    // TODO
}

static void
FxWaylandTouchFrameListener(void *data, struct wl_touch *wl_touch)
{
    // TODO
}

static void
FxWaylandTouchCancelListener(void *data, struct wl_touch *wl_touch)
{
    // TODO
}

static const struct wl_touch_listener fx_touch_listener = {
    .down = FxWaylandTouchDownListener,
    .up = FxWaylandTouchUpListener,
    .motion = FxWaylandTouchMotionListener,
    .frame = FxWaylandTouchFrameListener,
    .cancel = FxWaylandTouchCancelListener,
};

static void
FxWaylandSeatCapabilitiesHandler(void *data, struct wl_seat *seat,
                                 uint32_t capabilities)
{
    FcitxWaylandInput *input = data;
    /* FcitxWayland *wl = input->wl; */
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        if (!input->pointer) {
            input->pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(input->pointer,
                                    &fx_pointer_listener, input);
        }
    } else if (input->pointer) {
        wl_pointer_destroy(input->pointer);
        input->pointer = NULL;
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        if (!input->keyboard) {
            input->keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(input->keyboard,
                                     &fx_keyboard_listener, input);
        }
    } else if (input->keyboard) {
        wl_keyboard_destroy(input->keyboard);
        input->keyboard = NULL;
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
        if (!input->touch) {
            input->touch = wl_seat_get_touch(seat);
            wl_touch_add_listener(input->touch,
                                  &fx_touch_listener, input);
        }
    } else if (input->touch) {
        wl_touch_destroy(input->touch);
        input->touch = NULL;
    }
}

static const struct wl_seat_listener fx_seat_listener = {
    .capabilities = FxWaylandSeatCapabilitiesHandler,
};

static void
FxWaylandDataDeviceDataOfferHandler(void *data,
                                    struct wl_data_device *wl_data_device,
                                    struct wl_data_offer *id)
{
    // TODO
}

static void
FxWaylandDataDeviceEnterHandler(void *data,
                                struct wl_data_device *wl_data_device,
                                uint32_t serial,
                                struct wl_surface *surface,
                                wl_fixed_t x, wl_fixed_t y,
                                struct wl_data_offer *id)
{
    // TODO
}

static void
FxWaylandDataDeviceLeaveHandler(void *data,
                                struct wl_data_device *wl_data_device)
{
    printf("%s\n", __func__);
    // TODO
}

static void
FxWaylandDataDeviceMotionHandler(void *data,
                                 struct wl_data_device *wl_data_device,
                                 uint32_t time,
                                 wl_fixed_t x, wl_fixed_t y)
{
    printf("%s\n", __func__);
    // TODO
}

static void
FxWaylandDataDeviceDropHandler(void *data,
                               struct wl_data_device *wl_data_device)
{
    printf("%s\n", __func__);
    // TODO
}

static void
FxWaylandDataDeviceSelectionHandler(void *data,
                                    struct wl_data_device *wl_data_device,
                                    struct wl_data_offer *id)
{
    printf("%s\n", __func__);
    // TODO
}

static const struct wl_data_device_listener fx_data_device_listener = {
    .data_offer = FxWaylandDataDeviceDataOfferHandler,
    .enter = FxWaylandDataDeviceEnterHandler,
    .leave = FxWaylandDataDeviceLeaveHandler,
    .motion = FxWaylandDataDeviceMotionHandler,
    .drop = FxWaylandDataDeviceDropHandler,
    .selection = FxWaylandDataDeviceSelectionHandler,
};

static void
FxWaylandHandleSeatAdded(void *data, uint32_t name, const char *iface,
                         uint32_t ver)
{
    FcitxWayland *wl = data;
    FcitxWaylandInput *input = fcitx_utils_new(FcitxWaylandInput);
    input->wl = wl;
    input->seat_id = name;
    input->seat = wl_registry_bind(wl->registry, name, &wl_seat_interface, 1);
    wl_seat_add_listener(input->seat, &fx_seat_listener, wl);
    input->data_device =
        wl_data_device_manager_get_data_device(wl->data_device_manager,
                                               input->seat);
    printf("%s, data_device: %p\n", __func__, input->data_device);
    wl_data_device_add_listener(input->data_device,
                                &fx_data_device_listener, input);
    wl_list_insert(&wl->input_list, &input->link);
}

static void
FxWaylandHandleSeatRemoved(void *data, uint32_t name,
                           const char *iface, uint32_t ver)
{
    FcitxWayland *wl = data;
    FcitxWaylandInput *input;
    FcitxWaylandInput *tmp;
    wl_list_for_each_safe (input, tmp, &wl->input_list, link) {
        if (input->seat_id == name) {
            wl_list_remove(&input->link);
            wl_data_device_destroy(input->data_device);
            if (input->touch)
                wl_touch_destroy(input->touch);
            if (input->pointer)
                wl_pointer_destroy(input->pointer);
            if (input->keyboard)
                wl_keyboard_destroy(input->keyboard);
            wl_seat_destroy(input->seat);
            return;
        }
    }
}

void
FxWaylandInputInit(FcitxWayland *wl)
{
    wl_list_init(&wl->input_list);
    FxWaylandRegGlobalHandler(wl, "wl_seat", FxWaylandHandleSeatAdded,
                              FxWaylandHandleSeatRemoved, wl, true);
}
