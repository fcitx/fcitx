#ifndef _IME_H
#define _IME_H

#include <X11/keysym.h>
#include "xim.h"
#include "KeyList.h"

#define MAX_CAND_WORD	10
#define MAX_USER_INPUT 300

#define HOT_KEY_COUNT	2
#define MAX_HZ_SAVED    1024

#define MAX_IM_NAME	12

#define NAME_OF_PINYIN		"Æ´Òô"
#define NAME_OF_SHUANGPIN	"Ë«Æ´"
#define NAME_OF_QUWEI		"ÇøÎ»"

#define TEMP_FILE		"FCITX_DICT_TEMP"

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
    IRV_DISPLAY_LAST,
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
    char            strName[MAX_IM_NAME + 1];
    void            (*ResetIM) (void);
                    INPUT_RETURN_VALUE (*DoInput) (int);
                    INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE);
    char           *(*GetCandWord) (int);
    char           *(*GetLegendCandWord) (int);
                    Bool (*PhraseTips) (void);
    void            (*Init) (void);
    void            (*Destroy) (void);
} IM;

typedef int     HOTKEYS;

void            ProcessKey (IMForwardEventStruct * call_data);
void            ResetInput (void);
void            CloseIM (IMForwardEventStruct * call_data);
void		ChangeIMState (IMForwardEventStruct * call_data);
Bool            IsHotKey (int iKey, HOTKEYS * hotkey);
INPUT_RETURN_VALUE ChangeCorner (void);
INPUT_RETURN_VALUE ChangePunc (void);
INPUT_RETURN_VALUE ChangeGBK (void);
INPUT_RETURN_VALUE ChangeLegend (void);
INPUT_RETURN_VALUE ChangeTrack (void);

void            RegisterNewIM (char *strName, void (*ResetIM) (void), INPUT_RETURN_VALUE (*DoInput) (int), INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE), char *(*GetCandWord) (int), char *(*GetLegendCandWord) (int),
			       Bool (*PhraseTips) (void), void (*Init) (void), void (*Destroy) (void));
void            SwitchIM (INT8 index);
void            DoPhraseTips ();
Bool            IsIM (char *strName);
void            SaveIM (void);
void            SetIM (void);

#endif
