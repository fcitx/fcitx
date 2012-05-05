/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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

#include <stdlib.h>
#include <unistd.h>
#include "module/dbus/dbusstuff.h"
#include "frontend/ipc/ipc.h"
#include "fcitx/fcitx.h"
#include "fcitx/ime.h"
#include <fcitx/frontend.h>
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "client-im.h"
#include "marshall.h"

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name=\"org.fcitx.Fcitx.InputMethod\">"
    "    <method name=\"CreateICv3\">\n"
    "      <arg name=\"appname\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"pid\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"icid\" direction=\"out\" type=\"i\"/>\n"
    "      <arg name=\"enable\" direction=\"out\" type=\"b\"/>\n"
    "      <arg name=\"keyval1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state1\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"keyval2\" direction=\"out\" type=\"u\"/>\n"
    "      <arg name=\"state2\" direction=\"out\" type=\"u\"/>\n"
    "    </method>\n"
    "  </interface>"
    "</node>";


static const gchar ic_introspection_xml[] =
    "<node>\n"
    "  <interface name=\"" FCITX_IC_DBUS_INTERFACE "\">\n"
    "    <method name=\"FocusIn\">\n"
    "    </method>\n"
    "    <method name=\"FocusOut\">\n"
    "    </method>\n"
    "    <method name=\"Reset\">\n"
    "    </method>\n"
    "    <method name=\"SetCursorRect\">\n"
    "      <arg name=\"x\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"y\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"w\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"h\" direction=\"in\" type=\"i\"/>\n"
    "    </method>\n"
    "    <method name=\"SetCapacity\">\n"
    "      <arg name=\"caps\" direction=\"in\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"SetSurroundingText\">\n"
    "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"cursor\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"anchor\" direction=\"in\" type=\"u\"/>\n"
    "    </method>\n"
    "    <method name=\"DestroyIC\">\n"
    "    </method>\n"
    "    <method name=\"ProcessKeyEvent\">\n"
    "      <arg name=\"keyval\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"keycode\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"state\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"type\" direction=\"in\" type=\"i\"/>\n"
    "      <arg name=\"time\" direction=\"in\" type=\"u\"/>\n"
    "      <arg name=\"ret\" direction=\"out\" type=\"i\"/>\n"
    "    </method>\n"
    "    <signal name=\"EnableIM\">\n"
    "    </signal>\n"
    "    <signal name=\"CloseIM\">\n"
    "    </signal>\n"
    "    <signal name=\"CommitString\">\n"
    "      <arg name=\"str\" type=\"s\"/>\n"
    "    </signal>\n"
    "    <signal name=\"DeleteSurroundingText\">\n"
    "      <arg name=\"offset\" type=\"i\"/>\n"
    "      <arg name=\"nchar\" type=\"u\"/>\n"
    "    </signal>\n"
    "    <signal name=\"UpdateFormattedPreedit\">\n"
    "      <arg name=\"str\" type=\"a(si)\"/>\n"
    "      <arg name=\"cursorpos\" type=\"i\"/>\n"
    "    </signal>\n"
    "    <signal name=\"ForwardKey\">\n"
    "      <arg name=\"keyval\" type=\"u\"/>\n"
    "      <arg name=\"state\" type=\"u\"/>\n"
    "      <arg name=\"type\" type=\"i\"/>\n"
    "    </signal>\n"
    "  </interface>\n"
    "</node>\n";

G_DEFINE_TYPE(FcitxInputMethod, fcitx_inputmethod, G_TYPE_OBJECT);

