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

/**
 * @file   ime.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  Public Header for Input Method Develop
 *
 */
#ifndef _FCITX_IME_H_
#define _FCITX_IME_H_

#include <fcitx-utils/utf8.h>
#include <fcitx-config/hotkey.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CODE_LEN    63

#define MAX_IM_NAME    (8 * UTF8_MAX_LENGTH)

#define MAX_CAND_LEN    127
#define MAX_TIPS_LEN    9

#define MAX_CAND_WORD    10
#define MAX_USER_INPUT    300

#define HOT_KEY_COUNT   2

struct FcitxInputContext;
struct FcitxInstance;
struct FcitxAddon;
typedef enum _SEARCH_MODE {
    SM_FIRST,
    SM_NEXT,
    SM_PREV
} SEARCH_MODE;

typedef enum _KEY_RELEASED {
    KR_OTHER = 0,
    KR_CTRL,
    KR_2ND_SELECTKEY,
    KR_3RD_SELECTKEY,
    KR_CTRL_SHIFT
} KEY_RELEASED;

typedef enum _INPUT_RETURN_VALUE {
    //IRV_UNKNOWN = -1,
    IRV_DO_NOTHING = 0, /* do nothing */
    IRV_DONOT_PROCESS, /* key will be forward */
    IRV_DONOT_PROCESS_CLEAN, /* key will be forward and process as IRV_CLEAN */
    IRV_CLEAN, /* reset input */
    IRV_TO_PROCESS, /* key will passed to next flow*/
    IRV_DISPLAY_MESSAGE, /* it's a message, so next and prev will not be shown */
    IRV_DISPLAY_CANDWORDS, /* the only different with message it it will show next and prev button */
    IRV_DISPLAY_LAST, /* display the last input word */
    IRV_PUNC,
    IRV_ENG,
    IRV_GET_REMIND, /* remind word */
    IRV_GET_CANDWORDS, /* send the input to client, close input window */
    IRV_GET_CANDWORDS_NEXT /* send the input to client, dont close input window */
} INPUT_RETURN_VALUE;

typedef struct _SINGLE_HZ {
    char            strHZ[UTF8_MAX_LENGTH + 1];
} SINGLE_HZ;

typedef struct FcitxIMClass {
    void*              (*Create) (struct FcitxInstance* instance);
    void               (*Destroy) (void *arg);
} FcitxIMClass;

typedef boolean            (*FcitxIMInit) (void *arg);
typedef void               (*FcitxIMResetIM) (void *arg);
typedef INPUT_RETURN_VALUE (*FcitxIMDoInput) (void *arg, FcitxKeySym, unsigned int);
typedef INPUT_RETURN_VALUE (*FcitxIMGetCandWords) (void *arg, SEARCH_MODE);
typedef char              *(*FcitxIMGetCandWord) (void *arg, int);
typedef boolean            (*FcitxIMPhraseTips) (void *arg);
typedef void               (*FcitxIMSave) (void *arg);
typedef void               (*FcitxIMReloadConfig) (void *arg);

typedef struct FcitxIM {
    char               strName[MAX_IM_NAME + 1];
    char               strIconName[MAX_IM_NAME + 1];
    FcitxIMResetIM ResetIM;
    FcitxIMDoInput DoInput;
    FcitxIMGetCandWords GetCandWords;
    FcitxIMGetCandWord GetCandWord;
    FcitxIMPhraseTips PhraseTips;
    FcitxIMSave Save;
    FcitxIMInit Init;
    FcitxIMReloadConfig ReloadConfig;
    void*              uiprivate;
    void* klass;
    int iPriority;
    void* priv;
} FcitxIM;

typedef enum FcitxKeyEventType {
    FCITX_PRESS_KEY,
    FCITX_RELEASE_KEY
} FcitxKeyEventType;

typedef struct FcitxInputState {
    long unsigned int lastKeyPressedTime;
    boolean bIsDoInputOnly;
    KEY_RELEASED keyReleased;
    int iCodeInputCount;
    char strCodeInput[MAX_USER_INPUT + 1];
    char strStringGet[MAX_USER_INPUT + 1];  //保存输入法返回的需要送到客户程序中的字串
    boolean bIsInRemind;

    int iCandPageCount;
    int iCandWordCount;
    time_t timeStart;
    int iCursorPos;
    int iCurrentCandPage;
    int iRemindCandWordCount;
    int iRemindCandPageCount;
    int iCurrentRemindCandPage;
    int bShowNext;
    int bShowPrev;
    int iHZInputed;
    int lastIsSingleHZ;
    boolean bLastIsNumber;
    boolean bStartRecordType;
} FcitxInputState;

boolean IsHotKey(FcitxKeySym sym, int state, HOTKEYS * hotkey);
char* GetOutputString(FcitxInputState* input);
struct FcitxIM* GetCurrentIM(struct FcitxInstance *instance);
void EnableIM(struct FcitxInstance* instance, struct FcitxInputContext* ic, boolean keepState);
void ResetInput (struct FcitxInstance* instance);
/**
 * @brief Sometimes, we use INPUT_RETURN_VALUE not from ProcessKey, so use this function to do the correct thing.
 *
 * @param instance fcitx instance
 * @param retVal input return val
 * @return void
 **/
void ProcessInputReturnValue(
    struct FcitxInstance* instance,
    INPUT_RETURN_VALUE retVal
);
void FcitxRegisterIM(struct FcitxInstance *instance,
                     void *addonInstance,
                     const char* name,
                     const char* iconName,
                     FcitxIMInit Init,
                     FcitxIMResetIM ResetIM,
                     FcitxIMDoInput DoInput,
                     FcitxIMGetCandWords GetCandWords,
                     FcitxIMGetCandWord GetCandWord,
                     FcitxIMPhraseTips PhraseTips,
                     FcitxIMSave Save,
                     FcitxIMReloadConfig ReloadConfig,
                     void *priv,
                     int priority
                    );

INPUT_RETURN_VALUE ProcessKey(struct FcitxInstance* instance, FcitxKeyEventType event, long unsigned int timestamp, FcitxKeySym sym, unsigned int state);
void ForwardKey(struct FcitxInstance* instance, struct FcitxInputContext* ic, FcitxKeyEventType event, FcitxKeySym sym, unsigned int state);
void SaveAllIM (struct FcitxInstance* instance);
void ReloadConfig(struct FcitxInstance* instance);
void SwitchIM (struct FcitxInstance* instance, int index);

#ifdef __cplusplus
}
#endif

#endif
