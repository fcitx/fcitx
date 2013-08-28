/*
 * this file is ported from kdelibs/kdeui/kcharselectdata.cpp
 *
 * original file is licensed under GPLv2+
 */

#include <stdint.h>
#include <ctype.h>
#include <libintl.h>
#include <fcitx-utils/uthash.h>
#include <fcitx-utils/utils.h>
#include <fcitx-config/xdg.h>
#include <fcitx/fcitx.h>
#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif
#include "charselectdata.h"

/* constants for hangul (de)composition, see UAX #15 */
#define SBase 0xAC00
#define LBase 0x1100
#define VBase 0x1161
#define TBase 0x11A7
#define LCount 19
#define VCount 21
#define TCount 28
#define NCount (VCount * TCount)
#define SCount (LCount * NCount)
#define HASH_FIND_UNICODE(head,findint,out)                                         \
    HASH_FIND(hh,head,findint,sizeof(uint32_t),out)
#define HASH_ADD_UNICODE(head,intfield,add)                                         \
    HASH_ADD(hh,head,intfield,sizeof(uint32_t),add)

typedef struct _UnicodeSet {
    uint32_t unicode;
    UT_hash_handle hh;
} UnicodeSet;

static const char JAMO_L_TABLE[][4] = {
    "G", "GG", "N", "D", "DD", "R", "M", "B", "BB",
    "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H"
};

static const char JAMO_V_TABLE[][4] = {
    "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O",
    "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI",
    "YU", "EU", "YI", "I"
};

static const char JAMO_T_TABLE[][4] = {
    "", "G", "GG", "GS", "N", "NJ", "NH", "D", "L", "LG", "LM",
    "LB", "LS", "LT", "LP", "LH", "M", "B", "BS",
    "S", "SS", "NG", "J", "C", "K", "T", "P", "H"
};

int uni_cmp(const void* a, const void* b) {
    const UnicodeSet* sa = a;
    const UnicodeSet* sb = b;
    return sa->unicode - sb->unicode;
}

int pindex_cmp(const void* a, const void* b) {
    CharSelectDataIndex* const* pa = a;
    CharSelectDataIndex* const* pb = b;

    return strcasecmp((*pa)->key, (*pb)->key);
}

int index_search_cmp(const void* a, const void* b) {
    const char* s = a;
    CharSelectDataIndex* const* pb = b;

    return strcasecmp(s, (*pb)->key);
}

int index_search_a_cmp(const void* a, const void* b) {
    const char* s = a;
    CharSelectDataIndex* const* pb = b;

    int res, len;
    len = strlen(s);
    res = strncasecmp(s, (*pb)->key, len);
    if (res)
        return res;
    else
        return 1;
}

UT_array* SplitString(const char* s);

char* FormatCode(uint32_t code, int length, const char* prefix);
UnicodeSet* CharSelectDataGetMatchingChars(CharSelectData* charselect, const char* s);

uint32_t FromLittleEndian32(const char* d)
{
    const uint8_t* data = (const uint8_t*) d;
    uint32_t t;
    memcpy(&t, data, sizeof(t));
    return le32toh(t);
}

uint16_t FromLittleEndian16(const char* d)
{
    const uint8_t* data = (const uint8_t*) d;
    uint16_t t;
    memcpy(&t, data, sizeof(t));
    return le16toh(t);
}

CharSelectData* CharSelectDataCreate()
{
    CharSelectData* charselect = fcitx_utils_new(CharSelectData);

    do {

        FILE* fp = FcitxXDGGetFileWithPrefix("data", "charselectdata", "r", NULL);
        if (!fp)
            break;

        fseek(fp, 0, SEEK_END);
        long int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        charselect->size = size;
        charselect->dataFile = fcitx_utils_malloc0(size);
        fread(charselect->dataFile, 1, size, fp);

        fclose(fp);

        CharSelectDataCreateIndex(charselect);

        return charselect;
    } while(0);

    free(charselect);
    return NULL;
}

UT_array* CharSelectDataUnihanInfo(CharSelectData* charselect, uint32_t unicode)
{
    UT_array* res = fcitx_utils_new_string_list();

    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+36);
    const uint32_t offsetEnd = charselect->size;

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 32) - 1;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint32_t midUnicode = FromLittleEndian16(data + offsetBegin + mid*32);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            int i;
            for(i = 0; i < 7; i++) {
                uint32_t offset = FromLittleEndian32(data + offsetBegin + mid*32 + 4 + i*4);
                const char* empty = "";
                if(offset != 0) {
                    const char* r = data + offset;
                    utarray_push_back(res, &r);
                } else {
                    utarray_push_back(res, &empty);
                }
            }
            return res;
        }
    }

    return res;
}

