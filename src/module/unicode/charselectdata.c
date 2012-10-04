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
    HASH_FIND(hh,head,findint,sizeof(uint16_t),out)
#define HASH_ADD_UNICODE(head,intfield,add)                                         \
    HASH_ADD(hh,head,intfield,sizeof(uint16_t),add)

typedef struct _UnicodeSet {
    uint16_t unicode;
    UT_hash_handle hh;
} UnicodeSet;

const UT_icd int16_icd = {sizeof(int16_t), NULL, NULL, NULL};
const UT_icd pidx_icd = {sizeof(CharSelectDataIndex*), NULL, NULL, NULL};

static const char JAMO_L_TABLE[][4] =
    {
        "G", "GG", "N", "D", "DD", "R", "M", "B", "BB",
        "S", "SS", "", "J", "JJ", "C", "K", "T", "P", "H"
    };

static const char JAMO_V_TABLE[][4] =
    {
        "A", "AE", "YA", "YAE", "EO", "E", "YEO", "YE", "O",
        "WA", "WAE", "OE", "YO", "U", "WEO", "WE", "WI",
        "YU", "EU", "YI", "I"
    };

static const char JAMO_T_TABLE[][4] =
    {
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

char* FormatCode(uint16_t code, int length, const char* prefix);
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

UT_array* CharSelectDataUnihanInfo(CharSelectData* charselect, uint16_t unicode)
{
    UT_array* res = fcitx_utils_new_string_list();

    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+36);
    const uint32_t offsetEnd = charselect->size;

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 30) - 1;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint16_t midUnicode = FromLittleEndian16(data + offsetBegin + mid*30);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            int i;
            for(i = 0; i < 7; i++) {
                uint32_t offset = FromLittleEndian32(data + offsetBegin + mid*30 + 2 + i*4);
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

uint32_t CharSelectDataGetDetailIndex(CharSelectData* charselect, uint16_t unicode)
{
    const char* data = charselect->dataFile;
    // Convert from little-endian, so that this code works on PPC too.
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=482286
    const uint32_t offsetBegin = FromLittleEndian32(data+12);
    const uint32_t offsetEnd = FromLittleEndian32(data+16);

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 27) - 1;

    static uint16_t most_recent_searched;
    static uint32_t most_recent_result;


    if (unicode == most_recent_searched)
        return most_recent_result;

    most_recent_searched = unicode;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint16_t midUnicode = FromLittleEndian16(data + offsetBegin + mid*27);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            most_recent_result = offsetBegin + mid*27;

            return most_recent_result;
        }
    }

    most_recent_result = 0;
    return 0;
}

#if 0

QList<QChar> KCharSelectData::blockContents(int block)
{
    if(!openDataFile()) {
        return QList<QChar>();
    }


    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+20);
    const uint32_t offsetEnd = FromLittleEndian32(data+24);

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    QList<QChar> res;

    if(block > max)
        return res;

    uint16_t unicodeBegin = FromLittleEndian16(data + offsetBegin + block*4);
    uint16_t unicodeEnd = FromLittleEndian16(data + offsetBegin + block*4 + 2);

    while(unicodeBegin < unicodeEnd) {
        res.append(unicodeBegin);
        unicodeBegin++;
    }
    res.append(unicodeBegin); // Be carefull when unicodeEnd==0xffff

    return res;
}

QList<int> KCharSelectData::sectionContents(int section)
{
    if(!openDataFile()) {
        return QList<int>();
    }


    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+28);
    const uint32_t offsetEnd = FromLittleEndian32(data+32);

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    QList<int> res;

    if(section > max)
        return res;

    for(int i = 0; i <= max; i++) {
        const uint16_t currSection = FromLittleEndian16(data + offsetBegin + i*4);
        if(currSection == section) {
            res.append( FromLittleEndian16(data + offsetBegin + i*4 + 2) );
        }
    }

    return res;
}

UT_array* KCharSelectData::sectionList()
{
    if(!openDataFile()) {
        return fcitx_utils_new_string_list();
    }


    const char* data = charselect->dataFile;
    const uint32_t stringBegin = FromLittleEndian32(data+24);
    const uint32_t stringEnd = FromLittleEndian32(data+28);

    const char* data = dataFile.constData();
    UT_array* list;
    uint32_t i = stringBegin;
    while(i < stringEnd) {
        list.append(i18nc("KCharSelect section name", data + i));
        i += strlen(data + i) + 1;
    }

    return list;
}

