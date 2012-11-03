#ifndef _FCITX_MODULE_SIMPLE_H
#define _FCITX_MODULE_SIMPLE_H

#include <stdint.h>
#include <fcitx/instance.h>
#include <fcitx/module.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _FcitxSimpleEventType {
    SE_KeyEventPress,
    SE_KeyEventRelease,
} FcitxSimpleEventType;

typedef struct _FcitxSimplePackage {
    uint32_t type;
    union {
        struct {
            uint32_t key;
            uint32_t state;
            uint32_t keycode;
        };
    };
} FcitxSimplePackage;

#define FCITX_SIMPLE_MODULE_NAME "fcitx-simple-module"
#define FCITX_SIMPLE_MODULE_GETFD 0
#define FCITX_SIMPLE_MODULE_GETFD_RETURNTYPE int
#define FCITX_SIMPLE_MODULE_GETSEMAPHORE 1
#define FCITX_SIMPLE_MODULE_GETSEMAPHORE_RETURNTYPE sem_t*

#define FCITX_SIMPLE_FRONTEND_NAME "fcitx-simple-frontend"
#define FCITX_SIMPLE_FRONTEND_PROCESS_KEY 0
#define FCITX_SIMPLE_FRONTEND_PROCESS_KEY_RETURNTYPE void


static inline FcitxAddon*
FcitxSimpleGetAddon(FcitxInstance *instance)
{
    static FcitxAddon *addon = NULL;
    if (!addon) {
        addon = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance),
                                          FCITX_SIMPLE_MODULE_NAME);
    }
    return addon;
}

static inline int FcitxSimpleGetFD(FcitxInstance* instance) {
    static int fd = -1;
    if (fd < 0) {
        int* pfd = (int*) FcitxModuleInvokeVaArgs(
            FcitxSimpleGetAddon(instance), FCITX_SIMPLE_MODULE_GETFD);
        if (pfd)
            fd = *pfd;
        else
            fd = 0;
    }
    return fd;
}

static inline sem_t* FcitxSimpleGetSemaphore(FcitxInstance* instance) {
    static sem_t* sem = NULL;
    if (!sem) {
        sem = (sem_t*) FcitxModuleInvokeVaArgs(
                FcitxSimpleGetAddon(instance), FCITX_SIMPLE_MODULE_GETSEMAPHORE);
    }
    return sem;
}

static inline void FcitxSimpleSendKeyEvent(FcitxInstance* instance, boolean release, FcitxKeySym key, unsigned int state, unsigned int keycode) {
    int fd = FcitxSimpleGetFD(instance);
    if (fd) {
        FcitxSimplePackage package;
        package.type = release ? SE_KeyEventRelease : SE_KeyEventPress;
        package.key = key;
        package.state = state;
        package.keycode = keycode;
        write(fd, &package, sizeof(package));

        sem_t* sem = FcitxSimpleGetSemaphore(instance);
        sem_wait(sem);
    }
}

#ifdef __cplusplus
}
#endif

#endif // _FCITX_MODULE_SIMPLE_H