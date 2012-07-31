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

#include <unicode/unorm.h>

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "spell-internal.h"
#include "spell-custom.h"
#define EN_DICT_FORMAT "%s/data/%s_dict.txt"

static boolean
SpellCustomSimpleCompare(char c1, char c2)
{
    return c1 == c2;
}

static boolean
SpellCustomEnglishCompare(char c1, char c2)
{
    switch (c1) {
    case 'A' ... 'Z':
        c1 += 'a' - 'A';
        break;
    case 'a' ... 'z':
        break;
    default:
        return c1 == c2;
    }
    switch (c2) {
    case 'A' ... 'Z':
        c2 += 'a' - 'A';
        break;
    case 'a' ... 'z':
        break;
    default:
        break;
    }
    return c1 == c2;
}

static boolean
SpellCustomEnglishIsFirstCapital(const char *str)
{
    if (!str || !*str)
        return false;
    switch (*str) {
    case 'A' ... 'Z':
        break;
    default:
        return false;
    }
    while (*(++str)) {
        switch (*str) {
        case 'a' ... 'z':
            continue;
        default:
            return false;
        }
    }
    return true;
}

static boolean
SpellCustomEnglishIsAllCapital(const char *str)
{
    if (!str || !*str)
        return false;
    do {
        switch (*str) {
        case 'A' ... 'Z':
            continue;
        default:
            return false;
        }
    } while (*(++str));
    return true;
}

enum {
    CUSTOM_DEFAULT,
    CUSTOM_FIRST_CAPITAL,
    CUSTOM_ALL_CAPITAL,
};

static int
SpellCustomEnglishCheck(const char *str)
{
    if (SpellCustomEnglishIsFirstCapital(str))
        return CUSTOM_FIRST_CAPITAL;
    if (SpellCustomEnglishIsAllCapital(str))
        return CUSTOM_ALL_CAPITAL;
    return CUSTOM_DEFAULT;
}

static void
SpellUpperString(char *str)
{
    if (!str || !*str)
        return;
    do {
        switch (*str) {
        case 'a' ... 'z':
            *str += 'A' - 'a';
            break;
        default:
            break;
        }
    } while (*(++str));
}

static void
SpellCustomEnglishComplete(SpellHint *hint, int type)
{
    switch (type) {
    case CUSTOM_ALL_CAPITAL:
        for (;hint->commit;hint++)
            SpellUpperString(hint->commit);
        break;
    case CUSTOM_FIRST_CAPITAL:
        for (;hint->commit;hint++) {
            switch (*hint->commit) {
            case 'a' ... 'z':
                *hint->commit += 'A' - 'a';
                break;
            default:
                break;
            }
        }
    default:
        break;
    }
}

/**
 * Open the dict file, return -1 if failed.
 **/
static int
SpellCustomGetDictFile(FcitxSpell *spell, const char *lang)
{
    int fd;
    char *path;
    char *fname = NULL;
    path = fcitx_utils_get_fcitx_path("pkgdatadir");
    asprintf(&fname, EN_DICT_FORMAT, path, lang);
    free(path);
    fd = open(fname, O_RDONLY);
    free(fname);
    return fd;
}

/**
 * Try to read the who dict file into memory.... similiar to mmap but won't be
 * affected if the file is modified on disk. (can also write to this memory
 * area, e.g. to remove white spaces, with no side effect). If the dictionary
 * file is not found, try to keep whatever dictionary successfully loaded.
 **/
static off_t
SpellCustomMapDict(FcitxSpell *spell, const char *lang)
{
    int fd;
    struct stat stat_buf;
    off_t flen = 0;
    SpellCustom *custom = &spell->custom;
    fd = SpellCustomGetDictFile(spell, lang);

    /* try to save whatever loaded. */
    if (fd == -1)
        goto try_save;
    if (fstat(fd, &stat_buf) == -1) {
        close(fd);
        goto try_save;
    }
    if (custom->map) {
        free(custom->map);
        custom->map = NULL;
    }
    if (custom->saved_lang) {
        free(custom->saved_lang);
        custom->saved_lang = NULL;
    }
    if (!stat_buf.st_size)
        return 0;
    custom->map = fcitx_utils_malloc0(stat_buf.st_size + 1);
    if (!custom->map) {
        close(fd);
        return 0;
    }
    do {
        int c;
        c = read(fd, custom->map, stat_buf.st_size - flen);
        if (c <= 0)
            break;
        flen += c;
    } while (flen < stat_buf.st_size);
    if (!flen) {
        close(fd);
        free(custom->map);
        custom->map = NULL;
        return 0;
    } else if (flen < stat_buf.st_size) {
        custom->map = realloc(custom->map, flen + 1);
        return flen;
    } else {
        return stat_buf.st_size;
    }

try_save:
    if (custom->saved_lang)
        return 0;
    if (!custom->map)
        return 0;
    /* Actually shouldn't reach.... */
    if (!custom->words || !spell->dictLang) {
        free(custom->map);
        custom->map = NULL;
        return 0;
    }
    /* NOTE: dictLang is still the old language here */
    custom->saved_lang = strdup(spell->dictLang);
    return 0;
}

/**
 * update custom dict, if the dictionary of that language
 * cannot be found, keep the current one until the next successful loading.
 **/