uint32_t CharSelectDataGetDetailIndex(CharSelectData* charselect, uint32_t unicode)
{
    const char* data = charselect->dataFile;
    // Convert from little-endian, so that this code works on PPC too.
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=482286
    const uint32_t offsetBegin = FromLittleEndian32(data+12);
    const uint32_t offsetEnd = FromLittleEndian32(data+16);

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 29) - 1;

    static uint32_t most_recent_searched;
    static uint32_t most_recent_result;


    if (unicode == most_recent_searched)
        return most_recent_result;

    most_recent_searched = unicode;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint32_t midUnicode = FromLittleEndian16(data + offsetBegin + mid*29);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            most_recent_result = offsetBegin + mid*29;

            return most_recent_result;
        }
    }

    most_recent_result = 0;
    return 0;
}

char* CharSelectDataName(CharSelectData* charselect, uint32_t unicode)
{
    char* result = NULL;
    do {
        if ((unicode >= 0x3400 && unicode <= 0x4DB5)
                || (unicode >= 0x4e00 && unicode <= 0x9fa5)
                || (unicode >= 0x20000 && unicode <= 0x2A6D6)) {
            asprintf(&result, "CJK UNIFIED IDEOGRAPH-%x", unicode);
        } else if (unicode >= 0xac00 && unicode <= 0xd7af) {
            /* compute hangul syllable name as per UAX #15 */
            int SIndex = unicode - SBase;
            int LIndex, VIndex, TIndex;

            if (SIndex < 0 || SIndex >= SCount) {
                result = strdup("");
                break;
            }

            LIndex = SIndex / NCount;
            VIndex = (SIndex % NCount) / TCount;
            TIndex = SIndex % TCount;

            fcitx_utils_alloc_cat_str(result, "HANGUL SYLLABLE ",
                                      JAMO_L_TABLE[LIndex],
                                      JAMO_V_TABLE[VIndex],
                                      JAMO_T_TABLE[TIndex]);
        } else if (unicode >= 0xD800 && unicode <= 0xDB7F)
            result = strdup(_("<Non Private Use High Surrogate>"));
        else if (unicode >= 0xDB80 && unicode <= 0xDBFF)
            result = strdup(_("<Private Use High Surrogate>"));
        else if (unicode >= 0xDC00 && unicode <= 0xDFFF)
            result = strdup(_("<Low Surrogate>"));
        else if (unicode >= 0xE000 && unicode <= 0xF8FF)
            result = strdup(_("<Private Use>"));
        else {

        const char* data = charselect->dataFile;
            const uint32_t offsetBegin = FromLittleEndian32(data+4);
            const uint32_t offsetEnd = FromLittleEndian32(data+8);

            int min = 0;
            int mid;
            int max = ((offsetEnd - offsetBegin) / 8) - 1;

            while (max >= min) {
                mid = (min + max) / 2;
                const uint32_t midUnicode = FromLittleEndian32(data + offsetBegin + mid*8);
                if (unicode > midUnicode)
                    min = mid + 1;
                else if (unicode < midUnicode)
                    max = mid - 1;
                else {
                    uint32_t offset = FromLittleEndian32(data + offsetBegin + mid*8 + 4);
                    result = strdup(charselect->dataFile + offset + 1);
                    break;
                }
            }
        }
    } while(0);

    if (!result) {
        result = strdup(_("<not assigned>"));
    }
    return result;
}

char* Simplified(const char* src)
{
    char* s = strdup(src);
    char* o = s;
    char* p = s;
    int lastIsSpace = 0;
    while(*s) {
        char c = *s;

        if (isspace(c)) {
            if (!lastIsSpace) {
                *p = ' ';
                p ++;
            }
            lastIsSpace = 1;
        }
        else {
            *p = c;
            p++;
            lastIsSpace = 0;
        }
        s++;
    }
    return o;
}

