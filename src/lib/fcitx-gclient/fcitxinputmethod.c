/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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

#include "fcitx/fcitx.h"
#include "module/dbus/dbusstuff.h"
#include "frontend/ipc/ipc.h"
#include "fcitxinputmethod.h"

/**
 * FcitxInputMethod:
 *
 * A #FcitxInputMethod allow you to control fcitx via DBus.
 */
/**
 * FcitxIMItem:
 * @name: name of im
 * @unique_name: unique_name of im
 * @langcode: language code
 * @enable: enabled or not
 *
 * A #FcitxIMItem contains some metadata for an input method in fcitx
 */

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name=\"org.fcitx.Fcitx.InputMethod\">"
    "    <method name=\"GetCurrentIM\">\n"
    "      <arg name=\"im\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"GetCurrentUI\">\n"
    "      <arg name=\"addon\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"SetCurrentIM\">\n"
    "      <arg name=\"im\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"ReloadConfig\">\n"
    "    </method>\n"
    "    <method name=\"Restart\">\n"
    "    </method>\n"
    "    <method name=\"Configure\">\n"
    "    </method>\n"
    "    <method name=\"ConfigureAddon\">\n"
    "      <arg name=\"addon\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"GetIMAddon\">\n"
    "      <arg name=\"im\" direction=\"in\" type=\"s\"/>\n"
    "      <arg name=\"addon\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"ActivateIM\">\n"
    "    </method>\n"
    "    <method name=\"InactivateIM\">\n"
    "    </method>\n"
    "    <method name=\"ToggleIM\">\n"
    "    </method>\n"
    "    <method name=\"GetCurrentState\">\n"
    "      <arg name=\"state\" direction=\"out\" type=\"i\"/>\n"
    "    </method>\n"
    "    <property access=\"readwrite\" type=\"a(sssb)\" name=\"IMList\">"
    "      <annotation name=\"org.freedesktop.DBus.Property.EmitsChangedSignal\" value=\"true\"/>"
    "    </property>"
    "  </interface>"
    "</node>";


enum {
    IMLIST_CHANGED_SIGNAL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};

FCITX_EXPORT_API
GType        fcitx_input_method_get_type(void) G_GNUC_CONST;

FCITX_EXPORT_API
GType        fcitx_im_item_get_type(void) G_GNUC_CONST;

G_DEFINE_TYPE(FcitxInputMethod, fcitx_input_method, G_TYPE_DBUS_PROXY)

static GDBusInterfaceInfo * _fcitx_input_method_get_interface_info(void);
static void _fcitx_im_item_foreach_cb(gpointer data, gpointer user_data);

static GDBusInterfaceInfo *
_fcitx_input_method_get_interface_info(void)
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

static void
fcitx_input_method_finalize(GObject *object)
{
    G_GNUC_UNUSED FcitxInputMethod *im = FCITX_INPUT_METHOD(object);

    if (G_OBJECT_CLASS(fcitx_input_method_parent_class)->finalize != NULL)
        G_OBJECT_CLASS(fcitx_input_method_parent_class)->finalize(object);
}

static void
fcitx_input_method_init(FcitxInputMethod *im)
{
    /* Sets the expected interface */
    g_dbus_proxy_set_interface_info(G_DBUS_PROXY(im), _fcitx_input_method_get_interface_info());
}

/**
 * fcitx_input_method_get_imlist: (skip)
 **/
FCITX_EXPORT_API
GPtrArray*
fcitx_input_method_get_imlist(FcitxInputMethod* im)
{
    GPtrArray *array = fcitx_input_method_get_imlist_nofree(im);
    if (array)
        g_ptr_array_set_free_func(array, (GDestroyNotify)fcitx_im_item_free);
    return array;
}

/**
 * fcitx_input_method_get_imlist_nofree:
 * @im: A #FcitxInputMethod
 *
 * Get Fcitx all im list
 *
 * Returns: (transfer full) (element-type FcitxIMItem): A #FcitxIMItem List
 *
 * Rename to: fcitx_input_method_get_imlist
 **/
