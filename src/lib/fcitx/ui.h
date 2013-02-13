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
 * @file ui.h
 *
 * this file provides the UI component for Fcitx, fcitx doesn't provides any platform specific
 * UI element, all UI are implemented via Fcitx's addon.
 *
 * For input method and module, they can register UI component and get native UI for free from
 * Fcitx.
 *
 * There is two abstract UI element in Fcitx, one is Status, and the other is Menu. Both of them
 * require a unique name in Fcitx. Usually this name is meaningful in order to increase
 * readability.
 *
 * For status, due to backward compatible problem, there are two kinds of status, simple and complex
 * one. Status have three property, short description, long description and icon name. The actual
 * UI depends on specific UI implementation, but usually, status are displayed as a button, with
 * an icon. Icon name can be absolute png file path. All description need to be I18N'ed by addon
 * themselves.
 *
 * For menu, each menu item contains a string, like status, it need to I18N'ed before set. Though
 * there is different type in menu item, DONOT use submenu or divline, they are not supported by
 * specific implementation, and suppose to be only used internally.
 *
 * For display string, there are five field can be used to display string:
 *  - Client Preedit
 *  - Preedit
 *  - AuxUp
 *  - AuxDown
 *  - Candidate words
 *
 * In spite of candidate words, other four fields are FcitxMessages pointer. FcitxMessages is
 * basically a fixed length string array. Each string inside FcitxMessages can have a type.
 */

#ifndef _FCITX_UI_H_
#define _FCITX_UI_H_

#include <stdarg.h>
#include <fcitx/fcitx.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utf8.h>
#include <fcitx-utils/utarray.h>

#ifdef __cplusplus

