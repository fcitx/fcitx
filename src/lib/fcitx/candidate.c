/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
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

#include "candidate-internal.h"

static const UT_icd cand_icd = {
    sizeof(FcitxCandidateWord), NULL, NULL, FcitxCandidateWordFree
};

FCITX_EXPORT_API
FcitxCandidateWordList* FcitxCandidateWordNewList()
{
    FcitxCandidateWordList* candList = fcitx_utils_new(FcitxCandidateWordList);

    utarray_init(&candList->candWords, &cand_icd);
    utarray_reserve(&candList->candWords, 128);
    candList->wordPerPage = 5; /* anyway put a default value for safety */
    candList->layoutHint = CLH_NotSet;
    strncpy(candList->strChoose, DIGIT_STR_CHOOSE, MAX_CAND_WORD);

    return candList;
}

FCITX_EXPORT_API
void FcitxCandidateWordFreeList(FcitxCandidateWordList* list)
{
    utarray_done(&list->candWords);
    free(list);
}

FCITX_EXPORT_API
void FcitxCandidateWordInsert(FcitxCandidateWordList* candList,
                              FcitxCandidateWord* candWord, int position)
{
    fcitx_array_insert(&candList->candWords, candWord, position);
}

FCITX_EXPORT_API
void
FcitxCandidateWordMerge(FcitxCandidateWordList* candList,
                        FcitxCandidateWordList* newList, int position)
{
    void *p;
    if (!newList)
        return;
    if (position >= 0) {
        fcitx_array_inserta(&candList->candWords, &newList->candWords,
                            position);
    } else {
        utarray_concat(&candList->candWords, &newList->candWords);
    }
    utarray_steal(&newList->candWords, p);
    newList->currentPage = 0;
    free(p);
}

INPUT_RETURN_VALUE DummyHandler(void* arg, FcitxCandidateWord* candWord)
{
    FCITX_UNUSED(arg);
    FCITX_UNUSED(candWord);
    return IRV_DO_NOTHING;
}

FCITX_EXPORT_API
void FcitxCandidateWordInsertPlaceHolder(FcitxCandidateWordList* candList,
                                         int position)
{
    FcitxCandidateWord candWord;
    memset(&candWord, 0, sizeof(FcitxCandidateWord));
    candWord.callback = DummyHandler;
    fcitx_array_insert(&candList->candWords, &candWord, position);
}

FCITX_EXPORT_API
void FcitxCandidateWordMove(FcitxCandidateWordList* candList, int from, int to)
{
    fcitx_array_move(&candList->candWords, from, to);
}

FCITX_EXPORT_API void
FcitxCandidateWordMoveByWord(FcitxCandidateWordList *candList,
                             FcitxCandidateWord *candWord, int to)
{
    int from = utarray_eltidx(&candList->candWords, candWord);
    FcitxCandidateWordMove(candList, from, to);
}

FCITX_EXPORT_API void
FcitxCandidateWordRemoveByIndex(FcitxCandidateWordList *candList, int idx)
{
    fcitx_array_erase(&candList->candWords, idx, 1);
}

FCITX_EXPORT_API void
FcitxCandidateWordRemove(FcitxCandidateWordList *candList,
                         FcitxCandidateWord *candWord)
{
    int idx = utarray_eltidx(&candList->candWords, candWord);
    FcitxCandidateWordRemoveByIndex(candList, idx);
}

FCITX_EXPORT_API void
FcitxCandidateWordSetPage(FcitxCandidateWordList *candList, int index)
{
    if (index >= 0 && index < FcitxCandidateWordPageCount(candList)) {
        candList->currentPage = index;
    }
}

FCITX_EXPORT_API void
FcitxCandidateWordSetFocus(FcitxCandidateWordList* candList, int index)
{
    FcitxCandidateWordSetPage(candList, index / candList->wordPerPage);
}

