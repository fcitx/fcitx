#include "fcitx-utils/utils.h"
#include "dbussocket.h"

dbus_bool_t DBusAddWatch(DBusWatch *watch, void *data)
{
    FcitxDBusWatch *w;
    FcitxDBusWatchList *watches = (FcitxDBusWatchList*) data;

    for (w = watches->head; w; w = w->next) {
        if (w->watch == watch) {
            return TRUE;
        }
    }

    if (!(w = fcitx_utils_new(FcitxDBusWatch)))
        return FALSE;

    w->watch = watch;
    w->next = watches->head;
    watches->head = w;
    watches->listModified = 1;
    return TRUE;
}

void DBusRemoveWatch(DBusWatch *watch, void *data)
{
    FcitxDBusWatch *w;
    FcitxDBusWatch *next, *prev;
    FcitxDBusWatchList *watches = (FcitxDBusWatchList*)data;

    prev = NULL;
    for (w = watches->head; w; w = next) {
        next = w->next;
        if (w->watch == watch) {
            free(w);
            if (prev) {
                prev->next = next;
            } else {
                watches->head = next;
            }
            watches->listModified = 1;
        } else {
            prev = w;
        }
    }
}

int DBusUpdateFDSet(FcitxDBusWatchList* watches, fd_set* rfds, fd_set* wfds, fd_set* efds)
{
    int maxfd = 0;
    FcitxDBusWatch* w;
    for (w = watches->head; w; w = w->next)
        if (dbus_watch_get_enabled(w->watch)) {
            unsigned int flags = dbus_watch_get_flags(w->watch);
            int fd = dbus_watch_get_unix_fd(w->watch);

            if (maxfd < fd)
                maxfd = fd;

            if (flags & DBUS_WATCH_READABLE)
                FD_SET(fd, rfds);

            if (flags & DBUS_WATCH_WRITABLE)
                FD_SET(fd, wfds);

            FD_SET(fd, efds);
        }

    return maxfd;
}

void DBusProcessEventForWatches(FcitxDBusWatchList* watches, fd_set* rfds, fd_set* wfds, fd_set* efds)
{
    FcitxDBusWatch *w;

    do {
        watches->listModified = 0;
        FcitxDBusWatch *next = NULL;
        for (w = watches->head; !watches->listModified && w; w = next) {
            next = w->next;
            if (dbus_watch_get_enabled(w->watch)) {
                unsigned int flags = 0;
                int fd = dbus_watch_get_unix_fd(w->watch);

                if (FD_ISSET(fd, rfds))
                    flags |= DBUS_WATCH_READABLE;

                if (FD_ISSET(fd, wfds))
                    flags |= DBUS_WATCH_WRITABLE;

                if (FD_ISSET(fd, efds))
                    flags |= DBUS_WATCH_ERROR;

                if (flags != 0)
                    dbus_watch_handle(w->watch, flags);
            }
        }
    } while (watches->listModified);
}

void DBusProcessEventForConnection(DBusConnection* connection)
{
    if (connection) {
        dbus_connection_ref(connection);
        while (dbus_connection_dispatch(connection) == DBUS_DISPATCH_DATA_REMAINS);
        dbus_connection_unref(connection);
    }
}
