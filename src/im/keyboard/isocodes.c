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
#include "common.h"

#include <libxml/parser.h>

#include <string.h>

#include <fcitx-utils/utils.h>
#include "isocodes.h"

static void IsoCodes639HandlerStartElement(void *ctx,
                                           const xmlChar *name,
                                           const xmlChar **atts);
static void IsoCodes3166HandlerStartElement(void *ctx,
                                            const xmlChar *name,
                                            const xmlChar **atts);
static void FcitxIsoCodes639EntryFree(FcitxIsoCodes639Entry* entry);
static void FcitxIsoCodes3166EntryFree(FcitxIsoCodes3166Entry* entry);

FcitxIsoCodes* FcitxXkbReadIsoCodes(const char* iso639, const char* iso3166)
{
    xmlSAXHandler handle;
    memset(&handle, 0, sizeof(xmlSAXHandler));
    
    xmlInitParser();
    
    FcitxIsoCodes* isocodes = (FcitxIsoCodes*) fcitx_utils_malloc0(sizeof(FcitxIsoCodes));
    
    handle.startElement = IsoCodes639HandlerStartElement;
    xmlSAXUserParseFile(&handle, isocodes, iso639);
    handle.startElement = IsoCodes3166HandlerStartElement;
    xmlSAXUserParseFile(&handle, isocodes, iso3166);
    xmlCleanupParser();

    return isocodes;
}

static void IsoCodes639HandlerStartElement(void *ctx,
                                           const xmlChar *name,
                                           const xmlChar **atts)
{
    FcitxIsoCodes* isocodes = ctx;
    if (strcmp(XMLCHAR_CAST name, "iso_639_entry") == 0) {
        FcitxIsoCodes639Entry* entry = fcitx_utils_malloc0(sizeof(FcitxIsoCodes639Entry));
        int i = 0;
        while(atts && atts[i*2] != 0) {
            if (strcmp(XMLCHAR_CAST atts[i * 2], "iso_639_2B_code") == 0)
                entry->iso_639_2B_code = strdup(XMLCHAR_CAST atts[i * 2 + 1]);
            else if (strcmp(XMLCHAR_CAST atts[i * 2], "iso_639_2T_code") == 0)
                entry->iso_639_2T_code = strdup(XMLCHAR_CAST atts[i * 2 + 1]);
            else if (strcmp(XMLCHAR_CAST atts[i * 2], "iso_639_1_code") == 0)
                entry->iso_639_1_code = strdup(XMLCHAR_CAST atts[i * 2 + 1]);
            else if (strcmp(XMLCHAR_CAST atts[i * 2], "name") == 0)
                entry->name = strdup(XMLCHAR_CAST atts[i * 2 + 1]);
            i++;
        }
        if (!entry->iso_639_2B_code || !entry->iso_639_2T_code || !entry->name)
            FcitxIsoCodes639EntryFree(entry);
        else {
            HASH_ADD_KEYPTR(hh1, isocodes->iso6392B, entry->iso_639_2B_code, strlen(entry->iso_639_2B_code), entry);
            HASH_ADD_KEYPTR(hh2, isocodes->iso6392T, entry->iso_639_2T_code, strlen(entry->iso_639_2T_code), entry);
        }
    }
}

void FcitxIsoCodes639EntryFree(FcitxIsoCodes639Entry* entry)
{
    FREE_IF_NOT_NULL(entry->iso_639_1_code);
    FREE_IF_NOT_NULL(entry->iso_639_2B_code);
    FREE_IF_NOT_NULL(entry->iso_639_2T_code);
    FREE_IF_NOT_NULL(entry->name);
    free(entry);
}

void FcitxIsoCodes3166EntryFree(FcitxIsoCodes3166Entry* entry)
{
    FREE_IF_NOT_NULL(entry->alpha_2_code);
    FREE_IF_NOT_NULL(entry->name);
    free(entry);
}


static void IsoCodes3166HandlerStartElement(void *ctx,
                                           const xmlChar *name,
                                           const xmlChar **atts)
{
    FcitxIsoCodes* isocodes = ctx;
    if (strcmp(XMLCHAR_CAST name, "iso_3166_entry") == 0) {
        FcitxIsoCodes3166Entry* entry = fcitx_utils_malloc0(sizeof(FcitxIsoCodes3166Entry));
        int i = 0;
        while(atts && atts[i*2] != 0) {
            if (strcmp(XMLCHAR_CAST atts[i * 2], "alpha_2_code") == 0)
                entry->alpha_2_code = strdup(XMLCHAR_CAST atts[i * 2 + 1]);
            else if (strcmp(XMLCHAR_CAST atts[i * 2], "name") == 0)
                entry->name = strdup(XMLCHAR_CAST atts[i * 2 + 1]);
            i++;
        }
        if (!entry->name || !entry->alpha_2_code)
            FcitxIsoCodes3166EntryFree(entry);
        else
            HASH_ADD_KEYPTR(hh, isocodes->iso3166, entry->alpha_2_code, strlen(entry->alpha_2_code), entry);
    }
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
