/* 
 * Copyright © 2012, 2013 Intel Corporation
 * 
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_keyboard_interface;
extern const struct wl_interface wl_input_method_context_interface;
extern const struct wl_interface wl_input_method_context_interface;
extern const struct wl_interface wl_input_panel_surface_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_output_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_keyboard_interface,
	&wl_input_method_context_interface,
	&wl_input_method_context_interface,
	&wl_input_panel_surface_interface,
	&wl_surface_interface,
	&wl_output_interface,
	NULL,
};

static const struct wl_message wl_input_method_context_requests[] = {
	{ "destroy", "", types + 0 },
	{ "commit_string", "us", types + 0 },
	{ "preedit_string", "uss", types + 0 },
	{ "preedit_styling", "uuu", types + 0 },
	{ "preedit_cursor", "i", types + 0 },
	{ "delete_surrounding_text", "iu", types + 0 },
	{ "cursor_position", "ii", types + 0 },
	{ "modifiers_map", "a", types + 0 },
	{ "keysym", "uuuuu", types + 0 },
	{ "grab_keyboard", "n", types + 5 },
	{ "key", "uuuu", types + 0 },
	{ "modifiers", "uuuuu", types + 0 },
	{ "language", "us", types + 0 },
	{ "text_direction", "uu", types + 0 },
};

static const struct wl_message wl_input_method_context_events[] = {
	{ "surrounding_text", "suu", types + 0 },
	{ "reset", "", types + 0 },
	{ "content_type", "uu", types + 0 },
	{ "invoke_action", "uu", types + 0 },
	{ "commit_state", "u", types + 0 },
	{ "preferred_language", "s", types + 0 },
};

WL_EXPORT const struct wl_interface wl_input_method_context_interface = {
	"wl_input_method_context", 1,
	14, wl_input_method_context_requests,
	6, wl_input_method_context_events,
};

static const struct wl_message wl_input_method_events[] = {
	{ "activate", "n", types + 6 },
	{ "deactivate", "o", types + 7 },
};

WL_EXPORT const struct wl_interface wl_input_method_interface = {
	"wl_input_method", 1,
	0, NULL,
	2, wl_input_method_events,
};

static const struct wl_message wl_input_panel_requests[] = {
	{ "get_input_panel_surface", "no", types + 8 },
};

WL_EXPORT const struct wl_interface wl_input_panel_interface = {
	"wl_input_panel", 1,
	1, wl_input_panel_requests,
	0, NULL,
};

static const struct wl_message wl_input_panel_surface_requests[] = {
	{ "set_toplevel", "ou", types + 10 },
	{ "set_overlay_panel", "", types + 0 },
};

WL_EXPORT const struct wl_interface wl_input_panel_surface_interface = {
	"wl_input_panel_surface", 1,
	2, wl_input_panel_surface_requests,
	0, NULL,
};