FCITX_EXPORT_API
GPtrArray*
fcitx_input_method_get_imlist_nofree(FcitxInputMethod* im)
{
    GPtrArray *array = NULL;
    GVariant* value;
    GVariantIter *iter;
    gchar *name, *unique_name, *langcode;
    gboolean enable;
    value = g_dbus_proxy_get_cached_property(G_DBUS_PROXY(im), "IMList");

    if (value == NULL) {
        GError* error = NULL;
        GVariant* result = g_dbus_connection_call_sync(g_dbus_proxy_get_connection(G_DBUS_PROXY(im)),
                           g_dbus_proxy_get_name(G_DBUS_PROXY(im)),
                           FCITX_IM_DBUS_PATH,
                           "org.freedesktop.DBus.Properties",
                           "Get",
                           g_variant_new("(ss)", FCITX_IM_DBUS_INTERFACE, "IMList"),
                           G_VARIANT_TYPE("(v)"),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,           /* timeout */
                           NULL,
                           &error);

        if (error) {
            g_warning("%s", error->message);
            g_error_free(error);
        } else if (result) {
            g_variant_get(result, "(v)", &value);
            g_variant_unref(result);
        }
    }

    if (value) {
        array = g_ptr_array_new();
        g_variant_get(value, "a(sssb)", &iter);
        while (g_variant_iter_next(iter, "(sssb)", &name, &unique_name, &langcode, &enable, NULL)) {
            FcitxIMItem *item = g_slice_new(FcitxIMItem);
            item->name = name;
            item->unique_name = unique_name;
            item->langcode = langcode;
            item->enable = enable;
            g_ptr_array_add(array, item);
        }
        g_variant_iter_free(iter);

        g_variant_unref(value);
    }

    return array;
}

/**
 * fcitx_input_method_set_imlist:
 * @im: A #FcitxInputMethod
 * @array: (element-type FcitxIMItem) (transfer none): A #FcitxIMItem List
 *
 * Set Fcitx all im list
 **/
FCITX_EXPORT_API
void
fcitx_input_method_set_imlist(FcitxInputMethod *im, GPtrArray* array)
{
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sssb)"));
    g_ptr_array_foreach(array, _fcitx_im_item_foreach_cb, &builder);
    GVariant* value = g_variant_builder_end(&builder);
    GError* error = NULL;
    GVariant* result = g_dbus_connection_call_sync(g_dbus_proxy_get_connection(G_DBUS_PROXY(im)),
                       g_dbus_proxy_get_name(G_DBUS_PROXY(im)),
                       FCITX_IM_DBUS_PATH,
                       "org.freedesktop.DBus.Properties",
                       "Set",
                       g_variant_new("(ssv)", FCITX_IM_DBUS_INTERFACE, "IMList", value),
                       G_VARIANT_TYPE_UNIT,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,           /* timeout */
                       NULL,
                       &error);

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    }

    g_variant_unref(result);
}

static void
fcitx_input_method_g_properties_changed(GDBusProxy          *proxy,
                                        GVariant            *changed_properties,
                                        const gchar* const  *invalidated_properties)
{
    FcitxInputMethod *user = FCITX_INPUT_METHOD(proxy);
    GVariantIter *iter;
    const gchar *key;

    if (changed_properties != NULL) {
        g_variant_get(changed_properties, "a{sv}", &iter);
        while (g_variant_iter_next(iter, "{&sv}", &key, NULL)) {
            if (g_strcmp0(key, "IMList") == 0)
                g_signal_emit(user, signals[IMLIST_CHANGED_SIGNAL], 0);
        }
        g_variant_iter_free(iter);
    }

    if (invalidated_properties != NULL) {
        const gchar*const* item = invalidated_properties;
        while (*item) {
            if (g_strcmp0(*item, "IMList") == 0)
                g_signal_emit(user, signals[IMLIST_CHANGED_SIGNAL], 0);
            item++;
        }
    }
}

static void
fcitx_input_method_g_signal(GDBusProxy *proxy, const gchar *sender_name,
                            const gchar *signal_name, GVariant *parameters)
{
    FCITX_UNUSED(proxy);
    FCITX_UNUSED(sender_name);
    FCITX_UNUSED(signal_name);
    FCITX_UNUSED(parameters);
}

