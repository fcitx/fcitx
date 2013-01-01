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

#ifndef _LUA_WRAP_H_
#define _LUA_WRAP_H_

#include <lua.h>

#if LUA_VERSION_NUM < 502
# define luaL_len lua_objlen
#endif

typedef struct _LuaResultItem {
    char *result;
    char *help;
    char *tip;
} LuaResultItem;

typedef struct _LuaModule LuaModule;
typedef struct _LuaExtension LuaExtension;

// alloc/free luamodule
LuaModule * LuaModuleAlloc(struct _FcitxInstance *fcitx);
void LuaModuleFree(LuaModule *luamodule);
FcitxInstance *GetFcitx(LuaModule *luamodule);

// Load lua extension, name is filename of lua source file
LuaExtension * LoadExtension(LuaModule *luamodule, const char *name);

// Unload extension by name
void UnloadExtensionByName(LuaModule *luamodule, const char *name);
void UnloadAllExtension(LuaModule *luamodule);

// call lua trigger, input is user input
// callback is called when valid candidate generated
UT_array * InputTrigger(LuaModule *luamodule, const char *input);
UT_array * InputCommand(LuaModule *module, const char *input);

#endif
