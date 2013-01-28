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

/**
 * @addtogroup Fcitx
 * @{
 */

/**
 * @file candidate.h
 *
 * Fcitx candidate word list related definition and function
 */

#ifndef FCITX_CANDIDATE_H
#define FCITX_CANDIDATE_H
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/ime.h>

#ifdef __cplusplus
extern "C" {
#endif

/** max candidate word number for single page */
#define MAX_CAND_WORD    10

    struct _FcitxCandidateWord;
    struct _FcitxCandidateWordList;

    /**
     * a hint to let the candidate list to show with specific layout
     *
     * it's only a soft hint for UI, it depends on UI implement it or not
     *
     * and it will be automatically reset to CLH_NotSet when Reset Candidate List
     *
     * @since 4.2.7
     */
    typedef enum _FcitxCandidateLayoutHint {
        CLH_NotSet,
        CLH_Vertical,
        CLH_Horizontal
    } FcitxCandidateLayoutHint;

    /** fcitx candidate workd list */
    typedef struct _FcitxCandidateWordList FcitxCandidateWordList;

    /** callback for a single candidate word being chosen */
    typedef INPUT_RETURN_VALUE(*FcitxCandidateWordCommitCallback)(void* arg, struct _FcitxCandidateWord* candWord);

    /**
     * A Single Candidate Word
     **/
    typedef struct _FcitxCandidateWord {
        /**
         * String display in the front
         **/
        char* strWord;
        /**
         * String display after strWord
         **/
        char* strExtra;
        /**
         * Callback when string is going to commit
         **/
        FcitxCandidateWordCommitCallback callback;
        /**
         * Store the candidateWord Type
         **/
        FcitxMessageType wordType;
        /**
         * Store the extra Type
         **/
        FcitxMessageType extraType;
        /**
         * Pointer can identify where the candidatework come from
         **/
        void* owner;
        /**
         * Store a candidateWord Specific data, usually index of input method data
         **/
        void* priv;
    } FcitxCandidateWord;

    /**
     * Initialize a word list
     *
     * @return Word List
     **/
    struct _FcitxCandidateWordList* FcitxCandidateWordNewList();

    /**
     * Free a word list
     *
     * @param list list to free
     * @return void
     */
    void FcitxCandidateWordFreeList(struct _FcitxCandidateWordList* list);

    /**
     * Insert a candidate to position
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @param position position to insert
     * @return void
     **/
    void FcitxCandidateWordInsert(FcitxCandidateWordList* candList,
                                  FcitxCandidateWord* candWord, int position);

    /**
     * Merge newList into candList at a certain position
     * (newList will be cleared)
     *
     * @param candList candidate words list
     * @param newList candidate words list to be inserted
     * @param position position to insert (less than 0 to append)
     * @return void
     *
     * @since 4.2.6
     **/
    void FcitxCandidateWordMerge(FcitxCandidateWordList* candList,
                                   FcitxCandidateWordList* newList,
                                   int position);
    /**
     * Insert non-display place holder candidate to position
     *
     * @param candList candidate word list
     * @param position position to insert
     * @return void
     **/
    void FcitxCandidateWordInsertPlaceHolder(struct _FcitxCandidateWordList* candList, int position);

    /**
     * move candidate word via shift policy, for example
     * 1, 2, 3, move from 0 to 2, result is 2, 3, 1
     * 1, 2, 3, move from 2 to 0, result is 3, 1, 2
     *
     * @param candList candidate word list
     * @param from from position
     * @param to to position
     *
     * @since 4.2.5
     **/
    void FcitxCandidateWordMove(FcitxCandidateWordList* candList, int from, int to);

    /**
     * @param candList candidate word list
     * @param from from position
     * @param to to position
     *
     * @see FcitxCandidateWordMove
     *
     * @since 4.2.5
     **/
    void FcitxCandidateWordMoveByWord(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord, int to);

    /**
     * add a candidate word at last
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @return void
     **/
    void FcitxCandidateWordAppend(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * remove a candidate word from list
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @return void
     *
     * @since 4.2
     **/
    void FcitxCandidateWordRemove(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * remove a candidate word at certain index from list
     *
     * @param candList candidate word list
     * @param idx index of the word to be removed
     * @return void
     *
     * @since 4.2.6
     **/
    void FcitxCandidateWordRemoveByIndex(FcitxCandidateWordList *candList,
                                         int idx);

    /**
     * set page by index
     *
     * @param candList candidate word list
     * @param index page index
     * @return void
     *
     * @since 4.2.5
     **/
    void FcitxCandidateWordSetPage(struct _FcitxCandidateWordList* candList, int index);

    /**
     * set page by word index
     *
     * @param candList candidate word list
     * @param index index of candidate word
     * @return void
     *
     * @since 4.2.1
     **/
    void FcitxCandidateWordSetFocus(struct _FcitxCandidateWordList* candList, int index);

    /**
     * Get index of the first word on current page
     *
     * @param candList candidate word list
     * @return int index of the current word
     *
     * @since 4.2.5
     **/
    int FcitxCandidateWordGetCurrentIndex(struct _FcitxCandidateWordList* candList);

    /**
     * Get the first word on current page
     *
     * @param candList candidate word list
     * @return FcitxCandidateWord* first candidate word of current page
     **/
    FcitxCandidateWord* FcitxCandidateWordGetCurrentWindow(struct _FcitxCandidateWordList* candList);

    /**
     * get next word of current page, useful when you want to iteration over current candidate words
     *
     * @param candList candidate word list
     * @param candWord current cand word
     * @return FcitxCandidateWord* next cand word
     **/
    FcitxCandidateWord* FcitxCandidateWordGetCurrentWindowNext(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * get prev word of current page, useful when you want to iteration over current candidate words
     *
     * @param candList candidate word list
     * @param candWord current cand word
     * @return FcitxCandidateWord* prev cand word
     *
     * @since 4.2.7
     **/
    FcitxCandidateWord* FcitxCandidateWordGetCurrentWindowPrev(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * get candidate word by total index
     *
     * @param candList candidate word list
     * @param index index of word
     * @return FcitxCandidateWord*
     *
     * @since 4.2.5
     **/
    FcitxCandidateWord* FcitxCandidateWordGetByTotalIndex(struct _FcitxCandidateWordList* candList, int index);

    /**
     * get the index of the candidate word
     *
     * @param candList candidate word list
     * @param FcitxCandidateWord*
     * @return index index of word
     *
     * @since 4.2.7
     **/
    int FcitxCandidateWordGetIndex(FcitxCandidateWordList *candList,
                                   FcitxCandidateWord *word);
    /**
     * get candidate word by index within current page
     *
     * @param candList candidate word list
     * @param index index of word on current page
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetByIndex(struct _FcitxCandidateWordList* candList, int index);

    /**
     * do the candidate word selection, will trigger the candidate word callback
     *
     * @param candList candidate word list
     * @param index index of current page
     * @return INPUT_RETURN_VALUE
     **/
    INPUT_RETURN_VALUE FcitxCandidateWordChooseByIndex(struct _FcitxCandidateWordList* candList, int index);

    /**
     * do the candidate word selection, will trigger the candidate word callback
     *
     * @param candList candidate word list
     * @param index index of current page
     * @return INPUT_RETURN_VALUE
     *
     * @since 4.2.7
     **/
    INPUT_RETURN_VALUE FcitxCandidateWordChooseByTotalIndex(struct _FcitxCandidateWordList* candList, int index);

    /**
     * Free a candidate word, used by utarray
     *
     * @param arg candidateWord
     * @return void
     **/
    void FcitxCandidateWordFree(void* arg);

    /**
     * check candidate word has next page or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordHasNext(struct _FcitxCandidateWordList* candList);

    /**
     * check candidate word has prev page or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordHasPrev(struct _FcitxCandidateWordList* candList);

    /**
     * get number of total page
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordPageCount(struct _FcitxCandidateWordList* candList);

    /**
     * clear all candidate words
     *
     * @param candList candidate word list
     * @return void
     **/
    void FcitxCandidateWordReset(struct _FcitxCandidateWordList* candList);

    /**
     * go to prev page, return operation successful or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordGoPrevPage(struct _FcitxCandidateWordList* candList);

    /**
     * go to next page, return operation successful or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordGoNextPage(struct _FcitxCandidateWordList* candList);

    /**
     * set the select key string, length up to 10, usually "1234567890"
     *
     * @param candList candidate word list
     * @param strChoose select key string
     * @return void
     **/
    void FcitxCandidateWordSetChoose(struct _FcitxCandidateWordList* candList, const char* strChoose);

    /**
     * set the select key string, length up to 10, usually "1234567890"
     *
     * @param candList candidate word list
     * @param strChoose select key string
     * @param state keystate
     * @return void
     **/
    void FcitxCandidateWordSetChooseAndModifier(struct _FcitxCandidateWordList* candList, const char* strChoose, unsigned int state);

    /**
     * get the select key string
     *
     * @param candList candidate word list
     * @return const char* select key string
     **/
    const char* FcitxCandidateWordGetChoose(struct _FcitxCandidateWordList* candList);


    /**
     * get select key state
     *
     * @param candList candidate word list
     * @return unsigned int
     **/
    unsigned int FcitxCandidateWordGetModifier(struct _FcitxCandidateWordList* candList);

    /**
     * resize the candidate word length
     *
     * @param candList candidate word list
     * @param length new length
     * @return void
     **/
    void FcitxCandidateWordResize(struct _FcitxCandidateWordList* candList, int length);

    /**
     * Get current page size of candidate list
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordGetPageSize(struct _FcitxCandidateWordList* candList);

    /**
     * Set current page size of candidate list
     *
     * @param candList candidate word list
     * @param size new page size
     * @return void
     **/
    void FcitxCandidateWordSetPageSize(struct _FcitxCandidateWordList* candList, int size);

    /**
     * get current page number
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordGetCurrentPage(struct _FcitxCandidateWordList* candList);

    /**
     * get current page window size, may less than max page size
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordGetCurrentWindowSize(struct _FcitxCandidateWordList* candList);

    /**
     * get total candidate word count
     *
     * @param candList candidate word list
     * @return int
     **/

    int FcitxCandidateWordGetListSize(struct _FcitxCandidateWordList* candList);

    /**
     * check this have been goto prev page or not
     *
     * @param candList candidate word list
     * @return boolean
     */
    boolean FcitxCandidateWordGetHasGoneToPrevPage(FcitxCandidateWordList* candList);

    /**
     * check this have been goto next page or not
     *
     * @param candList candidate word list
     * @return boolean
     */
    boolean FcitxCandidateWordGetHasGoneToNextPage(FcitxCandidateWordList* candList);

    /**
     * get first candidate word
     *
     * @param candList candidate word list
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord *FcitxCandidateWordGetFirst(FcitxCandidateWordList *candList);

    /**
     * get last candidate word
     *
     * @param candList candidate word list
     * @return FcitxCandidateWord*
     *
     * @since 4.2.7
     **/
    FcitxCandidateWord *FcitxCandidateWordGetLast(FcitxCandidateWordList *candList);

    /**
     * get next candidate word, useful when want to iterate over whole list
     *
     * @param candList candidate word list
     * @param candWord current candidate word
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetNext(FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * get previous candidate word
     *
     * @param candList candidate word list
     * @param candWord current candidate word
     * @return FcitxCandidateWord*
     *
     * @since 4.2.7
     **/
    FcitxCandidateWord *FcitxCandidateWordGetPrev(FcitxCandidateWordList *candList, FcitxCandidateWord *candWord);

    /**
     * check is choose key or not, if so, return the choose index
     *
     * @param candList candidate words
     * @param sym keysym
     * @param state keystate
     * @return int
     *
     * @since 4.2.6
     **/
    int FcitxCandidateWordCheckChooseKey(FcitxCandidateWordList *candList,
                                         FcitxKeySym sym, unsigned int state);

    /**
     * Set Candidate word layout hint
     *
     * @param candList candidate words
     * @param hint layout hint
     * @return void
     *
     * @since 4.2.7
     */
    void FcitxCandidateWordSetLayoutHint(FcitxCandidateWordList* candList, FcitxCandidateLayoutHint hint);

    /**
     * Get Candidate word layout hint
     *
     * @param candList candidate words
     * @return layout hint
     *
     * @since 4.2.7
     */
    FcitxCandidateLayoutHint FcitxCandidateWordGetLayoutHint(FcitxCandidateWordList* candList);

    typedef boolean (*FcitxPaging)(void* arg, boolean prev);

    /**
     * override default paging
     *
     * @param candList candidate words
     * @param hasPrev has prev page
     * @param hasNext has next page
     * @param paging callback
     * @param arg arg
     * @param destroyNotify destroyNotify
     * @return void
     *
     * @since 4.2.7
     **/
    void FcitxCandidateWordSetOverridePaging(FcitxCandidateWordList* candList,
                                             boolean hasPrev,
                                             boolean hasNext,
                                             FcitxPaging paging,
                                             void* arg,
                                             FcitxDestroyNotify destroyNotify
                                            );

    /**
     * override default highlight
     *
     * @param candList candidate words
     * @param overrideValue value
     *
     * @since 4.2.8
     */
    void FcitxCandidateWordSetOverrideDefaultHighlight(FcitxCandidateWordList* candList, boolean overrideValue);

/** convinient string for candidate word */
#define DIGIT_STR_CHOOSE "1234567890"

    static inline void
    FcitxCandidateWordSetType(FcitxCandidateWord *cand_word,
                              FcitxMessageType type)
    {
        cand_word->wordType = (FcitxMessageType)((cand_word->wordType &
                                                  ~MSG_REGULAR_MASK) | type);
    }

    static inline boolean
    FcitxCandidateWordCheckFocus(FcitxCandidateWord *cand_word, boolean clear)
    {
        if ((cand_word->wordType & MSG_REGULAR_MASK) == MSG_CANDIATE_CURSOR) {
            if (clear)
                FcitxCandidateWordSetType(cand_word, MSG_OTHER);
            return true;
        }
        return false;
    }

    FcitxCandidateWord *FcitxCandidateWordGetFocus(
        FcitxCandidateWordList *cand_list, boolean clear);

#ifdef __cplusplus
}
#endif

#endif
/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
