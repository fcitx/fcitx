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
    return true;
}

#define SHORT_WORD_LEN 6

/**
 * (from fcitx-en)
 **/
static float
SpellCustomDistance(const char *s1, const char *s2,
                    const int max_offset, int len2)
{
    int len1 = strlen(s1);
    if (len2 < 0)
        len2 = strlen(s2);
    if (!len1)
        return len2;
    if (!len2)
        return len1;
    int c = 0;
    int offset1 = 0;
    int offset2 = 0;
    int lcs = 0;
    while ((c + offset1 < len1) && (c + offset2 < len2)) {
        if (s1[c + offset1] == s2[c + offset2]) {
            lcs++;
        } else {
            offset1 = 0;
            offset2 = 0;
            int i;
            for (i = 0;i < max_offset;i++) {
                if ((c + i < len1)
                    && (s1[c + i] == s2[c])) {
                    offset1 = i;
                    break;
                }
                if ((c + i < len2)
                    && (s1[c] == s2[c + i])) {
                    offset2 = i;
                    break;
                }
            }
        }
        c++;
    }
    float avg = (len1 + len2) / 2.;
    return (avg - lcs) / avg;
}

/**
 * (from fcitx-en)
 **/
static int
SpellCustomCWordCompare(const void *a, const void *b)
{
    return (int)(((SpellCustomCWord*)a)->dist - ((SpellCustomCWord*)b)->dist);
}

/**
 * (from fcitx-en)
 **/
static boolean
SpellCustomGoodMatch(const char *current, const char *dict_word)
{
    int buf_len = strlen(current);
    if (buf_len <= SHORT_WORD_LEN) {
        return strncasecmp(current, dict_word, buf_len) == 0;
    } else {
        int dict_len = strlen(dict_word);
        if (dict_len < buf_len - 2 || dict_len > buf_len + 2)
            return false;
        /* search around 3 chars */
        return SpellCustomDistance(current, dict_word, 2, buf_len) < 0.33;
    }
}

SpellHint*
SpellCustomHintWords(FcitxSpell *spell, unsigned int len_limit)
{
    SpellCustomCWord clist[len_limit];
    int i;
    int num = 0;
    SpellCustom *custom = &spell->custom;
    if (!SpellCustomCheck(spell))
        return NULL;
    for (i = 0;i < custom->words_count;i++) {
        if (SpellCustomGoodMatch(spell->current_str,
                                 custom->words[i])) {
            clist[num].word = custom->words[i];
            // search around 3 chars
            clist[num].dist = SpellCustomDistance(spell->current_str,
                                                  custom->words[i],
                                                  2, -1);
            if (++num >= len_limit)
                break;
        }
    }
    if (strlen(spell->current_str) > SHORT_WORD_LEN)
        qsort((void*)clist, num, sizeof(SpellCustomCWord),
              SpellCustomCWordCompare);
    return SpellHintListWithSize(num, &clist->word, sizeof(SpellCustomCWord),
                                 NULL, 0);
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
