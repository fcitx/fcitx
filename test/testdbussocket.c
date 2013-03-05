#include <assert.h>
#include <stdlib.h>

#define _FCITX_TEST

#include "dbussocket.h"

int main()
{
    FcitxDBusWatch* watches = NULL;
    DBusAddWatch((void *)0x1, &watches);
    DBusRemoveWatch((void *)0x1, &watches);

    assert(watches == NULL);
    DBusRemoveWatch((void *)0x1, &watches);

    assert(watches == NULL);
    return 0;
}
