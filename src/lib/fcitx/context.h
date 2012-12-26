/***************************************************************************
 *   Copyright (C) 2010~2012 by CSSlayer                                   *
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

/**
 * @addtogroup Fcitx
 * @{
 */

#ifndef _FCITX_CONTEXT_H_
#define _FCITX_CONTEXT_H_

#include <fcitx/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

    /** context callback function prototype */
    typedef void (*FcitxContextCallback)(void* arg, const void* value);

    /** fcitx context type */
    typedef struct _FcitxContext FcitxContext;

    /** fcitx context flag */
    typedef enum _FcitxContextFlag {
        FCF_None = 0,
        FCF_ResetOnInputMethodChange = (1 << 0)
    } FcitxContextFlag;

    /** fcitx context value type */
    typedef enum _FcitxContextType {
        FCT_Hotkey,
        FCT_String,
        FCT_Void,
        FCT_Boolean
    } FcitxContextType;

    /** alternative prevpage key, if this input method requires different key for paging */
    #define CONTEXT_ALTERNATIVE_PREVPAGE_KEY "CONTEXT_ALTERNATIVE_PREVPAGE_KEY"
    /** alternative nextpage key, if this input method requires different key for paging */
    #define CONTEXT_ALTERNATIVE_NEXTPAGE_KEY "CONTEXT_ALTERNATIVE_NEXTPAGE_KEY"
    /** current input method language */
    #define CONTEXT_IM_LANGUAGE "CONTEXT_IM_LANGUAGE"
    /** current input method prefered layout, requires fcitx-xkb to really works */
    #define CONTEXT_IM_KEYBOARD_LAYOUT "CONTEXT_IM_KEYBOARD_LAYOUT"
    /** disable built-in autoeng module */
    #define CONTEXT_DISABLE_AUTOENG "CONTEXT_DISABLE_AUTOENG"
    /** disable built-in quichphrase module */
    #define CONTEXT_DISABLE_QUICKPHRASE "CONTEXT_DISABLE_QUICKPHRASE"
    /** show a built-in remind button or not */
    #define CONTEXT_SHOW_REMIND_STATUS "CONTEXT_SHOW_REMIND_STATUS"
    /** disable auto first candidate highlight */
    #define CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT "CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT"
    /** disable auto first candidate highlight */
    #define CONTEXT_DISABLE_FULLWIDTH "CONTEXT_DISABLE_FULLWIDTH"

    /**
     * @brief register a new global context variable
     *
     * @param instance fcitx instance
     * @param key context name
     * @param type contex value type
     * @param flag context flag
     * @return void
     **/
    void FcitxInstanceRegisterWatchableContext(FcitxInstance* instance, const char* key, FcitxContextType type, unsigned int flag );

    /**
     * @brief bind a callback function to this context, callback will be called when context value changed.
     *
     * @param instance fcitx instance
     * @param key context name
     * @param callback callback function
     * @param arg extra argument to the callback
     * @return void
     **/
    void FcitxInstanceWatchContext(FcitxInstance* instance, const char* key, FcitxContextCallback callback, void* arg);

    /**
     * @brief update the value of a context
     *
     * @param instance instance
     * @param key context name
     * @param value context value
     * @return void
     **/
    void FcitxInstanceSetContext(FcitxInstance* instance, const char* key, const void* value);
    /**
     * @brief get string type context value
     *
     * @param instance fcitx instance
     * @param key contex name
     * @return const char*
     **/
    const char* FcitxInstanceGetContextString(FcitxInstance* instance, const char* key);
    /**
     * @brief get boolean type context value
     *
     * @param instance fcitx instance
     * @param key context name
     * @return boolean
     **/
    boolean FcitxInstanceGetContextBoolean(FcitxInstance* instance, const char* key);
    /**
     * @brief get hotkey type context value
     *
     * @param instance fcitx instance
     * @param key key
     * @return const FcitxHotkey*
     **/
    const FcitxHotkey* FcitxInstanceGetContextHotkey(FcitxInstance* instance, const char* key);

    static inline const FcitxHotkey*
    FcitxConfigPrevPageKey(FcitxInstance *instance, FcitxGlobalConfig *fc)
    {
        const FcitxHotkey *prev = FcitxInstanceGetContextHotkey(
            instance, CONTEXT_ALTERNATIVE_PREVPAGE_KEY);
        if (!prev)
            return fc->hkPrevPage;
        return prev;
    }

    static inline const FcitxHotkey*
    FcitxConfigNextPageKey(FcitxInstance *instance, FcitxGlobalConfig *fc)
    {
        const FcitxHotkey *next = FcitxInstanceGetContextHotkey(
            instance, CONTEXT_ALTERNATIVE_NEXTPAGE_KEY);
        if (!next)
            return fc->hkNextPage;
        return next;
    }

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
