#include <assert.h>
#include <string.h>
#include "fcitx-config/xdg.h"

int main()
{
    char* ret = NULL;
    FcitxXDGGetFileUserWithPrefix("test", "/test", NULL, &ret);
    assert(ret);
    assert(strcmp(ret, "/test") == 0);
    free(ret);
    FcitxXDGGetFileWithPrefix("test", "/test", NULL, &ret);
    assert(ret);
    assert(strcmp(ret, "/test") == 0);
    free(ret);
    FcitxXDGGetLibFile("/test", NULL, &ret);
    assert(ret);
    assert(strcmp(ret, "/test") == 0);
    free(ret);

    return 0;
}
