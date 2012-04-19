/***************************************************************************
 *   Copyright (C) 2012~2012 by xubin                                      *
 *   nybux.tsui@gmail.com                                                  *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "fcitx/fcitx.h"
#include "fcitx/instance.h"
#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/uthash.h"
#include "fcitx-utils/utarray.h"

typedef struct _CommandItem {
    char input[4];
    char *function_name;
    lua_State *lua;
    UT_hash_handle hh;
} CommandItem;

typedef struct _FunctionItem {
    char *name;
    lua_State *lua;
} FunctionItem;

typedef struct _TriggerItem {
    char *text;
    UT_array *functions; // need linklist
    UT_hash_handle hh;
} TriggerItem;

typedef struct _ConverterItem {
    char *function_name;
    lua_State *lua;
    UT_hash_handle hh;
} ConverterItem;

typedef struct _LuaExtension {
    char *name;
    lua_State *lua;
    UT_hash_handle hh;
} LuaExtension;

typedef struct _LuaModule {
    FcitxInstance *fcitx;
    LuaExtension *extensions;
    CommandItem *commands;
    TriggerItem *input_triggers;
    TriggerItem *candidate_tiggers;
    ConverterItem *converters;
    ConverterItem *current_converter;
    size_t shortest_input_trigger_key_length;
} LuaModule;

typedef void (*TriggerFn)(LuaModule *luamodule, const char *in, const char *out);

static int RegisterInputTrigger(lua_State *lua, const char *input_string, const char *function_name); 
static lua_State * LuaCreateState(LuaModule *module); 
static void LuaPrintError(lua_State *lua); 
static void LuaPError(int err, const char *s);
static void FunctionItemCopy(void *_dst, const void *_src); 
static void FunctionItemDtor(void *_elt); 
void UnloadExtension(LuaModule *module, const char *name); 

const char *kLuaModuleName = "__fcitx_luamodule";
const char *kFcitxLua = "function __ime_call_function(function_name, p1) if type(_G[function_name]) ~= 'function' then return nil end return _G[function_name](p1) end; ime = {}; ime.register_trigger = function(lua_function_name, description, input_trigger_strings, candidate_trigger_strings) __ime_register_trigger(lua_function_name, desc, input_trigger_strings, candidate_trigger_strings); end";
const UT_icd FunctionItem_icd = {sizeof(FunctionItem), NULL, FunctionItemCopy, FunctionItemDtor};

LuaModule * LuaModuleAlloc(FcitxInstance *fcitx) {
    LuaModule *module;
    module = calloc(sizeof(*module), 1);
    if (module) {
        module->fcitx = fcitx;
    }
    return module;
}
void LuaModuleFree(LuaModule *luamodule) {
    free(luamodule);
}
FcitxInstance *GetFcitx(LuaModule *luamodule) {
    if (luamodule) {
        return luamodule->fcitx;
    } else {
        return NULL;
    }
}

static int FcitxLog_Export(lua_State *lua) {
    int c = lua_gettop(lua);
    if (c == 0) {
        return 0;
    }
    const char *msg = lua_tostring(lua, 1);
    if (msg) {
        FcitxLog(DEBUG, "%s", msg);
    }
    return 0;
}

static int ImeRegisterTrigger_Export(lua_State *lua) {
    int c = lua_gettop(lua);
    const char *function_name = NULL;
    const int kFunctionNameArg = 1;
    const int kInputStringsArg = 3;
    if (c >= kFunctionNameArg) {
        function_name = lua_tostring(lua, kFunctionNameArg);
        if (function_name == NULL || function_name[0] == 0) {
            FcitxLog(WARNING, "register trigger arugment function_name empty");
            return 0;
        }
    }
    if (c >= kInputStringsArg) {
        if (!lua_istable(lua, kInputStringsArg)) {
            FcitxLog(WARNING, "register trigger argument #3 is not table");
            return 0;
        }
        size_t i;
        size_t len = lua_objlen(lua, kInputStringsArg);
        for (i = 1; i <= len; ++i) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, kInputStringsArg);
            const char *text = lua_tostring(lua, -1);
            if (text == NULL) {
                FcitxLog(WARNING, "input_trigger_strings[%d] is not string", i);
            } else {
                if (RegisterInputTrigger(lua, text, function_name) == -1) {
                    FcitxLog(WARNING, "RegisterInputTrigger() failed");
                }
            }
            lua_pop(lua, 1);
        }
    }
    return 0;
}

static void FunctionItemCopy(void *_dst, const void *_src) {
    FunctionItem *dst = (FunctionItem *)_dst;
    FunctionItem *src = (FunctionItem *)_src;
    dst->lua = src->lua;
    dst->name = src->name ? strdup(src->name) : NULL;
}
static void FunctionItemDtor(void *_elt) {
    FunctionItem *elt = (FunctionItem *)_elt;
    if (elt->name) {
        free(elt->name);
    }
}

static void FreeTrigger(TriggerItem **triggers, LuaExtension *extension) {
    TriggerItem *trigger;
    for (trigger = *triggers; trigger != NULL; ) {
        int count = utarray_len(trigger->functions);
        int i;
        for (i = 0; i < count; ) {
            FunctionItem *f = (FunctionItem *)utarray_eltptr(trigger->functions, i);
            if (f->lua == extension->lua) {
                utarray_erase(trigger->functions, i, 1);
                --count;
            } else {
                ++i;
            }
        }
        TriggerItem *temp = trigger->hh.next;
        if (utarray_len(trigger->functions) == 0) {
            HASH_DEL(*triggers, trigger);
            utarray_free(trigger->functions);
            free(trigger->text);
            free(trigger);
        }
        trigger = temp;
    }
}

static void FreeCommand(CommandItem **commands, LuaExtension *extension) {
    CommandItem *command;
    for (command = *commands; command != NULL; ) {
        if (command->lua == extension->lua) {
            CommandItem *temp = command->hh.next;
            HASH_DEL(*commands, command);
            free(command->function_name);
            free(command);
            command = temp;
        } else {
            command = command->hh.next;
        }
    }
}

static void FreeConverter(ConverterItem **converters, LuaExtension *extension) {
    ConverterItem *converter;
    for (converter = *converters; converter != NULL; ) {
        if (converter->lua == extension->lua) {
            ConverterItem *temp = converter->hh.next;
            HASH_DEL(*converters, converter);
            free(converter->function_name);
            free(converter);
            converter = temp;
        } else {
            converter = converter->hh.next;
        }
    }
}

static void UpdateShortestInputTriggerKeyLength(LuaModule *module) {
    size_t length = UINT_MAX;
    TriggerItem *trigger;
    for (trigger = module->input_triggers; trigger; trigger = trigger->hh.next) {
        size_t keylen = strlen(trigger->text);
        if (keylen < length) {
            length = keylen;
        }
    }
    if (length != UINT_MAX) {
        module->shortest_input_trigger_key_length = length;
    }
}

LuaExtension * LoadExtension(LuaModule *module, const char *name) {
    LuaExtension *extension;
    HASH_FIND_STR(module->extensions, name, extension);
    if (extension) {
        FcitxLog(DEBUG, "extension:%s reload", name);
        UnloadExtension(module, name);
    }
    extension = calloc(sizeof(*extension), 1);
    if (extension == NULL) {
        FcitxLog(ERROR, "extension memory alloc failed");
        return NULL;
    }
    extension->name = strdup(name);
    if (extension->name == NULL) {
        FcitxLog(ERROR, "extension->name memory alloc failed");
        free(extension);
        return NULL;
    }
    extension->lua = LuaCreateState(module);
    if (extension->lua == NULL) {
        FcitxLog(ERROR, "extension->lua create failed");
        free(extension->name);
        free(extension);
        return NULL;
    }
    HASH_ADD_KEYPTR(hh, module->extensions, name, strlen(name), extension);

    int rv = lua_pcall(extension->lua, 0, 0, 0);
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(extension->lua);
        UnloadExtension(module, name);
        return NULL;
    }

    rv = luaL_loadfile(extension->lua, name);
    if (rv != 0) {
        LuaPError(rv, "luaL_loadfile() failed");
        LuaPrintError(extension->lua);
        UnloadExtension(module, name);
        return NULL;
    }
    rv = lua_pcall(extension->lua, 0, 0, 0);
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(extension->lua);
        UnloadExtension(module, name);
        return NULL;
    }
    UpdateShortestInputTriggerKeyLength(module);
    return extension;
}

void UnloadExtension(LuaModule *module, const char *name) {
    LuaExtension *extension;
    HASH_FIND_STR(module->extensions, name, extension);
    if (extension == NULL) {
        FcitxLog(WARNING, "extension:%s unload failed, not found", name);
        return;
    }
   
    FreeCommand(&module->commands, extension);
    FreeTrigger(&module->input_triggers, extension);
    FreeTrigger(&module->candidate_tiggers, extension);
    if (module->current_converter && module->current_converter->lua == extension->lua) {
        module->current_converter = NULL;
    }
    FreeConverter(&module->converters, extension);

    free(extension->name);
    lua_close(extension->lua);
    HASH_DEL(module->extensions, extension);
    free(extension);

    UpdateShortestInputTriggerKeyLength(module);
}

static LuaModule * GetModule(lua_State *lua) {
    lua_getglobal(lua, kLuaModuleName);
    LuaModule **module = lua_touserdata(lua, -1);
    lua_pop(lua, 1);
    return *module;
}

static LuaExtension * FindExtension(lua_State *lua) {
    LuaModule *module = GetModule(lua);
    if (!module) {
        FcitxLog(ERROR, "LuaModule not found");
        return NULL;
    }
    LuaExtension *e;
    for (e = module->extensions; e != NULL; e = e->hh.next) {
        if (e->lua == lua) {
            return e;
        }
    }
    return NULL;
}

static int RegisterInputTrigger(lua_State *lua,
                                const char *input,
                                const char *function_name) {
    if (lua == NULL || input == NULL || function_name == NULL) {
        FcitxLog(WARNING, "RegisterInputTrigger() argument error");
        return -1;
    }
    LuaExtension *extension = FindExtension(lua);
    if (extension == NULL) {
        FcitxLog(ERROR, "find extension failed");
        return -1;
    }
    LuaModule *module = GetModule(lua);
    if (!module) {
        FcitxLog(ERROR, "LuaModule not found");
        return -1;
    }
    TriggerItem *trigger = NULL;
    HASH_FIND_STR(module->input_triggers, input, trigger);
    if (trigger == NULL) {
        trigger = calloc(sizeof(*trigger), 1);
        if (trigger == NULL) {
            FcitxLog(ERROR, "trigger memory alloc failed");
            goto cleanup;
        }
        trigger->text = strdup(input);
        if (trigger->text == NULL) {
            FcitxLog(ERROR, "trigger->text memory alloc failed");
            goto cleanup;
        }
        utarray_new(trigger->functions, &FunctionItem_icd);
        if (trigger->functions == NULL) {
            FcitxLog(ERROR, "trigger->functions memory alloc failed");
            goto cleanup;
        }
        HASH_ADD_KEYPTR(hh, module->input_triggers, input, strlen(input), trigger);
    }
    FunctionItem function;
    function.lua = lua;
    function.name = strdup(function_name);
    if (function.name == NULL) {
        FcitxLog(ERROR, "function.name memory alloc failed");
        goto cleanup;
    }
    utarray_push_back(trigger->functions, &function);
    free(function.name);
    return 0;
cleanup:
    if (trigger) {
        if (trigger->functions) {
            utarray_free(trigger->functions);
        }
        if (trigger->text) {
            free(trigger->text);
        }
        free(trigger);
    }
    return -1;
}

static void LuaPrintError(lua_State *lua) {
    if (lua_gettop(lua) > 0) {
        FcitxLog(ERROR, "    %s", lua_tostring(lua, -1));
    }
}

static void LuaPError(int err, const char *s) {
    switch (err) {
        case LUA_ERRSYNTAX: 
            FcitxLog(ERROR, "%s:syntax error during pre-compilation", s);
            break;
        case LUA_ERRMEM:
            FcitxLog(ERROR, "%s:memory allocation error", s);
            break;
        case LUA_ERRFILE:
            FcitxLog(ERROR, "%s:cannot open/read the file", s);
            break;
        case LUA_ERRRUN:    
            FcitxLog(ERROR, "%s:a runtime error", s);
            break;
        case LUA_ERRERR:
            FcitxLog(ERROR, "%s:error while running the error handler function", s);
            break;
        case 0:
            FcitxLog(ERROR, "%s:ok", s);
            break;
        default:
            FcitxLog(ERROR, "%s:unknown error,%d", s, err);
            break;
    }
}

void LuaPrintStackInfo(lua_State *lua) {
    int count = lua_gettop(lua);
    FcitxLog(DEBUG, "lua stack count:%d", count);
    int i;
    for (i = count; i > 0 ; --i) {
        FcitxLog(DEBUG, "  %-3d(%02d):%s", i, lua_type(lua, i), lua_tostring(lua, i));
    }
}

static lua_State * LuaCreateState(LuaModule *module) {
    lua_State *lua = NULL;

    lua = luaL_newstate();
    if (lua == NULL) {
        FcitxLog(ERROR, "luaL_newstate() failed");
        goto cleanup;
    }

    luaL_openlibs(lua);
    lua_register(lua, "fcitx_log", FcitxLog_Export);
    lua_register(lua, "__ime_register_trigger", ImeRegisterTrigger_Export);
    LuaModule **ppmodule = lua_newuserdata(lua, sizeof(LuaModule *));
    *ppmodule = module;
    lua_setglobal(lua, kLuaModuleName);

    int rv = luaL_loadstring(lua, kFcitxLua);
    /*int rv = luaL_loadfile(lua, "fcitx.lua");*/
    if (rv != 0) {
        LuaPError(rv, "luaL_loadfile() failed");
        LuaPrintError(lua);
        goto cleanup;
    }
    return lua;
