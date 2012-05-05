#include <dbus/dbus.h>

static void
NewConnectionCallback (DBusServer     *server,
                       DBusConnection *new_connection,
                       void           *data)
{
  TestServiceData *testdata = data;

  if (!test_connection_setup (testdata->loop, new_connection))
    dbus_connection_close (new_connection);
}


dbus_bool_t
test_connection_setup (DBusLoop       *loop,
                       DBusConnection *connection)
{
  CData *cd;

  cd = NULL;

  dbus_connection_set_dispatch_status_function (connection, dispatch_status_function,
                                                loop, NULL);

  cd = cdata_new (loop, connection);
  if (cd == NULL)
    goto nomem;

  /* Because dbus-mainloop.c checks dbus_timeout_get_enabled(),
   * dbus_watch_get_enabled() directly, we don't have to provide
   * "toggled" callbacks.
   */

  if (!dbus_connection_set_watch_functions (connection,
                                            add_watch,
                                            remove_watch,
                                            NULL,
                                            cd, cdata_free))
    goto nomem;


  cd = cdata_new (loop, connection);
  if (cd == NULL)
    goto nomem;

  if (!dbus_connection_set_timeout_functions (connection,
                                              add_timeout,
                                              remove_timeout,
                                              NULL,
                                              cd, cdata_free))
    goto nomem;

  if (dbus_connection_get_dispatch_status (connection) != DBUS_DISPATCH_COMPLETE)
    {
      if (!_dbus_loop_queue_dispatch (loop, connection))
        goto nomem;
    }

  return TRUE;

 nomem:
  if (cd)
    cdata_free (cd);

  dbus_connection_set_dispatch_status_function (connection, NULL, NULL, NULL);
  dbus_connection_set_watch_functions (connection, NULL, NULL, NULL, NULL, NULL);
  dbus_connection_set_timeout_functions (connection, NULL, NULL, NULL, NULL, NULL);

  return FALSE;
}


static void
dispatch_status_function (DBusConnection    *connection,
                          DBusDispatchStatus new_status,
                          void              *data)
{
  DBusLoop *loop = data;

  if (new_status != DBUS_DISPATCH_COMPLETE)
    {
      while (!_dbus_loop_queue_dispatch (loop, connection))
        _dbus_wait_for_memory ();
    }
}