boolean
SpellCustomLoadDict(FcitxSpell *spell, const char *lang)
{
    int i;
    int j;
    off_t map_len;
    int lcount;
    SpellCustom *custom = &spell->custom;
    if (!lang || !lang[0])
        return false;
    if (SpellLangIsLang(lang, "en")) {
        spell->custom.word_comp_func = SpellCustomEnglishCompare;
        spell->custom.word_check_func = SpellCustomEnglishCheck;
        spell->custom.hint_cmplt_func = SpellCustomEnglishComplete;
    } else {
        spell->custom.word_comp_func = SpellCustomSimpleCompare;
        spell->custom.word_check_func = NULL;
        spell->custom.hint_cmplt_func = NULL;
    }
    /* Use the saved dictionary */
    if (custom->saved_lang &&
        !strcmp(custom->saved_lang, lang)) {
        free(custom->saved_lang);
        custom->saved_lang = NULL;
        return true;
    }
    map_len = SpellCustomMapDict(spell, lang);
    /* current state saved */
    if (custom->saved_lang)
        return false;
    /* fail */
    if (!custom->map)
        goto free_all;

    /* count line */
    lcount = 0;
    for (i = 0;i < map_len;i++) {
        int l = strlen(custom->map + i);
        if (!l)
            continue;
        lcount++;
        i += l;
    }
    if (custom->map[i])
        lcount++;
    if (!custom->words) {
        custom->words = malloc(lcount * sizeof(char*));
    } else {
        custom->words = realloc(custom->words,
                                lcount * sizeof(char*));
    }
    /* well, no likely though. */
    if (!custom->words)
        goto free_all;

    /* save words pointers. */
    for (i = 0, j = 0;i < map_len && j < lcount;i++) {
        int l = strlen(custom->map + i);
        if (!l)
            continue;
        custom->words[j++] = custom->map + i;
        i += l;
    }
    custom->words_count = j;
    return true;

free_all:
    if (custom->map) {
        free(custom->map);
        custom->map = NULL;
    }
    if (custom->words) {
        free(custom->words);
        custom->words = NULL;
    }
    custom->words_count = 0;
    return false;
}

/* Init work is done in set lang, nothing else to init here */
boolean
SpellCustomInit(FcitxSpell *spell)
{
    spell->custom.word_comp_func = SpellCustomSimpleCompare;
    return true;
}

static int
SpellCustomGetDistance(SpellCustom *custom, const char *word, const char *dict)
{
    int word_len;
    int replace = 0;
    int insert = 0;
    int remove = 0;
    int distance = 0;
    int maxdiff;
    int maxremove;
    word_len = strlen(word);
    maxdiff = word_len / 3;
    maxremove = (word_len - 2) / 3;
    while ((distance = replace + insert + remove) <= maxdiff &&
           remove <= maxremove) {
        if (!word[0])
            return distance * 2 + strlen(dict);
        if (!dict[0]) {
            if (word[1]) {
                return -1;
            } else {
                remove++;
                distance++;
                if (distance <= maxdiff && remove <= maxremove)
                    return distance * 2;
                return -1;
            }
        }
        if (word[0] == dict[0] || custom->word_comp_func(word[0], dict[0])) {
            word++;
            dict++;
            continue;
        }
        if (word[1] == dict[0] || custom->word_comp_func(word[1], dict[0])) {
            word += 2;
            dict++;
            remove++;
            continue;
        }
        if (word[0] == dict[1] || custom->word_comp_func(word[0], dict[1])) {
            word++;
            dict += 2;
            insert++;
            continue;
        }
        if (word[1] == dict[1] || custom->word_comp_func(word[1], dict[1])) {
            word += 2;
            dict += 2;
            replace++;
            continue;
        }
        break;
    }
    return -1;
}

static int
SpellCustomCWordCompare(const void *a, const void *b)
{
    return (int)(((SpellCustomCWord*)a)->dist - ((SpellCustomCWord*)b)->dist);
}

SpellHint*
SpellCustomHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    int list_len = len_limit * 2;
    SpellCustomCWord clist[list_len];
    int i;
    int num = 0;
    int word_type = 0;
    SpellCustom *custom = &spell->custom;
    const char *word;
    SpellHint *res;
    if (!SpellCustomCheck(spell))
        return NULL;
    word = spell->current_str;
    if (custom->word_check_func)
        word_type = custom->word_check_func(word);
    for (i = 0;i < custom->words_count;i++) {
        int dist;
        if ((dist = SpellCustomGetDistance(&spell->custom, word,
                                           custom->words[i])) >= 0) {
            clist[num].word = custom->words[i];
            clist[num].dist = dist;
            if (++num >= list_len)
                break;
        }
    }
    qsort((void*)clist, num, sizeof(SpellCustomCWord),
          SpellCustomCWordCompare);
    num = num > len_limit ? len_limit : num;
    res = SpellHintListWithSize(num, &clist->word, sizeof(SpellCustomCWord),
                                NULL, 0);
    if (!res)
        return NULL;
    if (custom->hint_cmplt_func)
        custom->hint_cmplt_func(res, word_type);
    return res;
}

boolean
SpellCustomCheck(FcitxSpell *spell)
{
    SpellCustom *custom = &spell->custom;
    if (custom->map && custom->words &&
        !custom->saved_lang)
        return true;
    return false;
}

void
SpellCustomDestroy(FcitxSpell *spell)
{
    SpellCustom *custom = &spell->custom;
    if (custom->map)
        free(custom->map);
    if (custom->words)
        free(custom->words);
    if (custom->saved_lang)
        free(custom->saved_lang);
}
