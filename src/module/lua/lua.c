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
#include "fcitx-config/xdg.h"

#include "luamod.h"
#include "luawrap.h"

static void *LuaCreate(FcitxInstance *instance);
static void LuaReloadConfig(void *arg);
DECLARE_ADDFUNCTIONS(Lua)

FCITX_DEFINE_PLUGIN(fcitx_lua, module, FcitxModule) = {
    LuaCreate,
    NULL,
    NULL,
    NULL,
    LuaReloadConfig
};

static int
LoadLuaConfig(LuaModule *luamodule)
{
    int count = 0;
    FcitxStringHashSet *sset = FcitxXDGGetFiles("lua", NULL, ".lua");
    FcitxStringHashSet *str;
    for (str = sset; str != NULL;) {
        FcitxStringHashSet *tmp = str->hh.next;
        char *path;
        FILE *f = FcitxXDGGetFileWithPrefix("lua", str->name, "r", &path);
        if (f && path) {
            if (LoadExtension(luamodule, path)) {
                FcitxLog(INFO, "lua load extension file:%s", path);
                ++count;
            } else {
                FcitxLog(ERROR, "LoadExtension() failed");
            }
        }
        if (f) {
            fclose(f);
        }
        if (path) {
            free(path);
        }
        HASH_DEL(sset, str);
        free(str->name);
        free(str);
        str = tmp;
    }
    return count;
}

static INPUT_RETURN_VALUE
LuaGetCandWord(void *arg, FcitxCandidateWord *candWord)
{
    FCITX_UNUSED(arg);
    LuaModule *luamodule = (LuaModule*)candWord->owner;
    FcitxInputState *input = FcitxInstanceGetInputState(GetFcitx(luamodule));
    strncpy(FcitxInputStateGetOutputString(input),
            candWord->strWord, MAX_USER_INPUT);
    return IRV_COMMIT_STRING;
}

static void
LuaCallCommand(LuaModule *luamodule, const char *input,
               FcitxCandidateWordCommitCallback callback, void *owner)
{
    UT_array *result = InputCommand(luamodule, input);
    if (result) {
        FcitxInputState *input = FcitxInstanceGetInputState(GetFcitx(luamodule));
        LuaResultItem *p = NULL;
        while ((p = (LuaResultItem *)utarray_next(result, p))) {
            FcitxCandidateWord candWord;
            if (callback && owner) {
                candWord.callback = callback;
                candWord.owner = owner;
            } else {
                candWord.callback = LuaGetCandWord;
                candWord.owner = luamodule;
            }
            candWord.priv = p->help ? strdup(p->help) : NULL;
            if (p->help || p->tip) {
                fcitx_utils_alloc_cat_str(candWord.strExtra, p->help,
                                          p->help && p->tip ? " " : "",
                                          p->tip);
            } else {
                candWord.strExtra = NULL;
            }
            candWord.strWord = strdup(p->result);
            candWord.wordType = MSG_TIPS;
            candWord.extraType = MSG_CODE;
            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input),
                                     &candWord);
        }
        utarray_free(result);
    }
}

void
AddToCandList(LuaModule *luamodule, const char *in, const char *out)
{
    FCITX_UNUSED(in);
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

void
LuaUpdateCandidateWordHookCallback(void *arg)
{
    LuaModule *luamodule = (LuaModule *)arg;
    FcitxInputState* input = FcitxInstanceGetInputState(GetFcitx(luamodule));
    char *text = FcitxInputStateGetRawInputBuffer(input);
    UT_array *result = InputTrigger(luamodule, text);
    if (result) {
        LuaResultItem *p = NULL;
        while ((p = (LuaResultItem*)utarray_next(result, p))) {
            AddToCandList(luamodule, text, p->result);
        }
        utarray_free(result);
    }
}

void*
LuaCreate(FcitxInstance* instance)
{
    LuaModule *luamodule = LuaModuleAlloc(instance);
    if (luamodule == NULL) {
        FcitxLog(ERROR, "LuaModule alloc failed");
        goto err;
    }
    LoadLuaConfig(luamodule);

    FcitxIMEventHook hook = {.arg = luamodule,
                             .func = LuaUpdateCandidateWordHookCallback};

    FcitxInstanceRegisterUpdateCandidateWordHook(instance, hook);

    FcitxLuaAddFunctions(instance);
    return luamodule;
err:
    if (luamodule) {
        LuaModuleFree(luamodule);
    }
    return NULL;
}

void LuaReloadConfig(void* arg)
{
    LuaModule* luamodule = arg;
    UnloadAllExtension(luamodule);
    LoadLuaConfig(luamodule);
}

#include "fcitx-lua-addfunctions.h"