cleanup:
    if (lua) {
        lua_close(lua);
    }
    return NULL;
}

static int LuaCallFunction(lua_State *lua,
                           const char *function_name,
                           const char *argument) {
    lua_getfield(lua, LUA_GLOBALSINDEX, "__ime_call_function");
    lua_pushstring(lua, function_name);
    lua_pushstring(lua, argument);
    int rv = lua_pcall(lua, 2, 1, 0);
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(lua);
        return -1;
    }
    if (lua_gettop(lua) == 0) {
        FcitxLog(WARNING, "lua_gettop() not retrun");
        return -1;
    } 
    return 0;
}

int InputTrigger(LuaModule *module, const char *input, TriggerFn callback) {
    if (strlen(input) < module->shortest_input_trigger_key_length) {
        return -1;
    }
    TriggerItem *trigger;
    HASH_FIND_STR(module->input_triggers, input, trigger);
    if (trigger == NULL) {
        return -1;
    }

    FunctionItem *f = NULL;
    while ((f = (FunctionItem *)utarray_next(trigger->functions, f))) {
        if (LuaCallFunction(f->lua, f->name, input) == 0) {
            int type = lua_type(f->lua, -1);
            if (type == LUA_TSTRING) {
                const char *str = lua_tostring(f->lua, -1);
                if (str) {
                    callback(module, input, str);
                } else {
                    FcitxLog(WARNING, "lua function return return null"); 
                }
            } else {
                FcitxLog(WARNING, "lua function return type not expected:%s",
                                  lua_typename(f->lua, type)); 
            }
        }
        lua_pop(f->lua, lua_gettop(f->lua));
    }
    return 0;
}

