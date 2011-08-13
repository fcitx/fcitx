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

    void LoadUserInterface(struct _FcitxInstance* instance);
    Messages* InitMessages();
    void AddMessageAtLast(Messages* message, MSG_TYPE type, const char *fmt, ...);
    void SetMessage(Messages* message, int position, MSG_TYPE type, const char* fmt, ...);
    void SetMessageText(Messages* message, int position, const char* fmt, ...);
    void MessageConcat(Messages* message, int position, const char* text);
    void MessageConcatLast(Messages* message, const char* text);
    void SetMessageV(Messages* message, int position, MSG_TYPE type, const char* fmt, va_list ap);
    void SetMessageCount(Messages* m, int s);
    int GetMessageCount(Messages* m);
    char* GetMessageString(Messages* m, int index);
    MSG_TYPE GetMessageType(Messages* m, int index);
    boolean IsMessageChanged(Messages* m);
    void SetMessageChanged(Messages* m, boolean changed);
    void AddMenuShell(FcitxUIMenu* menu, char* string, MenuShellType type, FcitxUIMenu* subMenu);
    void ClearMenuShell(FcitxUIMenu* menu);

    void MoveInputWindow(struct _FcitxInstance* instance);
    void CloseInputWindow(struct _FcitxInstance* instance);
    void UpdateStatus(struct _FcitxInstance* instance, const char* name);
    void RegisterStatus(struct _FcitxInstance* instance, void* arg, const char* name, const char* shortDesc, const char* longDesc, void (*toggleStatus)(void *arg), boolean (*getStatus)(void *arg));
    void RegisterMenu(struct _FcitxInstance* instance, FcitxUIMenu* menu);
    void OnInputFocus(struct _FcitxInstance* instance);
    void OnInputUnFocus(struct _FcitxInstance* instance);
    void OnTriggerOn(struct _FcitxInstance* instance);
    void OnTriggerOff(struct _FcitxInstance* instance);
    void DisplayMessage(struct _FcitxInstance *instance, char *title, char **msg, int length);
    FcitxUIStatus *GetUIStatus(struct _FcitxInstance* instance, const char* name);
    void UpdateMenuShell(FcitxUIMenu* menu);
    boolean IsInBox(int x0, int y0, int x1, int y1, int w, int h);
    boolean UISupportMainWindow(struct _FcitxInstance* instance);
    void GetMainWindowSize(struct _FcitxInstance* instance, int* x, int* y, int* w, int* h);
    int NewMessageToOldStyleMessage(struct _FcitxInstance* instance, Messages* msgUp, Messages* msgDown);
    char* MessagesToCString(Messages* messages);
    void UpdateInputWindow(struct _FcitxInstance* instance);

    static const UT_icd menuICD = {sizeof(MenuShell), NULL, NULL, NULL};

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
