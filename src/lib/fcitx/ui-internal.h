/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                     *
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
/**
 * @file   ui-internal.h
 *
 *  Private Header for UI
 *
 */

#ifndef _FCITX_UI_INTERNAL_H_
#define _FCITX_UI_INTERNAL_H_

#include "fcitx-config/fcitx-config.h"
#include "fcitx/instance.h"

/**
 * real input window updates, will trigger user interface module to redraw
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxUIUpdateInputWindowReal(FcitxInstance *instance);
/**
 * real move input window, will trigger user interface module to move
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxUIMoveInputWindowReal(FcitxInstance *instance);

/**
 * @brief Get string without MSG_DONOT_COMMIT_WHEN_UNFOCUS
 *
 * @param messages message
 * @return return string
 *
 * @see MSG_DONOT_COMMIT_WHEN_UNFOCUS
 **/
char* FcitxUIMessagesToCStringForCommit(FcitxMessages* messages);

typedef enum _FcitxUIFlag {
    UI_NONE = 0,
    UI_MOVE = (1 << 1),
    UI_UPDATE = (1 << 2),
    UI_EVENT_CHECK = (1 << 3),
} FcitxUIFlag;

#endif
