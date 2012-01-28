/***************************************************************************
 *   Copyright (C) 2011~2011 by CSSlayer                                   *
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

#include "candidate.h"

typedef struct _FcitxCandidateWordList {
    UT_array candWords;
    char strChoose[MAX_CAND_WORD + 1];
    unsigned int candiateModifier;
    int currentPage;
    int wordPerPage;
} FcitxCandidateWordList;

const UT_icd cand_icd = { sizeof(FcitxCandidateWord), NULL, NULL, FcitxCandidateWordFree };

FCITX_EXPORT_API
FcitxCandidateWordList* FcitxCandidateWordNewList()
{
    FcitxCandidateWordList* candList = fcitx_utils_malloc0(sizeof(FcitxCandidateWordList));

    utarray_init(&candList->candWords, &cand_icd);
    utarray_reserve(&candList->candWords, 128);
    candList->wordPerPage = 5; /* anyway put a default value for safety */
    strncpy(candList->strChoose, DIGIT_STR_CHOOSE, MAX_CAND_WORD);

    return candList;
}

FCITX_EXPORT_API
void FcitxCandidateWordInsert(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord, int position)
{
    utarray_insert(&candList->candWords, candWord, position);
}

FCITX_EXPORT_API
void FcitxCandidateWordRemove(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord)
{
    int idx = utarray_eltidx(&candList->candWords, candWord);
    if (idx < 0 || idx >= utarray_len(&candList->candWords))
        return;
    utarray_erase(&candList->candWords, idx, 1);
}

FCITX_EXPORT_API
void FcitxCandidateWordReset(FcitxCandidateWordList* candList)
{
    utarray_clear(&candList->candWords);
    candList->currentPage = 0;
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetCurrentWindow(FcitxCandidateWordList* candList)
{
    return (FcitxCandidateWord*) utarray_eltptr(&candList->candWords, candList->currentPage * candList->wordPerPage);
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetCurrentWindowNext(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord)
{
    FcitxCandidateWord* nextCandWord = (FcitxCandidateWord*) utarray_next(&candList->candWords, candWord);
    FcitxCandidateWord* startCandWord = FcitxCandidateWordGetCurrentWindow(candList);
    if (nextCandWord == NULL)
        return NULL;
    int delta = utarray_eltidx(&candList->candWords, nextCandWord) - utarray_eltidx(&candList->candWords, startCandWord);
    if (delta < 0 || delta >= candList->wordPerPage)
        return NULL;
    return nextCandWord;
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetByIndex(FcitxCandidateWordList* candList, int index)
{
    if (index < candList->wordPerPage && index >= 0)
        return (FcitxCandidateWord*) utarray_eltptr(&candList->candWords, candList->currentPage * candList->wordPerPage + index);
    return NULL;
}

FCITX_EXPORT_API
INPUT_RETURN_VALUE FcitxCandidateWordChooseByIndex(FcitxCandidateWordList* candList, int index)
{
    FcitxCandidateWord* candWord = FcitxCandidateWordGetByIndex(candList, index);
    if (candWord == NULL) {
        if (FcitxCandidateWordGetListSize(candList) > 0)
            return IRV_DO_NOTHING;
        else
            return IRV_TO_PROCESS;
    } else
        return candWord->callback(candWord->owner, candWord);
}

FCITX_EXPORT_API
boolean FcitxCandidateWordHasPrev(FcitxCandidateWordList* candList)
{
    if (candList->currentPage > 0)
        return true;
    else
        return false;
}

FCITX_EXPORT_API
boolean FcitxCandidateWordHasNext(FcitxCandidateWordList* candList)
{
    if (candList->currentPage + 1 < FcitxCandidateWordPageCount(candList))
        return true;
    else
        return false;
}

FCITX_EXPORT_API
int FcitxCandidateWordPageCount(FcitxCandidateWordList* candList)
{
    return (utarray_len(&candList->candWords) + candList->wordPerPage - 1) / candList->wordPerPage;
}

FCITX_EXPORT_API
void FcitxCandidateWordFree(void* arg)
{
    FcitxCandidateWord* candWord = (FcitxCandidateWord*) arg;
    if (candWord->strWord)
        free(candWord->strWord);
    if (candWord->strExtra)
        free(candWord->strExtra);
    if (candWord->priv)
        free(candWord->priv);
}

FCITX_EXPORT_API
void FcitxCandidateWordAppend(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord)
{
    utarray_push_back(&candList->candWords, candWord);
}

FCITX_EXPORT_API
boolean FcitxCandidateWordGoPrevPage(FcitxCandidateWordList* candList)
{
    if (!FcitxCandidateWordPageCount(candList))
        return false;
    if (FcitxCandidateWordHasPrev(candList))
        candList->currentPage -- ;
    return true;
}

FCITX_EXPORT_API
boolean FcitxCandidateWordGoNextPage(FcitxCandidateWordList* candList)
{
    if (!FcitxCandidateWordPageCount(candList))
        return false;
    if (FcitxCandidateWordHasNext(candList))
        candList->currentPage ++ ;
    return true;
}

FCITX_EXPORT_API
void FcitxCandidateWordSetChoose(FcitxCandidateWordList* candList, const char* strChoose)
{
    FcitxCandidateWordSetChooseAndModifier(candList, strChoose, FcitxKeyState_None);
}

FCITX_EXPORT_API
void FcitxCandidateWordSetChooseAndModifier(FcitxCandidateWordList* candList, const char* strChoose, unsigned int state)
{
    strncpy(candList->strChoose, strChoose, MAX_CAND_WORD);
    candList->candiateModifier = state;
}

FCITX_EXPORT_API
unsigned int FcitxCandidateWordGetModifier(FcitxCandidateWordList* candList)
{
    return candList->candiateModifier;
}

FCITX_EXPORT_API
const char* FcitxCandidateWordGetChoose(FcitxCandidateWordList* candList)
{
    return candList->strChoose;
}

FCITX_EXPORT_API
int FcitxCandidateWordGetCurrentPage(FcitxCandidateWordList* candList)
{
    return candList->currentPage;
}

FCITX_EXPORT_API
void FcitxCandidateWordResize(FcitxCandidateWordList* candList, int length)
{
    utarray_resize(&candList->candWords, length);
}

FCITX_EXPORT_API
int FcitxCandidateWordGetPageSize(FcitxCandidateWordList* candList)
{
    return candList->wordPerPage;
}

FCITX_EXPORT_API
void FcitxCandidateWordSetPageSize(FcitxCandidateWordList* candList, int size)
{
    if (size <= 0 || size > 10)
        size = 5;
    
    candList->wordPerPage = size;
}

FCITX_EXPORT_API
int FcitxCandidateWordGetCurrentWindowSize(FcitxCandidateWordList* candList)
{
    if (utarray_len(&candList->candWords) == 0)
        return 0;
    /* last page */
    if (candList->currentPage + 1 == FcitxCandidateWordPageCount(candList)) {
        int size = utarray_len(&candList->candWords) % candList->wordPerPage;
        if (size != 0)
            return size;
    }
    return candList->wordPerPage;
}

FCITX_EXPORT_API
int FcitxCandidateWordGetListSize(FcitxCandidateWordList* candList)
{
    return utarray_len(&candList->candWords);
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetFirst(FcitxCandidateWordList* candList)
{
    return (FcitxCandidateWord*) utarray_front(&candList->candWords);
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetNext(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord)
{
    return (FcitxCandidateWord*) utarray_next(&candList->candWords, candWord);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
