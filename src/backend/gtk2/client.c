#include <stdlib.h>
#include "fcitx/fcitx.h"
#include "client.h"
#include <fcitx-utils/utils.h>

struct FcitxIMClient {
};

FCITX_EXPORT_API
FcitxIMClient* FcitxIMClientOpen()
{
    FcitxIMClient* client = fcitx_malloc0(sizeof(FcitxIMClient));
    return client;
}

FCITX_EXPORT_API
void FcitxIMClientClose(FcitxIMClient* client)
{

}

FCITX_EXPORT_API
void FcitxIMClientFoucsIn(FcitxIMClient* client)
{

}

FCITX_EXPORT_API
void FcitxIMClientFoucsOut(FcitxIMClient* client)
{

}

FCITX_EXPORT_API
void FcitxIMClientReset(FcitxIMClient* client)
{

}

FCITX_EXPORT_API
void FcitxIMClientSetCursorLocation(FcitxIMClient* client, int x, int y)
{

}
