#ifndef _IME_H
#define _IME_H

#include <X11/keysym.h>
#include "xim.h"
#include "KeyList.h"

#define MAX_CAND_WORD	10
#define MAX_USER_INPUT 300
// #define MAX_PHRASE_LENGTH 10 //最大词组长度----与拼音词组长度相同
#define DATA_DIR                "/usr/share/fcitx/data/"

#define HOT_KEY_COUNT	2
#define MAX_HZ_SAVED    10

typedef enum {
    IME_WUBI = 0,
    IME_PINYIN = 1,
    IME_ERBI = 2
} IME;

typedef enum _SEARCH_MODE {
    SM_FIRST,
    SM_NEXT,
    SM_PREV
} SEARCH_MODE;

typedef enum _INPUT_RETURN_VALUE {
    //IRV_UNKNOWN = -1,
    IRV_DO_NOTHING = 0,
    IRV_DONOT_PROCESS,
    IRV_DONOT_PROCESS_CLEAN,
    IRV_CLEAN,
    IRV_TO_PROCESS,
    IRV_DISPLAY_MESSAGE,
    IRV_DISPLAY_CANDWORDS,
    IRV_PUNC,
    IRV_ENG,
    IRV_GET_LEGEND,
    IRV_GET_CANDWORDS,
    IRV_GET_CANDWORDS_NEXT
} INPUT_RETURN_VALUE;

typedef enum _ENTER_TO_DO {
    K_ENTER_NOTHING,
    K_ENTER_CLEAN,
    K_ENTER_SEND
} ENTER_TO_DO;

typedef struct _SINGLE_HZ {
    char            strHZ[3];
} SINGLE_HZ;

typedef enum _KEY_RELEASED {
    KR_OTHER = 0,
    KR_CTRL,
    KR_2ND_SELECTKEY,
    KR_3RD_SELECTKEY
} KEY_RELEASED;

typedef int     HOTKEYS;

void            ProcessKey (XIMS ims, IMForwardEventStruct * call_data);
void            ResetInput (void);
void            CloseIME (XIMS ims, IMForwardEventStruct * call_data);
Bool            IsHotKey (int iKey, HOTKEYS * hotkey);
void            UpdateHZLastInput (void);
INPUT_RETURN_VALUE ChangeCorner (void);
INPUT_RETURN_VALUE ChangePunc (void);
INPUT_RETURN_VALUE ChangeGBK (void);
INPUT_RETURN_VALUE ChangeLegend (void);

void            SwitchIME (BYTE index);
void            DoPhraseTips ();

#endif
