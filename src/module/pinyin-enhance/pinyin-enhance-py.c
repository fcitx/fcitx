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

#include "fcitx-utils/utils.h"
#include "fcitx-utils/memory.h"
#include "pinyin-enhance-py.h"

static const char*
py_enhance_get_vokal(int8_t index, int8_t tone, int *len)
{
    static const PyEnhanceStrLen vokals_table[][5] = {
        {
            PY_STR_LEN("a"),
            PY_STR_LEN("ā"),
            PY_STR_LEN("á"),
            PY_STR_LEN("ǎ"),
            PY_STR_LEN("à")
        }, {
            PY_STR_LEN("ai"),
            PY_STR_LEN("āi"),
            PY_STR_LEN("ái"),
            PY_STR_LEN("ǎi"),
            PY_STR_LEN("ài")
        }, {
            PY_STR_LEN("an"),
            PY_STR_LEN("ān"),
            PY_STR_LEN("án"),
            PY_STR_LEN("ǎn"),
            PY_STR_LEN("àn")
        }, {
            PY_STR_LEN("ang"),
            PY_STR_LEN("āng"),
            PY_STR_LEN("áng"),
            PY_STR_LEN("ǎng"),
            PY_STR_LEN("àng")
        }, {
            PY_STR_LEN("ao"),
            PY_STR_LEN("āo"),
            PY_STR_LEN("áo"),
            PY_STR_LEN("ǎo"),
            PY_STR_LEN("ào")
        }, {
            PY_STR_LEN("e"),
            PY_STR_LEN("ē"),
            PY_STR_LEN("é"),
            PY_STR_LEN("ě"),
            PY_STR_LEN("è")
        }, {
            PY_STR_LEN("ei"),
            PY_STR_LEN("ēi"),
            PY_STR_LEN("éi"),
            PY_STR_LEN("ěi"),
            PY_STR_LEN("èi")
        }, {
            PY_STR_LEN("en"),
            PY_STR_LEN("ēn"),
            PY_STR_LEN("én"),
            PY_STR_LEN("ěn"),
            PY_STR_LEN("èn")
        }, {
            PY_STR_LEN("eng"),
            PY_STR_LEN("ēng"),
            PY_STR_LEN("éng"),
            PY_STR_LEN("ěng"),
            PY_STR_LEN("èng")
        }, {
            PY_STR_LEN("er"),
            PY_STR_LEN("ēr"),
            PY_STR_LEN("ér"),
            PY_STR_LEN("ěr"),
            PY_STR_LEN("èr")
        }, {
            PY_STR_LEN("i"),
            PY_STR_LEN("ī"),
            PY_STR_LEN("í"),
            PY_STR_LEN("ǐ"),
            PY_STR_LEN("ì")
        }, {
            PY_STR_LEN("ia"),
            PY_STR_LEN("iā"),
            PY_STR_LEN("iá"),
            PY_STR_LEN("iǎ"),
            PY_STR_LEN("ià")
        }, {
            PY_STR_LEN("ian"),
            PY_STR_LEN("iān"),
            PY_STR_LEN("ián"),
            PY_STR_LEN("iǎn"),
            PY_STR_LEN("iàn")
        }, {
            PY_STR_LEN("iang"),
            PY_STR_LEN("iāng"),
            PY_STR_LEN("iáng"),
            PY_STR_LEN("iǎng"),
            PY_STR_LEN("iàng")
        }, {
            PY_STR_LEN("iao"),
            PY_STR_LEN("iāo"),
            PY_STR_LEN("iáo"),
            PY_STR_LEN("iǎo"),
            PY_STR_LEN("iào")
        }, {
            PY_STR_LEN("ie"),
            PY_STR_LEN("iē"),
            PY_STR_LEN("ié"),
            PY_STR_LEN("iě"),
            PY_STR_LEN("iè")
        }, {
            PY_STR_LEN("in"),
            PY_STR_LEN("īn"),
            PY_STR_LEN("ín"),
            PY_STR_LEN("ǐn"),
            PY_STR_LEN("ìn")
        }, {
            PY_STR_LEN("ing"),
            PY_STR_LEN("īng"),
            PY_STR_LEN("íng"),
            PY_STR_LEN("ǐng"),
            PY_STR_LEN("ìng")
        }, {
            PY_STR_LEN("iong"),
            PY_STR_LEN("iōng"),
            PY_STR_LEN("ióng"),
            PY_STR_LEN("iǒng"),
            PY_STR_LEN("iòng")
        }, {
            PY_STR_LEN("iu"),
            PY_STR_LEN("iū"),
            PY_STR_LEN("iú"),
            PY_STR_LEN("iǔ"),
            PY_STR_LEN("iù")
        }, {
            PY_STR_LEN("m"),
            PY_STR_LEN("m"),
            PY_STR_LEN("m"),
            PY_STR_LEN("m"),
            PY_STR_LEN("m")
        }, {
            PY_STR_LEN("n"),
            PY_STR_LEN("n"),
            PY_STR_LEN("ń"),
            PY_STR_LEN("ň"),
            PY_STR_LEN("ǹ")
        }, {
            PY_STR_LEN("ng"),
            PY_STR_LEN("ng"),
            PY_STR_LEN("ńg"),
            PY_STR_LEN("ňg"),
            PY_STR_LEN("ǹg")
        }, {
            PY_STR_LEN("o"),
            PY_STR_LEN("ō"),
            PY_STR_LEN("ó"),
            PY_STR_LEN("ǒ"),
            PY_STR_LEN("ò")
        }, {
            PY_STR_LEN("ong"),
            PY_STR_LEN("ōng"),
            PY_STR_LEN("óng"),
            PY_STR_LEN("ǒng"),
            PY_STR_LEN("òng")
        }, {
            PY_STR_LEN("ou"),
            PY_STR_LEN("ōu"),
            PY_STR_LEN("óu"),
            PY_STR_LEN("ǒu"),
            PY_STR_LEN("òu")
        }, {
            PY_STR_LEN("u"),
            PY_STR_LEN("ū"),
            PY_STR_LEN("ú"),
            PY_STR_LEN("ǔ"),
            PY_STR_LEN("ù")
        }, {
            PY_STR_LEN("ua"),
            PY_STR_LEN("uā"),
            PY_STR_LEN("uá"),
            PY_STR_LEN("uǎ"),
            PY_STR_LEN("uà")
        }, {
            PY_STR_LEN("uai"),
            PY_STR_LEN("uāi"),
            PY_STR_LEN("uái"),
            PY_STR_LEN("uǎi"),
            PY_STR_LEN("uài")
        }, {
            PY_STR_LEN("uan"),
            PY_STR_LEN("uān"),
            PY_STR_LEN("uán"),
            PY_STR_LEN("uǎn"),
            PY_STR_LEN("uàn")
        }, {
            PY_STR_LEN("uang"),
            PY_STR_LEN("uāng"),
            PY_STR_LEN("uáng"),
            PY_STR_LEN("uǎng"),
            PY_STR_LEN("uàng")
        }, {
            PY_STR_LEN("ue"),
            PY_STR_LEN("uē"),
            PY_STR_LEN("ué"),
            PY_STR_LEN("uě"),
            PY_STR_LEN("uè")
        }, {
            PY_STR_LEN("ueng"),
            PY_STR_LEN("uēng"),
            PY_STR_LEN("uéng"),
            PY_STR_LEN("uěng"),
            PY_STR_LEN("uèng")
        }, {
            PY_STR_LEN("ui"),
            PY_STR_LEN("uī"),
            PY_STR_LEN("uí"),
            PY_STR_LEN("uǐ"),
            PY_STR_LEN("uì")
        }, {
            PY_STR_LEN("un"),
            PY_STR_LEN("ūn"),
            PY_STR_LEN("ún"),
            PY_STR_LEN("ǔn"),
            PY_STR_LEN("ùn")
        }, {
            PY_STR_LEN("uo"),
            PY_STR_LEN("uō"),
            PY_STR_LEN("uó"),
            PY_STR_LEN("uǒ"),
            PY_STR_LEN("uò")
        }, {
            PY_STR_LEN("ü"),
            PY_STR_LEN("ǖ"),
            PY_STR_LEN("ǘ"),
            PY_STR_LEN("ǚ"),
            PY_STR_LEN("ǜ")
        }, {
            PY_STR_LEN("üan"),
            PY_STR_LEN("üān"),
            PY_STR_LEN("üán"),
            PY_STR_LEN("üǎn"),
            PY_STR_LEN("üàn")
        }, {
            PY_STR_LEN("üe"),
            PY_STR_LEN("üē"),
            PY_STR_LEN("üé"),
            PY_STR_LEN("üě"),
            PY_STR_LEN("üè")
        }, {
            PY_STR_LEN("ün"),
            PY_STR_LEN("ǖn"),
            PY_STR_LEN("ǘn"),
            PY_STR_LEN("ǚn"),
            PY_STR_LEN("ǜn")
        }
    };
    static const int8_t vokals_count = (sizeof(vokals_table) /
                                        sizeof(vokals_table[0]));
    if (index < 0 || index >= vokals_count) {
        *len = 0;
        return "";
    }
    if (tone < 0 || tone > 4)
        tone = 0;
    const PyEnhanceStrLen *res = &vokals_table[index][tone];
    *len = res->len;
    return res->str;
}

