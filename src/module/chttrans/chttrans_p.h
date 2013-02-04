#ifndef CHTTRANS_P_H
#define CHTTRANS_P_H

#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/hotkey.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/instance.h"
#include "fcitx-utils/stringmap.h"

typedef struct _simple2trad_t {
    int wc;
    char str[UTF8_MAX_LENGTH + 1];
    size_t len;
    UT_hash_handle hh;
} simple2trad_t;

typedef enum _ChttransEngine {
    ENGINE_NATIVE,
    ENGINE_OPENCC
} ChttransEngine;


typedef struct _FcitxChttrans {
    FcitxGenericConfig gconfig;
    ChttransEngine engine;
    FcitxHotkey hkToggle[2];
    simple2trad_t* s2t_table;
    simple2trad_t* t2s_table;
    FcitxStringMap* enableIM;
    char* strEnableForIM;
    void* ods2t;
    void* odt2s;
    FcitxInstance* owner;
    boolean openccLoaded;
} FcitxChttrans;

#endif // CHTTRANS_P_H
