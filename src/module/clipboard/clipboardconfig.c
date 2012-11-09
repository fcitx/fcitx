/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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
#include "clipboard-internal.h"

CONFIG_BINDING_BEGIN(FcitxClipboardConfig);
CONFIG_BINDING_REGISTER("Clipboard", "SaveHistoryToFile", save_history);
CONFIG_BINDING_REGISTER("Clipboard", "HistoryLength", history_len);
CONFIG_BINDING_REGISTER("Clipboard", "CandidateMaxLength", cand_max_len);
CONFIG_BINDING_REGISTER("Clipboard", "TriggerKey", trigger_key);
CONFIG_BINDING_REGISTER("Clipboard", "UsePrimary", use_primary);
CONFIG_BINDING_REGISTER("Clipboard", "ChooseModifier", choose_modifier);
CONFIG_BINDING_REGISTER("Clipboard", "IgnoreBlank", ignore_blank);
CONFIG_BINDING_END();
