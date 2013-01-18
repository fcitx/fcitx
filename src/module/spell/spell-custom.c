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

#include "fcitx/fcitx.h"
#include "config.h"

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utf8.h"
#include <sys/stat.h>
#include <time.h>
#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif

#include "spell-internal.h"
#include "spell-custom.h"
#include "spell-custom-dict.h"

/**
 * update custom dict, if the dictionary of that language
 * cannot be found, keep the current one until the next successful loading.
 **/
boolean
SpellCustomLoadDict(FcitxSpell *spell, const char *lang)
{
    SpellCustomDict *custom_dict;
    if (spell->custom_saved_lang &&
        !strcmp(spell->custom_saved_lang, lang)) {
        free(spell->custom_saved_lang);
        spell->custom_saved_lang = NULL;
        return false;
    }
    custom_dict = SpellCustomNewDict(spell, lang);
    if (custom_dict) {
        if (spell->custom_saved_lang) {
            free(spell->custom_saved_lang);
            spell->custom_saved_lang = NULL;
        }
        if (spell->custom_dict)
            SpellCustomFreeDict(spell, spell->custom_dict);
        spell->custom_dict = custom_dict;
        return true;
    }
    if (!spell->custom_dict || !spell->dictLang)
        return false;
    if (spell->custom_saved_lang)
        return false;
    spell->custom_saved_lang = strdup(spell->dictLang);
    return false;
}

/* Init work is done in set lang, nothing else to init here */
/* boolean */
/* SpellCustomInit(FcitxSpell *spell) */
/* { */
/*     return true; */
/* } */

static int
SpellCustomGetDistance(SpellCustomDict *custom_dict, const char *word,
                       const char *dict, int word_len)
{
#define REPLACE_WEIGHT 3
#define INSERT_WEIGHT 3
#define REMOVE_WEIGHT 3
#define END_WEIGHT 1
    /*
     * three kinds of error, replace, insert and remove
     * replace means apple vs aplle
     * insert means apple vs applee
     * remove means apple vs aple
     *
     * each error need to follow a correct match.
     *
     * number of "remove error" shoud be no more than "maxremove"
     * while maxremove equals to (length - 2) / 3
     *
     * and the total error number should be no more than "maxdiff"
     * while maxdiff equales to length / 3.
     */
    int replace = 0;
    int insert = 0;
    int remove = 0;
    int diff = 0;
    int maxdiff;
    int maxremove;
    unsigned int cur_word_c;
    unsigned int cur_dict_c;
    unsigned int next_word_c;
    unsigned int next_dict_c;
    maxdiff = word_len / 3;
    maxremove = (word_len - 2) / 3;
    word = fcitx_utf8_get_char(word, &cur_word_c);
    dict = fcitx_utf8_get_char(dict, &cur_dict_c);
    while ((diff = replace + insert + remove) <= maxdiff &&
           remove <= maxremove) {
        /*
         * cur_word_c and cur_dict_c are the current characters
         * and dict and word are pointing to the next one.
         */
        if (!cur_word_c) {
            return ((replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT
                     + remove * REMOVE_WEIGHT) +
                    (cur_dict_c ?
                     (fcitx_utf8_strlen(dict) + 1) * END_WEIGHT : 0));
        }
        word = fcitx_utf8_get_char(word, &next_word_c);

        /* check remove error */
        if (!cur_dict_c) {
            if (next_word_c)
                return -1;
            remove++;
            if (diff <= maxdiff && remove <= maxremove) {
                return (replace * REPLACE_WEIGHT + insert * INSERT_WEIGHT
                        + remove * REMOVE_WEIGHT);
            }
            return -1;
        }
        dict = fcitx_utf8_get_char(dict, &next_dict_c);
        if (cur_word_c == cur_dict_c ||
            (custom_dict->word_comp_func &&
             custom_dict->word_comp_func(cur_word_c, cur_dict_c))) {
            cur_word_c = next_word_c;
            cur_dict_c = next_dict_c;
            continue;
        }
        if (next_word_c == cur_dict_c ||
            (custom_dict->word_comp_func && next_word_c &&
             custom_dict->word_comp_func(next_word_c, cur_dict_c))) {
            word = fcitx_utf8_get_char(word, &cur_word_c);
            cur_dict_c = next_dict_c;
            remove++;
            continue;
        }

        /* check insert error */
        if (cur_word_c == next_dict_c ||
            (custom_dict->word_comp_func && next_dict_c &&
             custom_dict->word_comp_func(cur_word_c, next_dict_c))) {
            cur_word_c = next_word_c;
            dict = fcitx_utf8_get_char(dict, &cur_dict_c);
            insert++;
            continue;
        }

        /* check replace error */
        if (next_word_c == next_dict_c ||
            (custom_dict->word_comp_func && next_word_c && next_dict_c &&
             custom_dict->word_comp_func(next_word_c, next_dict_c))) {
            if (next_word_c) {
                dict = fcitx_utf8_get_char(dict, &cur_dict_c);
                word = fcitx_utf8_get_char(word, &cur_word_c);
            } else {
                cur_word_c = 0;
                cur_dict_c = 0;
            }
            replace++;
            continue;
        }
        break;
    }
    return -1;
}