extern "C" {
#endif

#define MESSAGE_TYPE_COUNT 7 /**< message color type count */

#define MESSAGE_MAX_CHARNUM (150)   /**< Maximum length per-message */

#define MESSAGE_MAX_LENGTH  (MESSAGE_MAX_CHARNUM*UTF8_MAX_LENGTH)   /**< maximum byte per message */

#define MAX_MESSAGE_COUNT 64 /**< maximum message string number */

    struct _FcitxAddon;

    /** fcitx menu item */
    typedef struct _FcitxMenuItem FcitxMenuItem;
    struct _FcitxUIMenu;

    /** fcitx menu state */
    typedef enum _FcitxMenuState {
        MENU_ACTIVE = 0,
        MENU_INACTIVE = 1
    } FcitxMenuState;

    /** fcitx menu item type */
    typedef enum _FcitxMenuItemType {
        MENUTYPE_SIMPLE,
        MENUTYPE_SUBMENU,/**< unfortunately this is only used internal, don't use it in your code */
        MENUTYPE_DIVLINE
    } FcitxMenuItemType;


    /** menu action prototype */
    typedef boolean (*FcitxMenuActionFunction)(struct _FcitxUIMenu *arg, int index);
    /** menu update prototype */
    typedef void    (*FcitxUpdateMenuFunction)(struct _FcitxUIMenu *arg);

    /**
     * a menu entry in a menu.
     **/
    struct _FcitxMenuItem {
        /**
         * The displayed string
         **/
        char *tipstr;
        /**
         * Can be used by ui to mark it's selected or not.
         **/
        int  isselect;
        /**
         * The type of menu shell
         **/
        FcitxMenuItemType type;
        /**
         * the submenu to this entry
         **/
        struct _FcitxUIMenu *subMenu;

        union {
            void* data;
            int dummy[2];
        };

        int padding[14]; /**< padding */
    };

    /**
     * Fcitx Menu Component, a UI doesn't need to support it,
     *        This struct is used by other module to register a menu.
     **/
    struct _FcitxUIMenu {
        /**
         * shell entries for this menu
         **/
        UT_array shell;
        /**
         * menu name, can be displayed on the ui
         **/
        char *name;
        /**
         * you might want to bind the menu on a status icon, but this is only a hint,
         * depends on the ui implementation
         **/
        char *candStatusBind;
        /**
         * update the menu content
         **/
        FcitxUpdateMenuFunction UpdateMenu;
        /**
         * function for process click on a menu entry
         **/
        FcitxMenuActionFunction MenuAction;
        /**
         * private data for this menu
         **/
        void *priv;
        /**
         * ui implementation private
         **/
        void *uipriv[2];
        /**
         * this is sub menu or not
         **/
        boolean isSubMenu;
        /**
         * mark of this menu
         **/
        int mark;

        boolean visible; /**< menu is visible or not */

        int padding[15]; /**< padding */
    };

    /**
     * Fcitx Status icon to be displayed on the UI
     **/
    struct _FcitxUIStatus {
        /**
         * status name, will not displayed on the UI.
         **/
        char *name;
        /**
         * short desription for this status, can be displayed on the UI
         **/
        char *shortDescription;
        /**
         * long description for this status, can be displayed on the UI
         **/
        char *longDescription;
        /**
         * toggle function
         **/
        void (*toggleStatus)(void *arg);
        /**
         * get current value function
         **/
        boolean(*getCurrentStatus)(void *arg);
        /**
         * private data for the UI implementation
         **/
        void *uipriv[2];
        /**
         * extra argument for tooglefunction
         **/
        void* arg;
        /**
         * visible
         */
        boolean visible;

        int padding[16]; /**< padding */
    };


    /**
     * Fcitx Status icon to be displayed on the UI
     **/
    struct _FcitxUIComplexStatus {
        /**
         * status name, will not displayed on the UI.
         **/
        char *name;
        /**
         * short desription for this status, can be displayed on the UI
         **/
        char *shortDescription;
        /**
         * long description for this status, can be displayed on the UI
         **/
        char *longDescription;
        /**
         * toggle function
         **/
        void (*toggleStatus)(void *arg);
        /**
         * get current value function
         **/
        const char*(*getIconName)(void *arg);
        /**
         * private data for the UI implementation
         **/
        void *uipriv[2];
        /**
         * extra argument for tooglefunction
         **/
        void* arg;
        /**
         * visible
         */
        boolean visible;

        int padding[16]; /**< padding */
    };

    struct _FcitxInstance;

    /** message type and flags */
    typedef enum _FcitxMessageType {
        MSG_TYPE_FIRST = 0,
        MSG_TYPE_LAST = 6,
        MSG_TIPS = 0,           /**< Hint Text */
        MSG_INPUT = 1,          /**< User Input */
        MSG_INDEX = 2,          /**< Index Number */
        MSG_CANDIATE_CURSOR = 3,/**< candidate cursor */
        MSG_FIRSTCAND = MSG_CANDIATE_CURSOR,      /**< deprecated */
        MSG_USERPHR = 4,        /**< User Phrase */
        MSG_CODE = 5,           /**< Typed character */
        MSG_OTHER = 6,          /**< Other Text */
        MSG_NOUNDERLINE = (1 << 3), /**< backward compatible, no underline is a flag */
        MSG_HIGHLIGHT = (1 << 4), /**< highlight the preedit */
        MSG_DONOT_COMMIT_WHEN_UNFOCUS = (1 << 5), /**< backward compatible */
        MSG_REGULAR_MASK = 0x7 /**< regular color type mask */
    } FcitxMessageType;

    /** an array of message for display */
    typedef struct _FcitxMessages FcitxMessages;
    /** Fcitx Menu */
    typedef struct _FcitxUIMenu FcitxUIMenu;
    /** Fcitx Status, supports active/inactive */
    typedef struct _FcitxUIStatus FcitxUIStatus;
    /** Fcitx Complex Status, ability to use custom icon */
    typedef struct _FcitxUIComplexStatus FcitxUIComplexStatus;

    /**
     * user interface implementation
     **/
    typedef struct _FcitxUI {
        /**
         * construct function for this ui
         */
        void* (*Create)(struct _FcitxInstance*);
        /**
         * close the input window
         */
        void (*CloseInputWindow)(void *arg);
        /**
         * show the input window
         */
        void (*ShowInputWindow)(void *arg);
        /**
         * move the input window
         */
        void (*MoveInputWindow)(void *arg);
        /**
         * action on update status
         */
        void (*UpdateStatus)(void *arg, FcitxUIStatus*);
        /**
         * action on register status
         */
        void (*RegisterStatus)(void *arg, FcitxUIStatus*);
        /**
         * action on register menu
         */
        void (*RegisterMenu)(void *arg, FcitxUIMenu*);
        /**
         * action on focus
         */
        void (*OnInputFocus)(void *arg);
        /**
         * action on unfocus
         */
        void (*OnInputUnFocus)(void *arg);
        /**
         * action on trigger on
         */
        void (*OnTriggerOn)(void *arg);
        /**
         * action on trigger off
         */
        void (*OnTriggerOff)(void *arg);
        /**
         * display a message is ui support it
         */
        void (*DisplayMessage)(void *arg, char *title, char **msg, int length);
        /**
         * get the main window size if ui support it
         */
        void (*MainWindowSizeHint)(void *arg, int* x, int* y, int* w, int* h);
        /**
         * reload config
         */
        void (*ReloadConfig)(void*);
        /**
         * suspend to switch from/to fallback
         */
        void (*Suspend)(void*);
        /**
         * resume from suspend
         */
        void (*Resume)(void*);

        void (*Destroy)(void*); /**< destroy user interface addon */
        void (*RegisterComplexStatus)(void*, FcitxUIComplexStatus*); /**< register complex status */
        void (*UpdateComplexStatus)(void *arg, FcitxUIComplexStatus*); /**< register complext status */
        void (*padding3)(void*); /**< padding */
    } FcitxUI;

    /**
     * load user interface module
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUILoad(struct _FcitxInstance* instance);

    /**
     * init messages
     *
     * @return FcitxMessages*
     **/
    FcitxMessages* FcitxMessagesNew();

    /**
     * add a message string at last
     *
     * @param message message
     * @param type message type
     * @param fmt  printf fmt
     * @param  ...
     * @return void
     **/
    void FcitxMessagesAddMessageAtLast(FcitxMessages* message, FcitxMessageType type, const char *fmt, ...);

    /**
     * add a message string at last, cat strings version
     *
     * @param message message
     * @param type message type
     * @return void
     **/
    void FcitxMessagesAddMessageVStringAtLast(FcitxMessages *message,
                                                  FcitxMessageType type,
                                                  size_t n, const char **strs);
#define FcitxMessagesAddMessageStringsAtLast(message, type, strs...) do { \
        const char *__msg_str_lst[] = {strs};                           \
        size_t __msg_str_count = sizeof(__msg_str_lst) / sizeof(char*); \
        FcitxMessagesAddMessageVStringAtLast(message, type,         \
                                                 __msg_str_count,       \
                                                 __msg_str_lst);        \
    } while (0)

    /**
     * set a message string at position
     *
     * @param message message
     * @param position position
     * @param type message type
     * @param fmt printf fmt
     * @param  ...
     * @return void
     **/
    void FcitxMessagesSetMessage(FcitxMessages* message, int position, int type, const char* fmt, ...);
    /**
     * set only message string
     *
     * @param message message
     * @param position position
     * @param fmt printf format
     * @param  ...
     * @return void
     **/
    void FcitxMessagesSetMessageText(FcitxMessages* message, int position, const char* fmt, ...);

    /**
     * set only message string, it doesn't insert string into position,
     * it concat all the string in strs, then replace the string at
     * position
     *
     * @param message message
     * @param position position
     * @param n strs length
     * @param strs string array
     * @return void
     *
     * @since 4.2.7
     **/
    void FcitxMessagesSetMessageTextVString(FcitxMessages *message,
                                            int position, size_t n,
                                            const char **strs);
