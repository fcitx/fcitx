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
 * @file   ime-internal.h
 * @date   2008-1-16
 *
 * @brief  Private Header for Input Method
 *
 */

#ifndef _FCITX_IME_INTERNAL_H_
#define _FCITX_IME_INTERNAL_H_

#include "fcitx-config/hotkey.h"
#include "ime.h"
#include "fcitx-utils/utarray.h"

struct FcitxInputContext;
struct FcitxInstance;

void InitFcitxIM(struct FcitxInstance* instance);
void InitBuiltInHotkey(struct FcitxInstance* instance);
void DoPhraseTips (struct FcitxInstance* instance);
void UnloadAllIM(UT_array* ims);
boolean LoadAllIM (struct FcitxInstance* instance);
void InitIMMenu(struct FcitxInstance* instance);
void ShowInputSpeed(struct FcitxInstance* instance);
INPUT_RETURN_VALUE ImProcessEnter(void *arg);
INPUT_RETURN_VALUE ImProcessEscape(void *arg);
INPUT_RETURN_VALUE ImProcessRemind(void *arg);
INPUT_RETURN_VALUE ImProcessReload(void *arg);
INPUT_RETURN_VALUE ImProcessSaveAll(void *arg);

#endif
