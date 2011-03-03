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

#ifndef _IME_INTERNAL_H
#define _IME_INTERNAL_H

#include <X11/keysym.h>
#include <cairo.h>
#include "fcitx-config/hotkey.h"
#include "core/fcitx.h"
#include "utils/utf8.h"
#include "fcitx-config/fcitx-config.h"
#include "core/addon.h"
#include "core/ui.h"

typedef enum _KEY_RELEASED {
    KR_OTHER = 0,
    KR_CTRL,
    KR_2ND_SELECTKEY,
    KR_3RD_SELECTKEY,
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
    void               (*Destroy) ();
    FcitxImage         image;
    cairo_surface_t   *icon;
    FcitxAddon        *addonInfo;
} IM;

//void            ProcessKey (IMForwardEventStruct * call_data);
void            ResetInput (void);
//void            CloseIM (IMForwardEventStruct * call_data);
void            ChangeIMState ();
Bool            IsHotKey(KeySym sym, int state, HOTKEYS * hotkey);
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

void            SwitchIM (INT8 index);
void            DoPhraseTips ();
Bool            IsIM (char *strName);
void            SaveAllIM (void);
void            UnloadAllIM();
void            LoadAllIM (void);
void            ConvertPunc (void);
void            ReloadConfig();
void            SelectIM(int imidx);
void            SelectVK(int vkidx);

#endif
