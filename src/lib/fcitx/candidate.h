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

#ifndef FCITX_CANDIDATE_H
#define FCITX_CANDIDATE_H
#include <fcitx-utils/utarray.h>
#include <fcitx-config/fcitx-config.h>
#include <fcitx/ime.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct _FcitxCandidateWord;
    struct _FcitxCandidateWordList;

    typedef INPUT_RETURN_VALUE(*FcitxCandidateWordCommitCallback)(void* arg, struct _FcitxCandidateWord* candWord);

    /**
     * @brief A Single Candidate Word
     **/
    typedef struct _FcitxCandidateWord {
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
        FcitxCandidateWordCommitCallback callback;
        /**
         * @brief Store the candidateWord Type
         **/
        FcitxMessageType wordType;
        /**
         * @brief Store the extra Type
         **/
        FcitxMessageType extraType;
        /**
         * @brief Pointer can identify where the candidatework come from
         **/
        void* owner;
        /**
         * @brief Store a candidateWord Specific data, usually index of input method data
         **/
        void* priv;
    } FcitxCandidateWord;

    /**
     * @brief Initialize a word list, should only used by runtime
     *
     * @return _FcitxCandidateWordList*
     **/
    struct _FcitxCandidateWordList* FcitxCandidateWordNewList();

    /**
     * @brief Insert a candidate to position
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @param position position to insert
     * @return void
     **/

    void FcitxCandidateWordInsert(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord, int position);

    /**
     * @brief add a candidate word at last
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @return void
     **/
    void FcitxCandidateWordAppend(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * @brief remove a candidate word from list
     *
     * @param candList candidate word list
     * @param candWord candidate word
     * @return void
     * 
     * @since 4.2
     **/
    void FcitxCandidateWordRemove(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * @brief Get first of current page
     *
     * @param candList candidate word list
     * @return FcitxCandidateWord* first candidate word of current page
     **/
    FcitxCandidateWord* FcitxCandidateWordGetCurrentWindow(struct _FcitxCandidateWordList* candList);

    /**
     * @brief get next word of current page, useful when you want to iteration over current candidate words
     *
     * @param candList candidate word list
     * @param candWord current cand word
     * @return FcitxCandidateWord* next cand word
     **/
    FcitxCandidateWord* FcitxCandidateWordGetCurrentWindowNext(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

    /**
     * @brief get candidate word by index
     *
     * @param candList candidate word list
     * @param index index of current page
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetByIndex(struct _FcitxCandidateWordList* candList, int index);

    /**
     * @brief do the candidate word selection, will trigger the candidate word callback
     *
     * @param candList candidate word list
     * @param index index of current page
     * @return INPUT_RETURN_VALUE
     **/
    INPUT_RETURN_VALUE FcitxCandidateWordChooseByIndex(struct _FcitxCandidateWordList* candList, int index);

    /**
     * @brief Free a candidate word, used by utarray
     *
     * @param arg candidateWord
     * @return void
     **/
    void FcitxCandidateWordFree(void* arg);

    /**
     * @brief check candidate word has next page or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordHasNext(struct _FcitxCandidateWordList* candList);

    /**
     * @brief check candidate word has prev page or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordHasPrev(struct _FcitxCandidateWordList* candList);

    /**
     * @brief get number of total page
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordPageCount(struct _FcitxCandidateWordList* candList);

    /**
     * @brief clear all candidate words
     *
     * @param candList candidate word list
     * @return void
     **/
    void FcitxCandidateWordReset(struct _FcitxCandidateWordList* candList);

    /**
     * @brief go to prev page, return operation successful or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordGoPrevPage(struct _FcitxCandidateWordList* candList);

    /**
     * @brief go to next page, return operation successful or not
     *
     * @param candList candidate word list
     * @return boolean
     **/
    boolean FcitxCandidateWordGoNextPage(struct _FcitxCandidateWordList* candList);

    /**
     * @brief set the select key string, length up to 10, usually "1234567890"
     *
     * @param candList candidate word list
     * @param strChoose select key string
     * @return void
     **/
    void FcitxCandidateWordSetChoose(struct _FcitxCandidateWordList* candList, const char* strChoose);
    
    /**
     * @brief set the select key string, length up to 10, usually "1234567890"
     *
     * @param candList candidate word list
     * @param strChoose select key string
     * @param state keystate
     * @return void
     **/
    void FcitxCandidateWordSetChooseAndModifier(struct _FcitxCandidateWordList* candList, const char* strChoose, unsigned int state);

    /**
     * @brief get the select key string
     *
     * @param candList candidate word list
     * @return void
     **/
    const char* FcitxCandidateWordGetChoose(struct _FcitxCandidateWordList* candList);
    
    
    /**
     * @brief get select key state
     *
     * @param candList candidate word list
     * @return unsigned int
     **/
    unsigned int FcitxCandidateWordGetModifier(struct _FcitxCandidateWordList* candList);

    /**
     * @brief resize the candidate word length
     *
     * @param candList candidate word list
     * @param length new length
     * @return void
     **/
    void FcitxCandidateWordResize(struct _FcitxCandidateWordList* candList, int length);

    /**
     * @brief Get current page size of candidate list
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordGetPageSize(struct _FcitxCandidateWordList* candList);

    /**
     * @brief Set current page size of candidate list
     *
     * @param candList candidate word list
     * @param size new page size
     * @return void
     **/
    void FcitxCandidateWordSetPageSize(struct _FcitxCandidateWordList* candList, int size);

    /**
     * @brief get current page number
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordGetCurrentPage(struct _FcitxCandidateWordList* candList);

    /**
     * @brief get current page window size, may less than max page size
     *
     * @param candList candidate word list
     * @return int
     **/
    int FcitxCandidateWordGetCurrentWindowSize(struct _FcitxCandidateWordList* candList);

    /**
     * @brief get total candidate word count
     *
     * @param candList candidate word list
     * @return int
     **/

    int FcitxCandidateWordGetListSize(struct _FcitxCandidateWordList* candList);

    /**
     * @brief get first candidate word
     *
     * @param candList candidate word list
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetFirst(struct _FcitxCandidateWordList* candList);

    /**
     * @brief get next candidate word, useful when want to iterate over whole list
     *
     * @param candList candidate word list
     * @param candWord current candidate word
     * @return FcitxCandidateWord*
     **/
    FcitxCandidateWord* FcitxCandidateWordGetNext(struct _FcitxCandidateWordList* candList, FcitxCandidateWord* candWord);

#define DIGIT_STR_CHOOSE "1234567890"

#ifdef __cplusplus
}
#endif

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
