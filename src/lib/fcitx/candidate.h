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
     * Initialize a word list, should only used by runtime
     *
     * @return _FcitxCandidateWordList*
     **/
    struct _FcitxCandidateWordList* FcitxCandidateWordNewList();

    /**
     * Insert a candidate to position
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @param position position to insert
     * @return void
     **/

    void FcitxCandidateWordInsert(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord, int position);

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
     * set page by index
     *
     * @param candList candidate word list
     * @param index candidate word
     * @return void
     *
     * @since 4.2.1
     **/
    void FcitxCandidateWordSetFocus(struct _FcitxCandidateWordList* candList, int index);

    /**
     * Get first of current page
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
     * get candidate word by index
     *
     * @param candList candidate word list
     * @param index index of current page
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
     * @return void
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
     * get first candidate word
     *
     * @param candList candidate word list
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetFirst(struct _FcitxCandidateWordList* candList);

    /**
     * get next candidate word, useful when want to iterate over whole list
     *
     * @param candList candidate word list
     * @param candWord current candidate word
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetNext(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

/** convinient string for candidate word */
#define DIGIT_STR_CHOOSE "1234567890"

#ifdef __cplusplus
}
#endif

#endif
/**
 * @}
 */
// kate: indent-mode cstyle; space-indent on; indent-width 0;
