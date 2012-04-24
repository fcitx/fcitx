#include <assert.h>
#include <string.h>
#include "fcitx-utils/utf8.h"

#define BUF_SIZE 9

int main()
{
    char buf[BUF_SIZE];
    const char string[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xe6\xb5\x8b\xe8\xaf\x95\xe5\xb8\x8c\xe6";
    const char result[] = {'\xe4', '\xbd', '\xa0', '\xe5', '\xa5', '\xbd', '\0', '\0', '\0'};
    fcitx_utf8_strncpy(buf, string, BUF_SIZE - 1);
    buf[BUF_SIZE - 1] = 0;
    assert(memcmp(buf, result, BUF_SIZE) == 0);

    return 0;
}