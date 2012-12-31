#include <fcitx-gclient/fcitxkbd.h>
#include <fcitx-gclient/fcitxclient.h>
#include <fcitx-utils/keysym.h>
#include <fcitx/fcitx.h>

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
    FCITX_UNUSED(user_data);
    FcitxLayoutItem* item = data;
    g_debug("%s %s %s %s", item->layout, item->variant, item->name, item->langcode);
}

static void
test_keyboard (void)
{
    FcitxKbd* kbd = fcitx_kbd_new(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, 0, NULL, NULL);

    GPtrArray* layouts = fcitx_kbd_get_layouts(kbd);

    g_ptr_array_foreach(layouts, foreach_cb, NULL);

    g_ptr_array_free(layouts, FALSE);

    g_object_unref(G_OBJECT(kbd));
}

static void
_process_cb(GObject* obj, GAsyncResult* res, gpointer user_data)
{
    FCITX_UNUSED(obj);
    FCITX_UNUSED(res);
    FCITX_UNUSED(user_data);
}

static void
_connect_cb(FcitxClient* client, void* user_data)
{
    FCITX_UNUSED(user_data);
    GCancellable* cancellable = g_cancellable_new();
    fcitx_client_process_key_async(client, FcitxKey_a, 0, 0, 0, 0, -1, cancellable, _process_cb, NULL);
    g_cancellable_cancel(cancellable);
    g_object_unref(cancellable);
    fcitx_client_process_key_async(client, FcitxKey_a, 0, 0, 0, 0, -1, NULL, _process_cb, NULL);
}

static void
test_client (void)
{
    FcitxClient* client = fcitx_client_new();
    g_signal_connect(client, "connected", G_CALLBACK(_connect_cb), NULL);
    run_loop_with_timeout (10000);
}

int main(int argc, char* argv[])
{
#if !GLIB_CHECK_VERSION(2, 35, 1)
    g_type_init();
#endif
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/fcitx/keyboard", test_keyboard);
    g_test_add_func ("/fcitx/client", test_client);

    return g_test_run ();
}