QString KCharSelectData::block(CharSelectData* charselect, uint16_t unicode)
{
    return blockName(blockIndex(c));
}

QString KCharSelectData::section(CharSelectData* charselect, uint16_t unicode)
{
    return sectionName(sectionIndex(blockIndex(c)));
}

int KCharSelectData::blockIndex(CharSelectData* charselect, uint16_t unicode)
{
    if(!openDataFile()) {
        return 0;
    }


    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+20);
    const uint32_t offsetEnd = FromLittleEndian32(data+24);
    const uint16_t unicode = c.unicode();

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    int i = 0;

    while (unicode > FromLittleEndian16(data + offsetBegin + i*4 + 2) && i < max) {
        i++;
    }

    return i;
}

int KCharSelectData::sectionIndex(int block)
{
    if(!openDataFile()) {
        return 0;
    }


    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+28);
    const uint32_t offsetEnd = FromLittleEndian32(data+32);

    int max = ((offsetEnd - offsetBegin) / 4) - 1;

    for(int i = 0; i <= max; i++) {
        if( FromLittleEndian16(data + offsetBegin + i*4 + 2) == block) {
            return FromLittleEndian16(data + offsetBegin + i*4);
        }
    }

    return 0;
}

QString KCharSelectData::blockName(int index)
{
    if(!openDataFile()) {
        return QString();
    }


    const char* data = charselect->dataFile;
    const uint32_t stringBegin = FromLittleEndian32(data+16);
    const uint32_t stringEnd = FromLittleEndian32(data+20);

    uint32_t i = stringBegin;
    int currIndex = 0;

    const char* data = dataFile.constData();
    while(i < stringEnd && currIndex < index) {
        i += strlen(data + i) + 1;
        currIndex++;
    }

    return i18nc("KCharselect unicode block name", data + i);
}

QString KCharSelectData::sectionName(int index)
{
    if(!openDataFile()) {
        return QString();
    }


    const char* data = charselect->dataFile;
    const uint32_t stringBegin = FromLittleEndian32(data+24);
    const uint32_t stringEnd = FromLittleEndian32(data+28);

    uint32_t i = stringBegin;
    int currIndex = 0;

    const char* data = dataFile.constData();
    while(i < stringEnd && currIndex < index) {
        i += strlen(data + i) + 1;
        currIndex++;
    }

    return i18nc("KCharselect unicode section name", data + i);
}

QChar::Category KCharSelectData::category(CharSelectData* charselect, uint16_t unicode)
{
    if(!openDataFile()) {
        return c.category();
    }

    uint16_t unicode = c.unicode();


    const char* data = charselect->dataFile;
    const uint32_t offsetBegin = FromLittleEndian32(data+4);
    const uint32_t offsetEnd = FromLittleEndian32(data+8);

    int min = 0;
    int mid;
    int max = ((offsetEnd - offsetBegin) / 6) - 1;
    QString s;

    while (max >= min) {
        mid = (min + max) / 2;
        const uint16_t midUnicode = FromLittleEndian16(data + offsetBegin + mid*6);
        if (unicode > midUnicode)
            min = mid + 1;
        else if (unicode < midUnicode)
            max = mid - 1;
        else {
            uint32_t offset = FromLittleEndian32(data + offsetBegin + mid*6 + 2);
            const uint8_t categoryCode = * (uint8_t *)(data + offset);
            return QChar::Category(categoryCode);
        }
    }

    return c.category();
}

bool KCharSelectData::isPrint(CharSelectData* charselect, uint16_t unicode)
{
    QChar::Category cat = category(c);
    return !(cat == QChar::Other_Control || cat == QChar::Other_NotAssigned);
}

bool KCharSelectData::isDisplayable(CharSelectData* charselect, uint16_t unicode)
{
    // Qt internally uses U+FDD0 and U+FDD1 to mark the beginning and the end of frames.
    // They should be seen as non-printable characters, as trying to display them leads
    //  to a crash caused by a Qt "noBlockInString" assertion.
    if(c == 0xFDD0 || c == 0xFDD1)
        return false;

    return !isIgnorable(c) && isPrint(c);
}

