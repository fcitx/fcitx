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
#include "fcitx/ime.h"
#include "fcitx/module.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/uthash.h"
#include "fcitx-utils/utarray.h"
#include "luawrap.h"

typedef struct _CommandItem {
    char *function_name;
    lua_State *lua;
    UT_hash_handle hh;
    char* command_name;
} CommandItem;

typedef struct _FunctionItem {
    char *name;
    lua_State *lua;
} FunctionItem;

typedef struct _TriggerItem {
    char *key;
    UT_array *functions; // need linklist
    UT_hash_handle hh;
} TriggerItem;

typedef struct _ConverterItem {
    char *function_name;
    lua_State *lua;
    UT_hash_handle hh;
} ConverterItem;

struct _LuaExtension {
    char *name;
    lua_State *lua;
    UT_hash_handle hh;
};

struct _LuaModule {
    FcitxInstance *fcitx;
    LuaExtension *extensions;
    CommandItem *commands;
    TriggerItem *input_triggers;
    TriggerItem *candidate_tiggers;
    ConverterItem *converters;
    ConverterItem *current_converter;
    size_t shortest_input_trigger_key_length;
};

typedef void (*LuaResultFn)(LuaModule *luamodule, const char *in, const char *out);

static int RegisterInputTrigger(lua_State *lua, const char *input_string, const char *function_name);
static int RegisterCommand(lua_State *lua, const char *command_name, const char *function_name);
static lua_State * LuaCreateState(LuaModule *module);
static void LuaPrintError(lua_State *lua);
static void LuaPError(int err, const char *s);
static void FunctionItemCopy(void *_dst, const void *_src);
static void FunctionItemDtor(void *_elt);
static void LuaResultItemCopy(void *_dst, const void *_src);
static void LuaResultItemDtor(void *_elt);
static LuaModule * GetModule(lua_State *lua);
static void UnloadExtension(LuaModule *module, LuaExtension* extension);

const char *kLuaModuleName = "__fcitx_luamodule";
const char *kFcitxLua =
    "function __ime_call_function(function_name, p1)"
    "    if type(_G[function_name]) ~= 'function' then"
    "        return nil"
    "    end"
    "    return _G[function_name](p1)"
    "end;"
    "ime = {};"
    "ime.register_trigger = function(lua_function_name, description, input_trigger_strings, candidate_trigger_strings)"
    "    __ime_register_trigger(lua_function_name, desc, input_trigger_strings, candidate_trigger_strings);"
    "end;"
    "ime.register_command = function(command_name, lua_function_name)"
    "    __ime_register_command(command_name, lua_function_name);"
    "end;"
    "ime.unique_name = function()"
    "    return __ime_unique_name();"
    "end;"
    "ime.get_last_commit = function()"
    "    return __ime_get_last_commit();"
    "end;";
static const UT_icd FunctionItem_icd = {
    sizeof(FunctionItem), NULL, FunctionItemCopy, FunctionItemDtor
};
static const UT_icd LuaResultItem_icd = {
    sizeof(LuaResultItem), NULL, LuaResultItemCopy, LuaResultItemDtor
};

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

static int GetUniqueName_Export(lua_State *lua) {
    FcitxIM *im = FcitxInstanceGetCurrentIM(GetModule(lua)->fcitx);
    if (im)
        lua_pushstring(lua, im->uniqueName);
    else
        lua_pushstring(lua, "");
    return 1;
}

