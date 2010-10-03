/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @file   tools.h
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 * 
 * @brief  配置文件读写
 * 
 * 
 */

#ifndef _TOOLS_H
#define _TOOLS_H

#include <stdio.h>
#include <errno.h>
#include "fcitx-config/uthash.h"

#define TABLE_GBKS2T "gbks2t.tab"

#include <stdlib.h>

typedef struct StringHashSet {
    char *name;
    UT_hash_handle hh;
} StringHashSet;

void *custom_bsearch(const void *key, const void *base,
        size_t nmemb, size_t size, int accurate,
        int (*compar)(const void *, const void *));

void FcitxInitThread();
int FcitxLock();
void InitAsDaemon();
int FcitxUnlock();

int             CalculateRecordNumber (FILE * fpDict);
void            SetSwitchKey (char *str);
int             CalHZIndex (char *strHZ);
char           *ConvertGBKSimple2Tradition (char *text);

/*
#if defined(DARWIN)
int             ReverseInt (unsigned int pc_int);
#endif
*/
#endif
