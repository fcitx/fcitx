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

#ifndef INPUT_METHOD_CLIENT_PROTOCOL_H
#define INPUT_METHOD_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_input_method_context;
struct wl_input_method;
struct wl_input_panel;
struct wl_input_panel_surface;

extern const struct wl_interface wl_input_method_context_interface;
extern const struct wl_interface wl_input_method_interface;
extern const struct wl_interface wl_input_panel_interface;
extern const struct wl_interface wl_input_panel_surface_interface;

/**
 * wl_input_method_context - input method context
 * @surrounding_text: surrounding text event
 * @reset: (none)
 * @content_type: (none)
 * @invoke_action: (none)
 * @commit_state: (none)
 * @preferred_language: (none)
 *
 * Corresponds to a text model on input method side. An input method
 * context is created on text mode activation on the input method side. It
 * allows to receive information about the text model from the application
 * via events. Input method contexts do not keep state after deactivation
 * and should be destroyed after deactivation is handled.
 *
 * Text is generally UTF-8 encoded, indices and lengths are in bytes.
 *
 * Serials are used to synchronize the state between the text input and an
 * input method. New serials are sent by the text input in the commit_state
 * request and are used by the input method to indicate the known text
 * input state in events like preedit_string, commit_string, and keysym.
 * The text input can then ignore events from the input method which are
 * based on an outdated state (for example after a reset).
 */
struct wl_input_method_context_listener {
	/**
	 * surrounding_text - surrounding text event
	 * @text: (none)
	 * @cursor: (none)
	 * @anchor: (none)
	 *
	 * The plain surrounding text around the input position. Cursor
	 * is the position in bytes within the surrounding text relative to
	 * the beginning of the text. Anchor is the position in bytes of
	 * the selection anchor within the surrounding text relative to the
	 * beginning of the text. If there is no selected text anchor is
	 * the same as cursor.
	 */
	void (*surrounding_text)(void *data,
				 struct wl_input_method_context *wl_input_method_context,
				 const char *text,
				 uint32_t cursor,
				 uint32_t anchor);
	/**
	 * reset - (none)
	 */
	void (*reset)(void *data,
		      struct wl_input_method_context *wl_input_method_context);
	/**
	 * content_type - (none)
	 * @hint: (none)
	 * @purpose: (none)
	 */
	void (*content_type)(void *data,
			     struct wl_input_method_context *wl_input_method_context,
			     uint32_t hint,
			     uint32_t purpose);
	/**
	 * invoke_action - (none)
	 * @button: (none)
	 * @index: (none)
	 */
	void (*invoke_action)(void *data,
			      struct wl_input_method_context *wl_input_method_context,
			      uint32_t button,
			      uint32_t index);
	/**
	 * commit_state - (none)
	 * @serial: serial of text input state
	 */
	void (*commit_state)(void *data,
			     struct wl_input_method_context *wl_input_method_context,
			     uint32_t serial);
	/**
	 * preferred_language - (none)
	 * @language: (none)
	 */
	void (*preferred_language)(void *data,
				   struct wl_input_method_context *wl_input_method_context,
				   const char *language);
};

static inline int
wl_input_method_context_add_listener(struct wl_input_method_context *wl_input_method_context,
				     const struct wl_input_method_context_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_input_method_context,
				     (void (**)(void)) listener, data);
}

#define WL_INPUT_METHOD_CONTEXT_DESTROY	0
#define WL_INPUT_METHOD_CONTEXT_COMMIT_STRING	1
#define WL_INPUT_METHOD_CONTEXT_PREEDIT_STRING	2
#define WL_INPUT_METHOD_CONTEXT_PREEDIT_STYLING	3
#define WL_INPUT_METHOD_CONTEXT_PREEDIT_CURSOR	4
#define WL_INPUT_METHOD_CONTEXT_DELETE_SURROUNDING_TEXT	5
#define WL_INPUT_METHOD_CONTEXT_CURSOR_POSITION	6
#define WL_INPUT_METHOD_CONTEXT_MODIFIERS_MAP	7
#define WL_INPUT_METHOD_CONTEXT_KEYSYM	8
#define WL_INPUT_METHOD_CONTEXT_GRAB_KEYBOARD	9
#define WL_INPUT_METHOD_CONTEXT_KEY	10
#define WL_INPUT_METHOD_CONTEXT_MODIFIERS	11
#define WL_INPUT_METHOD_CONTEXT_LANGUAGE	12
#define WL_INPUT_METHOD_CONTEXT_TEXT_DIRECTION	13