static void
fcitx_input_method_class_init(FcitxInputMethodClass *klass)
{
    GObjectClass *gobject_class;
    GDBusProxyClass *proxy_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = fcitx_input_method_finalize;

    proxy_class = G_DBUS_PROXY_CLASS(klass);
    proxy_class->g_signal             = fcitx_input_method_g_signal;
    proxy_class->g_properties_changed = fcitx_input_method_g_properties_changed;

    /* install signals */
    /**
     * FcitxInputMethod::imlist-changed:
     * @im: A #FcitxInputMethod
     *
     * Emit when input method list changed
     */
    signals[IMLIST_CHANGED_SIGNAL] = g_signal_new("imlist-changed",
                                     FCITX_TYPE_INPUT_METHOD,
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__VOID,
                                     G_TYPE_NONE,
                                     0);
}

/**
 * fcitx_input_method_new:
 * @bus_type: #GBusType
 * @flags:  #GDBusProxyFlags
 * @display_number: display_number
 * @cancellable: A #GCancellable or %NULL
 * @error: Error or %NULL
 *
 * New a #fcitxInputMethod.
 *
 * Returns: A newly allocated #FcitxInputMethod.
 */
FCITX_EXPORT_API
FcitxInputMethod*
fcitx_input_method_new(GBusType             bus_type,
                           GDBusProxyFlags      flags,
                           gint                 display_number,
                           GCancellable        *cancellable,
                           GError             **error)
{
    gchar servicename[64];
    sprintf(servicename, "%s-%d", FCITX_DBUS_SERVICE, display_number);

    char* name = servicename;
    FcitxInputMethod* im =  g_initable_new(FCITX_TYPE_INPUT_METHOD,
                                           cancellable,
                                           error,
                                           "g-flags", flags,
                                           "g-name", name,
                                           "g-bus-type", bus_type,
                                           "g-object-path", FCITX_IM_DBUS_PATH,
                                           "g-interface-name", FCITX_IM_DBUS_INTERFACE,
                                           NULL);
    return FCITX_INPUT_METHOD(im);
}

/**
 * fcitx_im_item_new:
 * @name: name of im
 * @unique_name: unique_name of im
 * @langcode: language code
 * @enable: enabled or not
 *
 * Returns: the new #FcitxIMItem
 */
FCITX_EXPORT_API
FcitxIMItem* fcitx_im_item_new(const gchar* name, const gchar* unique_name, const gchar* langcode, gboolean enable)
{
    FcitxIMItem* item = g_slice_new(FcitxIMItem);
    item->name = g_strdup(name);
    item->unique_name = g_strdup(unique_name);
    item->langcode = g_strdup(langcode);
    item->enable = enable;
    return item;
}

static
FcitxIMItem*
fcitx_im_item_copy(FcitxIMItem* src)
{
    return fcitx_im_item_new(src->name, src->unique_name,
                             src->langcode, src->enable);
}

/**
 * fcitx_im_item_free: (skip)
 * @data: A #FcitxIMItem
 *
 * free an im_item
 **/
FCITX_EXPORT_API
void fcitx_im_item_free(FcitxIMItem* data)
{
    FcitxIMItem* item = data;
    g_free(item->name);
    g_free(item->unique_name);
    g_free(item->langcode);
    g_slice_free(FcitxIMItem, data);
}

G_DEFINE_BOXED_TYPE(FcitxIMItem, fcitx_im_item, fcitx_im_item_copy,
                    fcitx_im_item_free)

void _fcitx_im_item_foreach_cb(gpointer       data,
                                   gpointer       user_data)
{
    FcitxIMItem* item = data;
    GVariantBuilder* builder = user_data;

    g_variant_builder_add(builder, "(sssb)", item->name, item->unique_name, item->langcode, item->enable);
}

/**
 * fcitx_input_method_exit:
 * @im: A #FcitxInputMethod
 *
 * Send exit command to fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_exit(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "Exit",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_configure:
 * @im: A #FcitxInputMethod
 *
 * Send configure command to fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_configure(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "Configure",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_configure_addon:
 * @im: A #FcitxInputMethod
 * @addon: (transfer none): addon name
 *
 * Send configure addon command to fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_configure_addon(FcitxInputMethod* im, gchar* addon)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "ConfigureAddon",
                      g_variant_new("(s)", addon),
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_configure_im:
 * @im: A #FcitxInputMethod
 * @imname: (transfer none): im name
 *
 * Send configure im command to fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_configure_im(FcitxInputMethod* im, gchar* imname)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "ConfigureIM",
                      g_variant_new("(s)", imname),
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_get_current_im:
 * @im: A #FcitxInputMethod
 *
 * Get im name
 *
 * Returns: (transfer full): get im name
 **/