static int GetLastCommit_Export(lua_State *lua) {
    FcitxInputState* input = FcitxInstanceGetInputState(GetModule(lua)->fcitx);
    lua_pushstring(lua, FcitxInputStateGetLastCommitString(input));
    return 1;
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

static int ImeRegisterCommand_Export(lua_State *lua) {
    int c = lua_gettop(lua);
    if (c < 2) {
        FcitxLog(WARNING, "register command arugment missing");
        return 0;
    }
    const char *command_name = lua_tostring(lua, 1);
    const char *function_name = lua_tostring(lua, 2);
    if (command_name == NULL || function_name == NULL) {
        FcitxLog(WARNING, "register command command_name or function_name empty");
        return 0;
    }
    if (strlen(command_name) > 2) {
        FcitxLog(WARNING, "register command command_name length great than 2");
        return 0;
    }
    if (RegisterCommand(lua, command_name, function_name) == -1) {
        FcitxLog(ERROR, "RegisterCommand() failed");
        return 0;
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
        size_t len = luaL_len(lua, kInputStringsArg);
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

static void LuaResultItemCopy(void *_dst, const void *_src) {
    LuaResultItem *dst = (LuaResultItem *)_dst;
    LuaResultItem *src = (LuaResultItem *)_src;
    dst->result = src->result ? strdup(src->result) : NULL;
    dst->help = src->help ? strdup(src->help) : NULL;
    dst->tip = src->tip ? strdup(src->tip) : NULL;
}
static void LuaResultItemDtor(void *_elt) {
    LuaResultItem *elt = (LuaResultItem *)_elt;
    fcitx_utils_free(elt->result);
    fcitx_utils_free(elt->help);
    fcitx_utils_free(elt->tip);
}

static void FreeTrigger(TriggerItem **triggers, LuaExtension *extension) {
    TriggerItem *trigger;
    for (trigger = *triggers; trigger != NULL; ) {
        unsigned int count = utarray_len(trigger->functions);
        unsigned int i;
        FunctionItem *f;
        for (i = 0;i < count;) {
            f = (FunctionItem*)utarray_eltptr(trigger->functions, i);
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
            free(trigger->key);
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
    HASH_FOREACH(trigger, module->input_triggers, TriggerItem) {
        size_t keylen = strlen(trigger->key);
        if (keylen < length) {
            length = keylen;
        }
    }
    if (length == UINT_MAX) {
        module->shortest_input_trigger_key_length = 0;
    } else {
        module->shortest_input_trigger_key_length = length;
    }
}

LuaExtension * LoadExtension(LuaModule *module, const char *name) {
    LuaExtension *extension;
    HASH_FIND_STR(module->extensions, name, extension);
    if (extension) {
        FcitxLog(DEBUG, "extension:%s reload", name);
        UnloadExtension(module, extension);
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
    HASH_ADD_KEYPTR(hh, module->extensions, extension->name, strlen(extension->name), extension);

    int rv = lua_pcall(extension->lua, 0, 0, 0);
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(extension->lua);
        UnloadExtensionByName(module, name);
        return NULL;
    }

    rv = luaL_loadfile(extension->lua, name);
    if (rv != 0) {
        LuaPError(rv, "luaL_loadfile() failed");
        LuaPrintError(extension->lua);
        UnloadExtensionByName(module, name);
        return NULL;
    }
    rv = lua_pcall(extension->lua, 0, 0, 0);
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(extension->lua);
        UnloadExtensionByName(module, name);
        return NULL;
    }
    UpdateShortestInputTriggerKeyLength(module);
    return extension;
}

void UnloadExtensionByName(LuaModule *module, const char *name) {
    LuaExtension *extension = NULL;
    HASH_FIND_STR(module->extensions, name, extension);
    if (extension == NULL) {
        FcitxLog(WARNING, "extension:%s unload failed, not found", name);
        return;
    }
    UnloadExtension(module, extension);
}

void UnloadExtension(LuaModule *module, LuaExtension* extension) {

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
    HASH_FOREACH(e, module->extensions, LuaExtension) {
        if (e->lua == lua) {
            return e;
        }
    }
    return NULL;
}

static int RegisterCommand(lua_State *lua,
                           const char *command_name,
                           const char *function_name) {
    if (lua == NULL || command_name == NULL || function_name == NULL) {
        FcitxLog(WARNING, "RegisterCommand() argument error");
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
    CommandItem *command = NULL;
    HASH_FIND_STR(module->commands, command_name, command);
    if (command != NULL) {
        FcitxLog(WARNING, "command:%s exist", command_name);
        return -1;
    }
    command = calloc(sizeof(*command), 1);
    command->lua = lua;
    if (command == NULL) {
        FcitxLog(ERROR, "command alloc failed");
        return -1;
    }
    command->function_name = strdup(function_name);
    if (command->function_name == NULL) {
        FcitxLog(ERROR, "Command::function_name alloc failed");
        goto err;
    }
    command->command_name = strdup(command_name);
    if (command->command_name == NULL) {
        FcitxLog(ERROR, "Command::command_name alloc failed");
        goto err;
    }
    HASH_ADD_KEYPTR(hh,
                    module->commands,
                    command->command_name,
                    strlen(command->command_name),
                    command);
    return 0;
err:
    if (command) {
        if (command->function_name) {
            free(command->function_name);
        }
        if (command->command_name) {
            free(command->command_name);
        }
        free(command);
    }
    return -1;
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
        trigger->key = strdup(input);
        if (trigger->key == NULL) {
            FcitxLog(ERROR, "trigger->key memory alloc failed");
            goto cleanup;
        }
        utarray_new(trigger->functions, &FunctionItem_icd);
        if (trigger->functions == NULL) {
            FcitxLog(ERROR, "trigger->functions memory alloc failed");
            goto cleanup;
        }
        HASH_ADD_KEYPTR(hh, module->input_triggers, trigger->key, strlen(trigger->key), trigger);
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
        if (trigger->key) {
            free(trigger->key);
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
    lua_register(lua, "__ime_register_command", ImeRegisterCommand_Export);
    lua_register(lua, "__ime_unique_name", GetUniqueName_Export);
    lua_register(lua, "__ime_get_last_commit", GetLastCommit_Export);
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

static UT_array * LuaCallFunction(lua_State *lua,
                                  const char *function_name,
                                  const char *argument) {
    UT_array *result = NULL;
    lua_getglobal(lua, "__ime_call_function");
    lua_pushstring(lua, function_name);
    lua_pushstring(lua, argument);
    int rv = lua_pcall(lua, 2, 1, 0);
    if (rv != 0) {
        LuaPError(rv, "lua_pcall() failed");
        LuaPrintError(lua);
        return result;
    }
    if (lua_gettop(lua) == 0) {
        FcitxLog(WARNING, "lua_gettop() not retrun");
        return result;
    }
    int type = lua_type(lua, -1);
    if (type == LUA_TSTRING) {
        const char *str = lua_tostring(lua, -1);
        if (str) {
            utarray_new(result, &LuaResultItem_icd);
            /* str is ok for now and it will be copied by utarray */
            LuaResultItem r = {.result = (char *)str, .help = NULL, .tip =  NULL};
            utarray_push_back(result, &r);
        } else {
            FcitxLog(WARNING, "lua function return return null");
        }
    } else if (type == LUA_TTABLE) {
        size_t i, len = luaL_len(lua, -1);
        if (len < 1) {
            return result;
        }
        utarray_new(result, &LuaResultItem_icd);
        for (i = 1; i <= len; ++i) {
            lua_pushinteger(lua, i);
            /* stack, table, integer */
            lua_gettable(lua, -2);
            char istable = 0;
            if (lua_type(lua, -1) == LUA_TTABLE) {
                istable = 1;
                /* stack, table, result-item */
                lua_pushstring(lua, "help");
                /* stack, table, result-item, "help" */
                lua_gettable(lua, -2);
            }
            LuaResultItem r = {NULL, NULL, NULL};
            const char *str = lua_tostring(lua, -1);
            if (str == NULL) {
                FcitxLog(WARNING, "function %s() result[%d] is not string", function_name, i);
            } else {
                r.result = strdup(str);
            }
            /* stack, table, result-item, string */
            lua_pop(lua, 1);
            /* stack, table, result-item */

            if (r.result) {
                if (istable) {
                    const char* p;
                    /* stack, table, result-item, "suggest" */
                    lua_pushstring(lua, "suggest");
                    lua_gettable(lua, -2);
                    /* stack, table, result-item, suggest-string */
                    p = lua_tostring(lua, -1);
                    if (p)
                        r.help = strdup(p);
                    lua_pop(lua, 1);
                    /* stack, table, result-item */

                    /* stack, table, result-item, "tip" */
                    lua_pushstring(lua, "tip");
                    lua_gettable(lua, -2);
                    /* stack, table, result-item, tip string */
                    p = lua_tostring(lua, -1);
                    if (p)
                        r.tip = strdup(p);
                    lua_pop(lua, 1);
                    /* stack, table, result-item */
                } else {
                    r.help = NULL;
                    r.tip = NULL;
                }
                utarray_push_back(result, &r);
            }
            LuaResultItemDtor(&r);
            if (istable) {
                lua_pop(lua, 1);
            }
        }
        if (utarray_len(result) == 0) {
            utarray_free(result);
            result = NULL;
        }
    } else {
        FcitxLog(WARNING, "lua function return type not expected:%s",
                lua_typename(lua, type));
    }
    lua_pop(lua, lua_gettop(lua));
    return result;
}

UT_array *
InputCommand(LuaModule *module, const char *input)
{
    CommandItem *command;
    char key[3];
    strncpy(key, input, sizeof(key));
    key[2] = 0;
    HASH_FIND_STR(module->commands, key, command);
    if (command == NULL) {
        return NULL;
    }
    const char *arg;
    if (strlen(input) > 2) {
        arg = input + 2;
    } else {
        arg = "";
    }
    return LuaCallFunction(command->lua, command->function_name, arg);
}

UT_array * InputTrigger(LuaModule *module, const char *input) {
    if (module->shortest_input_trigger_key_length == 0
        || strlen(input) < module->shortest_input_trigger_key_length) {
        return NULL;
    }
    TriggerItem *trigger;
    HASH_FIND_STR(module->input_triggers, input, trigger);
    if (trigger == NULL) {
        return NULL;
    }

    UT_array *result = NULL;
    FunctionItem *f = NULL;
    while ((f = (FunctionItem *)utarray_next(trigger->functions, f))) {
        UT_array *temp = LuaCallFunction(f->lua, f->name, input);
        if (temp) {
            if (result) {
                LuaResultItem *p;
                while ((p = (LuaResultItem *)utarray_next(temp, p))) {
                    utarray_push_back(result, p);
                }
            } else {
                result = temp;
            }
        }
    }
    return result;
}

void UnloadAllExtension(LuaModule* luamodule)
{
    while(luamodule->extensions)
        UnloadExtensionByName(luamodule, luamodule->extensions->name);
}