static inline void
wl_input_method_context_set_user_data(struct wl_input_method_context *wl_input_method_context, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_input_method_context, user_data);
}

static inline void *
wl_input_method_context_get_user_data(struct wl_input_method_context *wl_input_method_context)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_input_method_context);
}

static inline void
wl_input_method_context_destroy(struct wl_input_method_context *wl_input_method_context)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) wl_input_method_context);
}

static inline void
wl_input_method_context_commit_string(struct wl_input_method_context *wl_input_method_context, uint32_t serial, const char *text)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_COMMIT_STRING, serial, text);
}

static inline void
wl_input_method_context_preedit_string(struct wl_input_method_context *wl_input_method_context, uint32_t serial, const char *text, const char *commit)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_PREEDIT_STRING, serial, text, commit);
}

static inline void
wl_input_method_context_preedit_styling(struct wl_input_method_context *wl_input_method_context, uint32_t index, uint32_t length, uint32_t style)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_PREEDIT_STYLING, index, length, style);
}

static inline void
wl_input_method_context_preedit_cursor(struct wl_input_method_context *wl_input_method_context, int32_t index)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_PREEDIT_CURSOR, index);
}

static inline void
wl_input_method_context_delete_surrounding_text(struct wl_input_method_context *wl_input_method_context, int32_t index, uint32_t length)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_DELETE_SURROUNDING_TEXT, index, length);
}

static inline void
wl_input_method_context_cursor_position(struct wl_input_method_context *wl_input_method_context, int32_t index, int32_t anchor)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_CURSOR_POSITION, index, anchor);
}

static inline void
wl_input_method_context_modifiers_map(struct wl_input_method_context *wl_input_method_context, struct wl_array *map)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_MODIFIERS_MAP, map);
}

static inline void
wl_input_method_context_keysym(struct wl_input_method_context *wl_input_method_context, uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_KEYSYM, serial, time, sym, state, modifiers);
}

static inline struct wl_keyboard *
wl_input_method_context_grab_keyboard(struct wl_input_method_context *wl_input_method_context)
{
	struct wl_proxy *keyboard;

	keyboard = wl_proxy_create((struct wl_proxy *) wl_input_method_context,
			     &wl_keyboard_interface);
	if (!keyboard)
		return NULL;

	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_GRAB_KEYBOARD, keyboard);

	return (struct wl_keyboard *) keyboard;
}

static inline void
wl_input_method_context_key(struct wl_input_method_context *wl_input_method_context, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_KEY, serial, time, key, state);
}

static inline void
wl_input_method_context_modifiers(struct wl_input_method_context *wl_input_method_context, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_MODIFIERS, serial, mods_depressed, mods_latched, mods_locked, group);
}

static inline void
wl_input_method_context_language(struct wl_input_method_context *wl_input_method_context, uint32_t serial, const char *language)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_LANGUAGE, serial, language);
}

static inline void
wl_input_method_context_text_direction(struct wl_input_method_context *wl_input_method_context, uint32_t serial, uint32_t direction)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_method_context,
			 WL_INPUT_METHOD_CONTEXT_TEXT_DIRECTION, serial, direction);
}

/**
 * wl_input_method - input method
 * @activate: activate event
 * @deactivate: activate event
 *
 * An input method object is responsible to compose text in response to
 * input from hardware or virtual keyboards. There is one input method
 * object per seat. On activate there is a new input method context object
 * created which allows the input method to communicate with the text
 * model.
 */
