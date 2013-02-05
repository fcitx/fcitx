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
 * @file   ime.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  Public Header for Input Method Develop
 *
 * The input method is key event centric application. When a key event comes to fcitx,
 * the process handling the keyboard event can be separated into 7 phases, PreInput, DoInput, Update Candidates, Prev/Next Page
 * PostInput, Hotkey, and key blocker.
 *
 * The input method engine process key event inside "DoInput".
 *
 * Each phase will change the INPUT_RETURN_VALUE, if INPUT_RETURN_VALUE is non-zero (non IRV_TO_PROCESS), the following
 * phases will not run.
 *
 * If a key event goes through all phase and still the state is IRV_TO_PROCESS, it will be forwarded.
 *
 * When it comes to update candidates, if the flag contains IRV_FLAG_UPDATE_CANDIDATE_WORDS, it will trigger the GetCandWords
 * of input method engine and clean up all stiring in the input window, after that, it will trigger update candidates hook.
 *
 * Key blocker is useful if you want to do something in the post input phase, but you don't want forward key if they do nothing.
 * There is an default implemention inside fcitx, it will blocks key when raw input buffer is not empty. Those keys
 * contains direction key(left/right..), key will cause something input (a,b,c...), key will cause cursor move (home/end...).
 *
 * DoInput, Update Candidates, Key Blocker are belongs to input method engine, other can be registered by other addon.
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

/** max length of rawInputBuffer and outputString */
#define MAX_USER_INPUT    300

/** FcitxHotkey internally use 2 hotkeys for everycase */
#define HOT_KEY_COUNT   2

/**
 * Only keep for compatible
 * @deprecated
 */
#define MAX_CAND_LEN    127

/** max language code length, common 5 length is zh_CN
 * a shorter case is en
 */
#define LANGCODE_LENGTH 5

/** when input method priority is larger than 100, it will be disabled by default after install */
#define PRIORITY_DISABLE 100

