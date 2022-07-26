#include <assert.h>
#include <stdlib.h>

#define _FCITX_TEST

#include "dbussocket.h"

int main()
{
    FcitxDBusWatchList watches;
    watches.head = NULL;
    watches.listModified = 0;
    DBusAddWatch((void *)0x1, &watches);
    assert(watches.head != NULL);
    DBusRemoveWatch((void *)0x1, &watches);
    assert(watches.head == NULL);

    DBusRemoveWatch((void *)0x1, &watches);

    assert(watches.head == NULL);
    return 0;
}