struct wl_input_method_listener {
	/**
	 * activate - activate event
	 * @id: (none)
	 *
	 * A text model was activated. Creates an input method context
	 * object which allows communication with the text model.
	 */
	void (*activate)(void *data,
			 struct wl_input_method *wl_input_method,
			 struct wl_input_method_context *id);
	/**
	 * deactivate - activate event
	 * @context: (none)
	 *
	 * The text model corresponding to the context argument was
	 * deactivated. The input method context should be destroyed after
	 * deactivation is handled.
	 */
	void (*deactivate)(void *data,
			   struct wl_input_method *wl_input_method,
			   struct wl_input_method_context *context);
};

static inline int
wl_input_method_add_listener(struct wl_input_method *wl_input_method,
			     const struct wl_input_method_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_input_method,
				     (void (**)(void)) listener, data);
}

static inline void
wl_input_method_set_user_data(struct wl_input_method *wl_input_method, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_input_method, user_data);
}

static inline void *
wl_input_method_get_user_data(struct wl_input_method *wl_input_method)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_input_method);
}

static inline void
wl_input_method_destroy(struct wl_input_method *wl_input_method)
{
	wl_proxy_destroy((struct wl_proxy *) wl_input_method);
}

#define WL_INPUT_PANEL_GET_INPUT_PANEL_SURFACE	0

static inline void
wl_input_panel_set_user_data(struct wl_input_panel *wl_input_panel, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_input_panel, user_data);
}

static inline void *
wl_input_panel_get_user_data(struct wl_input_panel *wl_input_panel)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_input_panel);
}

static inline void
wl_input_panel_destroy(struct wl_input_panel *wl_input_panel)
{
	wl_proxy_destroy((struct wl_proxy *) wl_input_panel);
}

static inline struct wl_input_panel_surface *
wl_input_panel_get_input_panel_surface(struct wl_input_panel *wl_input_panel, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_create((struct wl_proxy *) wl_input_panel,
			     &wl_input_panel_surface_interface);
	if (!id)
		return NULL;

	wl_proxy_marshal((struct wl_proxy *) wl_input_panel,
			 WL_INPUT_PANEL_GET_INPUT_PANEL_SURFACE, id, surface);

	return (struct wl_input_panel_surface *) id;
}

#ifndef WL_INPUT_PANEL_SURFACE_POSITION_ENUM
#define WL_INPUT_PANEL_SURFACE_POSITION_ENUM
enum wl_input_panel_surface_position {
	WL_INPUT_PANEL_SURFACE_POSITION_CENTER_BOTTOM = 0,
};
#endif /* WL_INPUT_PANEL_SURFACE_POSITION_ENUM */

#define WL_INPUT_PANEL_SURFACE_SET_TOPLEVEL	0
#define WL_INPUT_PANEL_SURFACE_SET_OVERLAY_PANEL	1

static inline void
wl_input_panel_surface_set_user_data(struct wl_input_panel_surface *wl_input_panel_surface, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_input_panel_surface, user_data);
}

static inline void *
wl_input_panel_surface_get_user_data(struct wl_input_panel_surface *wl_input_panel_surface)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_input_panel_surface);
}

static inline void
wl_input_panel_surface_destroy(struct wl_input_panel_surface *wl_input_panel_surface)
{
	wl_proxy_destroy((struct wl_proxy *) wl_input_panel_surface);
}

static inline void
wl_input_panel_surface_set_toplevel(struct wl_input_panel_surface *wl_input_panel_surface, struct wl_output *output, uint32_t position)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_panel_surface,
			 WL_INPUT_PANEL_SURFACE_SET_TOPLEVEL, output, position);
}

static inline void
wl_input_panel_surface_set_overlay_panel(struct wl_input_panel_surface *wl_input_panel_surface)
{
	wl_proxy_marshal((struct wl_proxy *) wl_input_panel_surface,
			 WL_INPUT_PANEL_SURFACE_SET_OVERLAY_PANEL);
}

#ifdef  __cplusplus
}
#endif

#endif