#define FcitxMessagesSetMessageTextStrings(message, position, strs...) do { \
        const char *__msg_str_lst[] = {strs};                           \
        size_t __msg_str_count = sizeof(__msg_str_lst) / sizeof(char*); \
        FcitxMessagesSetMessageTextVString(message, position,       \
                                               __msg_str_count,         \
                                               __msg_str_lst);          \
    } while (0)

    /**
     * concat a string to message string at position
     *
     * @param message message
     * @param position position
     * @param text string
     * @return void
     **/
    void FcitxMessagesMessageConcat(FcitxMessages* message, int position, const char* text);
    /**
     * concat a string to message string at last
     *
     * @param message message
     * @param text string
     * @return void
     **/
    void FcitxMessagesMessageConcatLast(FcitxMessages* message, const char* text);
    /**
     * set message string vprintf version
     *
     * @param message message
     * @param position position
     * @param type message type
     * @param fmt printf format
     * @param ap arguments
     * @return void
     **/
    void FcitxMessagesSetMessageV(FcitxMessages* message, int position, int type, const char* fmt, va_list ap);
    /**
     * set message string cat strings version
     *
     * @param message message
     * @param position position
     * @param type message type
     * @param n number of strings
     * @param strs list of strings
     * @return void
     **/
    void FcitxMessagesSetMessageStringsReal(FcitxMessages *message,
                                            int position, int type,
                                            size_t n, const char **strs);
