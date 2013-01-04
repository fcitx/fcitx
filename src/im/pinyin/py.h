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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
#ifndef _PY_H
#define _PY_H

#include "fcitx/ime.h"
#include "fcitx/fcitx.h"
#include "fcitx-utils/memory.h"
#include "fcitx/candidate.h"
#include "fcitx/instance.h"
#include "pyconfig.h"
#include "pyParser.h"

#define PY_BASE_FILE    "pybase.mb"
#define PY_PHRASE_FILE  "pyphrase.mb"
#define PY_USERPHRASE_FILE "pyusrphrase.mb"
#define PY_INDEX_FILE   "pyindex.dat"
#define PY_FREQ_FILE    "pyfreq.mb"
#define PY_SYMBOL_FILE  "pySym.mb"

#define AUTOSAVE_PHRASE_COUNT   1024
#define AUTOSAVE_ORDER_COUNT    1024
#define AUTOSAVE_FREQ_COUNT     32

typedef enum {
    FIND_PHRASE,
    FIND_BASE,
    FIND_FREQ,
} FINDMODE;

typedef enum {
    PY_CAND_AUTO,
    PY_CAND_BASE,
    PY_CAND_SYSPHRASE,
    PY_CAND_USERPHRASE,
    PY_CAND_FREQ,
    PY_CAND_REMIND
} PY_CAND_WORD_TYPE;

typedef struct _HZ {
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    int32_t         iPYFA;
    uint32_t        iHit;
    uint32_t        iIndex;
    struct _HZ     *next;
} HZ;

typedef struct _PYFREQ {
    HZ             *HZList;
    char            strPY[MAX_PY_PHRASE_LENGTH * MAX_PY_LENGTH + 1];
    uint32_t        iCount;
    struct _PYFREQ  *next;
} PyFreq;

typedef struct {
    char           *strPhrase;
    char           *strMap;
    uint32_t       iIndex;
    uint32_t       iHit;
} PyPhrase;

typedef struct _PYUSRPHRASE {
    PyPhrase phrase;
    struct _PYUSRPHRASE* next;
} PyUsrPhrase;

#define USER_PHRASE_NEXT(x) (&((PyUsrPhrase*)(x))->next->phrase)

typedef struct {
    char            strHZ[UTF8_MAX_LENGTH + 1];
    PyPhrase *phrase;
    int             iPhrase;
    PyUsrPhrase *userPhrase;
    int             iUserPhrase;
    uint32_t        iIndex;
    uint32_t        iHit;
} PyBase;

typedef struct {
    char strMap[3];
    PyBase *pyBase;
    int32_t iBase;
} PYFA;

typedef struct {
    HZ             *hz;
    char           *strPY;
    PyFreq         *pyFreq;
} PYFreqCandWord;

typedef struct {
    int32_t iPYFA;
    int32_t iBase;
    PyPhrase *phrase;
} PYPhraseCandWord;

typedef struct {
    int32_t iPYFA;
    int32_t iBase;
} PYBaseCandWord;

typedef struct {
    PyPhrase *phrase;
    int      iLength;
} PYRemindCandWord;

typedef union {
    PYFreqCandWord  freq;
    PYBaseCandWord  base;
    PYPhraseCandWord phrase;
    PYRemindCandWord remind;
} PCand;

typedef struct {
    PCand                 cand;
    /**
     * 0->Auto
     * 1->System single HZ
     * 2->System phrase
     * 3->User phrase
     * 4->frequent HZ
     **/
    PY_CAND_WORD_TYPE     iWhich;
} PYCandWord;

typedef struct {
    int32_t iPYFA;
    int32_t iBase;
    int32_t iPhrase;
} PYCandIndex;

typedef struct {
    char            strPY[(MAX_PY_LENGTH + 1) * MAX_PY_PHRASE_LENGTH + 1];
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    char            strMap[MAX_PY_PHRASE_LENGTH * 2 + 1];
} PY_SELECTED;