static const char*
py_enhance_get_konsonant(int8_t index, int *len)
{
    static const PyEnhanceStrLen konsonants_table[] = {
        PY_STR_LEN("b"),
        PY_STR_LEN("c"),
        PY_STR_LEN("ch"),
        PY_STR_LEN("d"),
        PY_STR_LEN("f"),
        PY_STR_LEN("g"),
        PY_STR_LEN("h"),
        PY_STR_LEN("j"),
        PY_STR_LEN("k"),
        PY_STR_LEN("l"),
        PY_STR_LEN("m"),
        PY_STR_LEN("n"),
        PY_STR_LEN("ng"),
        PY_STR_LEN("p"),
        PY_STR_LEN("q"),
        PY_STR_LEN("r"),
        PY_STR_LEN("s"),
        PY_STR_LEN("sh"),
        PY_STR_LEN("t"),
        PY_STR_LEN("w"),
        PY_STR_LEN("x"),
        PY_STR_LEN("y"),
        PY_STR_LEN("z"),
        PY_STR_LEN("zh")
    };
    static const int8_t konsonants_count = (sizeof(konsonants_table) /
                                            sizeof(konsonants_table[0]));
    if (index < 0 || index >= konsonants_count) {
        *len = 0;
        return "";
    }
    const PyEnhanceStrLen *res = &konsonants_table[index];
    *len = res->len;
    return res->str;
}

