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

#ifndef UI_H
#define UI_H

#define INPUTWND_START_POS_DOWN	8
#define MESSAGE_MAX_CHARNUM	(150)	//输入条上显示的最长字数

#define MESSAGE_MAX_LENGTH	(MESSAGE_MAX_CHARNUM*UTF8_MAX_LENGTH)	//输入条上显示的最长长度，以字符计

/* 将输入条上显示的内容分为以下几类 */
#define MESSAGE_TYPE_COUNT	7

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

typedef struct {
    char            strMsg[MESSAGE_MAX_LENGTH + 1];
    MSG_TYPE        type;
} MESSAGE;

#define MAX_MESSAGE_COUNT 33
typedef struct Messages {
    MESSAGE msg[33];
    uint msgCount;
    boolean changed;
} Messages;

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
#define SetMessageCount(m,s) \
    do { \
        if ((s) <= MAX_MESSAGE_COUNT && s >= 0) \
            ((m)->msgCount = (s)); \
        (m)->changed = True; \
    } while(0)


#endif

void AddMessageAtLast(Messages* message, MSG_TYPE type, char *fmt, ...);
void SetMessage(Messages* message, int position, MSG_TYPE type, char* fmt, ...);
#define SetMessageText(m, p, fmt) SetMessage((m), (p), (m)->msg[(p)].type, (fmt))
void MessageConcat(Messages* message, int position, char* text);
void MessageConcatLast(Messages* message, char* text);
void SetMessageV(Messages* message, int position, MSG_TYPE type, char* fmt, va_list ap);