FCITX_EXPORT_API
gchar* fcitx_input_method_get_current_im(FcitxInputMethod* im)
{
    GError* error = NULL;
    GVariant* variant = g_dbus_proxy_call_sync(G_DBUS_PROXY(im),
                                               "GetCurrentIM",
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                               -1,
                                               NULL,
                                               &error
                                              );

    gchar* result = NULL;

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    } else if (variant) {
        g_variant_get(variant, "(s)", &result);
        g_variant_unref(variant);
    }

    return result;
}

/**
 * fcitx_input_method_get_current_ui:
 * @im: A #FcitxInputMethod
 *
 * Get im name
 *
 * Returns: (transfer full): get ui name
 **/
FCITX_EXPORT_API
gchar* fcitx_input_method_get_current_ui(FcitxInputMethod* im)
{
    GError* error = NULL;
    GVariant* variant = g_dbus_proxy_call_sync(G_DBUS_PROXY(im),
                                               "GetCurrentUI",
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                               -1,
                                               NULL,
                                               &error
                                              );

    gchar* result = NULL;

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    } else if (variant) {
        g_variant_get(variant, "(s)", &result);
        g_variant_unref(variant);
    }

    return result;
}

/**
 * fcitx_input_method_get_im_addon:
 * @im: A #FcitxInputMethod
 * @imname: (transfer none): imname
 *
 * Get addon name by im
 *
 * Returns: (transfer full): get addon name
 **/
FCITX_EXPORT_API
gchar* fcitx_input_method_get_im_addon(FcitxInputMethod* im, gchar* imname)
{
    GError* error = NULL;
    GVariant* variant = g_dbus_proxy_call_sync(G_DBUS_PROXY(im),
                                               "GetIMAddon",
                                               g_variant_new("(s)", imname),
                                               G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                               -1,
                                               NULL,
                                               &error
                                              );

    gchar* result = NULL;

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    } else if (variant) {
        g_variant_get(variant, "(s)", &result);
        g_variant_unref(variant);
    }

    return result;
}

/**
 * fcitx_input_method_reload_config:
 * @im: A #FcitxInputMethod
 *
 * Send reload config command to fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_reload_config(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "ReloadConfig",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_restart:
 * @im: A #FcitxInputMethod
 *
 * Send restart command to fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_restart(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "Restart",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_set_current_im:
 * @im: A #FcitxInputMethod
 * @imname: (transfer none): set im name
 *
 * Set im name
 **/
FCITX_EXPORT_API
void fcitx_input_method_set_current_im(FcitxInputMethod* im, gchar* imname)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "SetCurrentIM",
                      g_variant_new("(s)", imname),
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_activate:
 * @im: A #FcitxInputMethod
 *
 * Activate fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_activate(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "ActivateIM",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_inactivate:
 * @im: A #FcitxInputMethod
 *
 * Inactivate fcitx
 **/
FCITX_EXPORT_API
void fcitx_input_method_inactivate(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "InactivateIM",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_toggle:
 * @im: A #FcitxInputMethod
 *
 * toggle fcitx state
 **/
FCITX_EXPORT_API
void fcitx_input_method_toggle(FcitxInputMethod* im)
{
    g_dbus_proxy_call(G_DBUS_PROXY(im),
                      "ToggleIM",
                      NULL,
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

/**
 * fcitx_input_method_get_current_state:
 * @im: A #FcitxInputMethod
 *
 * Get current state
 *
 * Returns: current state, -1 for error
 **/
FCITX_EXPORT_API
gint fcitx_input_method_get_current_state(FcitxInputMethod* im)
{
    GError* error = NULL;
    GVariant* variant = g_dbus_proxy_call_sync(G_DBUS_PROXY(im),
                                               "GetCurrentState",
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                               -1,
                                               NULL,
                                               &error
                                              );

    gint result = -1;

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    } else if (variant) {
        g_variant_get(variant, "(i)", &result);
        g_variant_unref(variant);
    }

    return result;
}
