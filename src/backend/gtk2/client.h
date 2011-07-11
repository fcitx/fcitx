#ifndef FCITX_CLIENT_H
#define FCITX_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FcitxIMClient FcitxIMClient;

FcitxIMClient* FcitxIMClientOpen();
void FcitxIMClientClose(FcitxIMClient* client);
void FcitxIMClientFoucsIn(FcitxIMClient* client);
void FcitxIMClientFoucsOut(FcitxIMClient* client);
void FcitxIMClientSetCursorLocation(FcitxIMClient* client, int x, int y);
void FcitxIMClientReset(FcitxIMClient* client);

#ifdef __cplusplus
}
#endif

#endif