char*
py_enhance_py_to_str(char *buff, const int8_t *py, int *len)
{
    int8_t k_i = py[0] - 1;
    int8_t v_i = py[1] - 1;
    int8_t tone = py[2];
    int k_l = 0;
    int v_l = 0;
    const char *k_s = py_enhance_get_konsonant(k_i, &k_l);
    const char *v_s = py_enhance_get_vokal(v_i, tone, &v_l);
    int l = k_l + v_l;
    if (!buff) {
        buff = malloc(l + 1);
    }
    memcpy(buff, k_s, k_l);
    memcpy(buff + k_l, v_s, v_l);
    buff[l] = '\0';
    if (len)
        *len = l;
    return buff;
}

#define PY_TABLE_FILE  "py_table.mb"

static inline uint32_t
py_enhance_py_alloc_py(PyEnhanceBuff *buff, const char *word, int8_t word_l,
                       const int8_t *py_buff, int8_t py_size, int8_t **py_list,
                       int8_t count)
{
    uint32_t res;
    res = py_enhance_buff_alloc_noalign(buff, word_l + py_size + 3);
    *py_list = buff->data + res;
    (*py_list)[0] = word_l + 1;
    (*py_list)++;
    memcpy(*py_list, word, word_l);
    int8_t *tmp;
    tmp = *py_list + word_l;
    *tmp = 0;
    tmp++;
    *tmp = count;
    memcpy(tmp + 1, py_buff, py_size);
    return res + 1;
}

#define PY_ENHANCE_UINT32_ALIGN_SIZE                                    \
    fcitx_utils_align_to(sizeof(uint32_t), PY_ENHANCE_BUFF_ALIGH)

