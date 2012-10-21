/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#include "fcitx/module.h"
#include "im/pinyin/fcitx-pinyin.h"
#include "tablepinyinwrapper.h"
#include "table.h"
#include "fcitx/instance.h"

INPUT_RETURN_VALUE Table_PYGetCandWord(void* arg, FcitxCandidateWord* candidateWord)
{
    TableMetaData* table = arg;
    FcitxTableState* tbl = table->owner;
    INPUT_RETURN_VALUE retVal = tbl->pygetcandword(tbl->pyaddon->addonInstance,
                                                   candidateWord);
    FcitxPinyinReset(tbl->owner);
    if (!(retVal & IRV_FLAG_PENDING_COMMIT_STRING)) {
        strcpy(FcitxInputStateGetOutputString(FcitxInstanceGetInputState(tbl->owner)), candidateWord->strWord);
    }

    return IRV_COMMIT_STRING | IRV_FLAG_RESET_INPUT;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