// TODO add frequency
static int
SpellCustomCWordCompare(const void *a, const void *b)
{
    return (int)(((SpellCustomCWord*)a)->dist - ((SpellCustomCWord*)b)->dist);
}

SpellHint*
SpellCustomHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellCustomCWord clist[len_limit + 1];
    int i;
    unsigned int num = 0;
    int word_type = 0;
    SpellCustomDict *dict = spell->custom_dict;
    const char *word;
    const char *prefix = NULL;
    int prefix_len = 0;
    const char *real_word;
    SpellHint *res;
    int word_len;
    if (!SpellCustomCheck(spell))
        return NULL;
    if (!*spell->current_str)
        return NULL;
    word = spell->current_str;
    real_word = word;
    if (dict->delim && *dict->delim) {
        size_t delta;
        while (real_word[delta = strcspn(real_word, dict->delim)]) {
            prefix = word;
            real_word += delta + 1;
        }
        prefix_len = prefix ? real_word - prefix : 0;
    }
    if (!*real_word)
        return NULL;
    if (dict->word_check_func)
        word_type = dict->word_check_func(real_word);
    word_len = fcitx_utf8_strlen(real_word);
    for (i = 0;i < dict->words_count;i++) {
        int dist;
        if ((dist = SpellCustomGetDistance(
                 dict, real_word, dict->map + dict->words[i], word_len)) >= 0) {
            int j = num;
            clist[j].word = dict->map + dict->words[i];
            clist[j].dist = dist;
            if (num < len_limit)
                num++;
            for (;j > 0;j--) {
                if (SpellCustomCWordCompare(clist + j - 1, clist + j) > 0) {
                    SpellCustomCWord tmp = clist[j];
                    clist[j] = clist[j - 1];
                    clist[j - 1] = tmp;
                    continue;
                }
                break;
            }
        }
    }
    res = SpellHintListWithPrefix(num, prefix, prefix_len,
                                  &clist->word, sizeof(SpellCustomCWord));
    if (!res)
        return NULL;
    if (dict->hint_cmplt_func)
        dict->hint_cmplt_func(res, word_type);
    return res;
}

boolean
SpellCustomCheck(FcitxSpell *spell)
{
    if (spell->custom_dict && !spell->custom_saved_lang)
        return true;
    return false;
}

void
SpellCustomDestroy(FcitxSpell *spell)
{
    if (spell->custom_dict)
        SpellCustomFreeDict(spell, spell->custom_dict);
    if (spell->custom_saved_lang)
        free(spell->custom_saved_lang);
}
