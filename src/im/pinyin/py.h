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

#define strNameOfPinyin __("Pinyin")
#define strIconNameOfPinyin "pinyin"

struct FcitxInstance;
struct FcitxPinyinState;
struct MHPY;
struct MHPY_TEMPLATE;
struct FcitxModuleFunctionArg;

typedef enum FIND_MODE {
    FIND_PHRASE,
    FIND_BASE,
    FIND_FREQ,
    FIND_SYM
} FINDMODE;

typedef enum {
    PY_CAND_AUTO,
    PY_CAND_SYMBOL,
    PY_CAND_BASE,
    PY_CAND_SYMPHRASE,
    PY_CAND_USERPHRASE,
    PY_CAND_FREQ,
    PY_CAND_LEGEND
} PY_CAND_WORD_TYPE;

typedef struct _HZ {
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    int             iPYFA;
    uint            iHit;
    uint            iIndex;
    struct _HZ     *next;
    uint            flag:1;
} HZ;

typedef struct PYFREQ {
    HZ             *HZList;
    char            strPY[MAX_PY_PHRASE_LENGTH * MAX_PY_LENGTH + 1];
    uint            iCount;
    boolean            bIsSym;	//For special symbols
    struct PYFREQ  *next;
} PyFreq;

typedef struct PYPHRASE {
    char           *strPhrase;
    char           *strMap;
    struct PYPHRASE *next;
    uint            iIndex;
    uint            iHit;
    uint            flag:1;
} PyPhrase;

typedef struct PYBASE {
    char            strHZ[UTF8_MAX_LENGTH + 1];
    struct PYPHRASE *phrase;
    int             iPhrase;
    struct PYPHRASE *userPhrase;
    int             iUserPhrase;
    uint            iIndex;
    uint            iHit;
    uint            flag:1;
} PyBase;

typedef struct _PYFA {
    char            strMap[3];
    struct PYBASE  *pyBase;
    int             iBase;
} PYFA;

typedef struct PYINDEX {
    int             iPYFA;
    int             iBase;
    int             iPhrase;
    struct PYINDEXCANDWORD *next;
    struct PYINDEXCANDWORD *prev;
} PYIndex;

typedef struct PYFREQCANDWORD {
    HZ             *hz;
    char           *strPY;
} PYFreqCandWord;

typedef struct PYPHRASECANDWORD {
    int             iPYFA;
    int             iBase;
    struct PYPHRASE *phrase;
} PYPhraseCandWord;

typedef struct PYBASECANDWORD {
    int             iPYFA;
    int             iBase;
} PYBaseCandWord;

typedef struct PYLEGENDCANDWORD {
    PyPhrase       *phrase;
    int             iLength;
} PYLegendCandWord;

typedef union {
    PYFreqCandWord  sym;
    PYFreqCandWord  freq;
    PYBaseCandWord  base;
    PYPhraseCandWord phrase;
} PCand;

typedef struct PYCANDWORD {
    PCand           cand;
    uint            iWhich:3;	//0->Auto 1->System single HZ 2->System phrase 3->User phrase 4->frequent HZ
} PYCandWord;

typedef struct PYCANDINDEX {
    int             iPYFA;
    int             iBase;
    int             iPhrase;
} PYCandIndex;

typedef struct {
    char            strPY[(MAX_PY_LENGTH + 1) * MAX_PY_PHRASE_LENGTH + 1];
    char            strHZ[MAX_PY_PHRASE_LENGTH * UTF8_MAX_LENGTH + 1];
    char            strMap[MAX_PY_PHRASE_LENGTH * 2 + 1];
} PY_SELECTED;

typedef struct FcitxPinyinState 
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

    char strPYLegendSource[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYLegendMap[MAX_WORDS_USER_INPUT * 2 + 1];
    PyBase *pyBaseForLengend;
    PYLegendCandWord PYLegendCandWords[MAX_CAND_WORD];

    PY_SELECTED pySelected[MAX_WORDS_USER_INPUT + 1];
    uint iPYSelected;

    PYCandWord PYCandWords[MAX_CAND_WORD];
    char strPYAuto[MAX_WORDS_USER_INPUT * UTF8_MAX_LENGTH + 1];
    char strPYAutoMap[MAX_WORDS_USER_INPUT * 2 + 1];

    char iNewPYPhraseCount;
    char iOrderCount;
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
    struct FcitxInstance *owner;
} FcitxPinyinState;

