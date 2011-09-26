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

    /**
     * @brief key filter function
     **/
    typedef boolean(*FcitxKeyFilter)(void* arg, FcitxKeySym sym,
                                     unsigned int state,
                                     INPUT_RETURN_VALUE *retval
                                    );

    /**
     * @brief string filter function
     **/

    typedef char* (*FcitxStringFilter)(void* arg, const char* in);

    /**
     * @brief ime event hook function
     **/
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
        INPUT_RETURN_VALUE(*hotkeyhandle)(void*);
        /**
         * @brief Argument
         **/
        void* arg;
    } HotkeyHook;

    /**
     * @brief Key filter hook
     **/
    typedef struct _KeyFilterHook {
        /**
         * @brief Key filter function
         **/
        FcitxKeyFilter func;
        /**
         * @brief extra argument for filter function
         **/
        void *arg;
    } KeyFilterHook;

    /**
     * @brief Hook for string filter, this hook can change the output string.
     **/
    typedef struct _StringFilterHook {
        /**
         * @brief Filter function
         **/
        FcitxStringFilter func;
        /**
         * @brief Extra argument for the filter function.
         **/
        void *arg;
    } StringFilterHook;

    /**
     * @brief IME Event hook for Reset, Trigger On/Off, Focus/Unfocus
     **/
    typedef struct _FcitxIMEventHook {
        FcitxIMEventHookFunc func;
        void *arg;
    } FcitxIMEventHook;

    /**
     * @brief register pre input filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterPreInputFilter(struct _FcitxInstance* instance, KeyFilterHook hook) ;
    /**
     * @brief register post input filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterPostInputFilter(struct _FcitxInstance* instance, KeyFilterHook hook);
    /**
     * @brief register ouput string filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterOutputFilter(struct _FcitxInstance* instance, StringFilterHook hook);
    /**
     * @brief register hotkey
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterHotkeyFilter(struct _FcitxInstance* instance, HotkeyHook hook);
    /**
     * @brief register reset input hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterResetInputHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * @brief register trigger on hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterTriggerOnHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * @brief register trigger off hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterTriggerOffHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * @brief register focus in hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterInputFocusHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * @brief register focus out hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterInputUnFocusHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);

    /**
     * @brief register update candidate word hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterUpdateCandidateWordHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    
    /**
     * @brief register update input method list hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void RegisterUpdateIMListHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);

    /**
     * @brief process output filter, return string is malloced
     *
     * @param instance fcitx instance
     * @param in input string
     * @return char*
     **/
    char* ProcessOutputFilter(struct _FcitxInstance* instance, char *in);


#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