static inline void
py_enhance_add_word_p(PyEnhanceBuff *py_table, PyEnhanceBuff *array,
                      const char *word, uint32_t id)
{
    int32_t offset;
    for (offset = array->len - PY_ENHANCE_UINT32_ALIGN_SIZE;offset >= 0;
         offset -= PY_ENHANCE_UINT32_ALIGN_SIZE) {
        uint32_t table_offset = *(uint32_t*)(array->data + offset);
        if (strcmp(py_table->data + table_offset, word) < 0) {
            break;
        }
    }
    offset += PY_ENHANCE_UINT32_ALIGN_SIZE;
    int32_t old_len = array->len;
    py_enhance_buff_alloc(array, sizeof(uint32_t));
    if (offset < old_len)
        memmove(array->data + offset + PY_ENHANCE_UINT32_ALIGN_SIZE,
                array->data + offset, old_len - offset);
    *(uint32_t*)(array->data + offset) = id;
}

static void
py_enhance_load_py(PinyinEnhance *pyenhance)
{
    PyEnhanceBuff *array = &pyenhance->py_list;
    PyEnhanceBuff *py_table = &pyenhance->py_table;
    if (py_table->len)
        return;
    FILE *fp;
    char *fname;
    fname = fcitx_utils_get_fcitx_path_with_filename(
        "pkgdatadir", "py-enhance/"PY_TABLE_FILE);
    fp = fopen(fname, "r");
    free(fname);
    if (fp) {
        py_enhance_buff_reserve(py_table, 416 * 1024);
        py_enhance_buff_reserve(array, 192 * 1024);
        char buff[UTF8_MAX_LENGTH + 1];
        int buff_size = 33;
        int8_t *list_buff = malloc(buff_size);
        size_t res;
        int8_t word_l;
        int8_t count;
        int8_t py_size;
        int8_t *py_list;
        /**
         * Format:
         * int8_t word_l;
         * char word[word_l];
         * int8_t count;
         * int8_t py[count][3];
         **/
        while (true) {
            res = fread(&word_l, 1, 1, fp);
            if (!res || word_l < 0 || word_l > UTF8_MAX_LENGTH)
                break;
            res = fread(buff, word_l + 1, 1, fp);
            if (!res)
                break;
            count = buff[word_l];
            if (count < 0)
                break;
            if (count == 0)
                continue;
            py_size = count * 3;
            if (fcitx_unlikely(buff_size < py_size)) {
                buff_size = py_size;
                list_buff = realloc(list_buff, buff_size);
            }
            res = fread(list_buff, py_size, 1, fp);
            if (!res)
                break;
            uint32_t id = py_enhance_py_alloc_py(
                py_table, buff, word_l, list_buff, py_size, &py_list, count);
            py_enhance_add_word_p(py_table, array, (char*)py_list, id);
        }
        free(list_buff);
        py_enhance_buff_shrink(array);
        py_enhance_buff_shrink(py_table);
        fclose(fp);
    }
}

/* since bsearch doesn't support user data afaik. */
static const void *_evil_global_py_table_data = NULL;

static int
compare_func(const void *p1, const void *p2)
{
    const char *str1 = p1;
    const uint32_t *id_p = p2;
    const char *str2 = _evil_global_py_table_data + *id_p;
    return strcmp(str1, str2);
}

const int8_t*
py_enhance_py_find_py(PinyinEnhance *pyenhance, const char *str)
{
    py_enhance_load_py(pyenhance);
    if (!pyenhance->py_list.len)
        return NULL;
    uint32_t *py_list;
    _evil_global_py_table_data = pyenhance->py_table.data;
    py_list = bsearch(str, pyenhance->py_list.data,
                      pyenhance->py_list.len / PY_ENHANCE_UINT32_ALIGN_SIZE,
                      sizeof(uint32_t), compare_func);
    if (!py_list)
        return NULL;
    int8_t *res = pyenhance->py_table.data + *py_list;
    return res + *(res - 1);
}

void
py_enhance_py_destroy(PinyinEnhance *pyenhance)
{
    py_enhance_buff_free(&pyenhance->py_list);
    py_enhance_buff_free(&pyenhance->py_table);
}
