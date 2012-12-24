#include "fcitx/fcitx.h"
#include "stringmap.h"

typedef struct _FcitxStringMapItem {
    char* key;
    boolean value;
    UT_hash_handle hh;
} FcitxStringMapItem;

struct _FcitxStringMap {
    FcitxStringMapItem* items;
};

static inline void fcitx_string_map_item_free(FcitxStringMapItem* item) {
    free(item->key);
    free(item);
}

FCITX_EXPORT_API
FcitxStringMap* fcitx_string_map_new(const char* str, char delim)
{
    FcitxStringMap* map = fcitx_utils_new(FcitxStringMap);
    if (str) {
        fcitx_string_map_from_string(map, str, delim);
    }
    return map;
}

FCITX_EXPORT_API
void fcitx_string_map_from_string(FcitxStringMap* map, const char* str, char delim)
{
    fcitx_string_map_clear(map);
    UT_array* list = fcitx_utils_split_string(str, delim);
    utarray_foreach(s, list, char*) {
        UT_array* item = fcitx_utils_split_string(*s, ':');
        if (utarray_len(item) == 2) {
            char* key = *(char**) utarray_eltptr(item, 0);
            char* value = *(char**) utarray_eltptr(item, 1);
            boolean bvalue = strcmp(value, "true") == 0;
            fcitx_string_map_set(map, key, bvalue);
        }
        fcitx_utils_free_string_list(item);
    }
    fcitx_utils_free_string_list(list);
}

FCITX_EXPORT_API
boolean fcitx_string_map_get(FcitxStringMap* map, const char* key, boolean value)
{
    FcitxStringMapItem* item = NULL;
    HASH_FIND_STR(map->items, key, item);
    if (item)
        return item->value;
    return value;
}

FCITX_EXPORT_API
void fcitx_string_map_set(FcitxStringMap* map, const char* key, boolean value)
{
    FcitxStringMapItem* item = NULL;
    HASH_FIND_STR(map->items, key, item);
    if (!item) {
        item = fcitx_utils_new(FcitxStringMapItem);
        item->key = strdup(key);
        HASH_ADD_KEYPTR(hh, map->items, item->key, strlen(item->key), item);
    }
    item->value = value;
}

void fcitx_string_map_clear(FcitxStringMap* map)
{
    while (map->items) {
        FcitxStringMapItem* p = map->items;
        HASH_DEL(map->items, p);
        fcitx_string_map_item_free(p);
    }
}

FCITX_EXPORT_API
void fcitx_string_map_free(FcitxStringMap* map)
{
    fcitx_string_map_clear(map);
    free(map);
}

FCITX_EXPORT_API
void fcitx_string_map_remove(FcitxStringMap* map, const char* key)
{
    FcitxStringMapItem* item = NULL;
    HASH_FIND_STR(map->items, key, item);
    if (item) {
        HASH_DEL(map->items, item);
        fcitx_string_map_item_free(item);
    }
}

FCITX_EXPORT_API
char* fcitx_string_map_to_string(FcitxStringMap* map, char delim)
{
    if (HASH_COUNT(map->items) == 0)
        return strdup("");

    size_t len = 0;
    HASH_FOREACH(item, map->items, FcitxStringMapItem) {
        len += strlen(item->key) + 1 + (item->value ? strlen("true") : strlen("false")) + 1;
    }

    char* result = (char*)malloc(sizeof(char) * len);
    char* p = result;
    HASH_FOREACH(item2, map->items, FcitxStringMapItem) {
        size_t strl = strlen(item2->key);
        memcpy(p, item2->key, strl);
        p += strl;
        *p = ':';
        p++;
        const char* s = item2->value ? "true" : "false";
        strl = strlen(s);
        memcpy(p, s, strl);
        p += strl;
        *p = delim;
        p++;
    }
    result[len - 1] = '\0';

    return result;
}