int IsHexString(const char* s)
{
    size_t l = strlen(s);
    if (l < 6)
        return 0;
    if (!((s[0] == '0' && s[1] == 'x')
      || (s[0] == '0' && s[1] == 'X')
      || (s[0] == 'u' && s[1] == '+')
      || (s[0] == 'U' && s[1] == '+'))) {
        return 0;
    }

    s += 2;
    while (*s) {
        if (!isxdigit(*s))
            return 0;
        s++;
    }
    return 1;
}

void UnicodeSetFree(UnicodeSet* set) {
    while (set) {
        UnicodeSet* p = set;
        HASH_DEL(set, p);
        free(p);
    }
}

UnicodeSet* UnicodeSetIntersect(UnicodeSet* left, UnicodeSet* right)
{
    do {
        if (!left)
            break;

        if (!right)
            break;

        UnicodeSet* p = left;
        while (p) {
            UnicodeSet* find = NULL;
            HASH_FIND_UNICODE(right, &p->unicode, find);
            UnicodeSet* next = p->hh.next;
            if (!find) {
                HASH_DEL(left, p);
                free(p);
            }
            else {
                HASH_DEL(right, find);
                free(find);
            }

            p = next;
        }

        UnicodeSetFree(right);
        return left;
    } while(0);

    if (left)
        UnicodeSetFree(left);

    if (right)
        UnicodeSetFree(right);

    return NULL;
}

UT_array* CharSelectDataFind(CharSelectData* charselect, const char* needle)
{
    UnicodeSet *result = NULL;

    UT_array* returnRes;
    utarray_new(returnRes, fcitx_int32_icd);
    char* simplified = Simplified(needle);
    UT_array* searchStrings = SplitString(simplified);

    if (strlen(simplified) == 1) {
        // search for hex representation of the character
        utarray_clear(searchStrings);
        char* format = FormatCode(simplified[0], 4, "U+");
        utarray_push_back(searchStrings, &format);
        free(format);
    }
    free(simplified);

    if (utarray_len(searchStrings) == 0) {
        return returnRes;
    }

    utarray_foreach(s, searchStrings, char*) {
        char* end = NULL;
        if(IsHexString(*s)) {
            end = NULL;
            uint32_t uni = (uint32_t) strtoul(*s + 2, &end, 16);
            utarray_push_back(returnRes, &uni);

            // search for "1234" instead of "0x1234"
            char* news = strdup(*s + 2);
            free(*s);
            *s = news;
        }
        // try to parse string as decimal number
        end = NULL;
        uint32_t unicode = (uint32_t) strtoul(*s, &end, 10);
        if (end == NULL) {
            utarray_push_back(returnRes, &unicode);
        }
    }

    int firstSubString = 1;
    utarray_foreach(s2, searchStrings, char* ) {
        UnicodeSet* partResult = CharSelectDataGetMatchingChars(charselect, *s2);
        if (firstSubString) {
            result = partResult;
            firstSubString = 0;
        } else {
            result = UnicodeSetIntersect(result, partResult);
        }
        if (!result)
            break;
    }

    // remove results found by matching the code point to prevent duplicate results
    // while letting these characters stay at the beginning
    utarray_foreach(c, returnRes, uint32_t) {
        UnicodeSet* dup = NULL;
        HASH_FIND_UNICODE(result, c, dup);
        if (dup)
            HASH_DEL(result, dup);
    }

    HASH_SORT(result, uni_cmp);

    while (result) {
        UnicodeSet* p = result;
        HASH_DEL(result, p);
        uint32_t unicode = p->unicode;
        utarray_push_back(returnRes, &unicode);
        free(p);
    }

    utarray_free(searchStrings);

    return returnRes;
}

UnicodeSet* InsertResult(UnicodeSet* set, uint32_t unicode) {
    UnicodeSet* find = NULL;
    HASH_FIND_UNICODE(set, &unicode, find);
    if (!find) {
        find = fcitx_utils_new(UnicodeSet);
        find->unicode = unicode;
        HASH_ADD_UNICODE(set, unicode, find);
    }
    return set;
}

