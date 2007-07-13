#ifndef _IME_H
#define _IME_H

#include <X11/keysym.h>
#include "xim.h"
#include "KeyList.h"

#define MAX_CAND_WORD	10
#define MAX_USER_INPUT 300
// #define MAX_PHRASE_LENGTH 10 //最大词组长度----与拼音词组长度相同

#define HOT_KEY_COUNT	2
#define MAX_HZ_SAVED    10

#define MAX_IM_NAME	10

#define NAME_OF_PINYIN		"拼音"
#define NAME_OF_SHUANGPIN	"双拼"
#define NAME_OF_WUBI		"五笔"
#define NAME_OF_ERBI		"二笔"

typedef enum _SEARCH_MODE {
    SM_FIRST,
    SM_NEXT,
    SM_PREV
} SEARCH_MODE;

typedef enum ADJUST_ORDER {
    AD_NO,
    AD_FAST,
    AD_FREQ
} ADJUSTORDER;

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

typedef struct {
	char strName[MAX_IM_NAME+1];
	void (*ResetIM) (void);
	INPUT_RETURN_VALUE (*DoInput) (int);
	INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE);
	char *(*GetCandWord) (int);
	char *(*GetLegendCandWord) (int);
	Bool (*PhraseTips) (char *strPhrase);
	void (*Init) (void);
} IM;

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
void		RegisterNewIM ( char *strName, void (*ResetIM)(void), INPUT_RETURN_VALUE (*DoInput) (int), INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE), char *(*GetCandWord) (int), char *(*GetLegendCandWord) (int),Bool (*PhraseTips) (char *strPhrase),void (*Init)(void));

void            SwitchIME (BYTE index);
void            DoPhraseTips ();

Bool		IsIM(char *strName);

#endif
