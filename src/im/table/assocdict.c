#include <stdio.h>
#include <ctype.h>

#include "assocdict.h"

#include "fcitx-config/xdg.h"
#include "fcitx-utils/utf8.h"

/* Storage for one associative phrase */
struct AssocItem
{
    const char* key;
    const char* phrase;
};
static UT_icd AssocItem_icd = { sizeof(struct AssocItem), NULL, NULL, NULL };

/* Storage for one associative phrase dictionary */
struct AssocDict {
    UT_array* assocItems;
};
static UT_icd AssocDict_icd = { sizeof(struct AssocDict), NULL, NULL, NULL };

/* Storage for language code mapping */
struct MapItem
{
    const char* langCode;
    const struct AssocDict* dict;
};
static UT_icd MapItem_icd = { sizeof(struct MapItem), NULL, NULL, NULL };

/* Storage for a list of built-in dictionary */
struct AssocDictList {
    UT_array* assocDicts;
    UT_array* mapItems;
    FcitxMemoryPool* pool;
};

/* Makes a copy of a string inside a memory pool */
static char* MemoryPoolStrdup(FcitxMemoryPool* pool, const char* str, int len)
{
    if (len < 0)  len = strlen(str);
    char* out = (char*)fcitx_memory_pool_alloc(pool, len + 1);
    memcpy(out, str, len);
    out[len] = '\0';
    return out;
}

/* Language code comparison function */
static int CompareMapItem(const void* a, const void* b)
{
    const struct MapItem* lang_a = (const struct MapItem*)a;
    const struct MapItem* lang_b = (const struct MapItem*)b;
    return strcmp(lang_a->langCode, lang_b->langCode);
}

/* Associative phrase comparison function */
static int CompareAssocItem(const void* a, const void* b)
{
    const struct AssocItem* item_a = (const struct AssocItem*)a;
    const struct AssocItem* item_b = (const struct AssocItem*)b;
    return strcmp(item_a->key, item_b->key);
}

static const struct AssocDict* TableLoadAssocDict(
        struct AssocDictList* list, const char* filename)
{
    /* Open dictionary file */
    FILE* f = FcitxXDGGetFileWithPrefix("table", filename, "r", NULL);
    if (!f)  return NULL;

    /* Create new dictionary */
    struct AssocDict* dict = fcitx_utils_new(struct AssocDict);
    utarray_new(dict->assocItems, &AssocItem_icd);

    /* Load phrases from file */
    char* line = NULL;
    size_t len = 0;
    struct AssocItem item;
    while (getline(&line, &len, f) >= 0)
    {
        /* Skip over leading spaces */
        char* buf = line;
        while (*buf && isspace(*buf))  ++buf;
        if (*buf == '\0')  continue;

        /* Find end of phrase */
        char* p = buf;
        while (*p && !isspace(*p))  ++p;
        *p = '\0';

        /* Make sure the phrase is encoded in UTF-8 */
        if (!fcitx_utf8_check_string(buf))  continue;

        /* Make sure the phrase has at least two Unicode characters */
        int ulen = fcitx_utf8_strlen(buf);
        if (ulen < 2)  continue;

        /* Discard long phrases which may result from corrupted tables */
        if (ulen > 80)  continue;

        /* Copy the key and associative phrase*/
        size_t klen = fcitx_utf8_char_len(buf);
        item.key    = MemoryPoolStrdup(list->pool, buf, klen);
        item.phrase = MemoryPoolStrdup(list->pool, buf + klen, -1);

        /* Append record to array */
        utarray_push_back(dict->assocItems, &item);
    }
    if (line)  free(line);

    /* Sort the phrases */
    utarray_sort(dict->assocItems, CompareAssocItem);

    /* Add to built-in list of dictionaries */
    utarray_push_back(list->assocDicts, dict);

    return dict;
}

