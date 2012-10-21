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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
#ifndef _SP_H
#define _SP_H

#include "fcitx-config/fcitx-config.h"

typedef enum _SP_FROM {
    SP_FROM_USER = 0,
    SP_FROM_SYSTEM_CONFIG,
    SP_FROM_SYSTEM_SP_CONFIG
} SP_FROM;

typedef struct _SP_C {
    char            strQP[5];
    char            cJP;
} SP_C;

typedef struct _SP_S {
    char            strQP[3];
    char            cJP;
} SP_S;

struct _FcitxPinyinConfig;
struct _FcitxPinyinState;

void            LoadSPData(struct _FcitxPinyinState* pystate);

//void            QP2SP (char *strQP, char *strSP);
void            SP2QP(struct _FcitxPinyinConfig* pyconfig, const char* strSP, char* strQP);
int             GetSPIndexQP_C(struct _FcitxPinyinConfig* pyconfig, char *str);
int             GetSPIndexQP_S(struct _FcitxPinyinConfig* pyconfig, char *str);
int             GetSPIndexJP_C(struct _FcitxPinyinConfig* pyconfig, char c, int iStart);
int             GetSPIndexJP_S(struct _FcitxPinyinConfig* pyconfig, char c);

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
