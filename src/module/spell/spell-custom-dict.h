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
#ifndef _FCITX_MODULE_SPELL_DICT_H
#define _FCITX_MODULE_SPELL_DICT_H

typedef struct {
    char *word;
    int dist;
} SpellCustomCWord;

typedef struct {
    char *map;
    uint32_t *words;
    int words_count;
    const char *delim;
    boolean (*word_comp_func)(unsigned int, unsigned int);
    int (*word_check_func)(const char*);
    void (*hint_cmplt_func)(SpellHint*, int);
} SpellCustomDict;

SpellCustomDict *SpellCustomNewDict(FcitxSpell *spell, const char *lang);
void SpellCustomFreeDict(FcitxSpell *spell, SpellCustomDict *dict);

#endif /* _FCITX_MODULE_SPELL_DICT_H */
