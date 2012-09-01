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

#include <errno.h>
#include <iconv.h>
#include <unistd.h>
#include <ctype.h>

#include <libintl.h>

#include "pinyin-enhance-cfp.h"

#include "config.h"

static INPUT_RETURN_VALUE
CharFromParseString(PinyinEnhance *pyenhance,
                    FcitxKeySym sym, unsigned int state)
{
    if (!FcitxHotkeyIsHotKeySimple(sym, state))
        return IRV_TO_PROCESS;
    return IRV_TO_PROCESS;
}

boolean
PinyinEnhanceCharFromPhrasePost(PinyinEnhance *pyenhance, FcitxKeySym sym,
                                unsigned int state, INPUT_RETURN_VALUE *retval)
{
    if (*retval)
        return false;
    if ((*retval = CharFromParseString(pyenhance, sym, state)))
        return true;
    return false;
}

boolean
PinyinEnhanceCharFromPhrasePre(PinyinEnhance *pyenhance, FcitxKeySym sym,
                               unsigned int state, INPUT_RETURN_VALUE *retval)
{
    return false;
}
