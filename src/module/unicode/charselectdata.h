/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
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

#ifndef FCITX_CHARSELECTDATA_H
#define FCITX_CHARSELECTDATA_H

#include <fcitx-utils/utarray.h>
#include <fcitx-utils/uthash.h>

typedef struct _CharSelectDataIndex {
    char* key;
    UT_array* items;
    UT_hash_handle hh;
} CharSelectDataIndex;

typedef struct _CharSelectData {
    void* dataFile;
    CharSelectDataIndex* index;
    long int size;
    UT_array* indexList;
} CharSelectData;

CharSelectData* CharSelectDataCreate();
void CharSelectDataCreateIndex(CharSelectData* charselect);
void CharSelectDataDump(CharSelectData* charselect);
UT_array* CharSelectDataUnihanInfo(CharSelectData* charselect, uint16_t unicode);
uint32_t CharSelectDataGetDetailIndex(CharSelectData* charselect, uint16_t unicode);
UT_array* CharSelectDataAliases(CharSelectData* charselect, uint16_t unicode);
UT_array* CharSelectDataNotes(CharSelectData* charselect, uint16_t unicode);
UT_array* CharSelectDataSeeAlso(CharSelectData* charselect, uint16_t unicode);
UT_array* CharSelectDataEquivalents(CharSelectData* charselect, uint16_t unicode);
UT_array* CharSelectDataApproximateEquivalents(CharSelectData* charselect, uint16_t unicode);
UT_array* CharSelectDataFind(CharSelectData* charselect, const char* needle);
char* CharSelectDataName(CharSelectData* charselect, uint16_t unicode);
void CharSelectDataFree(CharSelectData* charselect);

#endif
