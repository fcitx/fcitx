#include <dbus/dbus.h>
#include <sys/select.h>

typedef struct _FcitxDBusWatch {
    DBusWatch *watch;
    struct _FcitxDBusWatch *next;
} FcitxDBusWatch;

dbus_bool_t DBusAddWatch(DBusWatch *watch, void *data);
void DBusRemoveWatch(DBusWatch *watch, void *data);
int DBusUpdateFDSet(FcitxDBusWatch* watches, fd_set* rfds, fd_set* wfds, fd_set* efds);
void DBusProcessEventForWatches(FcitxDBusWatch* watches, fd_set* rfds, fd_set* wfds, fd_set* efds);
void DBusProcessEventForConnection(DBusConnection* connection);