bool KCharSelectData::isIgnorable(CharSelectData* charselect, uint16_t unicode)
{
    /*
     * According to the Unicode standard, Default Ignorable Code Points
     * should be ignored unless explicitly supported. For example, U+202E
     * RIGHT-TO-LEFT-OVERRIDE ir printable according to Qt, but displaying
     * it gives the undesired effect of all text being turned RTL. We do not
     * have a way to "explicitly" support it, so we will treat it as
     * non-printable.
     *
     * There is a list of these on
     * http://unicode.org/Public/UNIDATA/DerivedCoreProperties.txt under the
     * property Default_Ignorable_Code_Point.
     */

    //NOTE: not very nice to hardcode these here; is it worth it to modify
    //      the binary data file to hold them?
    return c == 0x00AD || c == 0x034F || c == 0x115F || c == 0x1160 ||
           c == 0x17B4 || c == 0x17B5 || (c >= 0x180B && c <= 0x180D) ||
           (c >= 0x200B && c <= 0x200F) || (c >= 0x202A && c <= 0x202E) ||
           (c >= 0x2060 && c <= 0x206F) || c == 0x3164 ||
           (c >= 0xFE00 && c <= 0xFE0F) || c == 0xFEFF || c == 0xFFA0 ||
           (c >= 0xFFF0 && c <= 0xFFF8);
}

bool KCharSelectData::isCombining(const QChar &c)
{
    return section(c) == i18nc("KCharSelect section name", "Combining Diacritical Marks");
    //FIXME: this is an imperfect test. There are many combining characters
    //       that are outside of this section. See Grapheme_Extend in
    //       http://www.unicode.org/Public/UNIDATA/DerivedCoreProperties.txt
}

QString KCharSelectData::display(const QChar &c, const QFont &font)
{
    if (!isDisplayable(c)) {
        return QString("<b>") + i18n("Non-printable") + "</b>";
    } else {
        QString s = QString("<font size=\"+4\" face=\"") + font.family() + "\">";
        if (isCombining(c)) {
            s += displayCombining(c);
        } else {
            s += "&#" + QString::number(c.unicode()) + ';';
        }
        s += "</font>";
        return s;
    }
}

QString KCharSelectData::displayCombining(const QChar &c)
{
    /*
     * The purpose of this is to make it easier to see how a combining
     * character affects the text around it.
     * The initial plan was to use U+25CC DOTTED CIRCLE for this purpose,
     * as seen in pdfs from Unicode, but there seem to be a lot of alignment
     * problems with that.
     *
     * Eventually, it would be nice to determine whether the character
     * combines to the left or to the right, etc.
     */
    QString s = "&nbsp;&#" + QString::number(c.unicode()) + ";&nbsp;" +
                " (ab&#" + QString::number(c.unicode()) + ";c)";
    return s;
}

QString KCharSelectData::categoryText(QChar::Category category)
{
    switch (category) {
    case QChar::Other_Control: return i18n("Other, Control");
    case QChar::Other_Format: return i18n("Other, Format");
    case QChar::Other_NotAssigned: return i18n("Other, Not Assigned");
    case QChar::Other_PrivateUse: return i18n("Other, Private Use");
    case QChar::Other_Surrogate: return i18n("Other, Surrogate");
    case QChar::Letter_Lowercase: return i18n("Letter, Lowercase");
    case QChar::Letter_Modifier: return i18n("Letter, Modifier");
    case QChar::Letter_Other: return i18n("Letter, Other");
    case QChar::Letter_Titlecase: return i18n("Letter, Titlecase");
    case QChar::Letter_Uppercase: return i18n("Letter, Uppercase");
    case QChar::Mark_SpacingCombining: return i18n("Mark, Spacing Combining");
    case QChar::Mark_Enclosing: return i18n("Mark, Enclosing");
    case QChar::Mark_NonSpacing: return i18n("Mark, Non-Spacing");
    case QChar::Number_DecimalDigit: return i18n("Number, Decimal Digit");
    case QChar::Number_Letter: return i18n("Number, Letter");
    case QChar::Number_Other: return i18n("Number, Other");
    case QChar::Punctuation_Connector: return i18n("Punctuation, Connector");
    case QChar::Punctuation_Dash: return i18n("Punctuation, Dash");
    case QChar::Punctuation_Close: return i18n("Punctuation, Close");
    case QChar::Punctuation_FinalQuote: return i18n("Punctuation, Final Quote");
    case QChar::Punctuation_InitialQuote: return i18n("Punctuation, Initial Quote");
    case QChar::Punctuation_Other: return i18n("Punctuation, Other");
    case QChar::Punctuation_Open: return i18n("Punctuation, Open");
    case QChar::Symbol_Currency: return i18n("Symbol, Currency");
    case QChar::Symbol_Modifier: return i18n("Symbol, Modifier");
    case QChar::Symbol_Math: return i18n("Symbol, Math");
    case QChar::Symbol_Other: return i18n("Symbol, Other");
    case QChar::Separator_Line: return i18n("Separator, Line");
    case QChar::Separator_Paragraph: return i18n("Separator, Paragraph");
    case QChar::Separator_Space: return i18n("Separator, Space");
    default: return i18n("Unknown");
    }
}

