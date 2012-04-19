/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"
#include "fcitx/candidate.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx/hook.h"
#include "fcitx-utils/log.h"

#include "luawrap.h"

static void* LuaCreate(FcitxInstance* instance);

FCITX_EXPORT_API
FcitxModule module = {
    LuaCreate,
    NULL,
    NULL,
    NULL,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static int LoadLuaConfig(LuaModule *luamodule) {
    //TODO(xubin): read config
    LoadExtension(luamodule, "/home/xubin/a.lua"); 
    return 0;
}

INPUT_RETURN_VALUE LuaGetCandWord(void* arg, FcitxCandidateWord* candWord) {
    return IRV_COMMIT_STRING;
}

void TriggerCallback(LuaModule *luamodule, const char *in, const char *out) {
    FcitxCandidateWord candWord;
    candWord.callback = LuaGetCandWord;
    candWord.owner = luamodule;
    candWord.priv = NULL;
    candWord.wordType = MSG_TIPS;
    candWord.strExtra = NULL;
    candWord.strWord = strdup(out);
    
    FcitxInputState* input = FcitxInstanceGetInputState(GetFcitx(luamodule));
    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    FcitxCandidateWordInsert(candList, &candWord, 0);
}

void LuaUpdateCandidateWordHookCallback(void *arg) {
    LuaModule *luamodule = (LuaModule *)arg;
    FcitxInputState* input = FcitxInstanceGetInputState(GetFcitx(luamodule));
    char *text = FcitxUIMessagesToCString(FcitxInputStateGetPreedit(input));
    InputTrigger(luamodule, text, TriggerCallback);
    free(text);
}

void* LuaCreate(FcitxInstance* instance) {
    LuaModule *luamodule = LuaModuleAlloc(instance);
    if (luamodule == NULL) {
        FcitxLog(ERROR, "LuaModule alloc failed");
        goto err;
    }
    int rv = LoadLuaConfig(luamodule);
    if (rv == -1) {
        FcitxLog(ERROR, "LoadLuaConfig() failed");
        goto err;
    }

    FcitxIMEventHook hook = {.arg = luamodule,
                             .func = LuaUpdateCandidateWordHookCallback};
    
    FcitxInstanceRegisterUpdateCandidateWordHook(instance, hook);

    return luamodule;
err:
    if (luamodule) {
        LuaModuleFree(luamodule);
    }
    return NULL;
}