UnicodeSet* CharSelectDataGetMatchingChars(CharSelectData* charselect, const char* s)
{
    UnicodeSet *result = NULL;
    size_t s_l = strlen(s);
    CharSelectDataIndex **pos;
    CharSelectDataIndex **last;
    pos = utarray_custom_bsearch(s, charselect->indexList, 0, index_search_cmp);
    last = utarray_custom_bsearch(s, charselect->indexList,
                                  0, index_search_a_cmp);
    if (!pos)
        return NULL;
    if (!last)
        last = (CharSelectDataIndex**)utarray_back(charselect->indexList);
    while (pos != last && strncasecmp(s, (*pos)->key, s_l) == 0) {
        utarray_foreach (c, (*pos)->items, uint32_t) {
            result = InsertResult(result, *c);
        }
        ++pos;
    }

    return result;
}

UT_array* CharSelectDataAliases(CharSelectData* charselect, uint32_t unicode)
{
    const char* data = charselect->dataFile;
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const uint8_t count = * (uint8_t *)(data + detailIndex + 8);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 4);

    UT_array* aliases = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(aliases, &r);
        offset += strlen(data + offset) + 1;
    }
    return aliases;
}


UT_array* CharSelectDataNotes(CharSelectData* charselect, uint32_t unicode)
{
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 13);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 9);

    UT_array* notes = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(notes, &r);
        offset += strlen(data + offset) + 1;
    }

    return notes;
}

UT_array*
CharSelectDataSeeAlso(CharSelectData* charselect, uint32_t unicode)
{
    UT_array *seeAlso;
    utarray_new(seeAlso, fcitx_int32_icd);
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return seeAlso;
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 28);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 24);

    int i;
    for (i = 0;  i < count;  i++) {
        uint32_t c = FromLittleEndian16 (data + offset);
        utarray_push_back(seeAlso, &c);
        offset += 2;
    }

    return seeAlso;
}

UT_array* CharSelectDataEquivalents(CharSelectData* charselect, uint32_t unicode)
{
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 23);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 19);

    UT_array* equivalents = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(equivalents, &r);
        offset += strlen(data + offset) + 1;
    }

    return equivalents;
}

UT_array* CharSelectDataApproximateEquivalents(CharSelectData* charselect, uint32_t unicode)
{
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 18);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 14);

    UT_array* approxEquivalents = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(approxEquivalents, &r);
        offset += strlen(data + offset) + 1;
    }

    return approxEquivalents;
}


char* FormatCode(uint32_t code, int length, const char* prefix)
{
    char* s = NULL;
    char* fmt = NULL;
    asprintf(&fmt, "%%s%%0%dX", length);
    asprintf(&s, fmt, prefix, code);
    free(fmt);
    return s;
}

UT_array* SplitString(const char* s)
{
    UT_array* result = fcitx_utils_new_string_list();
    int start = 0;
    int end = 0;
    int length = strlen(s);
    while (end < length) {
        while (end < length && (isdigit(s[end]) || isalpha(s[end]) || s[end] == '+')) {
            end++;
        }
        if (start != end) {
            char* p = strndup(&s[start], end - start);
            utarray_push_back(result, &p);
            free(p);
        }
        start = end;
        while (end < length && !(isdigit(s[end]) || isalpha(s[end]) || s[end] == '+')) {
            end++;
            start++;
        }
    }
    return result;
}

CharSelectDataIndex* CharSelectDataIndexNew(const char* key)
{
    CharSelectDataIndex* idx = fcitx_utils_new(CharSelectDataIndex);
    idx->key = strdup(key);
    utarray_new(idx->items, fcitx_int32_icd);
    return idx;
}

void CharSelectDataAppendToIndex(CharSelectData* charselect, uint32_t unicode, const char* str)
{
    UT_array* strings = SplitString(str);
    utarray_foreach(s, strings, char*) {
        CharSelectDataIndex* item = NULL;
        HASH_FIND_STR(charselect->index, *s, item);
        if (!item) {
            item = CharSelectDataIndexNew(*s);
            HASH_ADD_KEYPTR(hh, charselect->index, item->key, strlen(item->key), item);
        }
        utarray_push_back(item->items, &unicode);
    }
    utarray_free(strings);
}

void CharSelectDataDump(CharSelectData* charselect)
{
    //CharSelectDataIndex* item = charselect->index;
    /*
    while(item) {
        fprintf(stderr, "%s\n", item->key);
        item = item->hh.next;
    } */

    utarray_foreach(p, charselect->indexList, CharSelectDataIndex*) {
        fprintf(stderr, "%s\n", (*p)->key);
    }
}

