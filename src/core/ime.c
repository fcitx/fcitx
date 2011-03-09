/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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

/*
 * @file   ime.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  按键和输入法通用功能处理
 *
 *
 */
#include <dlfcn.h>
#include <libintl.h>
#include "ime.h"
#include "addon.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/hotkey.h"
#include "utils/configfile.h"
#include "keys.h"
#include "utils/profile.h"
#include "ime-internal.h"
#include "ui.h"
#include <utils/utils.h>

static void UnloadIM(FcitxIM* ime);

FcitxInputState input;

UT_array* GetFcitxIMS()
{
    UT_icd im_icd = {sizeof(FcitxIM*), NULL ,NULL, NULL};
    static UT_array* ims = NULL;
    if (ims == NULL)
        utarray_new(ims, &im_icd);
    return ims;
}

void SaveAllIM(void)
{
    UT_array* ims = GetFcitxIMS();
    FcitxIM **pim;
    for ( pim = (FcitxIM **) utarray_front(ims);
          pim != NULL;
          pim = (FcitxIM **) utarray_next(ims, pim))
    {
        FcitxIM *im = *pim;
        if (im->Save)
            im->Save();
    }
}

void UnloadAllIM()
{
    UT_array* ims = GetFcitxIMS();
    FcitxIM **pim;
    for ( pim = (FcitxIM **) utarray_front(ims);
          pim != NULL;
          pim = (FcitxIM **) utarray_next(ims, pim))
    {
        FcitxIM *im = *pim;
        UnloadIM(im);
    }
    utarray_free(ims);
}

void UnloadIM(FcitxIM* im)
{
    //TODO: DestroyImage(&ime->icon);
    if (im->Destroy)
        im->Destroy();
}

void LoadAllIM(void)
{
    UT_array* ims = GetFcitxIMS();
    UT_array* addons = GetFcitxAddons();
    FcitxState* gs = GetFcitxGlobalState();
    FcitxAddon *addon;
    for ( addon = (FcitxAddon *) utarray_front(addons);
          addon != NULL;
          addon = (FcitxAddon *) utarray_next(addons, addon))
    {
        if (addon->bEnabled && addon->category == AC_INPUTMETHOD)
        {
            char *modulePath;
            switch (addon->type)
            {
                case AT_SHAREDLIBRARY:
                    {
                        FILE *fp = GetLibFile(addon->library, "r", &modulePath);
                        void *handle;
                        FcitxIM* im;
                        if (!fp)
                            break;
                        fclose(fp);
                        handle = dlopen(modulePath,RTLD_LAZY);
                        if(!handle)
                        {
                            FcitxLog(ERROR, _("IM: open %s fail %s") ,modulePath ,dlerror());
                            break;
                        }
                        im=dlsym(handle,"ime");
                        if(!im || !im->Init)
                        {
                            FcitxLog(ERROR, _("IM: bad im"));
                            dlclose(handle);
                            break;
                        }
                        if(!im->Init())
                        {
                            dlclose(handle);
                            return;
                        }
                        addon->im = im;
                        utarray_push_back(ims, &im);
                    }
                default:
                    break;
            }
            free(modulePath);
        }
    }
    if (gs->iIMIndex < 0)
        gs->iIMIndex = 0;
    if (gs->iIMIndex > utarray_len(ims))
        gs->iIMIndex = utarray_len(ims) - 1;
    if (utarray_len(ims) <= 0)
    {
        FcitxLog(ERROR, _("No available Input Method"));
        exit(1);
    }
}

boolean IsHotKey(KeySym sym, int state, HOTKEYS * hotkey)
{
    state &= KEY_CTRL_ALT_SHIFT_COMP;
    if (sym == hotkey[0].sym && (hotkey[0].state == state) )
        return True;
    if (sym == hotkey[1].sym && (hotkey[1].state == state) )
        return True;
    return False;
}

