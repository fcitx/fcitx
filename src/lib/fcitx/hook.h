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
#include <fcitx-config/fcitx-config.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/ime.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef boolean (*FcitxKeyFilter)(void* arg, FcitxKeySym sym, 
                             unsigned int state,
                             INPUT_RETURN_VALUE *retval
                            );
typedef char* (*FcitxStringFilter)(void* arg, const char* in);
typedef void (*FcitxIMEventHookFunc)(void* arg);

/**
 * @brief Hotkey process struct
 **/
typedef struct _HotkeyHook {
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

typedef struct _KeyFilterHook 
{
    FcitxKeyFilter func;
    void *arg;
} KeyFilterHook;

typedef struct _StringFilterHook 
{
    FcitxStringFilter func;
    void *arg;
} StringFilterHook;

typedef struct _FcitxIMEventHook 
{
    FcitxIMEventHookFunc func;
    void *arg;
} FcitxIMEventHook;

void RegisterPreInputFilter(struct _FcitxInstance* instance, KeyFilterHook) ;
void RegisterPostInputFilter(struct _FcitxInstance* instance, KeyFilterHook);
void RegisterOutputFilter(struct _FcitxInstance* instance, StringFilterHook);
void RegisterHotkeyFilter(struct _FcitxInstance* instance, HotkeyHook);
void RegisterResetInputHook(struct _FcitxInstance* instance, FcitxIMEventHook value);
void RegisterTriggerOnHook(struct _FcitxInstance* instance, FcitxIMEventHook value);
void RegisterTriggerOffHook(struct _FcitxInstance* instance, FcitxIMEventHook value);
void RegisterInputFocusHook(struct _FcitxInstance* instance, FcitxIMEventHook value);
void RegisterInputUnFocusHook(struct _FcitxInstance* instance, FcitxIMEventHook value);
char* ProcessOutputFilter(struct _FcitxInstance* instance, char *in);

#ifdef __cplusplus
}
#endif

#endif