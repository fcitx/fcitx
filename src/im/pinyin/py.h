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

struct _CandidateWord;
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
    PY_CAND_SYSPHRASE,
    PY_CAND_USERPHRASE,
    PY_CAND_FREQ,
    PY_CAND_REMIND
} PY_CAND_WORD_TYPE;

typedef struct _HZ {
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    int             iPYFA;
    unsigned int            iHit;
    unsigned int            iIndex;
    struct _HZ     *next;
} HZ;

typedef struct _PYFREQ {
    HZ             *HZList;
    char            strPY[MAX_PY_PHRASE_LENGTH * MAX_PY_LENGTH + 1];
    unsigned int            iCount;
    boolean            bIsSym;	//For special symbols
    struct _PYFREQ  *next;
} PyFreq;

typedef struct _PYPHRASE {
    char           *strPhrase;
    char           *strMap;
    unsigned int            iIndex;
    unsigned int            iHit;
} PyPhrase;

typedef struct _PYUSRPHRASE {
    PyPhrase phrase;
    struct _PYUSRPHRASE* next;
} PyUsrPhrase;

#define USER_PHRASE_NEXT(x) (&((PyUsrPhrase*)(x))->next->phrase)

typedef struct _PYBASE {
    char            strHZ[UTF8_MAX_LENGTH + 1];
    struct _PYPHRASE *phrase;
    int             iPhrase;
    struct _PYUSRPHRASE *userPhrase;
    int             iUserPhrase;
    unsigned int            iIndex;
    unsigned int            iHit;
} PyBase;

typedef struct _PYFA {
    char            strMap[3];
    struct _PYBASE  *pyBase;
    int             iBase;
} PYFA;

typedef struct _PYFREQCANDWORD {
    HZ             *hz;
    char           *strPY;
    PyFreq         *pyFreq;
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
    PYRemindCandWord remind;
} PCand;

typedef struct _PYCANDWORD {
    PCand                 cand;
    PY_CAND_WORD_TYPE     iWhich;	//0->Auto 1->System single HZ 2->System phrase 3->User phrase 4->frequent HZ
} PYCandWord;

typedef struct _PYCANDINDEX {
    int             iPYFA;
    int             iBase;
    int             iPhrase;
} PYCandIndex;

typedef struct _PY_SELECTED {
    char            strPY[(MAX_PY_LENGTH + 1) * MAX_PY_PHRASE_LENGTH + 1];
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    char            strMap[MAX_PY_PHRASE_LENGTH * 2 + 1];
} PY_SELECTED;

typedef struct _FcitxPinyinState
{
    FcitxPinyinConfig pyconfig;

    int iPYFACount;
    PYFA *PYFAList;
    unsigned int iCounter;
    unsigned int iOrigCounter;
    boolean bPYBaseDictLoaded;
    boolean bPYOtherDictLoaded;

    PyFreq *pyFreq;
    unsigned int iPYFreqCount;

    char strFindString[MAX_USER_INPUT + 2];
    ParsePYStruct findMap;
    int iPYInsertPoint;

    char strPYRemindSource[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYRemindMap[MAX_WORDS_USER_INPUT * 2 + 1];

    PY_SELECTED pySelected[MAX_WORDS_USER_INPUT + 1];
    unsigned int iPYSelected;

    char strPYAuto[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYAutoMap[MAX_WORDS_USER_INPUT * 2 + 1];

    int  iNewPYPhraseCount;
    int  iOrderCount;
    int iNewFreqCount;

    int8_t iYCDZ;

    boolean bIsPYAddFreq;
    boolean bIsPYDelFreq;
    boolean bIsPYDelUserPhr;

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

void            PYCreateAuto (struct _FcitxPinyinState* pystate);
INPUT_RETURN_VALUE PYGetCandWords (void* arg);
INPUT_RETURN_VALUE PYGetCandWord (void* arg, struct _CandidateWord* pycandWord);
void            PYGetSymCandWords (FcitxPinyinState* pystate, PyFreq* pCurFreq);
void PYAddSymCandWord (HZ* hz, PYCandWord* pycandWord);
void            PYGetBaseCandWords (FcitxPinyinState* pystate, PyFreq* pCurFreq);
void PYAddBaseCandWord (PYCandIndex pos, PYCandWord* pycandWord);
void            PYGetFreqCandWords (struct _FcitxPinyinState* pystate, PyFreq* pyFreq);
void            PYAddFreqCandWord (PyFreq* pyFreq, HZ* hz, char* strPY, PYCandWord* pycandWord);
void            PYGetPhraseCandWords (struct _FcitxPinyinState* pystate);
boolean         PYAddPhraseCandWord (struct _FcitxPinyinState* pystate, PYCandIndex pos, PyPhrase* phrase, boolean b, PYCandWord* pycandWord);
boolean PYAddUserPhrase (FcitxPinyinState* pystate, char* phrase, char* map);
void            PYDelUserPhrase (FcitxPinyinState* pystate, int iPYFA, int iBase, PyUsrPhrase* phrase);
int             GetBaseMapIndex (struct _FcitxPinyinState* pystate, char *strMap);
void            SavePYUserPhrase (struct _FcitxPinyinState* pystate);
void            SavePYFreq (struct _FcitxPinyinState* pystate);
void            SavePYIndex (struct _FcitxPinyinState* pystate);
void            SavePY (void *arg);

void            PYAddFreq (struct _FcitxPinyinState* pystate, PYCandWord* pycandWord);
void            PYDelFreq (struct _FcitxPinyinState* pystate, PYCandWord* pycandWord);
boolean            PYIsInFreq (PyFreq* pCurFreq, char* strHZ);

INPUT_RETURN_VALUE PYGetRemindCandWords (void* arg);
void            PYAddRemindCandWord (struct _FcitxPinyinState* pystate, PyPhrase * phrase, PYCandWord* pycandWord);
void		PYGetPYByHZ(struct _FcitxPinyinState* pystate, char *strHZ, char *strPY);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0; 
