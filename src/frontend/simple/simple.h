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

#ifdef __cplusplus
}
#endif

#endif // _FCITX_MODULE_SIMPLE_H
