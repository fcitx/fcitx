#ifndef FCITX_CLIENT_H
#define FCITX_CLIENT_H

#include "fcitx-config/fcitx-config.h"
#include "fcitx/ime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FcitxIMClient FcitxIMClient;
typedef void (*FcitxIMClientDestroyCallback)(FcitxIMClient* client, void* data);
typedef void (*FcitxIMClientConnectCallback)(FcitxIMClient* client, void* data);


FcitxIMClient* FcitxIMClientOpen(FcitxIMClientConnectCallback connectcb, FcitxIMClientDestroyCallback destroycb, GObject* data);
boolean IsFcitxIMClientValid(FcitxIMClient* client);
void FcitxIMClientClose(FcitxIMClient* client);
void FcitxIMClientFocusIn(FcitxIMClient* client);
void FcitxIMClientFocusOut(FcitxIMClient* client);
void FcitxIMClientSetCursorLocation(FcitxIMClient* client, int x, int y);
void FcitxIMClientReset(FcitxIMClient* client);
int FcitxIMClientProcessKey(FcitxIMClient* client, uint32_t keyval, uint32_t keycode, uint32_t state, FcitxKeyEventType type, uint32_t time);
void FcitxIMClientConnectSignal(FcitxIMClient* imclient,
    GCallback enableIM,
    GCallback closeIM,
    GCallback commitString,
    GCallback forwardKey,
    void* user_data,
    GClosureNotify freefunc
);

#ifdef __cplusplus
}
#endif

#endif