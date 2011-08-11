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
#ifndef _PY_PARSER_H
#define _PY_PARSER_H

#include <stdint.h>
#include "fcitx/fcitx.h"
#include "fcitx-config/fcitx-config.h"
#include "pydef.h"

#define PY_SEPARATOR '\''
#define PY_SEPARATOR_S "'"

typedef enum _PY_PARSE_INPUT_MODE {
    PARSE_ERROR = 0,
    PARSE_SINGLEHZ = 1,
    PARSE_PHRASE = 2,
    PARSE_ABBR = 4
} PY_PARSE_INPUT_MODE;

typedef enum _PYPARSEINPUTMODE {
    PY_PARSE_INPUT_USER = '0',
    PY_PARSE_INPUT_SYSTEM = ' '
} PYPARSEINPUTMODE;     //这个值不能随意修改

typedef struct _ParsePYStruct {
    char            strPYParsed[MAX_WORDS_USER_INPUT + 3][MAX_PY_LENGTH + 2];
    char            strMap[MAX_WORDS_USER_INPUT + 3][3];
    int8_t          iHZCount;
    char            iMode;
} ParsePYStruct;

struct _FcitxPinyinConfig;

int             IsSyllabary (const char *strPY, boolean bMode);
int             IsConsonant (const char *strPY, boolean bMode);
int             FindPYFAIndex (struct _FcitxPinyinConfig* pyconfig, const char *strPY, boolean bMode);
void            ParsePY (struct _FcitxPinyinConfig* pyconfig, const char* strPY, ParsePYStruct* parsePY, PYPARSEINPUTMODE mode, boolean bSP);
boolean            MapPY (struct _FcitxPinyinConfig* pyconfig, const char* strPYorigin, char strMap[3], PYPARSEINPUTMODE mode);

boolean            MapToPY (char strMap[3], char *strPY);
int             CmpMap (struct _FcitxPinyinConfig* pyconfig, char* strMap1, char* strMap2, int* iMatchedLength, boolean bSP);
int     Cmp1Map (struct _FcitxPinyinConfig* pyconfig, char map1, char map2, boolean b, boolean bUseMH, boolean bSP);
int             Cmp2Map (struct _FcitxPinyinConfig* pyconfig, char map1[3], char map2[3], boolean bSP);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0; 
