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

#ifndef _PINYIN_ENHANCE_CFP_H
#define _PINYIN_ENHANCE_CFP_H

#include "pinyin-enhance-internal.h"

#ifdef __cplusplus
extern "C" {
#endif

    boolean PinyinEnhanceCharFromPhrasePost(PinyinEnhance *pyenhance,
                                            FcitxKeySym sym, unsigned int state,
                                            INPUT_RETURN_VALUE *retval);
    boolean PinyinEnhanceCharFromPhrasePre(PinyinEnhance *pyenhance,
                                           FcitxKeySym sym, unsigned int state,
                                           INPUT_RETURN_VALUE *retval);
    void PinyinEnhanceCharFromPhraseCandidate(PinyinEnhance *pyenhance);
    void PinyinEnhanceCharFromPhraseReset(PinyinEnhance *pyenhance);

#ifdef __cplusplus
}
#endif

#endif /* _PINYIN_ENHANCE_CFP_H */
