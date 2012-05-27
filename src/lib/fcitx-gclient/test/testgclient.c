#include <fcitx-gclient/fcitxkbd.h>

static gboolean
timeout_cb (gpointer data)
{
    g_main_loop_quit ((GMainLoop *)data);
    return FALSE;
}

static void
run_loop_with_timeout (gint interval)
{
    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    g_timeout_add (interval, timeout_cb, loop);
    g_main_loop_run (loop);
    g_main_loop_unref (loop);
}

static void
foreach_cb(gpointer data, gpointer user_data)
{
    FcitxLayoutItem* item = data;
    g_debug("%s %s %s %s", item->layout, item->variant, item->name, item->langcode);
}

static void
test_keyboard (void)
{
    run_loop_with_timeout (1000);
    FcitxKbd* kbd = fcitx_kbd_new(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, 0, NULL, NULL);

    GPtrArray* layouts = fcitx_kbd_get_layouts(kbd);

    g_ptr_array_foreach(layouts, foreach_cb, NULL);

    g_ptr_array_free(layouts, FALSE);

    g_object_unref(G_OBJECT(kbd));
}

int main(int argc, char* argv[])
{
    g_type_init();
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/fcitx/keyboard", test_keyboard);

    return g_test_run ();
}