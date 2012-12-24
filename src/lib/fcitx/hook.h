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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

/**
 * @addtogroup Fcitx
 * @{
 */

/**
 * @file hook.h
 * Register function to be called automatically.
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
     * key filter function
     **/
    typedef boolean(*FcitxKeyFilter)(void* arg, FcitxKeySym sym,
                                     unsigned int state,
                                     INPUT_RETURN_VALUE *retval
                                    );

    /**
     * string filter function
     **/

    typedef char* (*FcitxStringFilter)(void* arg, const char* in);

    /**
     * ime event hook function
     **/
    typedef void (*FcitxIMEventHookFunc)(void* arg);

    /**
     * ic event hook function
     **/
    typedef void (*FcitxICEventHookFunc)(void* arg, struct _FcitxInputContext* ic);

    /**
     * Hotkey process struct
     **/
    typedef struct _FcitxHotkeyHook {
        /**
         * Pointer to fcitx hotkeys, fcitx hotkey is length 2 array.
         **/
        FcitxHotkey* hotkey;
        /**
         * Function to be called while hotkey is pressed.
         *
         * @return INPUT_RETURN_VALUE*
         **/
        INPUT_RETURN_VALUE(*hotkeyhandle)(void*);
        /**
         * Argument
         **/
        void* arg;
    } FcitxHotkeyHook;

    /**
     * Key filter hook
     **/
    typedef struct _FcitxKeyFilterHook {
        /**
         * Key filter function
         **/
        FcitxKeyFilter func;
        /**
         * extra argument for filter function
         **/
        void *arg;
    } FcitxKeyFilterHook;

    /**
     * Hook for string filter, this hook can change the output string.
     **/
    typedef struct _FcitxStringFilterHook {
        /**
         * Filter function
         **/
        FcitxStringFilter func;
        /**
         * Extra argument for the filter function.
         **/
        void *arg;
    } FcitxStringFilterHook;

    /**
     * IME Event hook for Reset, Trigger On/Off, Focus/Unfocus
     **/
    typedef struct _FcitxIMEventHook {
        FcitxIMEventHookFunc func; /**< callback function */
        void *arg; /**< argument for callback */
    } FcitxIMEventHook;

    /**
     * IC Event hook
     **/
    typedef struct _FcitxICEventHook {
        FcitxICEventHookFunc func; /**< callback function */
        void *arg; /**< argument for callback */
    } FcitxICEventHook;
    /**
     * register pre input filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterPreInputFilter(struct _FcitxInstance* instance, FcitxKeyFilterHook hook) ;
    /**
     * register post input filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterPostInputFilter(struct _FcitxInstance* instance, FcitxKeyFilterHook hook);
    /**
     * register ouput string filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterOutputFilter(struct _FcitxInstance* instance, FcitxStringFilterHook hook);
    /**
     * register hotkey
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterHotkeyFilter(struct _FcitxInstance* instance, FcitxHotkeyHook hook);
    /**
     * register reset input hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterResetInputHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * register trigger on hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterTriggerOnHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * register trigger off hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterTriggerOffHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * register focus in hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterInputFocusHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * register focus out hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterInputUnFocusHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);
    /**
     * register im changed hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterIMChangedHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);

    /**
     * register update candidate word hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterUpdateCandidateWordHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);

    /**
     * register update input method list hook
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     **/
    void FcitxInstanceRegisterUpdateIMListHook(struct _FcitxInstance* instance, FcitxIMEventHook hook);

    /**
     * process output filter, return string is malloced
     *
     * @param instance fcitx instance
     * @param in input string
     * @return char*
     **/
    char* FcitxInstanceProcessOutputFilter(struct _FcitxInstance* instance, const char *in);

    /**
     * process output filter, return string is malloced
     *
     * @param instance fcitx instance
     * @param in input string
     * @return char*
     **/
    char* FcitxInstanceProcessCommitFilter(struct _FcitxInstance* instance, const char *in);

    /**
     * register ouput string filter
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     *
     * @since 4.2.0
     **/
    void FcitxInstanceRegisterCommitFilter(struct _FcitxInstance* instance, FcitxStringFilterHook hook);

    /**
     * register a hook for watching when ic status changed
     *
     * @param instance fcitx instance
     * @param hook new hook
     * @return void
     *
     * @since 4.2.6
     **/
    void FcitxInstanceRegisterICStateChangedHook(struct _FcitxInstance* instance, FcitxICEventHook hook);

#ifdef __cplusplus
}
#endif

#endif
/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
