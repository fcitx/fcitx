#include "fcitx/fcitx.h"
#include "fcitx-config/fcitx-config.h"
#include "config.h"
#include "chttrans_p.h"

#include <stdlib.h>
#include <dlfcn.h>

#define _OPENCC_DEFAULT_CONFIG_SIMP_TO_TRAD "zhs2zht.ini"
#define _OPENCC_DEFAULT_CONFIG_TRAD_TO_SIMP "zhs2zht.ini"
static void* _opencc_handle = NULL;
static void* (*_opencc_open)(const char* config_file) = NULL;
static char* (*_opencc_convert_utf8)(void* opencc, const char* inbuf, size_t length) = NULL;

static boolean
OpenCCLoadLib()
{
    if (_opencc_handle)
        return true;
    _opencc_handle = dlopen(OPENCC_LIBRARY_FILENAME, RTLD_NOW | RTLD_GLOBAL);
    if (!_opencc_handle)
        goto fail;
#define OPENCC_LOAD_SYMBOL(sym) do {            \
        _##sym = dlsym(_opencc_handle, #sym);\
            if (!_##sym)                         \
                goto fail;                       \
    } while(0)
    OPENCC_LOAD_SYMBOL(opencc_open);
    OPENCC_LOAD_SYMBOL(opencc_convert_utf8);
    return true;
fail:
    if (_opencc_handle) {
        dlclose(_opencc_handle);
        _opencc_handle = NULL;
    }
    return false;
}

boolean
OpenCCInit(FcitxChttrans* transState)
{
    if (transState->ods2t || transState->odt2s)
        return true;
    if (transState->openccLoaded)
        return false;
    transState->openccLoaded = true;

    if (!OpenCCLoadLib())
        return false;
    transState->ods2t = _opencc_open(_OPENCC_DEFAULT_CONFIG_SIMP_TO_TRAD);
    transState->odt2s = _opencc_open(_OPENCC_DEFAULT_CONFIG_TRAD_TO_SIMP);
    if (!transState->ods2t && !transState->odt2s)
        return false;
    return true;
}

char* OpenCCConvert(void* od, const char* str, size_t size)
{
    if (!_opencc_convert_utf8)
        return NULL;
    return _opencc_convert_utf8(od, str, size);
}