void CharSelectDataCreateIndex(CharSelectData* charselect)
{
    // character names
    const char* data = charselect->dataFile;
    const uint32_t nameOffsetBegin = FromLittleEndian32(data+4);
    const uint32_t nameOffsetEnd = FromLittleEndian32(data+8);

    int max = ((nameOffsetEnd - nameOffsetBegin) / 8) - 1;

    int pos, j;

    for (pos = 0; pos <= max; pos++) {
        const uint32_t unicode = FromLittleEndian32(data + nameOffsetBegin + pos*8);
        uint32_t offset = FromLittleEndian32(data + nameOffsetBegin + pos*8 + 4);
        // TODO
        CharSelectDataAppendToIndex(charselect, unicode, (data + offset + 1));
    }

    // details
    const uint32_t detailsOffsetBegin = FromLittleEndian32(data+12);
    const uint32_t detailsOffsetEnd = FromLittleEndian32(data+16);

    max = ((detailsOffsetEnd - detailsOffsetBegin) / 29) - 1;
    for (pos = 0; pos <= max; pos++) {
        const uint32_t unicode = FromLittleEndian32(data + detailsOffsetBegin + pos*29);

        // aliases
        const uint8_t aliasCount = * (uint8_t *)(data + detailsOffsetBegin + pos*29 + 8);
        uint32_t aliasOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*29 + 4);

        for (j = 0;  j < aliasCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode, data + aliasOffset);
            aliasOffset += strlen(data + aliasOffset) + 1;
        }

        // notes
        const uint8_t notesCount = * (uint8_t *)(data + detailsOffsetBegin + pos*29 + 13);
        uint32_t notesOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*29 + 9);

        for (j = 0;  j < notesCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode, data + notesOffset);
            notesOffset += strlen(data + notesOffset) + 1;
        }

        // approximate equivalents
        const uint8_t apprCount = * (uint8_t *)(data + detailsOffsetBegin + pos*29 + 18);
        uint32_t apprOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*29 + 14);

        for (j = 0;  j < apprCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode,data + apprOffset);
            apprOffset += strlen(data + apprOffset) + 1;
        }

        // equivalents
        const uint8_t equivCount = * (uint8_t *)(data + detailsOffsetBegin + pos*29 + 23);
        uint32_t equivOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*29 + 19);

        for (j = 0;  j < equivCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode, data + equivOffset);
            equivOffset += strlen(data + equivOffset) + 1;
        }

        // see also - convert to string (hex)
        const uint8_t seeAlsoCount = * (uint8_t *)(data + detailsOffsetBegin + pos*29 + 28);
        uint32_t seeAlsoOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*29 + 24);

        for (j = 0;  j < seeAlsoCount;  j++) {
            uint32_t seeAlso = FromLittleEndian16 (data + seeAlsoOffset);
            char* code = FormatCode(seeAlso, 4, "");
            CharSelectDataAppendToIndex(charselect, unicode, code);
            free(code);
            equivOffset += strlen(data + equivOffset) + 1;
        }
    }

    // unihan data
    // temporary disabled due to the huge amount of data
     const uint32_t unihanOffsetBegin = FromLittleEndian32(data+36);
     const uint32_t unihanOffsetEnd = charselect->size;
     max = ((unihanOffsetEnd - unihanOffsetBegin) / 32) - 1;

     for (pos = 0; pos <= max; pos++) {
         const uint32_t unicode = FromLittleEndian32(data + unihanOffsetBegin + pos*32);
         for(j = 0; j < 7; j++) {
             uint32_t offset = FromLittleEndian32(data + unihanOffsetBegin + pos*32 + 4 + j*4);
             if(offset != 0) {
                 CharSelectDataAppendToIndex(charselect, unicode, (data + offset));
             }
         }
     }

     utarray_new(charselect->indexList, fcitx_ptr_icd);

     CharSelectDataIndex* idx = charselect->index;
     while(idx) {
         utarray_push_back(charselect->indexList, &idx);
         idx = idx->hh.next;
     }

     utarray_sort(charselect->indexList, pindex_cmp);
}

void CharSelectDataFree(CharSelectData* charselect)
{
    utarray_free(charselect->indexList);
    while(charselect->index) {
        CharSelectDataIndex* p = charselect->index;
        HASH_DEL(charselect->index, p);
        free(p->key);
        utarray_free(p->items);
        free(p);
    }
    free(charselect->dataFile);
    free(charselect);
}
