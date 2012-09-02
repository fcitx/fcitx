#include "config.h"

#include <limits.h>
#include <unistd.h>
#include <sys/signal.h>
#include <stdio.h>
#include <string.h>
#include <dbus/dbus.h>
#include "dbusstuff.h"
#include "dbussocket.h"
#include "fcitx-utils/utils.h"
#include "fcitx/frontend.h"
#include "frontend/ipc/ipc.h"

char*
_fcitx_get_socket_path()
{
    char* addressFile = NULL;
    char* machineId = dbus_get_local_machine_id();
    asprintf(&addressFile, "%s-%d", machineId, fcitx_utils_get_display_number());
    dbus_free(machineId);

    char* file = NULL;

    FcitxXDGGetFileUserWithPrefix("dbus", addressFile, NULL, &file);

    return file;

}

static char*
_fcitx_get_address ()
{
    char* address = NULL;
    char* env = getenv("FCITX_DBUS_ADDRESS");
    address = env ? strdup (env) : NULL;
    if (address)
        return address;

    char* path = _fcitx_get_socket_path();
    FILE* fp = fopen(path, "r");
    free(path);

    if (!fp)
        return NULL;

    const int BUFSIZE = 1024;

    char buffer[BUFSIZE];
    size_t sz = fread(buffer, sizeof(char), BUFSIZE, fp);
    fclose(fp);
    char *p = memchr(buffer, '\0', sz);
    if (!(p && sz == p - buffer + 2 * sizeof(pid_t) + 1))
        return NULL;

    /* skip '\0' */
    p++;
    pid_t *ppid = (pid_t*) p;
    pid_t daemonpid = ppid[0];
    pid_t fcitxpid = ppid[1];

    if (!fcitx_utils_pid_exists(daemonpid)
        || !fcitx_utils_pid_exists(fcitxpid))
        return NULL;

    address = strdup(buffer);

    return address;
}

void usage()
{
    printf("Usage: fcitx-remote [OPTION]\n"
           "\t-c\t\tinactivate input method\n"
           "\t-o\t\tactivate input method\n"
           "\t-r\t\treload fcitx config\n"
           "\t-t,-T\t\tswitch Active/Inactive\n"
           "\t-e\t\tAsk fcitx to exit\n"
           "\t[no option]\tdisplay fcitx state, %d for close, %d for inactive, %d for acitve\n"
           "\t-h\t\tdisplay this help and exit\n",
           IS_CLOSED, IS_INACTIVE, IS_ACTIVE);
}

enum {
    FCITX_DBUS_ACTIVATE,
    FCITX_DBUS_INACTIVATE,
    FCITX_DBUS_RELOAD_CONFIG,
    FCITX_DBUS_EXIT,
    FCITX_DBUS_TOGGLE,
    FCITX_DBUS_GET_CURRENT_STATE
};

int main (int argc, char* argv[])
{
    char* servicename = NULL;
    char *address = NULL;
    DBusMessage* message = NULL;
    DBusConnection* conn = NULL;
    int c;
    int ret = 1;
    int messageType = FCITX_DBUS_GET_CURRENT_STATE;
    while ((c = getopt(argc, argv, "chortTe")) != -1) {
        switch (c) {
        case 'o':
            messageType = FCITX_DBUS_ACTIVATE;
            break;

        case 'c':
            messageType = FCITX_DBUS_INACTIVATE;
            break;

        case 'r':
            messageType = FCITX_DBUS_RELOAD_CONFIG;
            break;

        case 't':
        case 'T':
            messageType = FCITX_DBUS_TOGGLE;
            break;

        case 'e':
            messageType = FCITX_DBUS_EXIT;
            break;

        case 'h':
        default:
            usage();
            return 0;
            break;
        }
    }

#define CASE(ENUMNAME, MESSAGENAME) \
        case FCITX_DBUS_##ENUMNAME: \
            message = dbus_message_new_method_call(servicename, FCITX_IM_DBUS_PATH, FCITX_IM_DBUS_INTERFACE, #MESSAGENAME); \
            break;

    asprintf(&servicename, "%s-%d", FCITX_DBUS_SERVICE, fcitx_utils_get_display_number());
    switch(messageType) {
        CASE(ACTIVATE, ActivateIM);
        CASE(INACTIVATE, InactivateIM);
        CASE(RELOAD_CONFIG, ReloadConfig);
        CASE(EXIT, Exit);
        CASE(TOGGLE, ToggleIM);
        CASE(GET_CURRENT_STATE, GetCurrentState);

        default:
            goto some_error;
    };
    if (!message) {
        goto some_error;
    }
    address = _fcitx_get_address();
    do {
        if (!address)
            break;
        conn = dbus_connection_open(address, NULL);
        if (!conn)
            break;
        if (!dbus_bus_register(conn, NULL)) {
            dbus_connection_unref(conn);
            conn = NULL;
            break;
        }
    } while(0);

    if (!conn) {
        conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);

        if (!conn) {
            goto some_error;
        }

        dbus_connection_set_exit_on_disconnect(conn, FALSE);
    }

    if (messageType == FCITX_DBUS_GET_CURRENT_STATE) {
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, message, 1000, NULL);
        int result = 0;
        if (reply && dbus_message_get_args(reply, NULL, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID)) {
            printf("%d\n", result);
            ret = 0;
        } else {
            fprintf(stderr, "Not get reply\n");
        }
    } else {
        dbus_connection_send(conn, message, NULL);
        dbus_connection_flush(conn);
        ret = 0;
    }

some_error:
    if (message)
        dbus_message_unref(message);
    if (conn)
        dbus_connection_unref(conn);
    fcitx_utils_free(address);
    fcitx_utils_free(servicename);
    return ret;
}