FCITX_EXPORT_API
void FcitxCandidateWordReset(FcitxCandidateWordList* candList)
{
    utarray_clear(&candList->candWords);
    if (candList->override) {
        candList->override = false;
        candList->hasPrev = false;
        candList->hasNext = false;
        candList->paging = NULL;
        if (candList->overrideDestroyNotify)
            candList->overrideDestroyNotify(candList->overrideArg);
        candList->overrideArg = NULL;
        candList->overrideDestroyNotify = NULL;
    }
    candList->overrideHighlight = false;
    candList->overrideHighlightValue = false;
    candList->currentPage = 0;
    candList->hasGonePrevPage = false;
    candList->hasGoneNextPage = false;
    candList->layoutHint = CLH_NotSet;
}

FCITX_EXPORT_API
int FcitxCandidateWordGetCurrentIndex(FcitxCandidateWordList* candList)
{
    return candList->currentPage * candList->wordPerPage;
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetCurrentWindow(FcitxCandidateWordList* candList)
{
    return FcitxCandidateWordGetByTotalIndex(
        candList, candList->currentPage * candList->wordPerPage);
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetCurrentWindowNext(FcitxCandidateWordList* candList,
                                       FcitxCandidateWord* candWord)
{
    FcitxCandidateWord *nextCandWord = (FcitxCandidateWord*)utarray_next(&candList->candWords, candWord);
    if (nextCandWord == NULL)
        return NULL;
    FcitxCandidateWord *startCandWord = FcitxCandidateWordGetCurrentWindow(candList);
    if (nextCandWord < startCandWord ||
        nextCandWord >= startCandWord + candList->wordPerPage)
        return NULL;
    return nextCandWord;
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetCurrentWindowPrev(FcitxCandidateWordList* candList,
                                       FcitxCandidateWord* candWord)
{
    FcitxCandidateWord *prevWord = utarray_prev(&candList->candWords, candWord);
    if (prevWord == NULL)
        return NULL;
    FcitxCandidateWord *startWord = FcitxCandidateWordGetCurrentWindow(candList);
    if (prevWord < startWord ||
        prevWord >= startWord + candList->wordPerPage)
        return NULL;
    return prevWord;
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetByTotalIndex(FcitxCandidateWordList* candList, int index)
{
    return fcitx_array_eltptr(&candList->candWords, index);
}

FCITX_EXPORT_API int
FcitxCandidateWordGetIndex(FcitxCandidateWordList *candList,
                           FcitxCandidateWord *word)
{
    return utarray_eltidx(&candList->candWords, word);
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetByIndex(FcitxCandidateWordList* candList, int index)
{
    if (index < candList->wordPerPage && index >= 0)
        return FcitxCandidateWordGetByTotalIndex(
            candList, candList->currentPage * candList->wordPerPage + index);
    return NULL;
}

FCITX_EXPORT_API INPUT_RETURN_VALUE
FcitxCandidateWordChooseByIndex(FcitxCandidateWordList* candList, int index)
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
INPUT_RETURN_VALUE FcitxCandidateWordChooseByTotalIndex(FcitxCandidateWordList* candList, int index)
{
    FcitxCandidateWord* candWord = FcitxCandidateWordGetByTotalIndex(candList, index);
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
    if (candList->override)
        return candList->hasPrev;

    if (candList->currentPage > 0)
        return true;
    else
        return false;
}

FCITX_EXPORT_API
boolean FcitxCandidateWordHasNext(FcitxCandidateWordList* candList)
{
    if (candList->override)
        return candList->hasNext;

    if (candList->currentPage + 1 < FcitxCandidateWordPageCount(candList))
        return true;
    else
        return false;
}

FCITX_EXPORT_API int
FcitxCandidateWordPageCount(FcitxCandidateWordList* candList)
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
    if (candList->override) {
        if (candList->paging) {
            return candList->paging(candList->overrideArg, true);
        } else {
            return true;
        }
    }

    if (!FcitxCandidateWordPageCount(candList))
        return false;
    if (FcitxCandidateWordHasPrev(candList)) {
        candList->currentPage -- ;
        candList->hasGonePrevPage = true;
        return true;
    }
    return false;
}

FCITX_EXPORT_API
boolean FcitxCandidateWordGoNextPage(FcitxCandidateWordList* candList)
{
    if (candList->override) {
        if (candList->paging) {
            return candList->paging(candList->overrideArg, false);
        } else {
            return true;
        }
    }

    if (!FcitxCandidateWordPageCount(candList))
        return false;
    if (FcitxCandidateWordHasNext(candList)) {
        candList->currentPage ++ ;
        candList->hasGoneNextPage = true;
        return true;
    }
    return false;
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
    fcitx_array_resize(&candList->candWords, length);
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
boolean FcitxCandidateWordGetHasGoneToPrevPage(FcitxCandidateWordList* candList)
{
    return candList->hasGonePrevPage;
}

FCITX_EXPORT_API
boolean FcitxCandidateWordGetHasGoneToNextPage(FcitxCandidateWordList* candList)
{
    return candList->hasGoneNextPage;
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetFirst(FcitxCandidateWordList* candList)
{
    return (FcitxCandidateWord*)utarray_front(&candList->candWords);
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetLast(FcitxCandidateWordList* candList)
{
    return (FcitxCandidateWord*)utarray_back(&candList->candWords);
}

FCITX_EXPORT_API
FcitxCandidateWord* FcitxCandidateWordGetNext(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord)
{
    return (FcitxCandidateWord*)utarray_next(&candList->candWords, candWord);
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetPrev(FcitxCandidateWordList *candList,
                          FcitxCandidateWord *candWord)
{
    return utarray_prev(&candList->candWords, candWord);
}

FCITX_EXPORT_API
int
FcitxCandidateWordCheckChooseKey(FcitxCandidateWordList *candList,
                                 FcitxKeySym sym, unsigned int state)
{
    return FcitxHotkeyCheckChooseKeyAndModifier(sym, state,
                                                candList->strChoose,
                                                candList->candiateModifier);
}

FCITX_EXPORT_API
FcitxCandidateLayoutHint FcitxCandidateWordGetLayoutHint(FcitxCandidateWordList* candList)
{
    return candList->layoutHint;
}

FCITX_EXPORT_API
void FcitxCandidateWordSetLayoutHint(FcitxCandidateWordList* candList, FcitxCandidateLayoutHint hint)
{
    candList->layoutHint = hint;
}

FCITX_EXPORT_API
void FcitxCandidateWordSetOverridePaging(FcitxCandidateWordList* candList, boolean hasPrev, boolean hasNext, FcitxPaging paging, void* arg, FcitxDestroyNotify destroyNotify)
{
    if (candList->override && candList->overrideDestroyNotify) {
        candList->overrideDestroyNotify(candList->overrideArg);
    }

    candList->override = true;
    candList->hasPrev = hasPrev;
    candList->hasNext = hasNext;
    candList->paging = paging;
    candList->overrideArg = arg;
    candList->overrideDestroyNotify = destroyNotify;
}

FCITX_EXPORT_API
void FcitxCandidateWordSetOverrideDefaultHighlight(FcitxCandidateWordList* candList, boolean overrideValue)
{
    candList->overrideHighlight = true;
    candList->overrideHighlightValue = overrideValue;
}

FCITX_EXPORT_API FcitxCandidateWord*
FcitxCandidateWordGetFocus(FcitxCandidateWordList *cand_list, boolean clear)
{
    FcitxCandidateWord *res = NULL;
    FcitxCandidateWord *cand_word;
    for (cand_word = FcitxCandidateWordGetCurrentWindow(cand_list);
         cand_word;cand_word = FcitxCandidateWordGetCurrentWindowNext(
             cand_list, cand_word)) {
        if (FcitxCandidateWordCheckFocus(cand_word, clear)) {
            res = cand_word;
        }
    }
    if (!res)
        return FcitxCandidateWordGetCurrentWindow(cand_list);
    return res;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
