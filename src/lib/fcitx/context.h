/***************************************************************************
 *   Copyright (C) 2010~2011 by CSSlayer                                   *
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

#ifndef _FCITX_CONTEXT_H_
#define _FCITX_CONTEXT_H_

#include <fcitx/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef void (*FcitxContextCallback)(void* arg, const void* value);

    typedef struct _FcitxContext FcitxContext;

    typedef enum _FcitxContextFlag {
        FCF_None = 0,
        FCF_ResetOnInputMethodChange = (1 << 0)
    } FcitxContextFlag;

    typedef enum _FcitxContextType {
        FCT_Hotkey,
        FCT_String,
        FCT_Void,
        FCT_Boolean
    } FcitxContextType;

    #define CONTEXT_ALTERNATIVE_PREVPAGE_KEY "CONTEXT_ALTERNATIVE_PREVPAGE_KEY"
    #define CONTEXT_ALTERNATIVE_NEXTPAGE_KEY "CONTEXT_ALTERNATIVE_NEXTPAGE_KEY"
    #define CONTEXT_IM_LANGUAGE "CONTEXT_IM_LANGUAGE"
    #define CONTEXT_IM_KEYBOARD_LAYOUT "CONTEXT_IM_KEYBOARD_LAYOUT"
    #define CONTEXT_DISABLE_AUTOENG "CONTEXT_DISABLE_AUTOENG"
    #define CONTEXT_DISABLE_QUICKPHRASE "CONTEXT_DISABLE_QUICKPHRASE"

    void FcitxInstanceRegisterWatchableContext(FcitxInstance* instance, const char* key, FcitxContextType type, unsigned int flag );
    void FcitxInstanceWatchContext(FcitxInstance* instance, const char* key, FcitxContextCallback callback, void* arg);
    void FcitxInstanceSetContext(FcitxInstance* instance, const char* key, const void* value);
    const char* FcitxInstanceGetContextString(FcitxInstance* instance, const char* key);
    boolean FcitxInstanceGetContextBoolean(FcitxInstance* instance, const char* key);
    const FcitxHotkey* FcitxInstanceGetContextHotkey(FcitxInstance* instance, const char* key);

#ifdef __cplusplus
}
#endif

#endif
