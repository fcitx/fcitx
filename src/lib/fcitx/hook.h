/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/**
 * @file hook.h
 * @brief Register function to be called automatically.
 */

#ifndef _HOOK_H
#define _HOOK_H
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/hotkey.h"
#include "ime.h"

typedef boolean (*FcitxKeyFilter)(void* arg, long unsigned int sym, 
                             unsigned int state,
                             INPUT_RETURN_VALUE *retval
                            );
typedef char* (*FcitxStringFilter)(void* arg, const char* in);
typedef void (*FcitxIMEventHookFunc)(void* arg);

/**
 * @brief Hotkey process struct
 **/
typedef struct HotkeyHook {
    /**
     * @brief Pointer to fcitx hotkeys, fcitx hotkey is length 2 array.
     **/
    HOTKEYS* hotkey;
    /**
     * @brief Function to be called while hotkey is pressed.
     *
     * @return INPUT_RETURN_VALUE*
     **/
    INPUT_RETURN_VALUE (*hotkeyhandle)(void*);
    /**
     * @brief Argument
     **/
    void* arg;
} HotkeyHook;

typedef struct KeyFilterHook 
{
    FcitxKeyFilter func;
    void *arg;
} KeyFilterHook;

typedef struct StringFilterHook 
{
    FcitxStringFilter func;
    void *arg;
} StringFilterHook;

typedef struct FcitxIMEventHook 
{
    FcitxIMEventHookFunc func;
    void *arg;
} FcitxIMEventHook;

void RegisterPreInputFilter(KeyFilterHook) ;
void RegisterPostInputFilter(KeyFilterHook);
void RegisterOutputFilter(StringFilterHook);
void RegisterHotkeyFilter(HotkeyHook);
void RegisterResetInputHook(FcitxIMEventHook value);
void RegisterTriggerOnHook(FcitxIMEventHook value);
void RegisterTriggerOffHook(FcitxIMEventHook value);
void RegisterInputFocusHook(FcitxIMEventHook value);
void RegisterInputUnFocusHook(FcitxIMEventHook value);

#endif