#define FcitxMessagesSetMessageStrings(message, position, type, strs...) do { \
        const char *__msg_str_lst[] = {strs};                           \
        size_t __msg_str_count = sizeof(__msg_str_lst) / sizeof(char*); \
        FcitxMessagesSetMessageStringsReal(message, position, type,     \
                                           __msg_str_count, __msg_str_lst); \
    } while (0)

    /**
     * set message count
     *
     * @param m message
     * @param s count
     * @return void
     **/
    void FcitxMessagesSetMessageCount(FcitxMessages* m, int s);
    /**
     * get message count
     *
     * @param m message
     * @return int
     **/
    int FcitxMessagesGetMessageCount(FcitxMessages* m);
    /**
     * get message string at index
     *
     * @param m message
     * @param index index
     * @return char*
     **/
    char* FcitxMessagesGetMessageString(FcitxMessages* m, int index);
    /**
     * get message type at index, will filter non regular type
     *
     * @param m message
     * @param index index
     * @return FcitxMessageType
     **/
    FcitxMessageType FcitxMessagesGetMessageType(FcitxMessages* m, int index);

    /**
     * get message type at index, will not filter non regular type
     *
     * @param m message
     * @param index index
     * @return FcitxMessageType
     *
     * @see FcitxMessagesGetMessageType
     * @since 4.2.1
     **/
    FcitxMessageType FcitxMessagesGetClientMessageType(FcitxMessages* m, int index);
    /**
     * check whether message is changed
     *
     * @param m message
     * @return boolean
     **/
    boolean FcitxMessagesIsMessageChanged(FcitxMessages* m);
    /**
     * set message is changed or not
     *
     * @param m message
     * @param changed changed or not
     * @return void
     **/
    void FcitxMessagesSetMessageChanged(FcitxMessages* m, boolean changed);
    /**
     * add a new menu shell
     *
     * @param menu menu
     * @param string menu text
     * @param type menu type
     * @param subMenu submenu pointer
     * @return void
     **/
    void FcitxMenuAddMenuItem(FcitxUIMenu* menu, const char* string, FcitxMenuItemType type, FcitxUIMenu* subMenu);
    /**
     * add a new menu shell
     *
     * @param menu menu
     * @param string menu text
     * @param type menu type
     * @param subMenu submenu pointer
     * @return void
     **/
    void FcitxMenuAddMenuItemWithData(FcitxUIMenu* menu, const char* string, FcitxMenuItemType type, FcitxUIMenu* subMenu, void* arg);

    /**
     * clear all menu shell
     *
     * @param menu menu
     * @return void
     **/
    void FcitxMenuClear(FcitxUIMenu* menu);

    /**
     * move input to cursor position
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIMoveInputWindow(struct _FcitxInstance* instance);

    /**
     * close input window
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUICloseInputWindow(struct _FcitxInstance* instance);
    /**
     * refresh a status info, since it changed outside the toggle function
     *
     * @param instance fcitx instance
     * @param name status name
     * @return void
     **/
    void FcitxUIRefreshStatus(struct _FcitxInstance* instance, const char* name);
    /**
     * toggle a user interface status
     *
     * @param instance fcitx instance
     * @param name status name
     * @return void
     **/
    void FcitxUIUpdateStatus(struct _FcitxInstance* instance, const char* name);
    /**
     * register a new ui status
     *
     * @param instance fcitx instance
     * @param arg private data, pass to callback
     * @param name name
     * @param shortDesc short description
     * @param longDesc long description
     * @param toggleStatus callback for toggle status
     * @param getStatus get current status
     * @return void
     **/
    void FcitxUIRegisterStatus(struct _FcitxInstance* instance,
                               void* arg,
                               const char* name,
                               const char* shortDesc,
                               const char* longDesc,
                               void (*toggleStatus)(void *arg),
                               boolean(*getStatus)(void *arg));
    /**
     * register a new ui status
     *
     * @param instance fcitx instance
     * @param arg private data, pass to callback
     * @param name name
     * @param shortDesc short description
     * @param longDesc long description
     * @param toggleStatus callback for toggle status
     * @param getIconName get current icon name
     * @return void
     **/
    void FcitxUIRegisterComplexStatus(struct _FcitxInstance* instance,
                                      void* arg,
                                      const char* name,
                                      const char* shortDesc,
                                      const char* longDesc,
                                      void (*toggleStatus)(void *arg),
                                      const char*(*getIconName)(void *arg));
    /**
     * register a new menu
     *
     * @param instance fcitx instance
     * @param menu menu
     * @return void
     **/
    void FcitxUIRegisterMenu(struct _FcitxInstance* instance, FcitxUIMenu* menu);

    /**
     * process focus in event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnInputFocus(struct _FcitxInstance* instance);

    /**
     * process focus out event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnInputUnFocus(struct _FcitxInstance* instance);

    /**
     * process trigger on event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnTriggerOn(struct _FcitxInstance* instance);

    /**
     * process trigger off event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnTriggerOff(struct _FcitxInstance* instance);

    /**
     * if user interface support, display a message window on the screen
     *
     * @param instance fcitx instance
     * @param title window title
     * @param msg message
     * @param length length or message
     * @return void
     **/
    void FcitxUIDisplayMessage(struct _FcitxInstance *instance, char *title, char **msg, int length);

    /**
     * get status by status name
     *
     * @param instance fcitx instance
     * @param name status name
     * @return FcitxUIStatus*
     **/
    FcitxUIStatus *FcitxUIGetStatusByName(struct _FcitxInstance* instance, const char* name);

    /**
     * get menu by status name
     *
     * @param instance fcitx instance
     * @param name status name
     * @return FcitxUIMenu*
     *
     * @since 4.2.1
     **/
    FcitxUIMenu* FcitxUIGetMenuByStatusName(struct _FcitxInstance* instance, const char* name);

    /**
     * get status by status name
     *
     * @param instance fcitx instance
     * @param name status name
     * @return FcitxUIStatus*
     **/
    FcitxUIComplexStatus *FcitxUIGetComplexStatusByName(struct _FcitxInstance* instance, const char* name);


    /**
     * set visibility for a status icon
     *
     * @param instance fcitx instance
     * @param name name
     * @param visible visibility
     * @return void
     **/
    void FcitxUISetStatusVisable(struct _FcitxInstance* instance, const char* name, boolean visible);

    /**
     * @brief set string for a status icon
     *
     * @param instance fcitx instance
     * @param name name
     * @param shortDesc short description
     * @param longDesc long description
     * @return void
     *
     * @since 4.2.1
     **/
    void FcitxUISetStatusString(struct _FcitxInstance* instance, const char* name, const char* shortDesc, const char* longDesc);

    /**
     * update menu shell of a menu
     *
     * @param menu menu
     * @return void
     **/
    void FcitxMenuUpdate(FcitxUIMenu* menu);

    /**
     * check point is in rectangle or not
     *
     * @param x0 point x
     * @param y0 point y
     * @param x1 rectangle x
     * @param y1 rectangle y
     * @param w rectangle width
     * @param h rectangle height
     * @return boolean
     **/
    boolean FcitxUIIsInBox(int x0, int y0, int x1, int y1, int w, int h);

    /**
     * check user interface support main window or not
     *
     * @param instance fcitx instance
     * @return boolean
     **/
    boolean FcitxUISupportMainWindow(struct _FcitxInstance* instance);

    /**
     * get main window geometry property if there is a main window
     *
     * @param instance fcitx instance
     * @param x x
     * @param y y
     * @param w w
     * @param h h
     * @return void
     **/
    void FcitxUIGetMainWindowSize(struct _FcitxInstance* instance, int* x, int* y, int* w, int* h);

    /**
     * convert new messages to old up and down style messages, return the new cursos pos
     *
     * @param instance fcitx instance
     * @param msgUp messages up
     * @param msgDown messages up
     * @return int
     **/
    int FcitxUINewMessageToOldStyleMessage(struct _FcitxInstance* instance, FcitxMessages* msgUp, FcitxMessages* msgDown);

    /**
     * convert messages to pure c string
     *
     * @param messages messages
     * @return char*
     **/
    char* FcitxUIMessagesToCString(FcitxMessages* messages);

    /**
     * convert candidate words to a string which can direct displayed
     *
     * @param instance fcitx instance
     * @return char*
     **/

    char* FcitxUICandidateWordToCString(struct _FcitxInstance* instance);


    /**
     * @brief commit current preedit string if any
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUICommitPreedit(struct _FcitxInstance* instance);

    /**
     * mark input window should update
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIUpdateInputWindow(struct _FcitxInstance* instance);


    /**
     * User interface should switch to the fallback
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUISwitchToFallback(struct _FcitxInstance* instance);

    /**
     * User interface should resume from the fallback
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIResumeFromFallback(struct _FcitxInstance* instance);

    /**
     * checkk a user interface is fallback or not
     *
     * @param instance fcitx instance
     * @param addon addon
     * @return boolean
     **/
    boolean FcitxUIIsFallback(struct _FcitxInstance* instance, struct _FcitxAddon* addon);

    /**
     * initialize a menu pointer
     *
     * @param menu menu
     * @return void
     **/
    void FcitxMenuInit(FcitxUIMenu* menu);

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */

// kate: indent-mode cstyle; space-indent on; indent-width 0;
