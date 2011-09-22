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

#ifndef FCITX_CANDIDATE_H
#define FCITX_CANDIDATE_H
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/ime.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _CandidateWord;
    struct _CandidateWordList;

    typedef INPUT_RETURN_VALUE(*CandidateWordCommitCallback)(void* arg, struct _CandidateWord* candWord);

    /**
     * @brief A Single Candidate Word
     **/
    typedef struct _CandidateWord {
        /**
         * @brief String display in the front
         **/
        char* strWord;
        /**
         * @brief String display before strWord
         **/
        char* strExtra;
        /**
         * @brief Callback when string is going to commit
         **/
        CandidateWordCommitCallback callback;
        /**
         * @brief Pointer can identify where the candidatework come from
         **/
        void* owner;
        /**
         * @brief Store a candidateWord Specific data, usually index of input method data
         **/
        void* priv;
    } CandidateWord;

    /**
     * @brief Initialize a word list, should only used by runtime
     *
     * @return _CandidateWordList*
     **/
    struct _CandidateWordList* CandidateWordInit();

    /**
     * @brief Insert a candidate to position
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @param position position to insert
     * @return void
     **/

    void CandidateWordInsert(struct _CandidateWordList* candList, CandidateWord* candWord, int position);
    /**
     * @brief add a candidate word at last
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @return void
     **/
    void CandidateWordAppend(struct _CandidateWordList* candList, CandidateWord* candWord);

    /**
     * @brief Get first of current page
     *
     * @param candList candidate word list
     * @return CandidateWord* first candidate word of current page
     **/
    CandidateWord* CandidateWordGetCurrentWindow(struct _CandidateWordList* candList);

    /**
     * @brief get next word of current page, useful when you want to iteration over current candidate words
     *
     * @param candList candidate word list
     * @param candWord current cand word
     * @return CandidateWord* next cand word
     **/
    CandidateWord* CandidateWordGetCurrentWindowNext(struct _CandidateWordList* candList, CandidateWord* candWord);

    /**
     * @brief get candidate word by index
     *
     * @param candList candidate word list
     * @param index index of current page
     * @return CandidateWord*
     **/
    CandidateWord* CandidateWordGetByIndex(struct _CandidateWordList* candList, int index);

    /**
     * @brief do the candidate word selection, will trigger the candidate word callback
     *
     * @param candList candidate word list
     * @param index index of current page
     * @return INPUT_RETURN_VALUE
     **/
    INPUT_RETURN_VALUE CandidateWordChooseByIndex(struct _CandidateWordList* candList, int index);

    /**
     * @brief Free a candidate word, used by utarray
     *
     * @param arg candidateWord
     * @return void
     **/
    void CandidateWordFree(void* arg);

    /**
     * @brief check candidate word has next page or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean CandidateWordHasNext(struct _CandidateWordList* candList);

    /**
     * @brief check candidate word has prev page or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean CandidateWordHasPrev(struct _CandidateWordList* candList);

    /**
     * @brief get number of total page
     *
     * @param candList candidate word list
     * @return int
     **/
    int CandidateWordPageCount(struct _CandidateWordList* candList);

    /**
     * @brief clear all candidate words
     *
     * @param candList candidate word list
     * @return void
     **/
    void CandidateWordReset(struct _CandidateWordList* candList);

    /**
     * @brief go to prev page, return operation successful or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean CandidateWordGoPrevPage(struct _CandidateWordList* candList);

    /**
     * @brief go to next page, return operation successful or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean CandidateWordGoNextPage(struct _CandidateWordList* candList);

    /**
     * @brief set the select key string, length up to 10, usually "1234567890"
     *
     * @param candList candidate word list
     * @param strChoose select key string
     * @return void
     **/
    void CandidateWordSetChoose(struct _CandidateWordList* candList, const char* strChoose);

    /**
     * @brief get the select key string
     *
     * @param candList candidate word list
     * @return void
     **/
    const char* CandidateWordGetChoose(struct _CandidateWordList* candList);

    /**
     * @brief resize the candidate word length
     *
     * @param candList candidate word list
     * @param length new length
     * @return void
     **/
    void CandidateWordResize(struct _CandidateWordList* candList, int length);

    /**
     * @brief Get current page size of candidate list
     *
     * @param candList candidate word list
     * @return int
     **/
    int CandidateWordGetPageSize(struct _CandidateWordList* candList);

    /**
     * @brief Set current page size of candidate list
     *
     * @param candList candidate word list
     * @param size new page size
     * @return void
     **/
    void CandidateWordSetPageSize(struct _CandidateWordList* candList, int size);

    /**
     * @brief get current page number
     *
     * @param candList candidate word list
     * @return int
     **/
    int CandidateWordGetCurrentPage(struct _CandidateWordList* candList);

    /**
     * @brief get current page window size, may less than max page size
     *
     * @param candList candidate word list
     * @return int
     **/
    int CandidateWordGetCurrentWindowSize(struct _CandidateWordList* candList);

    /**
     * @brief get total candidate word count
     *
     * @param candList candidate word list
     * @return int
     **/

    int CandidateWordGetListSize(struct _CandidateWordList* candList);

    /**
     * @brief get first candidate word
     *
     * @param candList candidate word list
     * @return CandidateWord*
     **/
    CandidateWord* CandidateWordGetFirst(struct _CandidateWordList* candList);

    /**
     * @brief get next candidate word, useful when want to iterate over whole list
     *
     * @param candList candidate word list
     * @param candWord current candidate word
     * @return CandidateWord*
     **/
    CandidateWord* CandidateWordGetNext(struct _CandidateWordList* candList, CandidateWord* candWord);

#define DIGIT_STR_CHOOSE "1234567890"

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
