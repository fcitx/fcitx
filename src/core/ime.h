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
/**
 * @file   ime.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  按键和输入法通用功能处理
 *
 *
 */

#ifndef _IME_H
#define _IME_H

#include <X11/keysym.h>
#include <cairo.h>
#include "core/xim.h"
#include "fcitx-config/hotkey.h"
#include "core/fcitx.h"
#include "tools/utf8.h"
#include "fcitx-config/fcitx-config.h"
#include "core/addon.h"

#define HOT_KEY_COUNT	2
#define TEMP_FILE		"FCITX_DICT_TEMP"

typedef enum _INPUT_METHOD {
    IM_PY = 0,
    IM_SP,
    IM_QW,
    IM_TABLE,
    IM_EXTRA
} INPUT_METHOD;

typedef struct _SINGLE_HZ {
    char            strHZ[UTF8_MAX_LENGTH + 1];
} SINGLE_HZ;

typedef enum _KEY_RELEASED {
    KR_OTHER = 0,
    KR_CTRL,
    KR_2ND_SELECTKEY,
    KR_2ND_SELECTKEY_OTHER,
    KR_3RD_SELECTKEY,
    KR_3RD_SELECTKEY_OTHER
} KEY_RELEASED;

typedef struct IM{
    char               strName[MAX_IM_NAME + 1];
    char               strIconName[MAX_IM_NAME + 1];
    void               (*ResetIM) (void);
    INPUT_RETURN_VALUE (*DoInput) (unsigned int, unsigned int, int);
    INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE);
    char              *(*GetCandWord) (int);
    char              *(*GetLegendCandWord) (int);
    Bool               (*PhraseTips) (void);
    void               (*Init) (void);
    void               (*Save) (void);
    FcitxImage         image;
    cairo_surface_t   *icon;
    FcitxAddon        *addonInfo;
} IM;

typedef struct FcitxState {
    char *fontZh;
    char *fontEn;
    INT8 iIMIndex;
    Bool bMutexInited;
} FcitxState;

void            ProcessKey (IMForwardEventStruct * call_data);
void            ResetInput (void);
void            CloseIM (IMForwardEventStruct * call_data);
void            ChangeIMState (CARD16 _connect_id);
Bool            IsHotKey (int iKey, HOTKEYS * hotkey);
INPUT_RETURN_VALUE ChangeCorner (void);
INPUT_RETURN_VALUE ChangePunc (void);
INPUT_RETURN_VALUE ChangeLegend (void);
INPUT_RETURN_VALUE ChangeTrack (void);
INPUT_RETURN_VALUE ChangeGBKT (void);
void		ChangeLock (void);

#ifdef _ENABLE_RECORDING
void		ChangeRecording (void);
void		ResetRecording (void);
#endif

void            RegisterNewIM (char *strName,
                               char *strIconName,
                               void (*ResetIM) (void),
                               INPUT_RETURN_VALUE (*DoInput) (unsigned int, unsigned int, int),
                               INPUT_RETURN_VALUE (*GetCandWords) (SEARCH_MODE),
                               char *(*GetCandWord) (int),
                               char *(*GetLegendCandWord) (int),
                               Bool (*PhraseTips) (void),
                               void (*Init) (void),
                               void (*Save) (void),
                               FcitxAddon *addon);
void            SwitchIM (INT8 index);
void            DoPhraseTips ();
Bool            IsIM (char *strName);
void            SaveIM (void);
void            UnloadIM();
void            SetIM (void);
void            ConvertPunc (void);
void            ReloadConfig();

extern FcitxState gs;

// Bool            IsKeyIgnored (int iKeyCode);

#endif