void *PYCreate(struct FcitxInstance* instance);
boolean            PYInit (void* arg);
boolean         LoadPYBaseDict (struct FcitxPinyinState* pystate);
boolean         LoadPYOtherDict (struct FcitxPinyinState* pystate);
void            ResetPYStatus (void* pystate);
int             GetBaseIndex (struct FcitxPinyinState* pystate, int iPYFA, char* strBase);
INPUT_RETURN_VALUE DoPYInput(void* arg, FcitxKeySym sym, unsigned int state);
void            UpdateCodeInputPY (struct FcitxPinyinState* pystate);
void            UpdateFindString (struct FcitxPinyinState* pystate, int val);
void            CalculateCursorPosition (struct FcitxPinyinState* pystate);

void            PYResetFlags (struct FcitxPinyinState* pystate);
void            PYCreateAuto (struct FcitxPinyinState* pystate);
INPUT_RETURN_VALUE PYGetCandWords (void* arg, SEARCH_MODE mode);
void		PYCreateCandString(struct FcitxPinyinState* pystate);
void		PYGetCandText(struct FcitxPinyinState* pystate, int iIndex, char* strText);
char           *PYGetCandWord (void* arg, int iIndex);
void            PYGetSymCandWords (struct FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddSymCandWord (struct FcitxPinyinState* pystate, HZ* hz, SEARCH_MODE mode);
void            PYGetBaseCandWords (struct FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddBaseCandWord (struct FcitxPinyinState* pystate, PYCandIndex pos, SEARCH_MODE mode);
void            PYGetFreqCandWords (struct FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddFreqCandWord (struct FcitxPinyinState* pystate,HZ * hz, char *strPY, SEARCH_MODE mode);
void            PYGetPhraseCandWords (struct FcitxPinyinState* pystate, SEARCH_MODE mode);
boolean         PYAddPhraseCandWord (struct FcitxPinyinState* pystate, PYCandIndex pos, PyPhrase* phrase, SEARCH_MODE mode, boolean b);
void            PYGetCandWordsForward (struct FcitxPinyinState* pystate);
void            PYGetCandWordsBackward (struct FcitxPinyinState* pystate);
boolean         PYCheckNextCandPage (struct FcitxPinyinState* pystate);
void            PYSetCandWordFlag (struct FcitxPinyinState* pystate, int iIndex, boolean flag);
void            PYSetCandWordsFlag (struct FcitxPinyinState* pystate, boolean flag);
boolean         PYAddUserPhrase (struct FcitxPinyinState* pystate, char *phrase, char *map);
void            PYDelUserPhrase (struct FcitxPinyinState* pystate, int iPYFA, int iBase, PyPhrase * phrase);
int             GetBaseMapIndex (struct FcitxPinyinState* pystate, char *strMap);
void            SavePYUserPhrase (struct FcitxPinyinState* pystate);
void            SavePYFreq (struct FcitxPinyinState* pystate);
void            SavePYIndex (struct FcitxPinyinState* pystate);
void            SavePY (void *arg);

void            PYAddFreq (struct FcitxPinyinState* pystate, int iIndex);
void            PYDelFreq (struct FcitxPinyinState* pystate, int iIndex);
boolean            PYIsInFreq (struct FcitxPinyinState* pystate, char *strHZ);

INPUT_RETURN_VALUE PYGetLegendCandWords (void* arg, SEARCH_MODE mode);
boolean            PYAddLengendCandWord (struct FcitxPinyinState* pystate,PyPhrase * phrase, SEARCH_MODE mode);
char           *PYGetLegendCandWord (void* arg, int iIndex);
void            PYSetLegendCandWordsFlag (struct FcitxPinyinState* pystate, boolean flag);
void		PYGetPYByHZ(struct FcitxPinyinState* pystate, char *strHZ, char *strPY);

#endif
