/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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
#ifndef _PY_H
#define _PY_H

#include "fcitx/ime.h"
#include "fcitx/fcitx.h"
#include "pyconfig.h"
#include "pyParser.h"

#define PY_BASE_FILE	"pybase.mb"
#define PY_PHRASE_FILE	"pyphrase.mb"
#define PY_USERPHRASE_FILE "pyusrphrase.mb"
#define PY_INDEX_FILE	"pyindex.dat"
#define PY_FREQ_FILE	"pyfreq.mb"
#define PY_SYMBOL_FILE	"pySym.mb"

#define AUTOSAVE_PHRASE_COUNT 	1024
#define AUTOSAVE_ORDER_COUNT  	1024
#define AUTOSAVE_FREQ_COUNT  	32

struct _FcitxInstance;
struct _FcitxPinyinState;
struct MHPY;
struct MHPY_TEMPLATE;
struct _FcitxModuleFunctionArg;

typedef enum _FIND_MODE {
    FIND_PHRASE,
    FIND_BASE,
    FIND_FREQ,
    FIND_SYM
} FINDMODE;

typedef enum _PY_CAND_WORD_TYPE {
    PY_CAND_AUTO,
    PY_CAND_SYMBOL,
    PY_CAND_BASE,
    PY_CAND_SYMPHRASE,
    PY_CAND_USERPHRASE,
    PY_CAND_FREQ,
    PY_CAND_REMIND
} PY_CAND_WORD_TYPE;

typedef struct _HZ {
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    int             iPYFA;
    uint            iHit;
    uint            iIndex;
    struct _HZ     *next;
    uint            flag:1;
} HZ;

typedef struct _PYFREQ {
    HZ             *HZList;
    char            strPY[MAX_PY_PHRASE_LENGTH * MAX_PY_LENGTH + 1];
    uint            iCount;
    boolean            bIsSym;	//For special symbols
    struct _PYFREQ  *next;
} PyFreq;

typedef struct _PYPHRASE {
    char           *strPhrase;
    char           *strMap;
    struct _PYPHRASE *next;
    uint            iIndex;
    uint            iHit;
    uint            flag:1;
} PyPhrase;

typedef struct _PYBASE {
    char            strHZ[UTF8_MAX_LENGTH + 1];
    struct _PYPHRASE *phrase;
    int             iPhrase;
    struct _PYPHRASE *userPhrase;
    int             iUserPhrase;
    uint            iIndex;
    uint            iHit;
    uint            flag:1;
} PyBase;

typedef struct _PYFA {
    char            strMap[3];
    struct _PYBASE  *pyBase;
    int             iBase;
} PYFA;

typedef struct _PYINDEX {
    int             iPYFA;
    int             iBase;
    int             iPhrase;
    struct _PYINDEXCANDWORD *next;
    struct _PYINDEXCANDWORD *prev;
} PYIndex;

typedef struct _PYFREQCANDWORD {
    HZ             *hz;
    char           *strPY;
} PYFreqCandWord;

typedef struct _PYPHRASECANDWORD {
    int             iPYFA;
    int             iBase;
    struct _PYPHRASE *phrase;
} PYPhraseCandWord;

typedef struct _PYBASECANDWORD {
    int             iPYFA;
    int             iBase;
} PYBaseCandWord;

typedef struct _PYREMINDCANDWORD {
    PyPhrase       *phrase;
    int             iLength;
} PYRemindCandWord;

typedef union {
    PYFreqCandWord  sym;
    PYFreqCandWord  freq;
    PYBaseCandWord  base;
    PYPhraseCandWord phrase;
} PCand;

typedef struct _PYCANDWORD {
    PCand           cand;
    uint            iWhich:3;	//0->Auto 1->System single HZ 2->System phrase 3->User phrase 4->frequent HZ
} PYCandWord;

typedef struct _PYCANDINDEX {
    int             iPYFA;
    int             iBase;
    int             iPhrase;
} PYCandIndex;

typedef struct _PY_SELECTED{
    char            strPY[(MAX_PY_LENGTH + 1) * MAX_PY_PHRASE_LENGTH + 1];
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    char            strMap[MAX_PY_PHRASE_LENGTH * 2 + 1];
} PY_SELECTED;

