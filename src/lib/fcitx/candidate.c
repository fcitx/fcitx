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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "candidate.h"

typedef struct _CandidateWordList
{
    UT_array candWords;
    char strChoose[MAX_CAND_WORD + 1];
    int currentPage;
    int wordPerPage;
} CandidateWordList;

const UT_icd cand_icd = { sizeof(CandidateWord), NULL, NULL, CandidateWordFree };

FCITX_EXPORT_API
CandidateWordList* CandidateWordInit()
{
    CandidateWordList* candList = fcitx_malloc0(sizeof(CandidateWordList));

    utarray_init(&candList->candWords, &cand_icd);
    utarray_reserve(&candList->candWords, 128);
    candList->wordPerPage = 5; /* anyway put a default value for safety */
    strncpy(candList->strChoose, DIGIT_STR_CHOOSE, MAX_CAND_WORD);

    return candList;
}

FCITX_EXPORT_API
void CandidateWordInsert(CandidateWordList* candList, CandidateWord* candWord, int position)
{
    utarray_insert(&candList->candWords, candWord, position);
}

FCITX_EXPORT_API
void CandidateWordReset(CandidateWordList* candList)
{
    utarray_clear(&candList->candWords);
    candList->currentPage = 0;
}

FCITX_EXPORT_API
CandidateWord* CandidateWordGetCurrentWindow(CandidateWordList* candList)
{
    return (CandidateWord*) utarray_eltptr(&candList->candWords, candList->currentPage * candList->wordPerPage);
}

FCITX_EXPORT_API
CandidateWord* CandidateWordGetCurrentWindowNext(CandidateWordList* candList, CandidateWord* candWord)
{
    CandidateWord* nextCandWord = (CandidateWord*) utarray_next(&candList->candWords, candWord);
    CandidateWord* startCandWord = CandidateWordGetCurrentWindow(candList);
    if (nextCandWord == NULL)
        return NULL;
    int delta = utarray_eltidx(&candList->candWords, nextCandWord) - utarray_eltidx(&candList->candWords, startCandWord);
    if (delta < 0 || delta >= candList->wordPerPage)
        return NULL;
    return nextCandWord;
}

FCITX_EXPORT_API
CandidateWord* CandidateWordGetByIndex(CandidateWordList* candList, int index)
{
    if (index < candList->wordPerPage && index >= 0)
        return (CandidateWord*) utarray_eltptr(&candList->candWords, candList->currentPage * candList->wordPerPage + index);
    return NULL;
}

FCITX_EXPORT_API
INPUT_RETURN_VALUE CandidateWordChooseByIndex(CandidateWordList* candList, int index)
{
    CandidateWord* candWord = CandidateWordGetByIndex(candList, index);
    if (candWord == NULL)
        return IRV_TO_PROCESS;
    else
        return candWord->callback(candWord->owner, candWord);
}

FCITX_EXPORT_API
boolean CandidateWordHasPrev(CandidateWordList* candList)
{
    if (candList->currentPage > 0)
        return true;
    else
        return false;
}

FCITX_EXPORT_API
boolean CandidateWordHasNext(CandidateWordList* candList)
{
    if (candList->currentPage + 1 < CandidateWordPageCount(candList))
        return true;
    else
        return false;
}

FCITX_EXPORT_API
int CandidateWordPageCount(CandidateWordList* candList)
{
    return ( utarray_len(&candList->candWords) + candList->wordPerPage - 1 ) / candList->wordPerPage;
}

FCITX_EXPORT_API
void CandidateWordFree(void* arg)
{
    CandidateWord* candWord = (CandidateWord*) arg;
    if (candWord->strWord)
        free(candWord->strWord);
    if (candWord->strExtra)
        free(candWord->strExtra);
    if (candWord->priv)
        free(candWord->priv);
}

FCITX_EXPORT_API
void CandidateWordAppend(CandidateWordList* candList, CandidateWord* candWord)
{
    utarray_push_back(&candList->candWords, candWord);
}

FCITX_EXPORT_API
boolean CandidateWordGoPrevPage(CandidateWordList* candList)
{
    if (!CandidateWordPageCount(candList))
        return false;
    if (CandidateWordHasPrev(candList))
        candList->currentPage -- ;
    return true;
}

FCITX_EXPORT_API
boolean CandidateWordGoNextPage(CandidateWordList* candList)
{
    if (!CandidateWordPageCount(candList))
        return false;
    if (CandidateWordHasNext(candList))
        candList->currentPage ++ ;
    return true;
}

FCITX_EXPORT_API
void CandidateWordSetChoose(CandidateWordList* candList, const char* strChoose)
{
    strncpy(candList->strChoose, strChoose, MAX_CAND_WORD);
}

FCITX_EXPORT_API
const char* CandidateWordGetChoose(CandidateWordList* candList)
{
    return candList->strChoose;
}

FCITX_EXPORT_API
int CandidateWordGetCurrentPage(CandidateWordList* candList)
{
    return candList->currentPage;
}

FCITX_EXPORT_API
void CandidateWordResize(CandidateWordList* candList, int length)
{
    utarray_resize(&candList->candWords, length);
}

FCITX_EXPORT_API
int CandidateWordGetPageSize(CandidateWordList* candList)
{
    return candList->wordPerPage;
}

FCITX_EXPORT_API
void CandidateWordSetPageSize(CandidateWordList* candList, int size)
{
    candList->wordPerPage = size;
}

FCITX_EXPORT_API
int CandidateWordGetCurrentWindowSize(CandidateWordList* candList)
{
    if (utarray_len(&candList->candWords) == 0)
        return 0;
    /* last page */
    if (candList->currentPage + 1 == CandidateWordPageCount(candList))
    {
        int size = utarray_len(&candList->candWords) % candList->wordPerPage;
        if (size != 0)
            return size;
    }
    return candList->wordPerPage;
}

FCITX_EXPORT_API
int CandidateWordGetListSize(CandidateWordList* candList)
{
    return utarray_len(&candList->candWords);
}

FCITX_EXPORT_API
CandidateWord* CandidateWordGetFirst(CandidateWordList* candList)
{
    return (CandidateWord*) utarray_front(&candList->candWords);
}

FCITX_EXPORT_API
CandidateWord* CandidateWordGetNext(CandidateWordList* candList, CandidateWord* candWord)
{
    return (CandidateWord*) utarray_next(&candList->candWords, candWord);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0; 