enum {
    CONNTECTED_SIGNAL,
    ENABLE_IM_SIGNAL,
    CLOSE_IM_SIGNAL,
    FORWARD_KEY_SIGNAL,
    COMMIT_STRING_SIGNAL,
    DELETE_SURROUNDING_TEXT_SIGNAL,
    UPDATED_FORMATTED_PREEDIT_SIGNAL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

static GDBusInterfaceInfo *
fcitx_inputmethod_get_interface_info(void);


static void         fcitx_inputmethod_create_ic(FcitxInputMethod* im);

static void
_fcitx_inputmethod_create_ic_cb(GObject *source_object,
                               GAsyncResult *res,
                               gpointer user_data);


static void
fcitx_inputmethod_g_signal(GDBusProxy *proxy,
                            gchar      *sender_name,
                            gchar      *signal_name,
                            GVariant   *parameters,
                            gpointer    user_data);

static GDBusInterfaceInfo *
fcitx_inputmethod_get_interface_info(void)
{
    static gsize has_info = 0;
    static GDBusInterfaceInfo *info = NULL;
    if (g_once_init_enter(&has_info)) {
        GDBusNodeInfo *introspection_data;
        introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
        info = introspection_data->interfaces[0];
        g_once_init_leave(&has_info, 1);
    }
    return info;
}

static GDBusInterfaceInfo *
fcitx_inputmethod_get_clientic_info(void)
{
    static gsize has_info = 0;
    static GDBusInterfaceInfo *info = NULL;
    if (g_once_init_enter(&has_info)) {
        GDBusNodeInfo *introspection_data;
        introspection_data = g_dbus_node_info_new_for_xml(ic_introspection_xml, NULL);
        info = introspection_data->interfaces[0];
        g_once_init_leave(&has_info, 1);
    }
    return info;
}

static void
fcitx_inputmethod_finalize(GObject *object)
{
    FcitxInputMethod *im = FCITX_INPUTMETHOD(object);

    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "DestroyIC", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
    g_bus_unwatch_name(im->watch_id);
    GDBusProxy* icproxy = im->icproxy;
    GDBusProxy* improxy = im->improxy;
    im->icproxy = NULL;
    im->improxy = NULL;
    if (icproxy)
        g_object_unref(icproxy);
    if (improxy)
        g_object_unref(improxy);

    if (G_OBJECT_CLASS(fcitx_inputmethod_parent_class)->finalize != NULL)
        G_OBJECT_CLASS(fcitx_inputmethod_parent_class)->finalize(object);
}

void fcitx_inputmethod_focusin(FcitxInputMethod* im)
{
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "FocusIn", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}


void fcitx_inputmethod_focusout(FcitxInputMethod* im)
{
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "FocusOut", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}


void fcitx_inputmethod_reset(FcitxInputMethod* im)
{
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "Reset", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}


void fcitx_inputmethod_set_capacity(FcitxInputMethod* im, FcitxCapacityFlags flags)
{
    uint32_t iflags = flags;
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "SetCapacity", g_variant_new("(u)", iflags), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}


void fcitx_inputmethod_set_cusor_rect(FcitxInputMethod* im, int x, int y, int w, int h)
{
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "SetCursorRect", g_variant_new("(iiii)", x, y, w, h), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

void fcitx_inputmethod_set_surrounding_text(FcitxInputMethod* im, gchar* text, guint cursor, guint anchor)
{
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy, "SetSurroundingText", g_variant_new("(suu)", text, cursor, anchor), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

void fcitx_inputmethod_process_key(FcitxInputMethod* im, GAsyncReadyCallback cb, gpointer user_data, guint32 keyval, guint32 keycode, guint32 state, FcitxKeyEventType type, guint32 t)
{
    int itype = type;
    if (im->icproxy) {
        g_dbus_proxy_call(im->icproxy,
                          "ProcessKeyEvent",
                          g_variant_new("(uuuiu)", keyval, keycode, state, itype, t),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1, NULL,
                          cb,
                          user_data);
    }
}

int fcitx_inputmethod_process_key_sync(FcitxInputMethod* im, guint32 keyval, guint32 keycode, guint32 state, FcitxKeyEventType type, guint32 t)
{
    int itype = type;
    GError *error = NULL;
    int ret = -1;
    if (im->icproxy) {
        GVariant* result =  g_dbus_proxy_call_sync(im->icproxy,
                          "ProcessKeyEvent",
                          g_variant_new("(uuuiu)", keyval, keycode, state, itype, t),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1, NULL,
                          &error);

        if (error)
        {
            g_error_free(error);
        }
        else {
            g_variant_get(result, "(i)", &ret);
        }
    }

    return ret;
}


void fcitx_inputmethod_appear (GDBusConnection *connection,
                                const gchar     *name,
                                const gchar     *name_owner,
                                gpointer         user_data)
{
    FcitxInputMethod* im = (FcitxInputMethod*) user_data;
    gboolean new_owner_good = name_owner && (name_owner[0] != '\0');
    if (new_owner_good) {
        if (im->improxy) {
            g_object_unref(im->improxy);
            im->improxy = NULL;
        }

        if (im->icproxy) {
            g_object_unref(im->icproxy);
            im->icproxy = NULL;
        }

        fcitx_inputmethod_create_ic(im);
    }
}

void fcitx_inputmethod_vanish (GDBusConnection *connection,
                               const gchar     *name,
                               gpointer         user_data)
{
    FcitxInputMethod* im = (FcitxInputMethod*) user_data;
    if (im->improxy) {
        g_object_unref(im->improxy);
        im->improxy = NULL;
    }

    if (im->icproxy) {
        g_object_unref(im->icproxy);
        im->icproxy = NULL;
    }
}

static void
fcitx_inputmethod_init(FcitxInputMethod *im)
{
    sprintf(im->servicename, "%s-%d", FCITX_DBUS_SERVICE, fcitx_utils_get_display_number());

    im->watch_id = g_bus_watch_name(
        G_BUS_TYPE_SESSION,
        im->servicename,
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        fcitx_inputmethod_appear,
        fcitx_inputmethod_vanish,
        im,
        NULL
    );
    im->improxy = NULL;
    im->icproxy = NULL;

    fcitx_inputmethod_create_ic(im);
}
static void
fcitx_inputmethod_create_ic(FcitxInputMethod *im)
{
    GError* error = NULL;
    im->improxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
                                            fcitx_inputmethod_get_interface_info(),
                                            im->servicename,
                                            FCITX_IM_DBUS_PATH,
                                            FCITX_IM_DBUS_INTERFACE,
                                            NULL,
                                            &error
                                           );

    gchar* owner_name = g_dbus_proxy_get_name_owner(im->improxy);

    if (!owner_name) {
        g_object_unref(im->improxy);
        im->improxy = NULL;
        return;
    }
    g_free(owner_name);

    char* appname = fcitx_utils_get_process_name();
    int pid = getpid();
    g_dbus_proxy_call(im->improxy,
                      "CreateICv3",
                      g_variant_new("(si)", appname, pid),
                      G_DBUS_CALL_FLAGS_NONE,
                      -1,           /* timeout */
                      NULL,
                      _fcitx_inputmethod_create_ic_cb,
                      im);
    free(appname);

}

void
_fcitx_inputmethod_create_ic_cb(GObject *source_object,
                               GAsyncResult *res,
                               gpointer user_data)
{
    GError* error = NULL;
    GVariant* result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), res, &error);
    FcitxInputMethod* im = (FcitxInputMethod*) user_data;