#endif


char* CharSelectDataName(CharSelectData* charselect, uint16_t unicode)
{
    char* result = NULL;
    do {
        if ((unicode >= 0x3400 && unicode <= 0x4DB5)
                || (unicode >= 0x4e00 && unicode <= 0x9fa5)) {
            // || (unicode >= 0x20000 && unicode <= 0x2A6D6) // useless, since limited to 16 bit
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
            int max = ((offsetEnd - offsetBegin) / 6) - 1;

            while (max >= min) {
                mid = (min + max) / 2;
                const uint16_t midUnicode = FromLittleEndian16(data + offsetBegin + mid*6);
                if (unicode > midUnicode)
                    min = mid + 1;
                else if (unicode < midUnicode)
                    max = mid - 1;
                else {
                    uint32_t offset = FromLittleEndian32(data + offsetBegin + mid*6 + 2);
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
    if (l != 6)
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
    utarray_new(returnRes, &int16_icd);
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
            uint16_t uni = (uint16_t) strtol(*s + 2, &end, 16);
            utarray_push_back(returnRes, &uni);

            // search for "1234" instead of "0x1234"
            char* news = strdup(*s + 2);
            free(*s);
            *s = news;
        }
        // try to parse string as decimal number
        end = NULL;
        int unicode = strtol(*s, &end, 10);
        if (end == NULL && unicode >= 0 && unicode <= 0xFFFF) {
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
    utarray_foreach(c, returnRes, uint16_t) {
        UnicodeSet* dup = NULL;
        HASH_FIND_UNICODE(result, c, dup);
        if (dup)
            HASH_DEL(result, dup);
    }

    HASH_SORT(result, uni_cmp);

    while (result) {
        UnicodeSet* p = result;
        HASH_DEL(result, p);
        utarray_push_back(returnRes, &p->unicode);
        free(p);
    }

    utarray_free(searchStrings);

    return returnRes;
}

UnicodeSet* InsertResult(UnicodeSet* set, uint16_t unicode) {
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
    UnicodeSet* result = NULL;

    CharSelectDataIndex** pos = utarray_custom_bsearch(s, charselect->indexList, 0, index_search_cmp);
    CharSelectDataIndex** last = utarray_custom_bsearch(s, charselect->indexList, 0, index_search_a_cmp);

    while (pos != last && strncasecmp(s, (*pos)->key, strlen(s)) == 0) {
        utarray_foreach (c, (*pos)->items, uint16_t) {
            result = InsertResult(result, *c);
        }
        ++pos;
    }

    return result;
}

UT_array* CharSelectDataAliases(CharSelectData* charselect, uint16_t unicode)
{
    const char* data = charselect->dataFile;
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const uint8_t count = * (uint8_t *)(data + detailIndex + 6);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 2);

    UT_array* aliases = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(aliases, &r);
        offset += strlen(data + offset) + 1;
    }
    return aliases;
}


UT_array* CharSelectDataNotes(CharSelectData* charselect, uint16_t unicode)
{
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 11);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 7);

    UT_array* notes = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(notes, &r);
        offset += strlen(data + offset) + 1;
    }

    return notes;
}

UT_array* CharSelectDataSeeAlso(CharSelectData* charselect, uint16_t unicode)
{
    UT_array* seeAlso;
    utarray_new(seeAlso, &int16_icd);
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return seeAlso;
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 26);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 22);

    int i;
    for (i = 0;  i < count;  i++) {
        uint16_t c = FromLittleEndian16 (data + offset);
        utarray_push_back(seeAlso, &c);
        offset += 2;
    }

    return seeAlso;
}

UT_array* CharSelectDataEquivalents(CharSelectData* charselect, uint16_t unicode)
{
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 21);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 17);

    UT_array* equivalents = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(equivalents, &r);
        offset += strlen(data + offset) + 1;
    }

    return equivalents;
}

