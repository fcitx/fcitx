/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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
#ifndef _FCITX_MODULE_SPELL_H
#define _FCITX_MODULE_SPELL_H

#include "fcitx/candidate.h"

typedef enum {
    EP_Default = 0,
    EP_Aspell,
    EP_Myspell
} EnchantProvider;

typedef struct {
    char *display;
    char *commit;
} SpellHint;

#define FCITX_SPELL_NAME "fcitx-spell"

enum {
    FCITX_SPELL_HINT_WORDS,
    FCITX_SPELL_ADD_PERSONAL,
    FCITX_SPELL_DICT_AVAILABLE,
    FCITX_SPELL_GET_CANDWORDS,
    FCITX_SPELL_CANDWORD_GET_COMMIT,
};

typedef INPUT_RETURN_VALUE (*FcitxSpellGetCandWordCb)(void *arg,
                                                      const char *commit);

typedef SpellHint* FCITX_SPELL_HINT_WORDS_RETURNTYPE;
typedef unsigned long FCITX_SPELL_ADD_PERSONAL_RETURNTYPE;
typedef unsigned long FCITX_SPELL_DICT_AVAILABLE_RETURNTYPE;
typedef FcitxCandidateWordList* FCITX_SPELL_GET_CANDWORDS_RETURNTYPE;
typedef const char* FCITX_SPELL_CANDWORD_GET_COMMIT_RETURNTYPE;

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
