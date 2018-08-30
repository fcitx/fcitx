/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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

#include "config.h"

#include <json-c/json.h>

#include <string.h>

#include <fcitx-utils/utils.h>
#include "isocodes.h"


static void IsoCodes639Handler(FcitxIsoCodes *isocodes, json_object *entry);
static void IsoCodes3166Handler(FcitxIsoCodes *isocodes, json_object *entry);
static void FcitxIsoCodes639EntryFree(FcitxIsoCodes639Entry* entry);
static void FcitxIsoCodes3166EntryFree(FcitxIsoCodes3166Entry* entry);

FcitxIsoCodes* FcitxXkbReadIsoCodes(const char* iso639, const char* iso3166)
{
    FcitxIsoCodes* isocodes = fcitx_utils_new(FcitxIsoCodes);

    json_object *obj;
    do {
        obj = json_object_from_file(iso639);
        if (!obj) {
            break;
        }
        json_object *obj639 = json_object_object_get(obj, "639-3");
        if (!obj639 || json_object_get_type(obj639) != json_type_array) {
            break;
        }

        for (size_t i = 0, e = json_object_array_length(obj639); i < e; i++) {
            json_object *entry = json_object_array_get_idx(obj639, i);
            IsoCodes639Handler(isocodes, entry);
        }
    } while(0);
    json_object_put(obj);

    do {
        obj = json_object_from_file(iso3166);
        if (!obj) {
            break;
        }
        json_object *obj3166 = json_object_object_get(obj, "3166-1");
        if (!obj3166 || json_object_get_type(obj3166) != json_type_array) {
            break;
        }

        for (size_t i = 0, e = json_object_array_length(obj3166); i < e; i++) {
            json_object *entry = json_object_array_get_idx(obj3166, i);
            IsoCodes3166Handler(isocodes, entry);
        }
    } while(0);
    return isocodes;
}

static void IsoCodes639Handler(FcitxIsoCodes *isocodes, json_object *entry)
{
    json_object *alpha2 = json_object_object_get(entry, "alpha_2");
    json_object *alpha3 = json_object_object_get(entry, "alpha_3");
    json_object *bibliographic = json_object_object_get(entry, "bibliographic");
    json_object *name= json_object_object_get(entry, "name");
    if (!name || json_object_get_type(name) != json_type_string) {
        return;
    }
    // there must be alpha3
    if (!alpha3 || json_object_get_type(alpha3) != json_type_string) {
        return;
    }

    // alpha2 is optional
    if (alpha2 && json_object_get_type(alpha2) != json_type_string) {
        return;
    }

    // bibliographic is optional
    if (bibliographic && json_object_get_type(bibliographic) != json_type_string) {
        return;
    }
    if (!bibliographic) {
        bibliographic = alpha3;
    }
    FcitxIsoCodes639Entry* e = fcitx_utils_new(FcitxIsoCodes639Entry);
    e->name = strndup(json_object_get_string(name), json_object_get_string_len(name));
    if (alpha2) {
        e->iso_639_1_code = strndup(json_object_get_string(alpha2), json_object_get_string_len(alpha2));
    }
    e->iso_639_2B_code = strndup(json_object_get_string(bibliographic), json_object_get_string_len(bibliographic));
    e->iso_639_2T_code = strndup(json_object_get_string(alpha3), json_object_get_string_len(alpha3));
    HASH_ADD_KEYPTR(hh1, isocodes->iso6392B, e->iso_639_2B_code, strlen(e->iso_639_2B_code), e);
    HASH_ADD_KEYPTR(hh2, isocodes->iso6392T, e->iso_639_2T_code, strlen(e->iso_639_2T_code), e);
}

void FcitxIsoCodes639EntryFree(FcitxIsoCodes639Entry* entry)
{
    fcitx_utils_free(entry->iso_639_1_code);
    fcitx_utils_free(entry->iso_639_2B_code);
    fcitx_utils_free(entry->iso_639_2T_code);
    fcitx_utils_free(entry->name);
    free(entry);
}

void FcitxIsoCodes3166EntryFree(FcitxIsoCodes3166Entry* entry)
{
    fcitx_utils_free(entry->alpha_2_code);
    fcitx_utils_free(entry->name);
    free(entry);
}

static void IsoCodes3166Handler(FcitxIsoCodes *isocodes, json_object *entry)
{
    json_object *alpha2 = json_object_object_get(entry, "alpha_2");
    json_object *name= json_object_object_get(entry, "name");
    if (!name || json_object_get_type(name) != json_type_string) {
        return;
    }
    // there must be alpha3
    if (!alpha2 || json_object_get_type(alpha2) != json_type_string) {
        return;
    }
    FcitxIsoCodes3166Entry* e = fcitx_utils_new(FcitxIsoCodes3166Entry);
    e->name = strndup(json_object_get_string(name), json_object_get_string_len(name));
    e->alpha_2_code = strndup(json_object_get_string(alpha2), json_object_get_string_len(alpha2));
    HASH_ADD_KEYPTR(hh, isocodes->iso3166, e->alpha_2_code, strlen(e->alpha_2_code), e);
}

FcitxIsoCodes639Entry* FcitxIsoCodesGetEntry(FcitxIsoCodes* isocodes, const char* lang)
{
    FcitxIsoCodes639Entry *entry = NULL;
    HASH_FIND(hh1,isocodes->iso6392B,lang,strlen(lang),entry);
    if (!entry) {
        HASH_FIND(hh2,isocodes->iso6392T,lang,strlen(lang),entry);
    }
    return entry;
}

void FcitxIsoCodesFree(FcitxIsoCodes* isocodes)
{
    FcitxIsoCodes639Entry* isocodes639 = isocodes->iso6392B;
    while (isocodes639) {
        FcitxIsoCodes639Entry* curisocodes639 = isocodes639;
        HASH_DELETE(hh1, isocodes639, curisocodes639);
    }

    isocodes639 = isocodes->iso6392T;
    while (isocodes639) {
        FcitxIsoCodes639Entry* curisocodes639 = isocodes639;
        HASH_DELETE(hh2, isocodes639, curisocodes639);
        FcitxIsoCodes639EntryFree(curisocodes639);
    }
    FcitxIsoCodes3166Entry* isocodes3166 = isocodes->iso3166;
    while (isocodes3166) {
        FcitxIsoCodes3166Entry* curisocodes3166 = isocodes3166;
        HASH_DEL(isocodes3166, curisocodes3166);
        FcitxIsoCodes3166EntryFree(curisocodes3166);
    }

    free(isocodes);
}