UT_array* CharSelectDataApproximateEquivalents(CharSelectData* charselect, uint16_t unicode)
{
    const int detailIndex = CharSelectDataGetDetailIndex(charselect, unicode);
    if(detailIndex == 0) {
        return fcitx_utils_new_string_list();
    }

    const char* data = charselect->dataFile;
    const uint8_t count = * (uint8_t *)(data + detailIndex + 16);
    uint32_t offset = FromLittleEndian32(data + detailIndex + 12);

    UT_array* approxEquivalents = fcitx_utils_new_string_list();

    int i;
    for (i = 0;  i < count;  i++) {
        const char* r = data + offset;
        utarray_push_back(approxEquivalents, &r);
        offset += strlen(data + offset) + 1;
    }

    return approxEquivalents;
}


char* FormatCode(uint16_t code, int length, const char* prefix)
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
    utarray_new(idx->items, &int16_icd);
    return idx;
}

void CharSelectDataAppendToIndex(CharSelectData* charselect, uint16_t unicode, const char* str)
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

    int max = ((nameOffsetEnd - nameOffsetBegin) / 6) - 1;

    int pos, j;

    for (pos = 0; pos <= max; pos++) {
        const uint16_t unicode = FromLittleEndian16(data + nameOffsetBegin + pos*6);
        uint32_t offset = FromLittleEndian32(data + nameOffsetBegin + pos*6 + 2);
        // TODO
        CharSelectDataAppendToIndex(charselect, unicode, (data + offset + 1));
    }

    // details
    const uint32_t detailsOffsetBegin = FromLittleEndian32(data+12);
    const uint32_t detailsOffsetEnd = FromLittleEndian32(data+16);

    max = ((detailsOffsetEnd - detailsOffsetBegin) / 27) - 1;
    for (pos = 0; pos <= max; pos++) {
        const uint16_t unicode = FromLittleEndian16(data + detailsOffsetBegin + pos*27);

        // aliases
        const uint8_t aliasCount = * (uint8_t *)(data + detailsOffsetBegin + pos*27 + 6);
        uint32_t aliasOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*27 + 2);

        for (j = 0;  j < aliasCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode, data + aliasOffset);
            aliasOffset += strlen(data + aliasOffset) + 1;
        }

        // notes
        const uint8_t notesCount = * (uint8_t *)(data + detailsOffsetBegin + pos*27 + 11);
        uint32_t notesOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*27 + 7);

        for (j = 0;  j < notesCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode, data + notesOffset);
            notesOffset += strlen(data + notesOffset) + 1;
        }

        // approximate equivalents
        const uint8_t apprCount = * (uint8_t *)(data + detailsOffsetBegin + pos*27 + 16);
        uint32_t apprOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*27 + 12);

        for (j = 0;  j < apprCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode,data + apprOffset);
            apprOffset += strlen(data + apprOffset) + 1;
        }

        // equivalents
        const uint8_t equivCount = * (uint8_t *)(data + detailsOffsetBegin + pos*27 + 21);
        uint32_t equivOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*27 + 17);

        for (j = 0;  j < equivCount;  j++) {
            CharSelectDataAppendToIndex(charselect, unicode, data + equivOffset);
            equivOffset += strlen(data + equivOffset) + 1;
        }

        // see also - convert to string (hex)
        const uint8_t seeAlsoCount = * (uint8_t *)(data + detailsOffsetBegin + pos*27 + 26);
        uint32_t seeAlsoOffset = FromLittleEndian32(data + detailsOffsetBegin + pos*27 + 22);

        for (j = 0;  j < seeAlsoCount;  j++) {
            uint16_t seeAlso = FromLittleEndian16 (data + seeAlsoOffset);
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
     max = ((unihanOffsetEnd - unihanOffsetBegin) / 30) - 1;

     for (pos = 0; pos <= max; pos++) {
         const uint16_t unicode = FromLittleEndian16(data + unihanOffsetBegin + pos*30);
         for(j = 0; j < 7; j++) {
             uint32_t offset = FromLittleEndian32(data + unihanOffsetBegin + pos*30 + 2 + j*4);
             if(offset != 0) {
                 CharSelectDataAppendToIndex(charselect, unicode, (data + offset));
             }
         }
     }

     utarray_new(charselect->indexList, &pidx_icd);

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
