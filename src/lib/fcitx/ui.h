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

#ifndef _FCITX_UI_H_
#define _FCITX_UI_H_

#include <fcitx/fcitx.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx-utils/utf8.h>
#include <fcitx-utils/utarray.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INPUTWND_START_POS_DOWN	8
#define MESSAGE_MAX_CHARNUM	(150)	//输入条上显示的最长字数

#define MESSAGE_MAX_LENGTH	(MESSAGE_MAX_CHARNUM*UTF8_MAX_LENGTH)	//输入条上显示的最长长度，以字符计

    /* 将输入条上显示的内容分为以下几类 */
#define MESSAGE_TYPE_COUNT	7

    typedef enum _FcitxUIFlag {
        UI_NONE = 0,
        UI_MOVE = (1 << 1),
        UI_UPDATE = (1 << 2),
    } FcitxUIFlag;

    struct _FcitxInstance;
    typedef enum _MSG_TYPE {
        MSG_TIPS = 0,			//提示文本
        MSG_INPUT = 1,			//用户的输入
        MSG_INDEX = 2,			//候选字前面的序号
        MSG_FIRSTCAND = 3,		//第一个候选字
        MSG_USERPHR = 4,		//用户词组
        MSG_CODE = 5,			//显示的编码
        MSG_OTHER = 6,			//其它文本
    } MSG_TYPE;

#define MAX_MESSAGE_COUNT 64
    typedef struct _MESSAGE MESSAGE;
    typedef struct _Messages Messages;

#define MESSAGE_IS_NOT_EMPTY (messageUp.msgCount || messageDown.msgCount)
#define MESSAGE_IS_EMPTY (!MESSAGE_IS_NOT_EMPTY)
#define MESSAGE_TYPE_IS(msg, t) ((msg).type == (t))
#define LAST_MESSAGE(m) ((m).msg[(m).msgCount - 1])
#define DecMessageCount(m) \
    do { \
        if ((m)->msgCount > 0) \
            ((m)->msgCount--); \
        (m)->changed = True; \
    } while(0)