INPUT_RETURN_VALUE ProcessKey(FcitxKeyEventType event,
                              long unsigned int timestamp,
                              long unsigned int sym,
                              unsigned int state,
                              int keyCount)
{
    FcitxState* gs = GetFcitxGlobalState();
    if (sym == 0) {
        return IRV_DONOT_PROCESS;
    }

    INPUT_RETURN_VALUE retVal = IRV_TO_PROCESS;
    char *pstr;
    UT_array* ims = GetFcitxIMS();
    FcitxIM** pcurrentIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
    FcitxIM* currentIM = *pcurrentIM;
    
    FcitxConfig *fc = (FcitxConfig*) GetConfig();

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (event == FCITX_RELEASE_KEY ) {
        if (GetCurrentIC()->state != IS_CLOSED) {
            if ((timestamp - input.lastKeyPressedTime) < 500 && (!input.bIsDoInputOnly)) {
                if (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                    if (GetCurrentIC()->state == IS_CHN)
                        SwitchIM(-1);
                    else if (IsHotKey(sym, state, fc->hkTrigger))
                        CloseIM(GetCurrentIC());
                    /* else if (bVK)
                        ChangVK(); */
                } else if (sym == fc->switchKey && input.keyReleased == KR_CTRL && !fc->bDoubleSwitchKey) {
                    retVal = IRV_DONOT_PROCESS;
                    if (fc->bSendTextWhenSwitchEng) {
                        if (input.iCodeInputCount != 0) {
                            strcpy(input.strStringGet, input.strCodeInput);
                            retVal = IRV_ENG;
                        }
                    }
                    input.keyReleased = KR_OTHER;
                    ChangeIMState(GetCurrentIC());
                } else if (IsHotKey(sym, state, fc->i2ndSelectKey) && input.keyReleased == KR_2ND_SELECTKEY) {
                    if (!input.bIsInLegend) {
                        pstr = currentIM->GetCandWord(1);
                        if (pstr) {
                            strcpy(input.strStringGet, pstr);
                            if (input.bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (input.iCandWordCount != 0)
                            retVal = IRV_DISPLAY_CANDWORDS;
                        else
                            retVal = IRV_TO_PROCESS;
                    } else {
                        strcpy(input.strStringGet, " ");
                        SetMessageCount(gs->messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }
                    input.keyReleased = KR_OTHER;
                } else if (IsHotKey(sym, state, fc->i3rdSelectKey) && input.keyReleased == KR_3RD_SELECTKEY) {
                    if (!input.bIsInLegend) {
                        pstr = currentIM->GetCandWord(2);
                        if (pstr) {
                            strcpy(input.strStringGet, pstr);
                            if (input.bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (input.iCandWordCount)
                            retVal = IRV_DISPLAY_CANDWORDS;
                    } else {
                        strcpy(input.strStringGet, "　");
                        SetMessageCount(gs->messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }

                    input.keyReleased = KR_OTHER;
                }
            }
        }
    }

    /* Added by hubert_star AT forum.ubuntu.com.cn */
    if (event == FCITX_RELEASE_KEY
        && IsHotKeySimple(sym, state)
        && retVal == IRV_TO_PROCESS)
        return IRV_DONOT_PROCESS;
 
    if (retVal == IRV_TO_PROCESS) {
        /* process key event for switch key */
        if (event == FCITX_PRESS_KEY) {
            if (sym != fc->switchKey)
                input.keyReleased = KR_OTHER;
            else {
                if ((input.keyReleased == KR_CTRL)
                    && (timestamp - input.lastKeyPressedTime < fc->iTimeInterval)
                    && fc->bDoubleSwitchKey) {
                    CommitString(GetCurrentIC(), input.strCodeInput);
                    ChangeIMState(GetCurrentIC());
                }
            }

            input.lastKeyPressedTime = timestamp;
            if (sym == fc->switchKey) {
                input.keyReleased = KR_CTRL;
                retVal = IRV_DO_NOTHING;
            } else if (IsHotKey(sym, state, fc->hkTrigger)) {
                /* trigger key has the highest priority, so we check it first */
                if (GetCurrentIC()->state == IS_ENG) {
                    ChangeIMState(GetCurrentIC());
                } else
                    CloseIM(GetCurrentIC());

                retVal = IRV_DO_NOTHING;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS && event != FCITX_PRESS_KEY)
        retVal = IRV_DONOT_PROCESS;

    if (retVal == IRV_TO_PROCESS) {
        if (GetCurrentIC()->state == IS_CHN) {
            /* TODO VK if (bVK)
                retVal = DoVKInput(sym, state, keyCount);
            else */
            if (IsHotKey(sym, state, fc->i2ndSelectKey)) {
                if (input.iCandWordCount >= 2)
                {
                    input.keyReleased = KR_2ND_SELECTKEY;
                    return IRV_DONOT_PROCESS;
                }
            } else if (IsHotKey(sym, state, fc->i3rdSelectKey)) {
                if (input.iCandWordCount >= 3)
                {
                    input.keyReleased = KR_3RD_SELECTKEY;
                    return IRV_DONOT_PROCESS;
                }
            }

            if (!IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                //调用输入法模块
                /* TODO: use module
                if (fcitxProfile.bCorner && (IsHotKeySimple(sym, state))) {
                    //有人报 空格 的全角不对，正确的是0xa1 0xa1
                    //但查资料却说全角符号总是以0xa3开始。
                    //由于0xa3 0xa0可能会显示乱码，因此采用0xa1 0xa1的方式
                    sprintf(strStringGet, "%s", sCornerTrans[sym - 32]);
                    retVal = IRV_GET_CANDWORDS;
                } else */

                retVal = currentIM->DoInput(sym, state, keyCount);
                if (!input.bCursorAuto)
                    input.iCursorPos = input.iCodeInputCount;
            }
        }

                     
        if (retVal == IRV_TO_PROCESS || retVal == IRV_DONOT_PROCESS) {
            CheckHotkey(sym, state, keyCount);
        }
    }
    
    return retVal;
}

void SwitchIM(int index)
{
    FcitxState* gs = GetFcitxGlobalState();
    UT_array* ims = GetFcitxIMS();
    int iIMCount = utarray_len(ims);
    
    FcitxIM* lastIM, *newIM;

    if (gs->iIMIndex >= iIMCount || gs->iIMIndex < 0)
        lastIM = NULL;
    else
    {
        FcitxIM** plastIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
        lastIM = *plastIM;
    }

    if (index == -1) {
        if (gs->iIMIndex >= (iIMCount - 1))
            gs->iIMIndex = 0;
        else
            gs->iIMIndex++;
    } 
    
    if (index >= iIMCount)
        gs->iIMIndex = iIMCount - 1;
    else if (index < 0)
        gs->iIMIndex = 0;
    else
        gs->iIMIndex = index;

    if (gs->iIMIndex >= iIMCount || gs->iIMIndex < 0)
        newIM = NULL;
    else
    {
        FcitxIM** pnewIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
        newIM = *pnewIM;
    }

    if (lastIM && lastIM->Save)
        lastIM->Save();
    if (newIM && newIM->Init)
        newIM->Init();

    ResetInput();
    SaveProfile();
}

/** 
 * @brief 重置输入状态
 */
void ResetInput(void)
{
    FcitxState* gs = GetFcitxGlobalState();
    input.iCandPageCount = 0;
    input.iCurrentCandPage = 0;
    input.iCandWordCount = 0;
    input.iLegendCandWordCount = 0;
    input.iCurrentLegendCandPage = 0;
    input.iLegendCandPageCount = 0;
    input.iCursorPos = 0;

    input.strCodeInput[0] = '\0';
    input.iCodeInputCount = 0;

    input.bIsDoInputOnly = false;

    input.bShowPrev = false;
    input.bShowNext = false;

    input.bIsInLegend = false;

    UT_array* ims = GetFcitxIMS();
    FcitxIM** pcurentIM = (FcitxIM**) utarray_eltptr(ims, gs->iIMIndex);
    FcitxIM* curentIM = *pcurentIM;

    if (curentIM && curentIM->ResetIM)
        curentIM->ResetIM();
}

INPUT_RETURN_VALUE CheckHotkey(FcitxKeySym keysym, unsigned int state, int count)
{
    return IRV_DO_NOTHING;
}

#if 0

#include "core/fcitx.h"

#include <ctype.h>
#include <time.h>

#include "core/xim.h"
#include "core/ime.h"
#include "core/keys.h"
#include "AboutWindow.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#include "font.h"
#include "skin.h"
#include "ui.h"
#include "im/special/punc.h"
#include "im/pinyin/py.h"
#include "im/pinyin/sp.h"
#include "im/qw/qw.h"
#include "im/table/table.h"
#include "im/special/vk.h"
#include "im/special/QuickPhrase.h"
#include "im/special/AutoEng.h"
#include "im/extra/extra.h"
#include "skin.h"
#include "interface/DBus.h"
#include "fcitx-config/cutils.h"

FcitxState gs;                  /* global state */

int8_t iIMCount = 0;
int8_t iState = 0;

int iCurrentCandPage;
int iCandWordCount;

int iLegendCandWordCount;
int iLegendCandPageCount;
int iCurrentLegendCandPage;

int iCodeInputCount;

// *************************************************************

// *************************************************************

Bool bIsDoInputOnly = False;    //表明是否只由输入法来处理键盘
Bool bLastIsNumber = False;     //上一次输入是不是阿拉伯数字
char cLastIsAutoConvert = 0;    //上一次输入是不是符合数字后自动转换的符号，如'.'/','，0表示不是这样的符号
int8_t iInCap = 0;                //是不是处于大写后的英文状态,0--不，1--按下大写键，2--按下分号键

/*
Bool            bAutoHideInputWindow = False;	//是否自动隐藏输入条
*/
int8_t lastIsSingleHZ = 0;

Bool bVK = False;

Time lastKeyPressedTime;

KEY_RELEASED keyReleased = KR_OTHER;


/* 计算打字速度 */
time_t timeStart;
Bool bStartRecordType;
uint iHZInputed = 0;

Bool bCursorAuto = False;

char *sCornerTrans[] = {
    "　", "！", "＂", "＃", "￥", "％", "＆", "＇", "（", "）",
    "＊",
    "＋", "，", "－", "．", "／",
    "０", "１", "２", "３", "４", "５", "６", "７", "８", "９",
    "：",
    "；", "＜", "＝", "＞", "？",
    "＠", "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ",
    "Ｊ",
    "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ",
    "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ", "Ｗ", "Ｘ", "Ｙ",
    "Ｚ",
    "［", "＼", "］", "＾", "＿",
    "｀", "ａ", "ｂ", "ｃ", "ｄ", "ｅ", "ｆ", "ｇ", "ｈ", "ｉ",
    "ｊ",
    "ｋ", "ｌ", "ｍ", "ｎ", "ｏ",
    "ｐ", "ｑ", "ｒ", "ｓ", "ｔ", "ｕ", "ｖ", "ｗ", "ｘ", "ｙ",
    "ｚ",
    "｛", "｜", "｝", "￣",
};

static void DoForwardEvent(IMForwardEventStruct * call_data);

/** 
 * @brief 转换strStringGet中的标点为中文标点
 */
void ConvertPunc(void)
{
    char strTemp[MAX_USER_INPUT + 1] = "\0";
    char *s1, *s2, *pPunc;

    s1 = strTemp;
    s2 = strStringGet;

    while (*s2) {
        pPunc = GetPunc(*s2);
        if (pPunc) {
            strcat(s1, pPunc);
            s1 += strlen(pPunc);
        } else
            *s1++ = *s2;
        s2++;
    }
    *s2 = '\0';

    strcpy(strStringGet, strTemp);
}

void DoForwardEvent(IMForwardEventStruct * call_data)
{
    IMForwardEventStruct fe;
    memset(&fe, 0, sizeof(fe));

    fe.major_code = XIM_FORWARD_EVENT;
    fe.icid = call_data->icid;
    fe.connect_id = call_data->connect_id;
    fe.sync_bit = 0;
    fe.serial_number = 0L;
    fe.event = call_data->event;

    IMForwardEvent(ims, (XPointer) & fe);
}

/** 
 * @brief 处理按键
 * 
 * @param call_data
 */
void ProcessKey(IMForwardEventStruct * call_data)
{
    KeySym sym;
    XKeyEvent *kev;
    int keyCount;
    INPUT_RETURN_VALUE retVal;
    unsigned int state;
    char strbuf[STRBUFLEN];
    char *pstr;
    int iLen;

    if (!CurrentIC) {
        CurrentIC = FindIC(call_data->icid);
        if (CurrentIC)
            return;
    }

    if (GetCurrentIC()->id != call_data->icid) {
        CurrentIC = FindIC(call_data->icid);
    }

    kev = (XKeyEvent *) & call_data->event;
    memset(strbuf, 0, STRBUFLEN);
    keyCount = XLookupString(kev, strbuf, STRBUFLEN, &sym, NULL);

    state = kev->state - (kev->state & KEY_NUMLOCK) - (kev->state & KEY_CAPSLOCK) - (kev->state & KEY_SCROLLLOCK);
    GetKey(sym, state, keyCount, &sym, &state);

#ifdef _DEBUG
    FcitxLog(INFO,
        "KeyRelease=%d  state=%d  KEYCODE=%d  KEYSYM=%d  keyCount=%d",
         (call_data->event.type == KeyRelease), state, kev->keycode, (int) sym, keyCount);
#endif

    if (!sym) {
        DoForwardEvent(call_data);
        return;
    }

    retVal = IRV_TO_PROCESS;

    /*
     * for following reason, we cannot just process switch key, 2nd, 3rd key as other simple hotkey
     * because ctrl, shift, alt are compose key, so hotkey like ctrl+a will also produce a key
     * release event for ctrl key, so we must make sure the key release right now is the key just
     * pressed.
     */

    /* process keyrelease event for switch key and 2nd, 3rd key */
    if (call_data->event.type == KeyRelease) {
        if (GetCurrentIC()->state != IS_CLOSED) {
            if ((kev->time - lastKeyPressedTime) < 500 && (!bIsDoInputOnly)) {
                if (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                    if (!fcitxProfile.bLocked) {
                        if (GetCurrentIC()->state == IS_CHN)
                            SwitchIM(-1);
                        else if (IsHotKey(sym, state, fc->hkTrigger))
                            CloseIM(call_data);
                    } else if (bVK)
                        ChangVK();
                } else if (kev->keycode == fc->switchKey && keyReleased == KR_CTRL && !fc->bDoubleSwitchKey) {
                    retVal = IRV_DONOT_PROCESS;
                    if (fc->bSendTextWhenSwitchEng) {
                        if (iCodeInputCount) {
                            strcpy(strStringGet, strCodeInput);
                            retVal = IRV_ENG;
                        }
                    }
                    keyReleased = KR_OTHER;
                    ChangeIMState(call_data->connect_id);
                } else if (IsHotKey(sym, state, fc->i2ndSelectKey) && keyReleased == KR_2ND_SELECTKEY) {
                    if (!bIsInLegend) {
                        pstr = im[gs->iIMIndex].GetCandWord(1);
                        if (pstr) {
                            strcpy(strStringGet, pstr);
                            if (bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (iCandWordCount)
                            retVal = IRV_DISPLAY_CANDWORDS;
                        else
                            retVal = IRV_TO_PROCESS;
                    } else {
                        strcpy(strStringGet, " ");
                        SetMessageCount(&messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }
                    keyReleased = KR_OTHER;
                } else if (IsHotKey(sym, state, fc->i3rdSelectKey) && keyReleased == KR_3RD_SELECTKEY) {
                    if (!bIsInLegend) {
                        pstr = im[gs->iIMIndex].GetCandWord(2);
                        if (pstr) {
                            strcpy(strStringGet, pstr);
                            if (bIsInLegend)
                                retVal = IRV_GET_LEGEND;
                            else
                                retVal = IRV_GET_CANDWORDS;
                        } else if (iCandWordCount)
                            retVal = IRV_DISPLAY_CANDWORDS;
                    } else {
                        strcpy(strStringGet, "　");
                        SetMessageCount(&messageDown, 0);
                        retVal = IRV_GET_CANDWORDS;
                    }

                    keyReleased = KR_OTHER;
                }
            }
        }
    }

    /* Added by hubert_star AT forum.ubuntu.com.cn */
    if ((call_data->event.type == KeyRelease)
        && IsHotKeySimple(sym, state)
        && retVal == IRV_TO_PROCESS)
        return;
 
    if (retVal == IRV_TO_PROCESS) {
        /* process key event for switch key */
        if (call_data->event.type == KeyPress) {
            if (kev->keycode != fc->switchKey)
                keyReleased = KR_OTHER;
            else {
                if ((keyReleased == KR_CTRL)
                    && (kev->time - lastKeyPressedTime < fc->iTimeInterval)
                    && fc->bDoubleSwitchKey) {
                    SendHZtoClient(call_data, strCodeInput);
                    ChangeIMState(call_data->connect_id);
                }
            }

            lastKeyPressedTime = kev->time;
            if (kev->keycode == fc->switchKey) {
                keyReleased = KR_CTRL;
                retVal = IRV_DO_NOTHING;
            } else if (IsHotKey(sym, state, fc->hkTrigger)) {
                /* trigger key has the highest priority, so we check it first */
                if (GetCurrentIC()->state == IS_ENG) {
                    GetCurrentIC()->state = IS_CHN;

                    EnterChineseMode(False);
                    if (!fc->bUseDBus)
                        DrawMainWindow();

                    if (fc->bShowInputWindowTriggering && !fcitxProfile.bCorner) {
                        DisplayInputWindow();
                    }
                } else
                    CloseIM(call_data);

                retVal = IRV_DO_NOTHING;
            }
        }
    }

    if (retVal == IRV_TO_PROCESS && call_data->event.type != KeyPress)
        retVal = IRV_DONOT_PROCESS;

    if (retVal == IRV_TO_PROCESS) {
        if (GetCurrentIC()->state == IS_CHN) {
            if (bVK)
                retVal = DoVKInput(sym, state, keyCount);
            else {
                if (IsHotKey(sym, state, fc->i2ndSelectKey)) {
                    if (iCandWordCount >= 2)
                    {
                        keyReleased = KR_2ND_SELECTKEY;
                        return;
                    }
                } else if (IsHotKey(sym, state, fc->i3rdSelectKey)) {
                    if (iCandWordCount >= 3)
                    {
                        keyReleased = KR_3RD_SELECTKEY;
                        return;
                    }
                }

                if (IsHotKey(sym, state, FCITX_LCTRL_LSHIFT)) {
                    if (fcitxProfile.bLocked)
                        retVal = IRV_TO_PROCESS;
                } else {
                    //调用输入法模块
                    if (fcitxProfile.bCorner && (IsHotKeySimple(sym, state))) {
                        //有人报 空格 的全角不对，正确的是0xa1 0xa1
                        //但查资料却说全角符号总是以0xa3开始。
                        //由于0xa3 0xa0可能会显示乱码，因此采用0xa1 0xa1的方式
                        sprintf(strStringGet, "%s", sCornerTrans[sym - 32]);
                        retVal = IRV_GET_CANDWORDS;
                    } else {
                        if (!iInCap) {
                            char strTemp[MAX_USER_INPUT];

                            retVal = im[gs->iIMIndex].DoInput(sym, state, keyCount);
                            if (!bCursorAuto && !IsIM(strNameOfPinyin)
                                && !IsIM(strNameOfShuangpin))
                                iCursorPos = iCodeInputCount;

                            //为了实现自动英文转换
                            strcpy(strTemp, strCodeInput);
                            if (retVal == IRV_TO_PROCESS) {
                                strTemp[strlen(strTemp) + 1] = '\0';
                                strTemp[strlen(strTemp)] = sym;
                            }

                            if (SwitchToEng(strTemp)) {
                                iInCap = 3;
                                if (retVal != IRV_TO_PROCESS) {
                                    iCodeInputCount--;
                                    retVal = IRV_TO_PROCESS;
                                }
                            }

                            if (!IsHotKey(sym, state, FCITX_BACKSPACE))
                                cLastIsAutoConvert = 0;
                        } else if (iInCap == 2
                                   && fc->semicolonToDo == K_SEMICOLON_QUICKPHRASE && !iLegendCandWordCount)
                            retVal = QuickPhraseDoInput(sym, state, keyCount);

                        if (!bIsDoInputOnly && retVal == IRV_TO_PROCESS) {
                            if (!iInCap && IsHotKeyUAZ(sym,state) && fc->bEngAfterCap && !(kev->state & KEY_CAPSLOCK)) {
                                iInCap = 1;
                                if (!bIsInLegend && iCandWordCount) {
                                    pstr = im[gs->iIMIndex].GetCandWord(0);
                                    iCandWordCount = 0;
                                    if (pstr) {
                                        SendHZtoClient(call_data, pstr);
                                        strcpy(strStringGet, pstr);
                                        //粗略统计字数
                                        iHZInputed += (int) (utf8_strlen(strStringGet));
                                        iCodeInputCount = 0;
                                    }
                                }
                            } else if (IsHotKey(sym, state, FCITX_SEMICOLON)
                                       && fc->semicolonToDo != K_SEMICOLON_NOCHANGE && !iCodeInputCount) {
                                if (iInCap != 2)
                                    iInCap = 2;
                                else
                                    sym = XK_space; //使用第2个分号输入中文分号
                            } else if (!iInCap) {
                                if (IsHotKey(sym, state, fc->hkPrevPage))
                                    retVal = im[gs->iIMIndex].GetCandWords(SM_PREV);
                                else if (IsHotKey(sym, state, fc->hkNextPage))
                                    retVal = im[gs->iIMIndex].GetCandWords(SM_NEXT);
                            }

                            if (retVal == IRV_TO_PROCESS) {
                                if (iInCap) {
                                    if (IsHotKey(sym, state, FCITX_SPACE)
                                        && (iCodeInputCount == 0)) {
                                        strcpy(strStringGet, "；");
                                        retVal = IRV_ENG;
                                        SetMessageCount(&messageDown, 0);
                                        SetMessageCount(&messageUp, 0);
                                        iInCap = 0;
                                    } else {
                                        if (IsHotKeySimple(sym, state)) {
                                            if (iCodeInputCount == MAX_USER_INPUT)
                                                retVal = IRV_DO_NOTHING;
                                            else {
                                                if (!(iInCap == 2 && !iCodeInputCount && IsHotKey(sym, state, FCITX_SEMICOLON))) {
                                                    strCodeInput[iCodeInputCount++]
                                                        = sym;
                                                    strCodeInput[iCodeInputCount]
                                                        = '\0';
                                                    inputWindow.bShowCursor = True;
                                                    iCursorPos = iCodeInputCount;
                                                    if (fc->semicolonToDo
                                                        == K_SEMICOLON_QUICKPHRASE && iInCap == 2) {
                                                        if (iFirstQuickPhrase == -1)
                                                            retVal = QuickPhraseGetCandWords(SM_FIRST);
                                                        else {
                                                            if (IsHotKey(sym, state, fc->hkPrevPage)
                                                                || IsHotKey(sym, state, fc->hkNextPage)) {
                                                                if (iCodeInputCount)
                                                                    iCodeInputCount--;
                                                                strCodeInput[iCodeInputCount]
                                                                    = '\0';
                                                                iCursorPos = iCodeInputCount;
                                                            }

                                                            if (IsHotKey(sym, state, fc->hkPrevPage))
                                                                retVal = QuickPhraseGetCandWords(SM_PREV);
                                                            else if (IsHotKey(sym, state, fc->hkNextPage))
                                                                retVal = QuickPhraseGetCandWords(SM_NEXT);
                                                            else
                                                                retVal = QuickPhraseGetCandWords(SM_FIRST);
                                                        }
                                                    } else
                                                        retVal = IRV_DISPLAY_MESSAGE;
                                                } else
                                                    retVal = IRV_DISPLAY_MESSAGE;
                                            }
                                        } else if (IsHotKey(sym, state, FCITX_BACKSPACE) || IsHotKey(sym, state, FCITX_CTRL_H)) {
                                            if (iCodeInputCount)
                                                iCodeInputCount--;
                                            strCodeInput[iCodeInputCount] = '\0';
                                            iCursorPos = iCodeInputCount;
                                            if (!iCodeInputCount)
                                                retVal = IRV_CLEAN;
                                            else if (fc->semicolonToDo == K_SEMICOLON_QUICKPHRASE && iInCap == 2) {
                                                if (iFirstQuickPhrase == -1)
                                                    retVal = QuickPhraseGetCandWords(SM_FIRST);
                                                else if (IsHotKey(sym, state, fc->hkPrevPage))
                                                    retVal = QuickPhraseGetCandWords(SM_PREV);
                                                else if (IsHotKey(sym, state, fc->hkNextPage))
                                                    retVal = QuickPhraseGetCandWords(SM_NEXT);
                                                else
                                                    retVal = QuickPhraseGetCandWords(SM_FIRST);
                                            } else
                                                retVal = IRV_DISPLAY_MESSAGE;
                                        }

                                        SetMessageCount(&messageUp, 0);
                                        if (iInCap == 2) {
                                            if (fc->semicolonToDo == K_SEMICOLON_ENG) {
                                                AddMessageAtLast(&messageUp, MSG_TIPS, "英文输入 ");
                                                iCursorPos += strlen("英文输入 ");
                                            } else {
                                                AddMessageAtLast(&messageUp, MSG_TIPS, "自定义输入 ");
                                                iCursorPos += strlen("自定义输入 ");
                                            }

                                            if (iCodeInputCount) {
                                                AddMessageAtLast(&messageUp, MSG_INPUT, "%s", strCodeInput);
                                            }

                                            if (retVal != IRV_DISPLAY_CANDWORDS) {
                                                SetMessageCount(&messageDown, 0);
                                                if (iCodeInputCount)
                                                    AddMessageAtLast
                                                        (&messageDown, MSG_TIPS, "按 Enter 输入英文");
                                                else
                                                    AddMessageAtLast
                                                        (&messageDown, MSG_TIPS, "空格输入；Enter输入;");
                                            }
                                        } else {
                                            SetMessageCount(&messageDown, 0);
                                            AddMessageAtLast(&messageDown, MSG_TIPS, "按 Enter 输入英文");
                                            AddMessageAtLast(&messageUp, MSG_INPUT, strCodeInput);
                                        }
                                    }
                                } else if ((bLastIsNumber && fc->bEngPuncAfterNumber)
                                           && (IsHotKey(sym, state, FCITX_PERIOD) 
                                               || IsHotKey(sym, state, FCITX_SEMICOLON) 
                                               || IsHotKey(sym, state, FCITX_COMMA))
                                           && !iCandWordCount) {
                                    cLastIsAutoConvert = sym;
                                    bLastIsNumber = False;
                                    retVal = IRV_TO_PROCESS;
                                } else {
                                    if (fcitxProfile.bChnPunc) {
                                        char *pPunc = NULL;

                                        pstr = NULL;
                                        if (state == KEY_NONE)
                                            pPunc = GetPunc(sym);

                                        if (pPunc) {
                                            strStringGet[0] = '\0';
                                            if (!bIsInLegend)
                                                pstr = im[gs->iIMIndex].GetCandWord(0);
                                            if (pstr)
                                                strcpy(strStringGet, pstr);
                                            strcat(strStringGet, pPunc);
                                            SetMessageCount(&messageDown, 0);
                                            SetMessageCount(&messageUp, 0);

                                            retVal = IRV_PUNC;
                                        } else if ((IsHotKey(sym, state, FCITX_BACKSPACE) || IsHotKey(sym, state, FCITX_CTRL_H))
                                                   && cLastIsAutoConvert) {
                                            char *pPunc;

                                            DoForwardEvent(call_data);
                                            pPunc = GetPunc(cLastIsAutoConvert);
                                            if (pPunc)
                                                SendHZtoClient(call_data, pPunc);

                                            retVal = IRV_DO_NOTHING;
                                        } else if (IsHotKeySimple(sym, state)) {
                                            if (IsHotKeyDigit(sym, state))
                                                bLastIsNumber = True;
                                            else {
                                                bLastIsNumber = False;
                                                if (IsHotKey(sym, state, FCITX_SPACE))
                                                    retVal = IRV_DONOT_PROCESS_CLEAN;   //为了与mozilla兼容
                                                else {
                                                    strStringGet[0]
                                                        = '\0';
                                                    if (!bIsInLegend)
                                                        pstr = im[gs->iIMIndex].GetCandWord(0);
                                                    if (pstr)
                                                        strcpy(strStringGet, pstr);
                                                    iLen = strlen(strStringGet);
                                                    SetMessageCount(&messageDown, 0);
                                                    SetMessageCount(&messageUp, 0);
                                                    strStringGet[iLen] = sym;
                                                    strStringGet[iLen + 1] = '\0';
                                                    retVal = IRV_ENG;
                                                }
                                            }
                                        }
                                    }
                                    cLastIsAutoConvert = 0;
                                }
                            }
                        }

                        if (retVal == IRV_TO_PROCESS) {
                            if (IsHotKey(sym, state, FCITX_ESCAPE)) {
                                if (iCodeInputCount || iInCap || bIsInLegend)
                                    retVal = IRV_CLEAN;
                                else
                                    retVal = IRV_DONOT_PROCESS;
                            } else if (IsHotKey(sym, state, FCITX_CTRL_5)) {
                                ReloadConfig();
                                retVal = IRV_DO_NOTHING;
                            } else if (IsHotKey(sym, state, FCITX_ENTER)) {
                                if (iInCap) {
                                    if (!iCodeInputCount)
                                        strcpy(strStringGet, ";");
                                    else
                                        strcpy(strStringGet, strCodeInput);
                                    retVal = IRV_PUNC;
                                    SetMessageCount(&messageDown, 0);
                                    SetMessageCount(&messageUp, 0);
                                    iInCap = 0;
                                } else if (!iCodeInputCount)
                                    retVal = IRV_DONOT_PROCESS;
                                else {
                                    switch (fc->enterToDo) {
                                    case K_ENTER_NOTHING:
                                        retVal = IRV_DO_NOTHING;
                                        break;
                                    case K_ENTER_CLEAN:
                                        retVal = IRV_CLEAN;
                                        break;
                                    case K_ENTER_SEND:
                                        SetMessageCount(&messageDown, 0);
                                        SetMessageCount(&messageUp, 0);
                                        strcpy(strStringGet, strCodeInput);
                                        retVal = IRV_ENG;
                                        break;
                                    }
                                }
                            } else if (IsHotKeySimple(sym, state))
                                retVal = IRV_DONOT_PROCESS_CLEAN;
                            else
                                retVal = IRV_DONOT_PROCESS;
                        }
                    }
                }
            }
        }

        if (retVal == IRV_TO_PROCESS || retVal == IRV_DONOT_PROCESS) {
            if (IsHotKey(sym, state, fc->hkCorner))
                retVal = ChangeCorner();
            else if (IsHotKey(sym, state, fc->hkPunc))
                retVal = ChangePunc();
            else if (IsHotKey(sym, state, fc->hkLegend))
                retVal = ChangeLegend();
            else if (IsHotKey(sym, state, fc->hkTrack))
                retVal = ChangeTrack();
            else if (IsHotKey(sym, state, fc->hkGBT))
                retVal = ChangeGBKT();
            else if (IsHotKey(sym, state, fc->hkHideMainWindow)) {
                if (bMainWindow_Hiden) {
                    bMainWindow_Hiden = False;
                    if (!fc->bUseDBus) {
                        DisplayMainWindow();
                        DrawMainWindow();
                    }
                } else {
                    bMainWindow_Hiden = True;
                    if (!fc->bUseDBus)
                        XUnmapWindow(dpy, mainWindow.window);
                }
                retVal = IRV_DO_NOTHING;
            } else if (IsHotKey(sym, state, fc->hkSaveAll)) {
                SaveIM();
                SetMessageCount(&messageDown, 0);
                AddMessageAtLast(&messageDown, MSG_TIPS, "词库已保存");
                retVal = IRV_DISPLAY_MESSAGE;
            } else if (IsHotKey(sym, state, fc->hkVK))
                SwitchVK();
#ifdef _ENABLE_RECORDING
            else if (IsHotKey(sym, state, fc->hkRecording))
                ChangeRecording();
            else if (IsHotKey(sym, state, fc->hkResetRecording))
                ResetRecording();
#endif
        }
    }

    switch (retVal) {
    case IRV_DO_NOTHING:
        break;
    case IRV_TO_PROCESS:
    case IRV_DONOT_PROCESS:
    case IRV_DONOT_PROCESS_CLEAN:
        DoForwardEvent(call_data);

        if (retVal != IRV_DONOT_PROCESS_CLEAN)
            return;
    case IRV_CLEAN:
        ResetInput();
        ResetInputWindow();
        CloseInputWindow();

        return;
    case IRV_DISPLAY_CANDWORDS:
        inputWindow.bShowNext = inputWindow.bShowPrev = False;
        if (bIsInLegend) {
            if (iCurrentLegendCandPage > 0)
                inputWindow.bShowPrev = True;
            if (iCurrentLegendCandPage < iLegendCandPageCount)
                inputWindow.bShowNext = True;
        } else {
            if (iCurrentCandPage > 0)
                inputWindow.bShowPrev = True;
            if (iCurrentCandPage < iCandPageCount)
                inputWindow.bShowNext = True;
        }

        DisplayInputWindow();
        if (!fc->bUseDBus) {
            DrawInputWindow();
        }

        break;
    case IRV_DISPLAY_LAST:
        inputWindow.bShowNext = inputWindow.bShowPrev = False;
        SetMessageCount(&messageUp, 0);
        AddMessageAtLast(&messageUp, MSG_INPUT, "%c", strCodeInput[0]);
        SetMessageCount(&messageDown, 0);
        AddMessageAtLast(&messageDown, MSG_TIPS, "%s", strStringGet);
        DisplayInputWindow();
        if (!fc->bUseDBus) {
            DrawInputWindow();
        }
        
        break;
    case IRV_DISPLAY_MESSAGE:
        inputWindow.bShowNext = False;
        inputWindow.bShowPrev = False;

        DisplayInputWindow();
        if (!fc->bUseDBus) {
            DrawInputWindow();
        }

        break;
    case IRV_GET_LEGEND:
        SendHZtoClient(call_data, strStringGet);
        iHZInputed += (int) (utf8_strlen(strStringGet));        //粗略统计字数
        if (iLegendCandWordCount) {
            inputWindow.bShowNext = inputWindow.bShowPrev = False;
            if (iCurrentLegendCandPage > 0)
                inputWindow.bShowPrev = True;
            if (iCurrentLegendCandPage < iLegendCandPageCount)
                inputWindow.bShowNext = True;
            bLastIsNumber = False;
            iCodeInputCount = 0;
            DisplayInputWindow();
            if (!fc->bUseDBus) {
                DrawInputWindow();
            }
        } else {
            ResetInput();
            CloseInputWindow();
        }

        break;
    case IRV_GET_CANDWORDS:
        SendHZtoClient(call_data, strStringGet);
        bLastIsNumber = False;
        if (fc->bPhraseTips && im[gs->iIMIndex].PhraseTips && !bVK)
            DoPhraseTips();
        iHZInputed += (int) (utf8_strlen(strStringGet));

        if (bVK || (!messageDown.msgCount && (!fc->bPhraseTips || (fc->bPhraseTips && !lastIsSingleHZ))))
            CloseInputWindow();
        else {
            DisplayInputWindow();
            if (!fc->bUseDBus) {
                DrawInputWindow();
            }
        }

        ResetInput();
        lastIsSingleHZ = 0;
        break;
    case IRV_ENG:
        //如果处于中文标点模式，应该将其中的标点转换为全角
        if (fcitxProfile.bChnPunc && fc->bConvertPunc)
            ConvertPunc();
    case IRV_PUNC:
        iHZInputed += (int) (utf8_strlen(strStringGet));        //粗略统计字数
        ResetInput();
        if (!messageDown.msgCount)
            CloseInputWindow();
    case IRV_GET_CANDWORDS_NEXT:
        SendHZtoClient(call_data, strStringGet);
        bLastIsNumber = False;
        lastIsSingleHZ = 0;

        if (retVal == IRV_GET_CANDWORDS_NEXT || lastIsSingleHZ == -1) {
            iHZInputed += (int) (utf8_strlen(strStringGet));    //粗略统计字数
            DisplayInputWindow();
            DrawInputWindow();
        }

        break;
    default:
        ;
    }

    //计算打字速度的功能
    if (retVal == IRV_DISPLAY_MESSAGE || retVal == IRV_DISPLAY_CANDWORDS || retVal == IRV_PUNC) {
        if (!bStartRecordType) {
            bStartRecordType = True;
            timeStart = time(NULL);
        }
    }
}

INPUT_RETURN_VALUE ChangeCorner(void)
{
    ResetInput();
    ResetInputWindow();

    fcitxProfile.bCorner = !fcitxProfile.bCorner;

    SwitchIM(gs->iIMIndex);

    if (!fc->bUseDBus) {
        DrawMainWindow();
        CloseInputWindow();
    }
#ifdef _ENABLE_DBUS
    else
        updateProperty(&corner_prop);
#endif

    SaveProfile();

    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE ChangePunc(void)
{
    ResetInput();
    ResetInputWindow();

    fcitxProfile.bChnPunc = !fcitxProfile.bChnPunc;

    SwitchIM(gs->iIMIndex);

    if (!fc->bUseDBus) {
        DrawMainWindow();
        CloseInputWindow();
    }
#ifdef _ENABLE_DBUS
    else
        updateProperty(&punc_prop);
#endif
    return IRV_DO_NOTHING;
}

INPUT_RETURN_VALUE ChangeGBKT(void)
{
    fcitxProfile.bUseGBKT = !fcitxProfile.bUseGBKT;
    ResetInput();
    ResetInputWindow();

    if (!fc->bUseDBus) {
        DrawMainWindow();
        CloseInputWindow();
    }

    SaveProfile();

#ifdef _ENABLE_DBUS
    if (fc->bUseDBus)
        updateProperty(&gbkt_prop);
#endif

    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeLegend(void)
{
    fcitxProfile.bUseLegend = !fcitxProfile.bUseLegend;
    ResetInput();

    if (!fc->bUseDBus) {
        ResetInputWindow();

        DrawMainWindow();
        CloseInputWindow();
    }

    SaveProfile();

#ifdef _ENABLE_DBUS
    if (fc->bUseDBus)
        updateProperty(&legend_prop);
#endif
    return IRV_CLEAN;
}

INPUT_RETURN_VALUE ChangeTrack(void)
{
    fcitxProfile.bTrackCursor = !fcitxProfile.bTrackCursor;
    SaveProfile();

    return IRV_DO_NOTHING;;
}

void ChangeLock(void)
{
    fcitxProfile.bLocked = !fcitxProfile.bLocked;

    if (!fc->bUseDBus)
        DrawMainWindow();

    SaveProfile();
}

#ifdef _ENABLE_RECORDING
void ChangeRecording(void)
{
    fcitxProfile.bRecording = !fcitxProfile.bRecording;
    ResetInput();
    ResetInputWindow();
    CloseInputWindow();

    CloseRecording();
    if (fcitxProfile.bRecording)
        OpenRecording(True);

    SaveProfile();
}

void ResetRecording(void)
{
    if (fpRecord) {
        fclose(fpRecord);
        fpRecord = NULL;
        bWrittenRecord = False;
    }

    if (fcitxProfile.bRecording)
        OpenRecording(False);
}
#endif


void SelectIM(int imidx)
{
    int8_t iLastIM;
    gs->iIMIndex = imidx;

    iLastIM = (gs->iIMIndex >= iIMCount) ? (iIMCount - 1) : gs->iIMIndex;

    if (im[iLastIM].Save)
        im[iLastIM].Save();
    if (im[gs->iIMIndex].Init)
        im[gs->iIMIndex].Init();
    ResetInput();
    DrawMainWindow();
}

void DoPhraseTips(void)
{
    if (!fc->bPhraseTips)
        return;

    if (im[gs->iIMIndex].PhraseTips())
        lastIsSingleHZ = -1;
    else
        lastIsSingleHZ = 0;
}

void ReloadConfig()
{
    LoadConfig();
    if (fc->inputMethods[IM_SP])
        LoadSPData();

    if (!fc->bUseDBus) {
        if (!mainWindow.window)
            CreateMainWindow();
        if (fc->hideMainWindow != HM_HIDE) {
            DisplayMainWindow();
            DrawMainWindow();
        }
#ifdef _ENABLE_TRAY
        if (!tray.window) {
            CreateTrayWindow();
            DrawTrayWindow(INACTIVE_ICON, 0, 0, tray.size, tray.size);
        }
#endif
        if (!aboutWindow.window)
            CreateAboutWindow();

        DisplaySkin(fc->skinType);
    } else {
        XUnmapWindow(dpy, mainWindow.window);
#ifdef _ENABLE_TRAY
        XDestroyWindow(dpy, tray.window);
        tray.window = (Window) NULL;
        tray.bTrayMapped = False;
#endif
    }

    SaveIM();

    UnloadIM();

    LoadAddonInfo();

    SetIM();
    if (!fc->bUseDBus) {
#ifndef _ENABLE_PANGO
        CreateFont();
#endif
        CalculateInputWindowHeight();
    }

    FreeQuickPhrase();
    LoadQuickPhrase();

    FreeAutoEng();
    LoadAutoEng();

    FreePunc();
    LoadPuncDict();
    SwitchIM(-2);
    if (!fc->bUseDBus) {
        DrawMainWindow();
        DisplaySkin(fc->skinType);
    }
}
#endif