    if (error) {
        g_error_free(error);
        return;
    }

    gboolean enable;
    guint32 key1, state1, key2, state2;
    g_variant_get(result, "(ibuuuu)", &im->id, &enable, &key1, &state1, &key2, &state2);

    sprintf(im->icname, FCITX_IC_DBUS_PATH, im->id);

    im->icproxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        fcitx_inputmethod_get_clientic_info(),
        im->servicename,
        im->icname,
        FCITX_IC_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error) {
        g_error_free(error);
    }

    if (!im->icproxy)
        return;

    gchar* owner_name = g_dbus_proxy_get_name_owner(im->icproxy);

    if (!owner_name) {
        g_object_unref(im->icproxy);
        im->icproxy = NULL;
        return;
    }
    g_free(owner_name);

    g_signal_connect(im->icproxy, "g-signal", G_CALLBACK(fcitx_inputmethod_g_signal), im);
    g_signal_emit(user_data, signals[CONNTECTED_SIGNAL], 0);
}

void _item_free(gpointer arg)
{
    FcitxPreeditItem* item = arg;
    free(item->string);
    free(item);
}

static void
fcitx_inputmethod_g_signal(GDBusProxy *proxy,
                            gchar      *sender_name,
                            gchar      *signal_name,
                            GVariant   *parameters,
                            gpointer    user_data)
{
    g_warning("%s %s", signal_name,  g_variant_print(parameters, true));
    if (strcmp(signal_name, "EnableIM") == 0) {
        g_signal_emit(user_data, signals[ENABLE_IM_SIGNAL], 0);
    }
    else if (strcmp(signal_name, "CloseIM") == 0) {
        g_signal_emit(user_data, signals[CLOSE_IM_SIGNAL], 0);
    }
    else if (strcmp(signal_name, "CommitString") == 0) {
        const gchar* data = NULL;
        g_variant_get(parameters, "(s)", &data);
        if (data) {
            g_signal_emit(user_data, signals[COMMIT_STRING_SIGNAL], 0, data);
        }
    }
    else if (strcmp(signal_name, "ForwardKey") == 0) {
        guint32 key, state;
        gint32 type;
        g_variant_get(parameters, "(uui)", &key, &state, &type);
        g_signal_emit(user_data, signals[FORWARD_KEY_SIGNAL], 0, key, state, type);
    }
    else if (strcmp(signal_name, "DeleteSurroundingText") == 0) {
        guint32 nchar;
        gint32 offset;
        g_variant_get(parameters, "(iu)", &offset, &nchar);
        g_signal_emit(user_data, signals[DELETE_SURROUNDING_TEXT_SIGNAL], 0, offset, nchar);
    }
    else if (strcmp(signal_name, "UpdateFormattedPreedit") == 0) {
        int cursor_pos;
        GPtrArray* array = g_ptr_array_new_with_free_func(_item_free);
        GVariantIter* iter;
        g_variant_get(parameters, "(a(si)i)", &iter, &cursor_pos);

        gchar* string;
        int type;
        while (g_variant_iter_next(iter, "(si)", &string, &type, NULL)) {
            FcitxPreeditItem* item = g_malloc0(sizeof(FcitxPreeditItem));
            item->string = strdup(string);
            item->type = type;
            g_ptr_array_add(array, item);
            g_free(string);
        }
        g_variant_iter_free(iter);
        g_signal_emit(user_data, signals[UPDATED_FORMATTED_PREEDIT_SIGNAL], 0, array, cursor_pos);
        g_ptr_array_free(array, TRUE);
    }
}

