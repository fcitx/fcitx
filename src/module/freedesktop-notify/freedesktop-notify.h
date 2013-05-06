#ifndef _FCITX_MODULE_FREEDESKTOP_NOTIFY_H
#define _FCITX_MODULE_FREEDESKTOP_NOTIFY_H

#include <stdint.h>

typedef void (*FcitxFreedesktopNotifyActionCallback)(void *arg,
                                                     uint32_t id,
                                                     const char *action_id);
typedef struct {
    const char *id;
    const char *name;
} FcitxFreedesktopNotifyAction;

#endif // _FCITX_MODULE_FREEDESKTOP_NOTIFY_H