typedef struct _FcitxPinyinState 
{
    FcitxPinyinConfig pyconfig;

    int iPYFACount;
    PYFA *PYFAList;
    uint iCounter;
    uint iOrigCounter;
    boolean bPYBaseDictLoaded;
    boolean bPYOtherDictLoaded;

    PyFreq *pyFreq, *pCurFreq;
    uint iPYFreqCount;

    char strFindString[MAX_USER_INPUT + 2];
    ParsePYStruct findMap;
    int iPYInsertPoint;

    char strPYRemindSource[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYRemindMap[MAX_WORDS_USER_INPUT * 2 + 1];
    PyBase *pyBaseForLengend;
    PYRemindCandWord PYRemindCandWords[MAX_CAND_WORD];

    PY_SELECTED pySelected[MAX_WORDS_USER_INPUT + 1];
    uint iPYSelected;

    PYCandWord PYCandWords[MAX_CAND_WORD];
    char strPYAuto[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYAutoMap[MAX_WORDS_USER_INPUT * 2 + 1];

    int  iNewPYPhraseCount;
    int  iOrderCount;
    char iNewFreqCount;

    int8_t iYCDZ;

    boolean bIsPYAddFreq;
    boolean bIsPYDelFreq;
    boolean bIsPYDelUserPhr;

    boolean isSavingPYUserPhrase;
    boolean isSavingPYIndex;
    boolean isSavingPYFreq;

    boolean bSP_UseSemicolon;
    boolean bSP;
    struct _FcitxInstance *owner;
} FcitxPinyinState;

void *PYCreate(struct _FcitxInstance* instance);
boolean            PYInit (void* arg);
boolean         LoadPYBaseDict (struct _FcitxPinyinState* pystate);
boolean         LoadPYOtherDict (struct _FcitxPinyinState* pystate);
void            ResetPYStatus (void* pystate);
int             GetBaseIndex (struct _FcitxPinyinState* pystate, int iPYFA, char* strBase);
INPUT_RETURN_VALUE DoPYInput(void* arg, FcitxKeySym sym, unsigned int state);
void            UpdateCodeInputPY (struct _FcitxPinyinState* pystate);
void            UpdateFindString (struct _FcitxPinyinState* pystate, int val);
void            CalculateCursorPosition (struct _FcitxPinyinState* pystate);

void            PYResetFlags (struct _FcitxPinyinState* pystate);
void            PYCreateAuto (struct _FcitxPinyinState* pystate);
INPUT_RETURN_VALUE PYGetCandWords (void* arg, SEARCH_MODE mode);
void		PYCreateCandString(struct _FcitxPinyinState* pystate);
void		PYGetCandText(struct _FcitxPinyinState* pystate, int iIndex, char* strText);
char           *PYGetCandWord (void* arg, int iIndex);
void            PYGetSymCandWords (struct _FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddSymCandWord (struct _FcitxPinyinState* pystate, HZ* hz, SEARCH_MODE mode);
void            PYGetBaseCandWords (struct _FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddBaseCandWord (struct _FcitxPinyinState* pystate, PYCandIndex pos, SEARCH_MODE mode);
void            PYGetFreqCandWords (struct _FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddFreqCandWord (struct _FcitxPinyinState* pystate,HZ * hz, char *strPY, SEARCH_MODE mode);
void            PYGetPhraseCandWords (struct _FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddPhraseCandWord (struct _FcitxPinyinState* pystate, PYCandIndex pos, PyPhrase* phrase, SEARCH_MODE mode, boolean b);
void            PYGetCandWordsForward (struct _FcitxPinyinState* pystate);
void            PYGetCandWordsBackward (struct _FcitxPinyinState* pystate);
boolean         PYCheckNextCandPage (struct _FcitxPinyinState* pystate);
void            PYSetCandWordFlag (struct _FcitxPinyinState* pystate, int iIndex, boolean flag);
void            PYSetCandWordsFlag (struct _FcitxPinyinState* pystate, boolean flag);
boolean         PYAddUserPhrase (struct _FcitxPinyinState* pystate, char *phrase, char *map);
void            PYDelUserPhrase (struct _FcitxPinyinState* pystate, int iPYFA, int iBase, PyPhrase * phrase);
int             GetBaseMapIndex (struct _FcitxPinyinState* pystate, char *strMap);
void            SavePYUserPhrase (struct _FcitxPinyinState* pystate);
void            SavePYFreq (struct _FcitxPinyinState* pystate);
void            SavePYIndex (struct _FcitxPinyinState* pystate);
void            SavePY (void *arg);

void            PYAddFreq (struct _FcitxPinyinState* pystate, int iIndex);
void            PYDelFreq (struct _FcitxPinyinState* pystate, int iIndex);
boolean            PYIsInFreq (struct _FcitxPinyinState* pystate, char *strHZ);

INPUT_RETURN_VALUE PYGetRemindCandWords (void* arg, SEARCH_MODE mode);
boolean            PYAddLengendCandWord (struct _FcitxPinyinState* pystate,PyPhrase * phrase, SEARCH_MODE mode);
char           *PYGetRemindCandWord (void* arg, int iIndex);
void            PYSetRemindCandWordsFlag (struct _FcitxPinyinState* pystate, boolean flag);
void		PYGetPYByHZ(struct _FcitxPinyinState* pystate, char *strHZ, char *strPY);

#endif

