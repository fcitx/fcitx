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

#include "fcitx/fcitx.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-utils/utf8.h"
#include <fcitx-utils/utarray.h>

#define INPUTWND_START_POS_DOWN	8
#define MESSAGE_MAX_CHARNUM	(150)	//输入条上显示的最长字数

#define MESSAGE_MAX_LENGTH	(MESSAGE_MAX_CHARNUM*UTF8_MAX_LENGTH)	//输入条上显示的最长长度，以字符计

/* 将输入条上显示的内容分为以下几类 */
#define MESSAGE_TYPE_COUNT	7

struct  FcitxInstance;
typedef enum {
    MSG_TIPS = 0,			//提示文本
    MSG_INPUT = 1,			//用户的输入
    MSG_INDEX = 2,			//候选字前面的序号
    MSG_FIRSTCAND = 3,		//第一个候选字
    MSG_USERPHR = 4,		//用户词组
    MSG_CODE = 5,			//显示的编码
    MSG_OTHER = 6,			//其它文本
#ifdef _ENABLE_RECORDING
    MSG_RECORDING = 7              //记录提示
#endif
} MSG_TYPE;

#define MAX_MESSAGE_COUNT 33
typedef struct MESSAGE MESSAGE;
typedef struct Messages Messages;

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

typedef struct FcitxUIStatus {
    char name[MAX_STATUS_NAME + 1];
    void (*toggleStatus)(void *arg);
    boolean (*getCurrentStatus)(void *arg);
    void *priv;
    void* arg;
} FcitxUIStatus;

typedef enum MenuState
{
    MENU_ACTIVE = 0,
    MENU_INACTIVE = 1
} MenuState;

typedef enum
{
    MENUSHELL, //暂时只有菜单项和分割线两种类型
    DIVLINE
} MenuShellType;

//菜单项属性
typedef struct MenuShell
{
    char tipstr[24];
    int  isselect;
    MenuShellType type;
} MenuShell;

typedef struct FcitxUIMenu {
    UT_array shell;
	void (*UpdateMenuShell)(void *arg);
	void (*MenuAction)(void *arg, MenuShell *shell);
} FcitxUIMenu;

typedef struct FcitxUI
{
    void* (*Create)(struct FcitxInstance*);
    void (*CloseInputWindow)(void *arg);
    void (*ShowInputWindow)(void *arg);
    void (*MoveInputWindow)(void *arg);
    void (*UpdateStatus)(void *arg, FcitxUIStatus* );
    void (*RegisterStatus)(void *arg, FcitxUIStatus* );
    void (*RegisterMenu)(void *arg, FcitxUIMenu* );
    void (*OnInputFocus)(void *arg);
    void (*OnInputUnFocus)(void *arg);
    void (*OnTriggerOn)(void *arg);
    void (*OnTriggerOff)(void *arg);
    void (*DisplayMessage)(void *arg, char *title, char **msg, int length);
} FcitxUI;

void LoadUserInterface(struct FcitxInstance* instance);

Messages* InitMessages();
void AddMessageAtLast(Messages* message, MSG_TYPE type, char *fmt, ...);
void SetMessage(Messages* message, int position, MSG_TYPE type, char* fmt, ...);
void SetMessageText(Messages* message, int position, char* fmt, ...);
void MessageConcat(Messages* message, int position, char* text);
void MessageConcatLast(Messages* message, char* text);
void SetMessageV(Messages* message, int position, MSG_TYPE type, char* fmt, va_list ap);
void SetMessageCount(Messages* m, int s);
int GetMessageCount(Messages* m);
char* GetMessageString(Messages* m, int index);
MSG_TYPE GetMessageType(Messages* m, int index);
boolean IsMessageChanged(Messages* m);
void SetMessageChanged(Messages* m, boolean changed);

void MoveInputWindow(struct FcitxInstance* instance);
void CloseInputWindow(struct FcitxInstance* instance);
void ShowInputWindow(struct FcitxInstance* instance);
void UpdateStatus(struct FcitxInstance* instance, const char* name);
void RegisterStatus(struct FcitxInstance* instance, void* arg, const char* name, void (*toggleStatus)(void *arg), boolean (*getStatus)(void *arg));
void OnInputFocus(struct FcitxInstance* instance);
void OnInputUnFocus(struct FcitxInstance* instance);
void OnTriggerOn(struct FcitxInstance* instance);
void OnTriggerOff(struct FcitxInstance* instance);
void DisplayMessage(struct FcitxInstance *instance, char *title, char **msg, int length);
FcitxUIStatus *GetUIStatus(struct FcitxInstance* instance, const char* name);

#endif
