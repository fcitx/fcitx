#ifndef _INPUT_WINDOW_H
#define _INPUT_WINDOW_H

#include <X11/Xlib.h>

#define INPUTWND_STARTX	300
#define INPUTWND_WIDTH	100
#define INPUTWND_STARTY	420
#define INPUTWND_HEIGHT	40

#define INPUTWND_START_POS_UP	5
#define INPUTWND_START_POS_DOWN	8

#define MESSAGE_MAX_LENGTH	300	//输入条上显示的最长长度，以字符计

/* 将输入条上显示的内容分为以下几类 */
#define MESSAGE_TYPE_COUNT	7

typedef enum {
    MSG_TIPS,			//提示文本
    MSG_INPUT,			//用户的输入
    MSG_INDEX,			//候选字前面的序号
    MSG_FIRSTCAND,		//第一个候选字
    MSG_USERPHR,		//用户词组
    MSG_CODE,			//显示的编码
    MSG_OTHER			//其它文本
} MSG_TYPE;

typedef struct {
    char            strMsg[MESSAGE_MAX_LENGTH + 1];
    MSG_TYPE        type;
} MESSAGE;

Bool            CreateInputWindow (void);
void            DisplayInputWindow (void);
void            InitInputWindowColor (void);
void            CalculateInputWindowHeight (void);
void            DrawCursor (int iPos);
void            DisplayMessageUp (void);
void            DisplayMessageDown (void);
void            DisplayMessage (void);
void            DrawInputWindow (void);
void            ResetInputWindow (void);

#endif
