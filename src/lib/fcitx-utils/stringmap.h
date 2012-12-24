#ifndef FCITX_UTILS_STRINGMAP_H
#define FCITX_UTILS_STRINGMAP_H
#include <fcitx-utils/utils.h>

typedef struct _FcitxStringMap FcitxStringMap;

FcitxStringMap* fcitx_string_map_new(const char* str, char delim);

void fcitx_string_map_from_string(FcitxStringMap* map, const char* str, char delim);

boolean fcitx_string_map_get(FcitxStringMap* map, const char* key, boolean value);

void fcitx_string_map_set(FcitxStringMap* map, const char* key, boolean value);

void fcitx_string_map_clear(FcitxStringMap* map);

char* fcitx_string_map_to_string(FcitxStringMap* map, char delim);

void fcitx_string_map_remove(FcitxStringMap* map, const char* key);

void fcitx_string_map_free(FcitxStringMap* map);

#endif // FCITX_UTILS_STRINGMAP_H
