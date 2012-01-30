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

#define INPUTWND_START_POS_DOWN 8
#define MESSAGE_MAX_CHARNUM (150)   //输入条上显示的最长字数

#define MESSAGE_MAX_LENGTH  (MESSAGE_MAX_CHARNUM*UTF8_MAX_LENGTH)   //输入条上显示的最长长度，以字符计

    /* 将输入条上显示的内容分为以下几类 */
#define MESSAGE_TYPE_COUNT  7

#define MAX_MESSAGE_COUNT 64

    typedef struct _FcitxMenuItem FcitxMenuItem;
    struct _FcitxUIMenu;

    typedef enum _FcitxMenuState {
        MENU_ACTIVE = 0,
        MENU_INACTIVE = 1
    } FcitxMenuState;

    typedef enum _FcitxMenuItemType {
        MENUTYPE_SIMPLE,
        MENUTYPE_SUBMENU,
        MENUTYPE_DIVLINE
    } FcitxMenuItemType;
    
    
    typedef boolean (*FcitxMenuActionFunction)(struct _FcitxUIMenu *arg, int index);
    typedef void    (*FcitxUpdateMenuFunction)(struct _FcitxUIMenu *arg);

    /**
     * @brief a menu entry in a menu.
     **/
    struct _FcitxMenuItem {
        /**
         * @brief The displayed string
         **/
        char *tipstr;
        /**
         * @brief Can be used by ui to mark it's selected or not.
         **/
        int  isselect;
        /**
         * @brief The type of menu shell
         **/
        FcitxMenuItemType type;
        /**
         * @brief the submenu to this entry
         **/
        struct _FcitxUIMenu *subMenu;
        
        int padding[16];
    };

    /**
     * @brief Fcitx Menu Component, a UI doesn't need to support it,
     *        This struct is used by other module to register a menu.
     **/
    struct _FcitxUIMenu {
        /**
         * @brief shell entries for this menu
         **/
        UT_array shell;
        /**
         * @brief menu name, can be displayed on the ui
         **/
        char *name;
        /**
         * @brief you might want to bind the menu on a status icon, but this is only a hint,
         * depends on the ui implementation
         **/
        char *candStatusBind;
        /**
         * @brief update the menu content
         **/
        FcitxUpdateMenuFunction UpdateMenu;
        /**
         * @brief function for process click on a menu entry
         **/
        FcitxMenuActionFunction MenuAction;
        /**
         * @brief private data for this menu
         **/
        void *priv;
        /**
         * @brief ui implementation private
         **/
        void *uipriv[2];
        /**
         * @brief this is sub menu or not
         **/
        boolean isSubMenu;
        /**
         * @brief mark of this menu
         **/
        int mark;
        
        int padding[16];
    };

    /**
     * @brief Fcitx Status icon to be displayed on the UI
     **/
    struct _FcitxUIStatus {
        /**
         * @brief status name, will not displayed on the UI.
         **/
        char *name;
        /**
         * @brief short desription for this status, can be displayed on the UI
         **/
        char *shortDescription;
        /**
         * @brief long description for this status, can be displayed on the UI
         **/
        char *longDescription;
        /**
         * @brief toogle function
         **/
        void (*toggleStatus)(void *arg);
        /**
         * @brief get current value function
         **/
        boolean(*getCurrentStatus)(void *arg);
        /**
         * @brief private data for the UI implementation
         **/
        void *uipriv[2];
        /**
         * @brief extra argument for tooglefunction
         **/
        void* arg;
        /**
         * @brief visible
         */
        boolean visible;
        
        int padding[16];
    };

    typedef enum _FcitxUIFlag {
        UI_NONE = 0,
        UI_MOVE = (1 << 1),
        UI_UPDATE = (1 << 2),
    } FcitxUIFlag;

    struct _FcitxInstance;
    typedef enum _FcitxMessageType {
        MSG_TYPE_FIRST = 0,
        MSG_TYPE_LAST = 6,
        MSG_TIPS = 0,           //提示文本
        MSG_INPUT = 1,          //用户的输入
        MSG_INDEX = 2,          //候选字前面的序号
        MSG_FIRSTCAND = 3,      //第一个候选字
        MSG_USERPHR = 4,        //用户词组
        MSG_CODE = 5,           //显示的编码
        MSG_OTHER = 6,          //其它文本
    } FcitxMessageType;

    typedef struct _FcitxMessages FcitxMessages;
    struct _FcitxAddon;

    typedef struct _FcitxUIMenu FcitxUIMenu;
    typedef struct _FcitxUIStatus FcitxUIStatus;

    /**
     * @brief user interface implementation
     **/
    typedef struct _FcitxUI {
        /**
         * @brief construct function for this ui
         */
        void* (*Create)(struct _FcitxInstance*);
        /**
         * @brief close the input window
         */
        void (*CloseInputWindow)(void *arg);
        /**
         * @brief show the input window
         */
        void (*ShowInputWindow)(void *arg);
        /**
         * @brief move the input window
         */
        void (*MoveInputWindow)(void *arg);
        /**
         * @brief action on update status
         */
        void (*UpdateStatus)(void *arg, FcitxUIStatus*);
        /**
         * @brief action on register status
         */
        void (*RegisterStatus)(void *arg, FcitxUIStatus*);
        /**
         * @brief action on register menu
         */
        void (*RegisterMenu)(void *arg, FcitxUIMenu*);
        /**
         * @brief action on focus
         */
        void (*OnInputFocus)(void *arg);
        /**
         * @brief action on unfocus
         */
        void (*OnInputUnFocus)(void *arg);
        /**
         * @brief action on trigger on
         */
        void (*OnTriggerOn)(void *arg);
        /**
         * @brief action on trigger off
         */
        void (*OnTriggerOff)(void *arg);
        /**
         * @brief display a message is ui support it
         */
        void (*DisplayMessage)(void *arg, char *title, char **msg, int length);
        /**
         * @brief get the main window size if ui support it
         */
        void (*MainWindowSizeHint)(void *arg, int* x, int* y, int* w, int* h);
        /**
         * @brief reload config
         */
        void (*ReloadConfig)(void*);
        /**
         * @brief suspend to switch from/to fallback
         */
        void (*Suspend)(void*);
        /**
         * @brief resume from suspend
         */
        void (*Resume)(void*);

        void (*Destroy)(void*);
        void (*padding1)(void*);
        void (*padding2)(void*);
        void (*padding3)(void*);
    } FcitxUI;

    /**
     * @brief load user interface module
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUILoad(struct _FcitxInstance* instance);

    /**
     * @brief init messages
     *
     * @return FcitxMessages*
     **/
    FcitxMessages* FcitxMessagesNew();

    /**
     * @brief add a message string at last
     *
     * @param message message
     * @param type message type
     * @param fmt  printf fmt
     * @param  ...
     * @return void
     **/
    void FcitxMessagesAddMessageAtLast(FcitxMessages* message, FcitxMessageType type, const char *fmt, ...);

    /**
     * @brief set a message string at position
     *
     * @param message message
     * @param position position
     * @param type message type
     * @param fmt printf fmt
     * @param  ...
     * @return void
     **/
    void FcitxMessagesSetMessage(FcitxMessages* message, int position, FcitxMessageType type, const char* fmt, ...);
    /**
     * @brief set only message string
     *
     * @param message message
     * @param position position
     * @param fmt printf format
     * @param  ...
     * @return void
     **/
    void FcitxMessagesSetMessageText(FcitxMessages* message, int position, const char* fmt, ...);
    /**
     * @brief concat a string to message string at position
     *
     * @param message message
     * @param position position
     * @param text string
     * @return void
     **/
    void FcitxMessagesMessageConcat(FcitxMessages* message, int position, const char* text);
    /**
     * @brief concat a string to message string at last
     *
     * @param message message
     * @param text string
     * @return void
     **/
    void FcitxMessagesMessageConcatLast(FcitxMessages* message, const char* text);
    /**
     * @brief set message string vprintf version
     *
     * @param message message
     * @param position position
     * @param type message type
     * @param fmt printf format
     * @param ap arguments
     * @return void
     **/
    void FcitxMessagesSetMessageV(FcitxMessages* message, int position, FcitxMessageType type, const char* fmt, va_list ap);
    /**
     * @brief set message count
     *
     * @param m message
     * @param s count
     * @return void
     **/
    void FcitxMessagesSetMessageCount(FcitxMessages* m, int s);
    /**
     * @brief get message count
     *
     * @param m message
     * @return int
     **/
    int FcitxMessagesGetMessageCount(FcitxMessages* m);
    /**
     * @brief get message string at index
     *
     * @param m message
     * @param index index
     * @return char*
     **/
    char* FcitxMessagesGetMessageString(FcitxMessages* m, int index);
    /**
     * @brief get message type at index
     *
     * @param m message
     * @param index index
     * @return FcitxMessageType
     **/
    FcitxMessageType FcitxMessagesGetMessageType(FcitxMessages* m, int index);
    /**
     * @brief check whether message is changed
     *
     * @param m message
     * @return boolean
     **/
    boolean FcitxMessagesIsMessageChanged(FcitxMessages* m);
    /**
     * @brief set message is changed or not
     *
     * @param m message
     * @param changed changed or not
     * @return void
     **/
    void FcitxMessagesSetMessageChanged(FcitxMessages* m, boolean changed);
    /**
     * @brief add a new menu shell
     *
     * @param menu menu
     * @param string menu text
     * @param type menu type
     * @param subMenu submenu pointer
     * @return void
     **/
    void FcitxMenuAddMenuItem(FcitxUIMenu* menu, char* string, FcitxMenuItemType type, FcitxUIMenu* subMenu);

    /**
     * @brief clear all menu shell
     *
     * @param menu menu
     * @return void
     **/
    void FcitxMenuClear(FcitxUIMenu* menu);

    /**
     * @brief move input to cursor position
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIMoveInputWindow(struct _FcitxInstance* instance);

    /**
     * @brief close input window
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUICloseInputWindow(struct _FcitxInstance* instance);
    /**
     * @brief toggle a user interface status
     *
     * @param instance fcitx instance
     * @param name status name
     * @return void
     **/
    void FcitxUIUpdateStatus(struct _FcitxInstance* instance, const char* name);
    /**
     * @brief register a new ui status
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
     * @brief register a new menu
     *
     * @param instance fcitx instance
     * @param menu menu
     * @return void
     **/
    void FcitxUIRegisterMenu(struct _FcitxInstance* instance, FcitxUIMenu* menu);

    /**
     * @brief process focus in event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnInputFocus(struct _FcitxInstance* instance);

    /**
     * @brief process focus out event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnInputUnFocus(struct _FcitxInstance* instance);

    /**
     * @brief process trigger on event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnTriggerOn(struct _FcitxInstance* instance);

    /**
     * @brief process trigger off event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIOnTriggerOff(struct _FcitxInstance* instance);

    /**
     * @brief if user interface support, display a message window on the screen
     *
     * @param instance fcitx instance
     * @param title window title
     * @param msg message
     * @param length length or message
     * @return void
     **/
    void FcitxUIDisplayMessage(struct _FcitxInstance *instance, char *title, char **msg, int length);

    /**
     * @brief get status by status name
     *
     * @param instance fcitx instance
     * @param name status name
     * @return FcitxUIStatus*
     **/
    FcitxUIStatus *FcitxUIGetStatusByName(struct _FcitxInstance* instance, const char* name);
    
    
    /**
     * @brief set visibility for a status icon
     *
     * @param instance fcitx instance
     * @param name name
     * @param visible visibility
     * @return void
     **/
    void FcitxUISetStatusVisable(struct _FcitxInstance* instance, const char* name, boolean visible);

    /**
     * @brief update menu shell of a menu
     *
     * @param menu menu
     * @return void
     **/
    void FcitxMenuUpdate(FcitxUIMenu* menu);

    /**
     * @brief check point is in rectangle or not
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
     * @brief check user interface support main window or not
     *
     * @param instance fcitx instance
     * @return boolean
     **/
    boolean FcitxUISupportMainWindow(struct _FcitxInstance* instance);

    /**
     * @brief get main window geometry property if there is a main window
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
     * @brief convert new messages to old up and down style messages, return the new cursos pos
     *
     * @param instance fcitx instance
     * @param msgUp messages up
     * @param msgDown messages up
     * @return int
     **/
    int FcitxUINewMessageToOldStyleMessage(struct _FcitxInstance* instance, FcitxMessages* msgUp, FcitxMessages* msgDown);

    /**
     * @brief convert messages to pure c string
     *
     * @param messages messages
     * @return char*
     **/
    char* FcitxUIMessagesToCString(FcitxMessages* messages);

    /**
     * @brief convert candidate words to a string which can direct displayed
     *
     * @param instance fcitx instance
     * @return char*
     **/

    char* FcitxUICandidateWordToCString(struct _FcitxInstance* instance);

    /**
     * @brief mark input window should update
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIUpdateInputWindow(struct _FcitxInstance* instance);


    /**
     * @brief User interface should switch to the fallback
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUISwitchToFallback(struct _FcitxInstance* instance);

    /**
     * @brief User interface should resume from the fallback
     *
     * @param instance fcitx instance
     * @return void
     **/
    void FcitxUIResumeFromFallback(struct _FcitxInstance* instance);

    boolean FcitxUIIsFallback(struct _FcitxInstance* instance, struct _FcitxAddon* addon);

    void FcitxMenuInit(FcitxUIMenu* menu);

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
