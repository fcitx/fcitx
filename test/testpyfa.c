#include "core/fcitx.h"
#include "im/pinyin/PYFA.h"
#include "tools/configfile.h"
#include <string.h>

extern PYTABLE PYTable[];
FcitxConfig fc;

void Assert(Bool b)
{
    if (!b)
        exit(1);
}

int main()
{
    int i;
    for (i = 0; PYTable[i].strPY[0] != '\0'; i++) {
        size_t len = strlen(PYTable[i].strPY);
        Assert(len <= 6);
        if (PYTable[i].pMH == &fc.bMisstype )
        {
            char strTemp[7];
            strcpy(strTemp, PYTable[i].strPY);
            Assert(len > 2);
            Assert(strTemp[len - 2] == 'g' && strTemp[len - 1] == 'n' );
            Assert(PYTable[i + 1].strPY[0] != '\0');
            Assert(strlen(PYTable[i + 1].strPY) == len);

            strcpy(strTemp, PYTable[i + 1].strPY);
            Assert(strTemp[len - 2] == 'n' && strTemp[len - 1] == 'g' );
            Assert(strncmp(strTemp, PYTable[i].strPY, len - 2) == 0);
        }
    }

    return 0;
}
