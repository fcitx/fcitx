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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"

#include "tabledict.h"
#include "table.h"

CONFIG_BINDING_BEGIN(TableConfig)
CONFIG_BINDING_REGISTER("Key", "AddPhrase", hkTableAddPhrase)
CONFIG_BINDING_REGISTER("Key", "DeletePhrase", hkTableDelPhrase)
CONFIG_BINDING_REGISTER("Key", "AdjustOrder", hkTableAdjustOrder)
CONFIG_BINDING_REGISTER("Key", "ClearFreq", hkTableClearFreq)
CONFIG_BINDING_REGISTER("Key", "LookupPinyin", hkLookupPinyin)
CONFIG_BINDING_END()

CONFIG_BINDING_BEGIN(TableMetaData)
CONFIG_BINDING_REGISTER("CodeTable", "UniqueName", uniqueName)
CONFIG_BINDING_REGISTER("CodeTable", "Name", strName)
CONFIG_BINDING_REGISTER("CodeTable", "IconName", strIconName)
CONFIG_BINDING_REGISTER("CodeTable", "File", strPath)
CONFIG_BINDING_REGISTER("CodeTable", "AdjustOrder", tableOrder)
CONFIG_BINDING_REGISTER("CodeTable", "SimpleCodeOrderLevel", iSimpleLevel)
CONFIG_BINDING_REGISTER("CodeTable", "Priority", iPriority)
CONFIG_BINDING_REGISTER("CodeTable", "UsePY", bUsePY)
CONFIG_BINDING_REGISTER("CodeTable", "PYKey", cPinyin)
CONFIG_BINDING_REGISTER("CodeTable", "UseAutoSend", bUseAutoSend)
CONFIG_BINDING_REGISTER("CodeTable", "AutoSend", iTableAutoSendToClient)
CONFIG_BINDING_REGISTER("CodeTable", "NoneMatchAutoSend", iTableAutoSendToClientWhenNone)
CONFIG_BINDING_REGISTER("CodeTable", "SendRawPreedit", bSendRawPreedit)
CONFIG_BINDING_REGISTER("CodeTable", "EndKey", strEndCode)
CONFIG_BINDING_REGISTER("CodeTable", "UseMatchingKey", bUseMatchingKey)
CONFIG_BINDING_REGISTER("CodeTable", "MatchingKey", cMatchingKey)
CONFIG_BINDING_REGISTER("CodeTable", "ExactMatch", bTableExactMatch)
CONFIG_BINDING_REGISTER("CodeTable", "NoMatchDontCommit", bNoMatchDontCommit)
CONFIG_BINDING_REGISTER("CodeTable", "AutoPhrase", bAutoPhrase)
CONFIG_BINDING_REGISTER("CodeTable", "AutoPhraseLength", iAutoPhraseLength)
CONFIG_BINDING_REGISTER("CodeTable", "AutoPhrasePhrase", bAutoPhrasePhrase)
CONFIG_BINDING_REGISTER("CodeTable", "SaveAutoPhrase", iSaveAutoPhraseAfter)
CONFIG_BINDING_REGISTER("CodeTable", "PromptTableCode", bPromptTableCode)
CONFIG_BINDING_REGISTER("CodeTable", "Symbol", strSymbol)
CONFIG_BINDING_REGISTER("CodeTable", "SymbolFile", strSymbolFile)
CONFIG_BINDING_REGISTER("CodeTable", "Choose", strChoose)
CONFIG_BINDING_REGISTER("CodeTable", "ChooseModifier", chooseModifier)
CONFIG_BINDING_REGISTER("CodeTable", "LangCode", langCode)
CONFIG_BINDING_REGISTER("CodeTable", "KeyboardLayout", kbdlayout)
CONFIG_BINDING_REGISTER("CodeTable", "UseCustomPrompt", customPrompt)
CONFIG_BINDING_REGISTER("CodeTable", "UseAlternativePageKey", bUseAlternativePageKey)
CONFIG_BINDING_REGISTER("CodeTable", "AlternativePrevPage", hkAlternativePrevPage)
CONFIG_BINDING_REGISTER("CodeTable", "AlternativeNextPage", hkAlternativeNextPage)
CONFIG_BINDING_REGISTER("CodeTable", "FirstCandidateAsPreedit", bFirstCandidateAsPreedit)
CONFIG_BINDING_REGISTER("CodeTable", "CommitAndPassByInvalidKey", bCommitAndPassByInvalidKey)
CONFIG_BINDING_REGISTER("CodeTable", "CommitKey", hkCommitKey)
CONFIG_BINDING_REGISTER("CodeTable", "CommitKeyCommitWhenNoMatch", bCommitKeyCommitWhenNoMatch)
CONFIG_BINDING_REGISTER("CodeTable", "IgnorePunc", bIgnorePunc)
CONFIG_BINDING_REGISTER("CodeTable", "Enabled", bEnabled)
CONFIG_BINDING_END()


// kate: indent-mode cstyle; space-indent on; indent-width 0;
