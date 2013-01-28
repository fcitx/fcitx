/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#ifndef _FCITX_CANDIDATE_INTERNAL_H_
#define _FCITX_CANDIDATE_INTERNAL_H_

#include "candidate.h"

struct _FcitxCandidateWordList {
    UT_array candWords;
    char strChoose[MAX_CAND_WORD + 1];
    unsigned int candiateModifier;
    int currentPage;
    int wordPerPage;
    boolean hasGonePrevPage;
    boolean hasGoneNextPage;
    FcitxCandidateLayoutHint layoutHint;
    boolean hasPrev;
    boolean hasNext;
    FcitxPaging paging;
    void* overrideArg;
    FcitxDestroyNotify overrideDestroyNotify;
    boolean override;
    boolean overrideHighlight;
    boolean overrideHighlightValue;
};

#endif