typedef struct _FcitxPinyinState {
    FcitxPinyinConfig pyconfig;

    int32_t iPYFACount;
    PYFA *PYFAList;
    uint32_t iCounter;
    uint32_t iOrigCounter;
    boolean bPYBaseDictLoaded;
    boolean bPYOtherDictLoaded;

    PyFreq *pyFreq;
    uint32_t iPYFreqCount;

    char strFindString[MAX_USER_INPUT + 2];
    ParsePYStruct findMap;
    int iPYInsertPoint;

    char strPYRemindSource[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYRemindMap[MAX_WORDS_USER_INPUT * 2 + 1];

    PY_SELECTED pySelected[MAX_WORDS_USER_INPUT + 1];
    uint iPYSelected;

    char strPYAuto[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYAutoMap[MAX_WORDS_USER_INPUT * 2 + 1];

    int iNewPYPhraseCount;
    int iOrderCount;
    int iNewFreqCount;

    boolean bIsPYAddFreq;
    boolean bIsPYDelFreq;
    boolean bIsPYDelUserPhr;

    boolean bSP_UseSemicolon;
    boolean bSP;

    FcitxMemoryPool *pool;

    FcitxInstance *owner;
} FcitxPinyinState;

void *PYCreate(FcitxInstance *instance);
void PYDestroy(void* arg);
boolean         PYInit(void* arg);
boolean         SPInit(void* arg);
boolean         LoadPYBaseDict(FcitxPinyinState* pystate);
boolean         LoadPYOtherDict(FcitxPinyinState* pystate);
void            ResetPYStatus(void* pystate);
int             GetBaseIndex(FcitxPinyinState* pystate, int32_t iPYFA,
                             char *strBase);
INPUT_RETURN_VALUE DoPYInput(void* arg, FcitxKeySym sym, unsigned int state);
void            UpdateCodeInputPY(FcitxPinyinState* pystate);
void            UpdateFindString(FcitxPinyinState* pystate, int val);
void            CalculateCursorPosition(FcitxPinyinState* pystate);

void            PYCreateAuto(FcitxPinyinState* pystate);
INPUT_RETURN_VALUE PYGetCandWords(void* arg);
INPUT_RETURN_VALUE PYGetCandWord(void* arg, FcitxCandidateWord *pycandWord);
void            PYGetBaseCandWords(FcitxPinyinState* pystate, PyFreq* pCurFreq);
void PYAddBaseCandWord(PYCandIndex pos, PYCandWord* pycandWord);
void            PYGetFreqCandWords(FcitxPinyinState* pystate, PyFreq* pyFreq);
void            PYAddFreqCandWord(PyFreq* pyFreq, HZ* hz, char* strPY, PYCandWord* pycandWord);
void            PYGetPhraseCandWords(FcitxPinyinState* pystate);
boolean         PYAddPhraseCandWord(FcitxPinyinState* pystate, PYCandIndex pos, PyPhrase* phrase, boolean b, PYCandWord* pycandWord);
boolean         PYAddUserPhrase(FcitxPinyinState* pystate, const char* phrase,
                                const char* map, boolean incHit);
void            PYDelUserPhrase(FcitxPinyinState* pystate, int32_t iPYFA,
                                int iBase, PyUsrPhrase* phrase);
int             GetBaseMapIndex(FcitxPinyinState* pystate, char *strMap);
void            SavePYUserPhrase(FcitxPinyinState* pystate);
void            SavePYFreq(FcitxPinyinState* pystate);
void            SavePYIndex(FcitxPinyinState* pystate);
void            SavePY(void *arg);

void            PYAddFreq(FcitxPinyinState* pystate, PYCandWord* pycandWord);
void            PYDelFreq(FcitxPinyinState* pystate, PYCandWord* pycandWord);
boolean         PYIsInFreq(PyFreq* pCurFreq, char* strHZ);

INPUT_RETURN_VALUE PYGetRemindCandWords(void* arg);
void            PYAddRemindCandWord(FcitxPinyinState* pystate, PyPhrase * phrase, PYCandWord* pycandWord);
void            PYGetPYByHZ(FcitxPinyinState* pystate, const char *strHZ, char *strPY);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