/** due to backward compatible, this priority will be the most priority one */
#define PRIORITY_MAGIC_FIRST 0xf1527

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
        void* (*Create)(struct _FcitxInstance* instance); /**< interface for create a input method */
        void  (*Destroy)(void *arg); /**< interface for destroy all input method created by this class */
    } FcitxIMClass;

    /**
     * Fcitx Input Method class, it can register more than one input
     *        method in create function
     **/
    typedef struct _FcitxIMClass2 {
        void* (*Create)(struct _FcitxInstance* instance); /**< interface for create a input method */
        void  (*Destroy)(void *arg); /**< interface for destroy all input method created by this class */
        void  (*ReloadConfig)(void *arg); /**< interface for destroy all input method created by this class */
        void  (*padding1)(void *arg); /**< padding */
        void  (*padding2)(void *arg); /**< padding */
        void  (*padding3)(void *arg); /**< padding */
        void  (*padding4)(void *arg); /**< padding */
        void  (*padding5)(void *arg); /**< padding */
    } FcitxIMClass2;

    typedef enum _FcitxIMCloseEventType {
        /**
         * when user press inactivate key, default behavior is commit raw preedit.
         * If you want to OVERRIDE this behavior, be sure to implement this function.
         *
         * in some case, your implementation of OnClose should respect the value of
         * [Output/SendTextWhenSwitchEng], when this value is true, commit something you
         * want.
         *
         * And no matter in which case, Reset will be called after that.
         */
        CET_ChangeByInactivate,
        /**
         * when using lost focus
         * this might be variance case to case. the default behavior is to commit
         * the preedit, and resetIM.
         *
         * Controlled by [Output/DontCommitPreeditWhenUnfocus], this option will not
         * work for application switch doesn't support async commit.
         *
         * So OnClose is called when preedit IS committed (not like CET_ChangeByInactivate,
         * this behavior cannot be overrided), it give im a chance to choose remember this
         * word or not.
         *
         * Input method need to notice, that the commit is already DONE, do not do extra commit.
         */
        CET_LostFocus,
        /**
         * when user switch to a different input method by hand
         * such as ctrl+shift by default, or by ui,
         * not implemented yet, default behavior is reset IM.
         */
        CET_ChangeByUser,
    } FcitxIMCloseEventType;

    typedef boolean(*FcitxIMInit)(void *arg); /**< FcitxIMInit */
    typedef void (*FcitxIMResetIM)(void *arg); /**< FcitxIMResetIM */
    typedef INPUT_RETURN_VALUE(*FcitxIMDoInput)(void *arg, FcitxKeySym, unsigned int); /**< FcitxIMDoInput */
    typedef INPUT_RETURN_VALUE(*FcitxIMGetCandWords)(void *arg); /**< FcitxIMGetCandWords */
    typedef boolean(*FcitxIMPhraseTips)(void *arg); /**< FcitxIMPhraseTips */
    typedef void (*FcitxIMSave)(void *arg); /**< FcitxIMSave */
    typedef void (*FcitxIMReloadConfig)(void *arg); /**< FcitxIMReloadConfig */
    typedef INPUT_RETURN_VALUE (*FcitxIMKeyBlocker)(void* arg, FcitxKeySym, unsigned int); /**< FcitxIMKeyBlocker */
    typedef void (*FcitxIMUpdateSurroundingText)(void* arg); /**< FcitxIMKeyBlocker */
    typedef void (*FcitxIMOnClose)(void* arg, FcitxIMCloseEventType);

    /**
     * a more fexible interface for input method
     *
     * @since 4.2.3
     **/
    typedef struct _FcitxIMIFace {
        FcitxIMResetIM ResetIM /**< Reset input method state */;
        FcitxIMDoInput DoInput /**< process key input */;
        FcitxIMGetCandWords GetCandWords; /**< get candidate words */
        FcitxIMPhraseTips PhraseTips; /**< don't use it */
        FcitxIMSave Save; /**< force save input method data */
        FcitxIMInit Init; /**< called when switch to this input method */
        FcitxIMReloadConfig ReloadConfig; /**< reload configuration */
        FcitxIMKeyBlocker KeyBlocker; /**< block unused key */
        FcitxIMUpdateSurroundingText UpdateSurroundingText; /**< surrounding text update trigger */
        FcitxIMDoInput DoReleaseInput; /**< process key release event */
        FcitxIMOnClose OnClose; /**< process when im being switched away */
        void* padding[62]; /**< padding */
    } FcitxIMIFace;

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

        void* unused; /**< unused */
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

        FcitxIMUpdateSurroundingText UpdateSurroundingText; /**< called when surrounding text updated */

        FcitxIMDoInput DoReleaseInput;

        FcitxIMOnClose OnClose;

        void* padding[8]; /**< padding */
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
     * create a new input state
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
     * @brief get last commit string
     *
     * @param input input state
     * @return const char*
     *
     * @since 4.2.3
     **/
    const char* FcitxInputStateGetLastCommitString(FcitxInputState * input);

    /**
     * get current input method, return result can be NULL.
     *
     * @param instance fcitx instance
     * @return _FcitxIM*
     **/
    struct _FcitxIM* FcitxInstanceGetCurrentIM(struct _FcitxInstance *instance);

    /**
     * get input method by name
     *
     * @param instance fcitx instance
     * @param index index
     * @return _FcitxIM*
     *
     * @since 4.2.7
     **/
    struct _FcitxIM* FcitxInstanceGetIMByIndex(struct _FcitxInstance* instance, int index);

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

    /**
     * get im index by im name
     *
     * @param instance fcitx instance
     * @param imName im name
     * @return int im index
     *
     * @since 4.2.7
     **/
    struct _FcitxIM* FcitxInstanceGetIMByName(struct _FcitxInstance* instance, const char* imName);

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
     * register a new input method
     *
     * @param instance fcitx instance
     * @param imclass pointer to input method class
     * @param uniqueName uniqueName which cannot be duplicated to others
     * @param name input method name
     * @param iconName icon name
     * @param iface interface
     * @param priority order of this input method
     * @param langCode language code for this input method
     * @return void
     *
     * @see FcitxInstanceRegisterIMv2
     *
     * @since 4.2.3
     **/
    void FcitxInstanceRegisterIMv2(struct _FcitxInstance *instance,
                       void *imclass,
                       const char* uniqueName,
                       const char* name,
                       const char* iconName,
                       FcitxIMIFace iface,
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
     * @brief choose candidate by index
     *
     * @param instance instance
     * @param index idx
     * @return INPUT_RETURN_VALUE
     **/
    void FcitxInstanceChooseCandidateByIndex(
        struct _FcitxInstance* instance,
        int index);

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
     * reload only an addon's configuration, there are some short hand for reloading
     * other configuration, "global" for ~/.config/fcitx/config, "profile" for
     * ~/.config/fcitx/profile, "addon" for addon info. "ui" for current user interface
     * Input method unique can be also used here.
     *
     * @param instance fcitx instance
     * @param addon addon name
     * @return void
     *
     * @since 4.2.7
     **/
    void FcitxInstanceReloadAddonConfig(struct _FcitxInstance* instance, const char* addon);

    /**
     * reload all config
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceReloadConfig(struct _FcitxInstance* instance);

    /**
     * switch to input method by index, if index is zero, it will be skipped
     *
     * @deprecated
     *
     * @see FcitxInstanceSwitchIMByIndex
     *
     * @param instance fcitx instance
     * @param index input method index
     * @return void
     **/
    FCITX_DEPRECATED void FcitxInstanceSwitchIM(struct _FcitxInstance* instance, int index);


    /**
     * switch to a input method by name, name need to be valid, otherwise it have no effect
     * And if the index is zero, the state will automatically change to inactive
     *
     * @param instance fcitx instance
     * @param name ...
     * @return void
     *
     * @since 4.2.4
     **/
    void FcitxInstanceSwitchIMByName(struct _FcitxInstance* instance, const char* name);

    /**
     * switch to a input method by index, index need to be valid, otherwise it have no effect
     * And if the object index is zero, the state will automatically change to inactive
     * -1 means scroll forward, and -2 means scroll backward.
     * -3 means scroll forward without first one, -4 mean scroll backward without first one.
     *
     *
     * @param instance fcitx instance
     * @param name ...
     * @return void
     *
     * @since 4.2.4
     **/
    void FcitxInstanceSwitchIMByIndex(struct _FcitxInstance* instance, int index);

    /**
     * check is choose key or not, if so, return the choose index
     *
     * @param sym keysym
     * @param state keystate
     * @param strChoose choose key string
     * @return int
     **/
    int FcitxHotkeyCheckChooseKey(FcitxKeySym sym, unsigned int state, const char* strChoose);

    /**
     * check is choose key or not, if so, return the choose index
     *
     * @param sym keysym
     * @param state keystate
     * @param strChoose choose key string
     * @param candState candidate keystate
     * @return int
     **/
    int FcitxHotkeyCheckChooseKeyAndModifier(FcitxKeySym sym, unsigned int state, const char* strChoose, int candState);

    /**
     * ...
     *
     * @param input ...
     * @return _FcitxCandidateWordList*
     **/
    struct _FcitxCandidateWordList* FcitxInputStateGetCandidateList(FcitxInputState* input);

    /**
     * get current is in remind or not.
     *
     * @param input input state
     * @return remind state
     **/
    boolean FcitxInputStateGetIsInRemind(FcitxInputState* input);

    /**
     * set remind state
     *
     * @param input input state
     * @param isInRemind remind state
     * @return void
     **/
    void FcitxInputStateSetIsInRemind(FcitxInputState* input, boolean isInRemind);

    /**
     * get current key will be only processed by DoInput or not.
     *
     * @param input input state
     * @return DoInput Only state
     **/
    boolean FcitxInputStateGetIsDoInputOnly(FcitxInputState* input);

    /**
     * set current key will be only processed by DoInput or not.
     *
     * @param input input state
     * @param isDoInputOnly DoInput Only state
     * @return void
     **/
    void FcitxInputStateSetIsDoInputOnly(FcitxInputState* input, boolean isDoInputOnly);

    /**
     * get a writable raw input buffer, which is used as a hint for other module
     *
     * @param input input state
     * @return char*
     **/
    char* FcitxInputStateGetRawInputBuffer(FcitxInputState* input);

    /**
     * get current cursor position, offset is counted by byte in Preedit String
     *
     * @param input input state
     * @return current cursor position
     **/
    int FcitxInputStateGetCursorPos(FcitxInputState* input);

    /**
     * set current cursor position, offset is counted by byte in Preedit String
     *
     * @param input input state
     * @param cursorPos current cursor position
     * @return void
     **/
    void FcitxInputStateSetCursorPos(FcitxInputState* input, int cursorPos);

    /**
     * get client cursor position, which is similar to cursor position, but used with client preedit
     *
     * @param input input state
     * @return current client cursor position
     **/
    int FcitxInputStateGetClientCursorPos(FcitxInputState* input);

    /**
     * set client cursor position, which is similar to cursor position, but used with client preedit
     *
     * @param input input state
     * @param cursorPos current client cursor position
     * @return void
     **/
    void FcitxInputStateSetClientCursorPos(FcitxInputState* input, int cursorPos);

    /**
     * get auxiliary string displayed in the upper side of input panel
     *
     * @param input input state
     * @return upper auxiliary string
     **/
    FcitxMessages* FcitxInputStateGetAuxUp(FcitxInputState* input);

    /**
     * get auxiliary string displayed in the lower side of input panel
     *
     * @param input input state
     * @return lower auxiliary string
     **/
    FcitxMessages* FcitxInputStateGetAuxDown(FcitxInputState* input);

    /**
     * get preedit string which will be displayed in the input panel with a cursor
     *
     * @param input input state
     * @return preedit string
     **/
    FcitxMessages* FcitxInputStateGetPreedit(FcitxInputState* input);

    /**
     * get preedit string which will be displayed in the client window with a cursor
     *
     * @param input input state
     * @return client preedit string
     **/
    FcitxMessages* FcitxInputStateGetClientPreedit(FcitxInputState* input);

    /**
     * get current raw input buffer size
     *
     * @param input input state
     * @return raw input buffer size
     **/
    int FcitxInputStateGetRawInputBufferSize(FcitxInputState* input);

    /**
     * set current raw input buffer size
     *
     * @param input input state
     * @param size raw input buffer size
     * @return void
     **/
    void FcitxInputStateSetRawInputBufferSize(FcitxInputState* input, int size);

    /**
     * get cursor is visible or not
     *
     * @param input input state
     * @return cursor visibility
     **/
    boolean FcitxInputStateGetShowCursor(FcitxInputState* input);

    /**
     * set cursor is visible or not
     *
     * @param input input state
     * @param showCursor cursor visibility
     * @return void
     **/
    void FcitxInputStateSetShowCursor(FcitxInputState* input, boolean showCursor);

    /**
     * get last char is single char or not
     *
     * @param input input state
     * @return int
     **/
    int FcitxInputStateGetLastIsSingleChar(FcitxInputState* input);

    /**
     * set last char is single char or not
     *
     * @param input input state
     * @param lastIsSingleChar ...
     * @return void
     **/
    void FcitxInputStateSetLastIsSingleChar(FcitxInputState* input, int lastIsSingleChar);

    /**
     * set keycode for current key event
     *
     * @param input input state
     * @param value keycode
     * @return void
     **/
    void FcitxInputStateSetKeyCode( FcitxInputState* input, uint32_t value );

    /**
     * set keysym for current key event
     *
     * @param input input state
     * @param value sym
     * @return void
     **/
    void FcitxInputStateSetKeySym( FcitxInputState* input, uint32_t value );

    /**
     * set keystate for current key state
     *
     * @param input input state
     * @param state key state
     * @return void
     **/
    void FcitxInputStateSetKeyState( FcitxInputState* input, uint32_t state );

    /**
     * get keycode for current key event
     *
     * @param input input state
     * @return uint32_t
     **/
    uint32_t FcitxInputStateGetKeyCode( FcitxInputState* input);

    /**
     * get keysym for current key event
     *
     * @param input input state
     * @return uint32_t
     **/
    uint32_t FcitxInputStateGetKeySym( FcitxInputState* input);

    /**
     * get keystate for current key event
     *
     * @param input input state
     * @return uint32_t
     **/
    uint32_t FcitxInputStateGetKeyState( FcitxInputState* input);

    /**
     * get input method from input method list by name
     *
     * @param instance fcitx instance
     * @param imas from available list or full list
     * @param name input method name
     * @return input method pointer
     **/
    FcitxIM* FcitxInstanceGetIMFromIMList(struct _FcitxInstance* instance, FcitxIMAvailableStatus imas, const char* name);

    /**
     * update current input method list
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxInstanceUpdateIMList(struct _FcitxInstance* instance);

    /**
     * notify surrounding text changed to im
     *
     * @param instance instance
     * @param ic ic
     * @return void
     **/
    void FcitxInstanceNotifyUpdateSurroundingText(struct _FcitxInstance* instance, struct _FcitxInputContext* ic);

    /**
     * an standard key blocker, block all the key that cause cursor move when raw input buffer is not empty.
     *
     * @param input input state
     * @param key keysym
     * @param state key state
     * @return INPUT_RETURN_VALUE
     **/
    INPUT_RETURN_VALUE FcitxStandardKeyBlocker(FcitxInputState* input, FcitxKeySym key, unsigned int state);

    /**
     * set local input method name
     *
     * @param instance Fcitx Instance
     * @param ic input context
     * @param imname im name
     * @return void
     **/
    void FcitxInstanceSetLocalIMName(struct _FcitxInstance* instance, struct _FcitxInputContext* ic, const char* imname);

    /**
     * unregister an im entry
     *
     * @param instance Fcitx Instance
     * @param name imname
     * @return void
     *
     * @since 4.2.6
     */
    void FcitxInstanceUnregisterIM(struct _FcitxInstance* instance, const char* name);

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
