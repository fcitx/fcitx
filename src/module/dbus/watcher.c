#include "config.h"

#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <dbus/dbus.h>
#include "dbusstuff.h"
#include "dbussocket.h"
#include "fcitx-utils/utils.h"
#include "fcitx/fcitx.h"

static char* servicename = NULL;
enum {
    FCITX_DIE = 0x2,
    DBUS_DIE = 0x1,
    WATCHER_WAITING = 0,
};

int status = WATCHER_WAITING;

DBusHandlerResult
WatcherDBusFilter(DBusConnection* connection, DBusMessage* msg, void* user_data)
{
    FCITX_UNUSED(connection);
    FCITX_UNUSED(user_data);
    if (dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
        const char *service, *oldowner, *newowner;
        DBusError error;
        dbus_error_init(&error);
        if (dbus_message_get_args(msg, &error,
                                  DBUS_TYPE_STRING, &service ,
                                  DBUS_TYPE_STRING, &oldowner ,
                                  DBUS_TYPE_STRING, &newowner ,
                                  DBUS_TYPE_INVALID)) {
            /* old die */
            if (strcmp(service, servicename) == 0 && *oldowner)
                status |= FCITX_DIE;
        }
        dbus_error_free(&error);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    else if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected")) {
        status |= DBUS_DIE;
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int main (int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    pid_t pid = atoi(argv[2]);
    if (pid <= 0)
        return 1;

    fcitx_utils_init_as_daemon();

    asprintf(&servicename, "%s-%d", FCITX_DBUS_SERVICE, fcitx_utils_get_display_number());

    DBusError err;
    dbus_error_init(&err);

    DBusConnection* conn = dbus_connection_open(argv[1], NULL);
    if (!conn)
        goto some_error;

    FcitxDBusWatch* watches = NULL;

    if (!dbus_connection_set_watch_functions(conn, DBusAddWatch, DBusRemoveWatch, NULL, &watches, NULL)) {
        goto some_error;
    }

    dbus_connection_set_exit_on_disconnect(conn, TRUE);
    if (!dbus_bus_register(conn, NULL))
        goto some_error;

    dbus_bus_add_match(conn,
            "type='signal',"
            "interface='" DBUS_INTERFACE_DBUS "',"
            "path='" DBUS_PATH_DBUS "',"
            "member='NameOwnerChanged'",
            &err);

    if (dbus_error_is_set(&err))
        goto some_error;

    if (!dbus_connection_add_filter(conn, WatcherDBusFilter, NULL, NULL))
        goto some_error;

#ifndef NZERO
#define NZERO 20
#endif
    nice(NZERO - 1);

    fd_set rfds, wfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    do {
        DBusProcessEventForWatches(watches, &rfds, &wfds, &efds);
        DBusProcessEventForConnection(conn);

        if (status != WATCHER_WAITING)
            break;

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&efds);
        int maxfd = DBusUpdateFDSet(watches, &rfds, &wfds, &efds);
        if (maxfd == 0)
            break;
        select(maxfd + 1, &rfds, &wfds, &efds, NULL);
    } while(1);

    if (status == FCITX_DIE) {
        kill(pid, SIGTERM);
    }

some_error:
    if (conn)
        dbus_connection_unref(conn);

    dbus_error_free(&err);

    fcitx_utils_free(servicename);
    return 1;
}
