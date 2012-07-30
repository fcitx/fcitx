#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include "dbuslauncher.h"

int main()
{
    DBusDaemonProperty daemonProp = DBusLaunch(NULL);
    if (daemonProp.pid != 0)
    {
        assert(daemonProp.address != NULL);
        printf("dbus-daemon address %s\n", daemonProp.address);
        int result = DBusKill(&daemonProp);
        printf("kill dbus-daemon result %d\n", result);
    }
    else {
        assert(daemonProp.address == NULL);
        printf("dbus-daemon launch failed\n");
    }

    return 0;
}
