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
#include "fcitxkbd.h"

static const gchar introspection_xml[] =
"<node>"
"    <interface name=\"org.fcitx.Fcitx.Keyboard\">"
"        <method name=\"SetDefaultLayout\">\n"
"           <arg name=\"layout\" direction=\"out\" type=\"s\"/>"
"           <arg name=\"variant\" direction=\"out\" type=\"s\"/>"
"        </method>\n"
"        <method name=\"GetLayouts\">"
"            <arg name=\"layouts\" direction=\"out\" type=\"a(ssss)\"/>"
"        </method>"
"        <method name=\"GetLayoutForIM\">"
"            <arg name=\"im\" direction=\"in\" type=\"s\"/>"
"            <arg name=\"layout\" direction=\"out\" type=\"s\"/>"
"            <arg name=\"variant\" direction=\"out\" type=\"s\"/>"
"        </method>"
"        <method name=\"SetLayoutForIM\">"
"            <arg name=\"im\" direction=\"in\" type=\"s\"/>"
"            <arg name=\"layout\" direction=\"in\" type=\"s\"/>"
"            <arg name=\"variant\" direction=\"in\" type=\"s\"/>"
"        </method>"
"    </interface>"
"</node>";


G_DEFINE_TYPE(FcitxKbd, fcitx_kbd, G_TYPE_DBUS_PROXY);

FCITX_EXPORT_API
GType        fcitx_kbd_get_type(void) G_GNUC_CONST;

static GDBusInterfaceInfo * _fcitx_kbd_get_interface_info(void);

static GDBusInterfaceInfo *
_fcitx_kbd_get_interface_info(void)
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
fcitx_kbd_finalize(GObject *object)
{
    G_GNUC_UNUSED FcitxKbd *im = FCITX_KBD(object);

    if (G_OBJECT_CLASS(fcitx_kbd_parent_class)->finalize != NULL)
        G_OBJECT_CLASS(fcitx_kbd_parent_class)->finalize(object);
}

static void
fcitx_kbd_init(FcitxKbd *im)
{
    /* Sets the expected interface */
    g_dbus_proxy_set_interface_info(G_DBUS_PROXY(im), _fcitx_kbd_get_interface_info());
}

static void
fcitx_kbd_class_init(FcitxKbdClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = fcitx_kbd_finalize;

}

FCITX_EXPORT_API
FcitxKbd*
fcitx_kbd_new(GBusType             bus_type,
              GDBusProxyFlags      flags,
              gint                 display_number,
              GCancellable        *cancellable,
              GError             **error)
{
    gchar servicename[64];
    sprintf(servicename, "%s-%d", FCITX_DBUS_SERVICE, display_number);

    char* name = servicename;
    FcitxKbd* im =  g_initable_new(FCITX_TYPE_KBD,
                                   cancellable,
                                   error,
                                   "g-flags", flags,
                                   "g-name", name,
                                   "g-bus-type", bus_type,
                                   "g-object-path", "/keyboard",
                                   "g-interface-name", "org.fcitx.Fcitx.Keyboard",
                                   NULL);

    if (im != NULL)
        return FCITX_KBD(im);
    return NULL;
}

FCITX_EXPORT_API
GPtrArray* fcitx_kbd_get_layouts(FcitxKbd* kbd)
{
    GError* error = NULL;
    GVariant* variant = g_dbus_proxy_call_sync(G_DBUS_PROXY(kbd),
                                               "GetLayouts",
                                               NULL,
                                               G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                               -1,
                                               NULL,
                                               &error
                                              );

    GPtrArray* array = NULL;

    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    } else if (variant) {
        array = g_ptr_array_new_with_free_func(fcitx_layout_item_free);
        GVariantIter *iter;
        gchar *layout, *kbdvariant, *name, *langcode;
        g_variant_get(variant, "(a(ssss))", &iter);
        while (g_variant_iter_next(iter, "(ssss)", &layout, &kbdvariant, &name, &langcode, NULL)) {
            FcitxLayoutItem* item = g_malloc0(sizeof(FcitxLayoutItem));
            item->layout = g_strdup(layout);
            item->variant = g_strdup(kbdvariant);
            item->name = g_strdup(name);
            item->langcode = g_strdup(langcode);
            g_ptr_array_add(array, item);
            g_free(name);
            g_free(kbdvariant);
            g_free(layout);
            g_free(langcode);
        }
        g_variant_iter_free(iter);
    }

    return array;
}

FCITX_EXPORT_API
void fcitx_kbd_get_layout_for_im(FcitxKbd* kbd, const gchar* imname, gchar** layout, gchar** variant)
{
    GError* error = NULL;
    GVariant* v = g_dbus_proxy_call_sync(G_DBUS_PROXY(kbd),
                                               "GetLayoutForIM",
                                               g_variant_new("(s)", imname),
                                               G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                               -1,
                                               NULL,
                                               &error
                                              );


    if (error) {
        g_warning("%s", error->message);
        g_error_free(error);
    } else if (v) {
        g_variant_get(v, "(ss)", layout, variant);
        g_variant_unref(v);
    }

}

FCITX_EXPORT_API
void fcitx_kbd_set_layout_for_im(FcitxKbd* kbd, const gchar* imname, const gchar* layout, const gchar* variant)
{
    g_dbus_proxy_call(G_DBUS_PROXY(kbd),
                      "SetLayoutForIM",
                      g_variant_new("(sss)", imname, layout, variant),
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

FCITX_EXPORT_API
void fcitx_kbd_set_default_layout(FcitxKbd* kbd, const gchar* layout, const gchar* variant)
{
    g_dbus_proxy_call(G_DBUS_PROXY(kbd),
                      "SetDefaultLayout",
                      g_variant_new("(ss)", layout, variant),
                      G_DBUS_CALL_FLAGS_NO_AUTO_START,
                      0,
                      NULL,
                      NULL,
                      NULL
                     );
}

void fcitx_layout_item_free(gpointer data)
{
    FcitxLayoutItem* item = data;
    g_free(item->langcode);
    g_free(item->name);
    g_free(item->variant);
    g_free(item->layout);
    g_free(data);
}