#define MAX_STATUS_NAME 32
#define MAX_MENU_STRING_LENGTH 32
#define MAX_STATUS_SDESC 32
#define MAX_STATUS_LDESC 32

    /**
     * @brief Fcitx Status icon to be displayed on the UI
     **/
    typedef struct _FcitxUIStatus {
        /**
         * @brief status name, will not displayed on the UI.
         **/
        char name[MAX_STATUS_NAME + 1];
        /**
         * @brief short desription for this status, can be displayed on the UI
         **/
        char shortDescription[MAX_STATUS_SDESC + 1];
        /**
         * @brief long description for this status, can be displayed on the UI
         **/
        char longDescription[MAX_STATUS_LDESC + 1];
        /**
         * @brief toogle function
         **/
        void (*toggleStatus)(void *arg);
        /**
         * @brief get current value function
         **/
        boolean (*getCurrentStatus)(void *arg);
        /**
         * @brief private data for the UI implementation
         **/
        void *priv;
        /**
         * @brief extra argument for tooglefunction
         **/
        void* arg;
    } FcitxUIStatus;

    typedef enum _MenuState
    {
        MENU_ACTIVE = 0,
        MENU_INACTIVE = 1
    } MenuState;

    typedef enum _MenuShellType
    {
        MENUTYPE_SIMPLE,
        MENUTYPE_SUBMENU,
        MENUTYPE_DIVLINE
    } MenuShellType;

    struct _FcitxUIMenu;

    /**
     * @brief a menu entry in a menu.
     **/
    typedef struct _MenuShell
    {
        /**
         * @brief The displayed string
         **/
        char tipstr[MAX_MENU_STRING_LENGTH + 1];
        /**
         * @brief Can be used by ui to mark it's selected or not.
         **/
        int  isselect;
        /**
         * @brief The type of menu shell
         **/
        MenuShellType type;
        /**
         * @brief the submenu to this entry
         **/
        struct _FcitxUIMenu *subMenu;
    } MenuShell;

    typedef boolean (*MenuActionFunction)(struct _FcitxUIMenu *arg, int index);
    typedef void (*UpdateMenuShellFunction)(struct _FcitxUIMenu *arg);

    /**
     * @brief Fcitx Menu Component, a UI doesn't need to support it,
     *        This struct is used by other module to register a menu.
     **/
    typedef struct _FcitxUIMenu {
        /**
         * @brief shell entries for this menu
         **/
        UT_array shell;
        /**
         * @brief menu name, can be displayed on the ui
         **/
        char name[MAX_MENU_STRING_LENGTH + 1];
        /**
         * @brief you might want to bind the menu on a status icon, but this is only a hint,
         * depends on the ui implementation
         **/
        char candStatusBind[MAX_STATUS_NAME + 1];
        /**
         * @brief update the menu content
         **/
        UpdateMenuShellFunction UpdateMenuShell;
        /**
         * @brief function for process click on a menu entry
         **/
        MenuActionFunction MenuAction;
        /**
         * @brief private data for this menu
         **/
        void *priv;
        /**
         * @brief ui implementation private
         **/
        void *uipriv;
        /**
         * @brief this is sub menu or not
         **/
        boolean isSubMenu;
        /**
         * @brief mark of this menu
         **/
        int mark;
    } FcitxUIMenu;

    /**
     * @brief user interface implementation
     **/
    typedef struct _FcitxUI
    {
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
        void (*UpdateStatus)(void *arg, FcitxUIStatus* );
        /**
         * @brief action on register status
         */
        void (*RegisterStatus)(void *arg, FcitxUIStatus* );
        /**
         * @brief action on register menu
         */
        void (*RegisterMenu)(void *arg, FcitxUIMenu* );
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
    } FcitxUI;

    /**
     * @brief load user interface module
     *
     * @param instance fcitx instance
     * @return void
     **/
    void LoadUserInterface(struct _FcitxInstance* instance);

    /**
     * @brief init messages
     *
     * @return Messages*
     **/
    Messages* InitMessages();

    /**
     * @brief add a message string at last
     *
     * @param message message
     * @param type message type
     * @param fmt  printf fmt
     * @param  ...
     * @return void
     **/
    void AddMessageAtLast(Messages* message, MSG_TYPE type, const char *fmt, ...);

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
    void SetMessage(Messages* message, int position, MSG_TYPE type, const char* fmt, ...);
    /**
     * @brief set only message string
     *
     * @param message message
     * @param position position
     * @param fmt printf format
     * @param  ...
     * @return void
     **/
    void SetMessageText(Messages* message, int position, const char* fmt, ...);
    /**
     * @brief concat a string to message string at position
     *
     * @param message message
     * @param position position
     * @param text string
     * @return void
     **/
    void MessageConcat(Messages* message, int position, const char* text);
    /**
     * @brief concat a string to message string at last
     *
     * @param message message
     * @param text string
     * @return void
     **/
    void MessageConcatLast(Messages* message, const char* text);
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
    void SetMessageV(Messages* message, int position, MSG_TYPE type, const char* fmt, va_list ap);
    /**
     * @brief set message count
     *
     * @param m message
     * @param s count
     * @return void
     **/
    void SetMessageCount(Messages* m, int s);
    /**
     * @brief get message count
     *
     * @param m message
     * @return int
     **/
    int GetMessageCount(Messages* m);
    /**
     * @brief get message string at index
     *
     * @param m message
     * @param index index
     * @return char*
     **/
    char* GetMessageString(Messages* m, int index);
    /**
     * @brief get message type at index
     *
     * @param m message
     * @param index index
     * @return MSG_TYPE
     **/
    MSG_TYPE GetMessageType(Messages* m, int index);
    /**
     * @brief check whether message is changed
     *
     * @param m message
     * @return boolean
     **/
    boolean IsMessageChanged(Messages* m);
    /**
     * @brief set message is changed or not
     *
     * @param m message
     * @param changed changed or not
     * @return void
     **/
    void SetMessageChanged(Messages* m, boolean changed);
    /**
     * @brief add a new menu shell
     *
     * @param menu menu
     * @param string menu text
     * @param type menu type
     * @param subMenu submenu pointer
     * @return void
     **/
    void AddMenuShell(FcitxUIMenu* menu, char* string, MenuShellType type, FcitxUIMenu* subMenu);
    /**
     * @brief clear all menu shell
     *
     * @param menu menu
     * @return void
     **/
    void ClearMenuShell(FcitxUIMenu* menu);

    /**
     * @brief move input to cursor position
     *
     * @param instance fcitx instance
     * @return void
     **/
    void MoveInputWindow(struct _FcitxInstance* instance);

    /**
     * @brief close input window
     *
     * @param instance fcitx instance
     * @return void
     **/
    void CloseInputWindow(struct _FcitxInstance* instance);
    /**
     * @brief toggle a user interface status
     *
     * @param instance fcitx instance
     * @param name status name
     * @return void
     **/
    void UpdateStatus(struct _FcitxInstance* instance, const char* name);
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
    void RegisterStatus(struct _FcitxInstance* instance,
                        void* arg,
                        const char* name,
                        const char* shortDesc,
                        const char* longDesc,
                        void (*toggleStatus)(void *arg),
                        boolean (*getStatus)(void *arg));
    /**
     * @brief register a new menu
     *
     * @param instance fcitx instance
     * @param menu menu
     * @return void
     **/
    void RegisterMenu(struct _FcitxInstance* instance, FcitxUIMenu* menu);

    /**
     * @brief process focus in event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void OnInputFocus(struct _FcitxInstance* instance);

    /**
     * @brief process focus out event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void OnInputUnFocus(struct _FcitxInstance* instance);

    /**
     * @brief process trigger on event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void OnTriggerOn(struct _FcitxInstance* instance);

    /**
     * @brief process trigger off event
     *
     * @param instance fcitx instance
     * @return void
     **/
    void OnTriggerOff(struct _FcitxInstance* instance);

    /**
     * @brief if user interface support, display a message window on the screen
     *
     * @param instance fcitx instance
     * @param title window title
     * @param msg message
     * @param length length or message
     * @return void
     **/
    void DisplayMessage(struct _FcitxInstance *instance, char *title, char **msg, int length);

    /**
     * @brief get status by status name
     *
     * @param instance fcitx instance
     * @param name status name
     * @return FcitxUIStatus*
     **/
    FcitxUIStatus *GetUIStatus(struct _FcitxInstance* instance, const char* name);

    /**
     * @brief update menu shell of a menu
     *
     * @param menu menu
     * @return void
     **/
    void UpdateMenuShell(FcitxUIMenu* menu);

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
    boolean IsInBox(int x0, int y0, int x1, int y1, int w, int h);

    /**
     * @brief check user interface support main window or not
     *
     * @param instance fcitx instance
     * @return boolean
     **/
    boolean UISupportMainWindow(struct _FcitxInstance* instance);

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
    void GetMainWindowSize(struct _FcitxInstance* instance, int* x, int* y, int* w, int* h);

    /**
     * @brief convert new messages to old up and down style messages, return the new cursos pos
     *
     * @param instance fcitx instance
     * @param msgUp messages up
     * @param msgDown messages up
     * @return int
     **/
    int NewMessageToOldStyleMessage(struct _FcitxInstance* instance, Messages* msgUp, Messages* msgDown);

    /**
     * @brief convert messages to pure c string
     *
     * @param messages messages
     * @return char*
     **/
    char* MessagesToCString(Messages* messages);

    /**
     * @brief convert candidate words to a string which can direct displayed
     *
     * @param instance fcitx instance
     * @return char*
     **/

    char* CandidateWordToCString(struct _FcitxInstance* instance);

    /**
     * @brief mark input window should update
     *
     * @param instance fcitx instance
     * @return void
     **/
    void UpdateInputWindow(struct _FcitxInstance* instance);

    static const UT_icd menuICD = {sizeof(MenuShell), NULL, NULL, NULL};

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