static void TableMapAssocDictToLangCode(struct AssocDictList* list,
        const struct AssocDict* dict, const char* langCode)
{
    /* Map sure there is no existing entry for the language code */
    if (TableFindAssocDictFromLangCode(list, langCode) != NULL)
        return;

    /* Add a new entry for the language */
    struct MapItem item;
    item.langCode = MemoryPoolStrdup(list->pool, langCode, -1);
    item.dict = dict;
    utarray_push_back(list->mapItems, &item);

    /* Sort the language map after update */
    utarray_sort(list->mapItems, CompareMapItem);
}

const struct AssocDictList* TableCreateAssocDictList()
{
    /* Create new list */
    struct AssocDictList* list = fcitx_utils_new(struct AssocDictList);
    utarray_new(list->assocDicts, &AssocDict_icd);
    utarray_new(list->mapItems, &MapItem_icd);
    list->pool = fcitx_memory_pool_create();

    const struct AssocDict* dict;

    /* Load built-in Traditional Chinese associative phrases */
    dict = TableLoadAssocDict(list, "assoc-cht.mb");
    if (dict)
    {
        TableMapAssocDictToLangCode(list, dict, "zh_TW");
        TableMapAssocDictToLangCode(list, dict, "zh_HK");
    }

    /* Load built-in Simplified Chinese associative phrases */
    dict = TableLoadAssocDict(list, "assoc-chs.mb");
    if (dict)
    {
        TableMapAssocDictToLangCode(list, dict, "zh_CN");
    }

    return list;
}

const struct AssocDict* TableFindAssocDictFromLangCode(
        const struct AssocDictList* list, const char* langCode)
{
    struct MapItem target;
    target.langCode = langCode;
    struct MapItem* result = utarray_custom_bsearch(&target,
            list->mapItems, true, CompareMapItem);
    if (!result)  return NULL;
    return result->dict;
}

static INPUT_RETURN_VALUE AssocPhraseCallback(void* arg,
        FcitxCandidateWord* candWord)
{
    FcitxInstance *instance = (FcitxInstance*) arg;
    FcitxInputState *input = FcitxInstanceGetInputState(instance);

    /* Commit the associative phrase and exit remind mode */
    FcitxInputStateSetIsInRemind(input, false);
    strcpy(FcitxInputStateGetOutputString(input), candWord->strWord);
    return IRV_COMMIT_STRING;
}

void TableGetAssocPhrases(FcitxInstance* instance,
        FcitxCandidateWordList* cand_list, int count,
        const struct AssocDict* dict, const char* str)
{
    /* Perform lookup using last Unicode character */
    int len = fcitx_utf8_strlen(str);
    if (!len)  return;
    const char* key = fcitx_utf8_get_nth_char(str, len - 1);
    struct AssocItem target;
    target.key = key;
    struct AssocItem* item = utarray_custom_bsearch(
            &target, dict->assocItems, true, CompareAssocItem);
    if (!item)  return;
    if (strcmp(item->key, key) != 0)  return;

    /* Find first matching phrase */
    int pos = utarray_eltidx(dict->assocItems, item);
    while (pos > 0)
    {
        struct AssocItem* prev =
            (struct AssocItem*)_utarray_eltptr(dict->assocItems, pos - 1);
        if (strcmp(prev->key, key) != 0)  break;
        item = prev;
        --pos;
    }

    /* Copy matching phrases into candidate list */ 
    FcitxCandidateWord cand;
    memset(&cand, 0, sizeof(cand));
    cand.callback = AssocPhraseCallback;
    cand.owner    = instance;
    cand.wordType = MSG_OTHER;
    while (pos < utarray_len(dict->assocItems))
    {
        /* Make sure next item still matches the word */
        item = (struct AssocItem*)_utarray_eltptr(dict->assocItems, pos);
        if (strcmp(item->key, key) != 0)  break;
        ++pos;

        /* Append candidate */
        cand.strWord = strdup(item->phrase);
        FcitxCandidateWordAppend(cand_list, &cand);

        /* Limit the number of candidates if count is positive */
        if (count > 0)
        {
            --count;
            if (count == 0)  break;
        }
    }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