static void
fcitx_inputmethod_class_init(FcitxInputMethodClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = fcitx_inputmethod_finalize;

    signals[CONNTECTED_SIGNAL] = g_signal_new("connected",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

    signals[ENABLE_IM_SIGNAL] = g_signal_new("enable-im",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

    signals[CLOSE_IM_SIGNAL] = g_signal_new("close-im",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);

    signals[FORWARD_KEY_SIGNAL] = g_signal_new("forward-key",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     fcitx_marshall_VOID__UINT_UINT_INT,
                                     G_TYPE_NONE,
                                     3,
                                     G_TYPE_UINT, G_TYPE_INT, G_TYPE_INT
                                              );

    signals[COMMIT_STRING_SIGNAL] = g_signal_new("commit-string",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__STRING,
                                     G_TYPE_NONE,
                                     1,
                                     G_TYPE_STRING);

    signals[DELETE_SURROUNDING_TEXT_SIGNAL] = g_signal_new("delete-surrounding-text",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     fcitx_marshall_VOID__INT_UINT,
                                     G_TYPE_NONE,
                                     2,
                                     G_TYPE_INT, G_TYPE_UINT);

    signals[UPDATED_FORMATTED_PREEDIT_SIGNAL] = g_signal_new("update-formatted-preedit",
                                     FCITX_TYPE_INPUTMETHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     fcitx_marshall_VOID__BOXED_INT,
                                     G_TYPE_NONE,
                                     2,
                                     G_TYPE_PTR_ARRAY, G_TYPE_INT
                                                            );
}

FcitxInputMethod*
fcitx_inputmethod_new()
{
    FcitxInputMethod* im = g_object_new(FCITX_TYPE_INPUTMETHOD, NULL);

    if (im != NULL) {
        return FCITX_INPUTMETHOD(im);
    }
    else
        return NULL;
    return im;
}

gboolean fcitx_inputmethod_is_valid(FcitxInputMethod* im)
{
    return im->icproxy != NULL;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
