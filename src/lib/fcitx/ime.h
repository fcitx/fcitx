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
 * @file   ime.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  Public Header for Input Method Develop
 *
 */
#ifndef _FCITX_IME_H_
#define _FCITX_IME_H_

#include <time.h>
#include <fcitx-utils/utf8.h>
#include <fcitx-config/hotkey.h>
#include <fcitx/ui.h>
#include <fcitx/addon.h>

#ifdef __cplusplus

extern "C" {
#endif

#define MAX_CODE_LEN    63

#define MAX_CAND_LEN    127
#define MAX_TIPS_LEN    9

#define MAX_CAND_WORD    10
#define MAX_USER_INPUT    300

#define HOT_KEY_COUNT   2

#define LANGCODE_LENGTH 5

#define PRIORITY_DISABLE 100

    struct _FcitxInputContext;
    struct _FcitxInstance;
    struct _FcitxAddon;
    struct _FcitxCandidateWordList;
    
    /** input method available status */
    typedef enum _FcitxIMAvailableStatus {
        IMAS_Enable,
        IMAS_Disable,
    } FcitxIMAvailableStatus;

    /** do input function return value */
    typedef enum _INPUT_RETURN_VALUE {
        IRV_TO_PROCESS = 0, /* do something */
        IRV_FLAG_BLOCK_FOLLOWING_PROCESS = 1 << 0, /* nothing to do, actually non-zero is blocking, but you need a flag for do nothing */
        IRV_FLAG_FORWARD_KEY = 1 << 1, /* the key will be forwarded */
        IRV_FLAG_RESET_INPUT = 1 << 2, /* reset input */
        IRV_FLAG_PENDING_COMMIT_STRING = 1 << 3, /* there is something in input strStringGet buffer, commit it */
        IRV_FLAG_UPDATE_INPUT_WINDOW = 1 << 4, /* something updated in input window, let the UI update */
        IRV_FLAG_UPDATE_CANDIDATE_WORDS = 1 << 5, /* update the candidate words */
        IRV_FLAG_ENG = 1 << 6, /* special */
        IRV_FLAG_PUNC = 1 << 7, /* special */
        IRV_FLAG_DISPLAY_LAST = 1 << 8, /* special */
        IRV_FLAG_DO_PHRASE_TIPS = 1 << 9, /* special */
        /* compatible */
        IRV_DONOT_PROCESS = IRV_FLAG_FORWARD_KEY,
        IRV_COMMIT_STRING = IRV_FLAG_PENDING_COMMIT_STRING | IRV_FLAG_DO_PHRASE_TIPS,
        IRV_DO_NOTHING = IRV_FLAG_BLOCK_FOLLOWING_PROCESS,
        IRV_CLEAN = IRV_FLAG_RESET_INPUT,
        IRV_COMMIT_STRING_REMIND = IRV_FLAG_PENDING_COMMIT_STRING | IRV_FLAG_UPDATE_INPUT_WINDOW,
        IRV_DISPLAY_CANDWORDS = IRV_FLAG_UPDATE_INPUT_WINDOW | IRV_FLAG_UPDATE_CANDIDATE_WORDS,
        IRV_DONOT_PROCESS_CLEAN = IRV_FLAG_FORWARD_KEY | IRV_FLAG_RESET_INPUT,
        IRV_COMMIT_STRING_NEXT =  IRV_FLAG_PENDING_COMMIT_STRING | IRV_FLAG_UPDATE_INPUT_WINDOW,
        IRV_DISPLAY_MESSAGE = IRV_FLAG_UPDATE_INPUT_WINDOW,
        IRV_ENG = IRV_FLAG_PENDING_COMMIT_STRING | IRV_FLAG_ENG | IRV_FLAG_RESET_INPUT,
        IRV_PUNC = IRV_FLAG_PENDING_COMMIT_STRING | IRV_FLAG_PUNC | IRV_FLAG_RESET_INPUT,
        IRV_DISPLAY_LAST = IRV_FLAG_UPDATE_INPUT_WINDOW | IRV_FLAG_DISPLAY_LAST
    } INPUT_RETURN_VALUE;

    /**
     * Fcitx Input Method class, it can register more than one input
     *        method in create function
     **/
    typedef struct _FcitxIMClass {
        void* (*Create)(struct _FcitxInstance* instance);
        void  (*Destroy)(void *arg);
    } FcitxIMClass;

    typedef boolean(*FcitxIMInit)(void *arg); /**< FcitxIMInit */
    typedef void (*FcitxIMResetIM)(void *arg); /**< FcitxIMResetIM */
    typedef INPUT_RETURN_VALUE(*FcitxIMDoInput)(void *arg, FcitxKeySym, unsigned int); /**< FcitxIMDoInput */
    typedef INPUT_RETURN_VALUE(*FcitxIMGetCandWords)(void *arg); /**< FcitxIMGetCandWords */
    typedef boolean(*FcitxIMPhraseTips)(void *arg); /**< FcitxIMPhraseTips */
    typedef void (*FcitxIMSave)(void *arg); /**< FcitxIMSave */
    typedef void (*FcitxIMReloadConfig)(void *arg); /**< FcitxIMReloadConfig */
    typedef INPUT_RETURN_VALUE (*FcitxIMKeyBlocker)(void* arg, FcitxKeySym, unsigned int); /**< FcitxIMKeyBlocker */

    /**
     * Fcitx Input method instance
     **/
    typedef struct _FcitxIM {
        /**
         * The name that can be display on the UI
         **/
        char              *strName;
        /**
         * icon name used to find icon
         **/
        char              *strIconName;
        /**
         * reset im status
         **/
        FcitxIMResetIM ResetIM;
        /**
         * process key input
         **/
        FcitxIMDoInput DoInput;
        /**
         * update candidate works function
         **/
        FcitxIMGetCandWords GetCandWords;
        /**
         * phrase tips function
         **/
        FcitxIMPhraseTips PhraseTips;
        /**
         * save function
         **/
        FcitxIMSave Save;
        /**
         * init function
         **/
        FcitxIMInit Init;
        /**
         * reload config function
         **/
        FcitxIMReloadConfig ReloadConfig;

        void* unused;
        /**
         * the pointer to im class
         **/
        void* klass;
        /**
         * the priority order
         **/
        int iPriority;
        /**
         * Language Code
         **/
        char langCode[LANGCODE_LENGTH + 1];

        /**
         * uniqueName
         **/
        char *uniqueName;

        /**
         * input method initialized or not
         */
        boolean initialized;

        /**
         * Fcitx Addon
         **/
        FcitxAddon* owner;
        /**
         * reload config function
         **/
        FcitxIMKeyBlocker KeyBlocker;

        void* padding[10];
    } FcitxIM;

    /** a key event is press or release */
    typedef enum _FcitxKeyEventType {
        FCITX_PRESS_KEY,
        FCITX_RELEASE_KEY
    } FcitxKeyEventType;

    /**
     * Global Input State, including displayed message.
     **/
    typedef struct _FcitxInputState FcitxInputState;

    /**
     * @brief create a new input state
     *
     * @return FcitxInputState*
     **/
    FcitxInputState* FcitxInputStateCreate();

    /**
     * the string pending commit
     *
     * @param input input state
     * @return char*
     **/
    char* FcitxInputStateGetOutputString(FcitxInputState* input);

    /**
     * get current input method
     *
     * @param instance fcitx instance
     * @return _FcitxIM*
     **/
    struct _FcitxIM* FcitxInstanceGetCurrentIM(struct _FcitxInstance *instance);

    /**
     * enable im
     *
     * @param instance fcitx instance
     * @param ic input context
     * @param keepState keep current state or not
     * @return void
     **/
    void FcitxInstanceEnableIM(struct _FcitxInstance* instance, struct _FcitxInputContext* ic, boolean keepState);

    /**
     * End Input
     *
     * @param instance
     * @param ic input context
     * @return void
     **/
    void FcitxInstanceCloseIM(struct _FcitxInstance* instance, struct _FcitxInputContext* ic);

    /**
     * Change im state between IS_ACTIVE and IS_ENG
     *
     * @param instance fcitx instance
     * @param ic input context
     * @return void
     **/
    void FcitxInstanceChangeIMState(struct _FcitxInstance* instance, struct _FcitxInputContext* ic);

    /**
     * reset input state
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceResetInput(struct _FcitxInstance* instance);

    /**
     * clean whole input window
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceCleanInputWindow(struct _FcitxInstance *instance);

    /**
     * clean preedit string and aux up
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceCleanInputWindowUp(struct _FcitxInstance *instance);

    /**
     * clean candidate word list and aux down
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceCleanInputWindowDown(struct _FcitxInstance *instance);

    /**
     * Sometimes, we use INPUT_RETURN_VALUE not from ProcessKey, so use this function to do the correct thing.
     *
     * @param instance fcitx instance
     * @param retVal input return val
     * @return void
     **/
    void FcitxInstanceProcessInputReturnValue(
        struct _FcitxInstance* instance,
        INPUT_RETURN_VALUE retVal
    );

    /**
     * register a new input method
     *
     * @param instance fcitx instance
     * @param imclass pointer to input method class
     * @param uniqueName uniqueName which cannot be duplicated to others
     * @param name input method name
     * @param iconName icon name
     * @param Init init callback
     * @param ResetIM reset callback
     * @param DoInput do input callback
     * @param GetCandWords get candidate words callback
     * @param PhraseTips phrase tips callback
     * @param Save save callback
     * @param ReloadConfig reload config callback
     * @param KeyBlocker key blocker callback
     * @param priority order of this input method
     * @param langCode language code for this input method
     * @return void
     **/
    void FcitxInstanceRegisterIM(struct _FcitxInstance *instance,
                           void *imclass,
                           const char* uniqueName,
                           const char* name,
                           const char* iconName,
                           FcitxIMInit Init,
                           FcitxIMResetIM ResetIM,
                           FcitxIMDoInput DoInput,
                           FcitxIMGetCandWords GetCandWords,
                           FcitxIMPhraseTips PhraseTips,
                           FcitxIMSave Save,
                           FcitxIMReloadConfig ReloadConfig,
                           FcitxIMKeyBlocker KeyBlocker,
                           int priority,
                           const char *langCode
                          );
    /**
     * process a key event, should only used by frontend
     *
     * @param instance fcitx instance
     * @param event event type
     * @param timestamp timestamp
     * @param sym keysym
     * @param state key state
     * @return INPUT_RETURN_VALUE
     **/
    INPUT_RETURN_VALUE FcitxInstanceProcessKey(struct _FcitxInstance* instance, FcitxKeyEventType event, long unsigned int timestamp, FcitxKeySym sym, unsigned int state);

    /**
     * another half part for process key, will be called by FcitxInstanceProcessKey()
     *
     * @param instance fcitx instance
     * @param retVal last return value
     * @param event event type
     * @param timestamp timestamp
     * @param sym keysym
     * @param state key state
     * @return INPUT_RETURN_VALUE
     **/
    INPUT_RETURN_VALUE FcitxInstanceDoInputCallback(
        struct _FcitxInstance* instance,
        INPUT_RETURN_VALUE retVal,
        FcitxKeyEventType event,
        long unsigned int timestamp,
        FcitxKeySym sym,
        unsigned int state);

    /**
     * send a new key event to client
     *
     * @param instance fcitx instance
     * @param ic input context
     * @param event event tpye
     * @param sym keysym
     * @param state key state
     * @return void
     **/
    void FcitxInstanceForwardKey(struct _FcitxInstance* instance, struct _FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);

    /**
     * save all input method data
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceSaveAllIM(struct _FcitxInstance* instance);

    /**
     * reload all config
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceReloadConfig(struct _FcitxInstance* instance);

    /**
     * switch to input method by index
     *
     * @param instance fcitx instance
     * @param index input method index
     * @return void
     **/
    void FcitxInstanceSwitchIM(struct _FcitxInstance* instance, int index);

    /**
     * check is choose key or not, if so, return the choose index
     *
     * @param sym keysym
     * @param state keystate
     * @param strChoose choose key string
     * @return int
     **/
    int FcitxHotkeyCheckChooseKey(FcitxKeySym sym, int state, const char* strChoose);

    /**
     * check is choose key or not, if so, return the choose index
     *
     * @param sym keysym
     * @param state keystate
     * @param strChoose choose key string
     * @param candState candidate keystate
     * @return int
     **/
    int FcitxHotkeyCheckChooseKeyAndModifier(FcitxKeySym sym, int state, const char* strChoose, int candState);
    
    /**
     * get im index by im name
     *
     * @param instance fcitx instance
     * @param imName im name
     * @return int im index
     * 
     * @since 4.2
     **/
    int FcitxInstanceGetIMIndexByName(struct _FcitxInstance* instance, const char* imName);

    struct _FcitxCandidateWordList* FcitxInputStateGetCandidateList(FcitxInputState* input);

    boolean FcitxInputStateGetIsInRemind(FcitxInputState* input);

    void FcitxInputStateSetIsInRemind(FcitxInputState* input, boolean isInRemind);

    boolean FcitxInputStateGetIsDoInputOnly(FcitxInputState* input);

    void FcitxInputStateSetIsDoInputOnly(FcitxInputState* input, boolean isDoInputOnly);

    char* FcitxInputStateGetRawInputBuffer(FcitxInputState* input);

    int FcitxInputStateGetCursorPos(FcitxInputState* input);

    void FcitxInputStateSetCursorPos(FcitxInputState* input, int cursorPos);

    int FcitxInputStateGetClientCursorPos(FcitxInputState* input);

    void FcitxInputStateSetClientCursorPos(FcitxInputState* input, int cursorPos);

    FcitxMessages* FcitxInputStateGetAuxUp(FcitxInputState* input);

    FcitxMessages* FcitxInputStateGetAuxDown(FcitxInputState* input);

    FcitxMessages* FcitxInputStateGetPreedit(FcitxInputState* input);

    FcitxMessages* FcitxInputStateGetClientPreedit(FcitxInputState* input);

    int FcitxInputStateGetRawInputBufferSize(FcitxInputState* input);

    void FcitxInputStateSetRawInputBufferSize(FcitxInputState* input, int size);

    boolean FcitxInputStateGetShowCursor(FcitxInputState* input);

    void FcitxInputStateSetShowCursor(FcitxInputState* input, boolean showCursor);

    int FcitxInputStateGetLastIsSingleChar(FcitxInputState* input);

    void FcitxInputStateSetLastIsSingleChar(FcitxInputState* input, int lastIsSingleChar);
    
    void FcitxInputStateSetKeyCode( FcitxInputState* input, uint32_t value );
    
    uint32_t FcitxInputStateGetKeyCode( FcitxInputState* input);

    FcitxIM* FcitxInstanceGetIMFromIMList(struct _FcitxInstance* instance, FcitxIMAvailableStatus imas, const char* name);

    void FcitxInstanceUpdateIMList(struct _FcitxInstance* instance);

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
