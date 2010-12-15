/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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
#ifndef _IM_H_
#define _IM_H_

#define MAX_IM_NAME    (8 * 6)

#define MAX_CAND_LEN    127
#define MAX_TIPS_LEN    9

#define MAX_CAND_WORD    10
#define MAX_USER_INPUT    300

typedef enum _SEARCH_MODE {
    SM_FIRST,
    SM_NEXT,
    SM_PREV
} SEARCH_MODE;

typedef enum _INPUT_RETURN_VALUE {
    //IRV_UNKNOWN = -1,
    IRV_DO_NOTHING = 0, /* do nothing */
    IRV_DONOT_PROCESS, /* key will be forward */
    IRV_DONOT_PROCESS_CLEAN, /* key will be forward and process as IRV_CLEAN */
    IRV_CLEAN, /* reset input */
    IRV_TO_PROCESS, /* key will passed to next flow*/ 
    IRV_DISPLAY_MESSAGE, /* it's a message, so next and prev will not be shown */
    IRV_DISPLAY_CANDWORDS, /* the only different with message it it will show next and prev button */
    IRV_DISPLAY_LAST, /* display the last input word */
    IRV_PUNC,
    IRV_ENG,
    IRV_GET_LEGEND, /* legend word */
    IRV_GET_CANDWORDS, /* send the input to client, close input window */
    IRV_GET_CANDWORDS_NEXT /* send the input to client, dont close input window */
} INPUT_RETURN_VALUE;

typedef struct {
    char Name[MAX_IM_NAME + 1];
    char IconName[MAX_IM_NAME + 1];

    void (*Reset) (void);
    INPUT_RETURN_VALUE (*DoInput) (unsigned int, unsigned int, int);
    INPUT_RETURN_VALUE (*GetCandWords)(SEARCH_MODE);
    char *(*GetCandWord) (int);
    int (*Init) (char *arg);
    int (*Destroy) (void);
    void *Bihua;

    char *CodeInput;
    char *StringGet;
    char (*CandTable)[MAX_CAND_LEN+1];
    char (*CodeTips)[MAX_TIPS_LEN+1];
    int CandWordMax;

    int CodeLen;
    int CurCandPage;
    int CandWordCount;
    int CandPageCount;
    int SelectIndex;
    int CaretPos;
    void *fc;
    void *profile;
}EXTRA_IM;

#endif
